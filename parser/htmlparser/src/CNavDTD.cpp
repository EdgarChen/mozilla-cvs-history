/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */


#include "nsIDTDDebug.h"
#include "CNavDTD.h"
#include "nsHTMLTokens.h"
#include "nsCRT.h"
#include "nsParser.h"
#include "nsIHTMLContentSink.h" 
#include "nsScanner.h"
#include "nsParserTypes.h"
#include "nsVoidArray.h"
#include "nsTokenHandler.h"
#include "nsIDTDDebug.h"
#include "prenv.h"  //this is here for debug reasons...
#include "prtypes.h"  //this is here for debug reasons...
#include "prio.h"
#include "plstr.h"

#ifdef XP_PC
#include <direct.h> //this is here for debug reasons...
#endif
#include "prmem.h"


static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);                 
static NS_DEFINE_IID(kIDTDIID,      NS_IDTD_IID);
static NS_DEFINE_IID(kClassIID,     NS_INAVHTML_DTD_IID); 

//static const char* kNullURL = "Error: Null URL given";
//static const char* kNullFilename= "Error: Null filename given";
static const char* kNullToken = "Error: Null token given";
static const char* kInvalidTagStackPos = "Error: invalid tag stack position";
static const char* kHTMLTextContentType = "text/html";
static char* kVerificationDir = "c:/temp";

static nsAutoString gEmpty;

static char formElementTags[]= {  
    eHTMLTag_button,  eHTMLTag_fieldset,  eHTMLTag_input,
    eHTMLTag_isindex, eHTMLTag_label,     eHTMLTag_legend,
    eHTMLTag_option,  eHTMLTag_select,    eHTMLTag_textarea,0};

static char gHeadingTags[]={
  eHTMLTag_h1,  eHTMLTag_h2,  eHTMLTag_h3,  
  eHTMLTag_h4,  eHTMLTag_h5,  eHTMLTag_h6, 
  0};

static char  gStyleTags[]={
  eHTMLTag_a,
  eHTMLTag_acronym,
  eHTMLTag_b,
  eHTMLTag_bdo,
  eHTMLTag_big,
  eHTMLTag_blink,
  eHTMLTag_center,
  eHTMLTag_cite,
  eHTMLTag_code,
  eHTMLTag_del,
  eHTMLTag_dfn,
  eHTMLTag_em,
  eHTMLTag_font,
  eHTMLTag_i,
  eHTMLTag_ins,
  eHTMLTag_kbd,
  eHTMLTag_nobr,
  eHTMLTag_q,
  eHTMLTag_s,
  eHTMLTag_samp,
  eHTMLTag_small,
  eHTMLTag_span,
  eHTMLTag_strike,
  eHTMLTag_strong,
  eHTMLTag_sub,
  eHTMLTag_sup,
  eHTMLTag_tt,
  eHTMLTag_u,     
  eHTMLTag_var,     
  0};

static char gTableTags[]={ 
  eHTMLTag_caption, eHTMLTag_col, eHTMLTag_colgroup,  eHTMLTag_tbody,   
  eHTMLTag_tfoot,   eHTMLTag_tr,  eHTMLTag_thead,     eHTMLTag_td, 
  0};
  
static char  gWhitespaceTags[]={
  eHTMLTag_newline, eHTMLTag_whitespace,
  0};


/************************************************************************
  CTagStack class implementation.
  The reason we use this class is so that we can view the stack state 
  in the usual way via the debugger.
 ************************************************************************/


CTagStack::CTagStack(int aDefaultSize) {
#ifdef _dynstack
  mSize=aDefaultSize;
  mTags =new eHTMLTags[mSize];
  mBits =new PRBool[mSize];
#else
  mSize=eStackSize;
#endif
  mCount=0;
  mPrevious=0;
  nsCRT::zero(mTags,mSize*sizeof(eHTMLTags));
  nsCRT::zero(mBits,mSize*sizeof(PRBool));
}

/**
 * Default constructor
 * @update  gess7/9/98
 * @param   aDefaultsize tells the stack what size to start out.
 *          however, we'll autosize as needed.
 */
CTagStack::~CTagStack() {
#ifdef _dynstack
    delete mTags;
    delete mBits;
    mTags=0;
    mBits=0;
#endif
    mSize=mCount=0;
  }

/**
 * Resets state of stack to be empty.
 * @update  gess7/9/98
 */
void CTagStack::Empty(void) {
  mCount=0;
}

/**
 * 
 * @update  gess7/9/98
 * @param 
 * @return
 */
void CTagStack::Push(eHTMLTags aTag) {

  if(mCount>=mSize) {

#ifdef _dynstack
    eHTMLTags* tmp=new eHTMLTags[2*mSize];
    nsCRT::zero(tmp,2*mSize*sizeof(eHTMLTag_html));
    nsCRT::memcpy(tmp,mTags,mSize*sizeof(eHTMLTag_html));
    delete mTags;
    mTags=tmp;

    PRBool* tmp2=new PRBool[2*mSize];
    nsCRT::zero(tmp2,2*mSize*sizeof(PRBool));
    nsCRT::memcpy(tmp2,mBits,mSize*sizeof(PRBool));
    delete mBits;
    mBits=tmp2;
    mSize*=2;
#endif
  }
  mTags[mCount++]=aTag;
}

/**
 * 
 * @update  gess7/9/98
 * @param 
 * @return
 */
eHTMLTags CTagStack::Pop() {
  eHTMLTags result=eHTMLTag_unknown;
  if(mCount>0) {
    result=mTags[--mCount];
    mTags[mCount]=eHTMLTag_unknown;
    mBits[mCount]=PR_FALSE;
  }
  return result;
}

/**
 * 
 * @update  gess7/9/98
 * @param 
 * @return
 */
eHTMLTags CTagStack::First() const {
  if(mCount>0)
    return mTags[0];
  return eHTMLTag_unknown;
}

/**
 * 
 * @update  gess7/9/98
 * @param 
 * @return
 */
eHTMLTags CTagStack::Last() const {
  if(mCount>0)
    return mTags[mCount-1];
  return eHTMLTag_unknown;
}

/**************************************************************
  Now define the token deallocator class...
 **************************************************************/
class CNavTokenDeallocator: public nsDequeFunctor{
public:
  virtual void* operator()(void* anObject) {
    CToken* aToken = (CToken*)anObject;
    delete aToken;
    return 0;
  }
};
static CNavTokenDeallocator gTokenKiller;


/**************************************************************
  Now define the tokenrecycler class...
 **************************************************************/

/************************************************************************
  CTokenRecycler class implementation.
  This class is used to recycle tokens. 
  By using this simple class, we cut WAY down on the number of tokens
  that get created during the run of the system.
 ************************************************************************/
class nsCTokenRecycler : public nsITokenRecycler {
public:
  
//      enum {eCacheMaxSize=100}; 

                  nsCTokenRecycler();
  virtual         ~nsCTokenRecycler();
  virtual void    RecycleToken(CToken* aToken);
  virtual CToken* CreateTokenOfType(eHTMLTokenTypes aType,eHTMLTags aTag, const nsString& aString);

protected:
    nsDeque*  mTokenCache[eToken_last-1];
//    PRInt32   mTotals[eToken_last-1];
};


/**
 * 
 * @update  gess7/25/98
 * @param 
 */
nsCTokenRecycler::nsCTokenRecycler() : nsITokenRecycler() {
  int i=0;
  for(i=0;i<eToken_last-1;i++) {
    mTokenCache[i]=new nsDeque(gTokenKiller);
//    mTotals[i]=0;
  }
}

/**
 * Destructor for the token factory
 * @update  gess7/25/98
 */
nsCTokenRecycler::~nsCTokenRecycler() {
  //begin by deleting all the known (recycled) tokens...
  //We're also deleting the cache-deques themselves.
  int i;
  for(i=0;i<eToken_last-1;i++) {
    delete mTokenCache[i];
  }
}


/**
 * This method gets called when someone wants to recycle a token
 * @update  gess7/24/98
 * @param   aToken -- token to be recycled.
 * @return  nada
 */
void nsCTokenRecycler::RecycleToken(CToken* aToken) {
  if(aToken) {
    PRInt32 theType=aToken->GetTokenType();
    mTokenCache[theType-1]->Push(aToken);
  }
}


/**
 * 
 * @update	gess8/4/98
 * @param 
 * @return
 */
CToken* nsCTokenRecycler::CreateTokenOfType(eHTMLTokenTypes aType,eHTMLTags aTag, const nsString& aString) {

  CToken* result=(CToken*)mTokenCache[aType-1]->Pop();

  if(result) {
    result->Reinitialize(aTag,aString);
  }
  else {
//    mTotals[aType-1]++;
    switch(aType){
      case eToken_start:      result=new CStartToken(aTag); break;
      case eToken_end:        result=new CEndToken(aTag); break;
      case eToken_comment:    result=new CCommentToken(); break;
      case eToken_attribute:  result=new CAttributeToken(); break;
      case eToken_entity:     result=new CEntityToken(); break;
      case eToken_whitespace: result=new CWhitespaceToken(); break;
      case eToken_newline:    result=new CNewlineToken(); break;
      case eToken_text:       result=new CTextToken(aString); break;
      case eToken_script:     result=new CScriptToken(); break;
      case eToken_style:      result=new CStyleToken(); break;
      case eToken_skippedcontent: result=new CSkippedContentToken(aString); break;
        default:
          break;
    }
  }
  return result;
}


nsCTokenRecycler gTokenRecycler;


/************************************************************************
  And now for the main class -- CNavDTD...
 ************************************************************************/

/**
 *  This method gets called as part of our COM-like interfaces.
 *  Its purpose is to create an interface to parser object
 *  of some type.
 *  
 *  @update   gess 4/8/98
 *  @param    nsIID  id of object to discover
 *  @param    aInstancePtr ptr to newly discovered interface
 *  @return   NS_xxx result code
 */
nsresult CNavDTD::QueryInterface(const nsIID& aIID, void** aInstancePtr)  
{                                                                        
  if (NULL == aInstancePtr) {                                            
    return NS_ERROR_NULL_POINTER;                                        
  }                                                                      

  if(aIID.Equals(kISupportsIID))    {  //do IUnknown...
    *aInstancePtr = (nsIDTD*)(this);                                        
  }
  else if(aIID.Equals(kIDTDIID)) {  //do IParser base class...
    *aInstancePtr = (nsIDTD*)(this);                                        
  }
  else if(aIID.Equals(kClassIID)) {  //do this class...
    *aInstancePtr = (CNavDTD*)(this);                                        
  }                 
  else {
    *aInstancePtr=0;
    return NS_NOINTERFACE;
  }
  ((nsISupports*) *aInstancePtr)->AddRef();
  return NS_OK;                                                        
}


/**
 *  This method is defined in nsIParser. It is used to 
 *  cause the COM-like construction of an nsParser.
 *  
 *  @update  gess 4/8/98
 *  @param   nsIParser** ptr to newly instantiated parser
 *  @return  NS_xxx error result
 */
NS_HTMLPARS nsresult NS_NewNavHTMLDTD(nsIDTD** aInstancePtrResult)
{
  CNavDTD* it = new CNavDTD();

  if (it == 0) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  return it->QueryInterface(kClassIID, (void **) aInstancePtrResult);
}


NS_IMPL_ADDREF(CNavDTD)
NS_IMPL_RELEASE(CNavDTD)



/**
 *  
 *  
 *  @update  gess 6/9/98
 *  @param   
 *  @return  
 */
PRInt32 NavDispatchTokenHandler(CToken* aToken,nsIDTD* aDTD) {
  PRInt32         result=0;
  CHTMLToken*     theToken= (CHTMLToken*)(aToken);
  eHTMLTokenTypes theType= (eHTMLTokenTypes)theToken->GetTokenType();
  CNavDTD*        theDTD=(CNavDTD*)aDTD;
  
  if(aDTD) {
    switch(theType) {
      case eToken_start:
        result=theDTD->HandleStartToken(aToken); break;
      case eToken_end:
        result=theDTD->HandleEndToken(aToken); break;
      case eToken_comment:
        result=theDTD->HandleCommentToken(aToken); break;
      case eToken_entity:
        result=theDTD->HandleEntityToken(aToken); break;
      case eToken_whitespace:
        result=theDTD->HandleStartToken(aToken); break;
      case eToken_newline:
        result=theDTD->HandleStartToken(aToken); break;
      case eToken_text:
        result=theDTD->HandleStartToken(aToken); break;
      case eToken_attribute:
        result=theDTD->HandleAttributeToken(aToken); break;
      case eToken_style:
        result=theDTD->HandleStyleToken(aToken); break;
      case eToken_skippedcontent:
        result=theDTD->HandleSkippedContentToken(aToken); break;
      default:
        result=0;
    }//switch
  }//if
  return result;
}

/**
 *  init the set of default token handlers...
 *  
 *  @update  gess 3/25/98
 *  @param   
 *  @return  
 */
void CNavDTD::InitializeDefaultTokenHandlers() {
  AddTokenHandler(new CTokenHandler(NavDispatchTokenHandler,eToken_start));

  AddTokenHandler(new CTokenHandler(NavDispatchTokenHandler,eToken_end));
  AddTokenHandler(new CTokenHandler(NavDispatchTokenHandler,eToken_comment));
  AddTokenHandler(new CTokenHandler(NavDispatchTokenHandler,eToken_entity));

  AddTokenHandler(new CTokenHandler(NavDispatchTokenHandler,eToken_whitespace));
  AddTokenHandler(new CTokenHandler(NavDispatchTokenHandler,eToken_newline));
  AddTokenHandler(new CTokenHandler(NavDispatchTokenHandler,eToken_text));
  
  AddTokenHandler(new CTokenHandler(NavDispatchTokenHandler,eToken_attribute));
//  AddTokenHandler(new CTokenHandler(NavDispatchTokenHandler,eToken_script));
  AddTokenHandler(new CTokenHandler(NavDispatchTokenHandler,eToken_style));
  AddTokenHandler(new CTokenHandler(NavDispatchTokenHandler,eToken_skippedcontent));
}


/**
 *  Default constructor
 *  
 *  @update  gess 4/9/98
 *  @param   
 *  @return  
 */
CNavDTD::CNavDTD() : nsIDTD(), mContextStack(), mTokenDeque(gTokenKiller)  {
  NS_INIT_REFCNT();
  mParser=0;
  mSink = nsnull;
  mDTDDebug=0;
  mLineNumber=1;
  mParseMode=eParseMode_navigator;
  mStyleStack=new CTagStack();
  nsCRT::zero(mTokenHandlers,sizeof(mTokenHandlers));
  mHasOpenForm=PR_FALSE;
  mHasOpenMap=PR_FALSE;
  InitializeDefaultTokenHandlers();
}

/**
 *  Default destructor
 *  
 *  @update  gess 4/9/98
 *  @param   
 *  @return  
 */
CNavDTD::~CNavDTD(){
  DeleteTokenHandlers();
  delete mStyleStack;
  NS_IF_RELEASE(mDTDDebug);

}

/**
 * Call this method if you want the DTD to construct a fresh 
 * instance of itself. 
 * @update  gess7/23/98
 * @param 
 * @return
 */
nsresult CNavDTD::CreateNewInstance(nsIDTD** aInstancePtrResult){
  return NS_NewNavHTMLDTD(aInstancePtrResult);
}


/**
 * 
 * @update	gess8/4/98
 * @param 
 * @return
 */
nsITokenRecycler* CNavDTD::GetTokenRecycler(void){
//  return 0;
  return &gTokenRecycler;
}

/**
 * Called by the parser to initiate dtd verification of the
 * internal context stack.
 * @update  gess 7/23/98
 * @param 
 * @return
 */
PRBool CNavDTD::Verify(nsString& aURLRef){
  PRBool result=PR_TRUE;

  if(!mDTDDebug){;
    nsresult rval = NS_NewDTDDebug(&mDTDDebug);
    if (NS_OK != rval) {
      fputs("Cannot create parser debugger.\n", stdout);
      result=-PR_FALSE;
    }
    else mDTDDebug->SetVerificationDirectory(kVerificationDir);
  }
  if(mDTDDebug) {
    mDTDDebug->Verify(this,mParser,mContextStack.mCount,mContextStack.mTags,aURLRef);
  }
  return result;
}


/**
 * This method adds a new parser context to the list,
 * pushing the current one to the next position.
 * @update  gess7/22/98
 * @param   ptr to new context
 * @return  nada
 */
void CNavDTD::PushStack(CTagStack& aStack) {
  aStack.mPrevious=mStyleStack;  
  mStyleStack=&aStack;
}

/**
 * This method pops the topmost context off the stack,
 * returning it to the user. The next context  (if any)
 * becomes the current context.
 * @update  gess7/22/98
 * @return  prev. context
 */
CTagStack* CNavDTD::PopStack() {
  CTagStack* oldStack=mStyleStack;
  if(oldStack) {
    mStyleStack=oldStack->mPrevious;
  }
  return oldStack;
}

/**
 * This method is called to determine if the given DTD can parse
 * a document in a given source-type. 
 * NOTE: Parsing always assumes that the end result will involve
 *       storing the result in the main content model.
 * @update  gess6/24/98
 * @param   
 * @return  TRUE if this DTD can satisfy the request; FALSE otherwise.
 */
PRBool CNavDTD::CanParse(nsString& aContentType, PRInt32 aVersion){
  PRBool result=aContentType.Equals(kHTMLTextContentType);
  return result;
}

/**
 * 
 * @update  gess7/7/98
 * @param 
 * @return
 */
eAutoDetectResult CNavDTD::AutoDetectContentType(nsString& aBuffer,nsString& aType){
  eAutoDetectResult result=eUnknownDetect;
  if(PR_TRUE==aType.Equals(kHTMLTextContentType)) 
    result=eValidDetect;
  return result;
}

/**
 * 
 * @update  gess5/18/98
 * @param 
 * @return
 */
nsresult CNavDTD::WillBuildModel(nsString& aFilename,PRBool aNotifySink){
  nsresult result=NS_OK;

  mFilename=aFilename;

  if((aNotifySink) && (mSink)) {
    mLineNumber=1;
    result = mSink->WillBuildModel();
  }

  return result;
}

/**
 * 
 * @update  gess5/18/98
 * @param 
 * @return
 */
nsresult CNavDTD::DidBuildModel(PRInt32 anErrorCode,PRBool aNotifySink){
  nsresult result= NS_OK;

  if((kNoError==anErrorCode) && (mContextStack.mCount>0)) {
    result = CloseContainersTo(0,eHTMLTag_unknown,PR_FALSE);
  }

  if((aNotifySink) && (mSink)) {
    result = mSink->DidBuildModel(1);
  }

  if(mDTDDebug) {
    mDTDDebug->DumpVectorRecord();
  }
  return result;
}


/**
 *  This big dispatch method is used to route token handler calls to the right place.
 *  What's wrong with it? This table, and the dispatch methods themselves need to be 
 *  moved over to the delegate. Ah, so much to do...
 *  
 *  @update  gess 5/21/98
 *  @param   aType
 *  @param   aToken
 *  @param   aParser
 *  @return  
 */
nsresult CNavDTD::HandleToken(CToken* aToken){
  nsresult result=NS_OK;

  if(aToken) {
    CHTMLToken*     theToken= (CHTMLToken*)(aToken);
    eHTMLTokenTypes theType=eHTMLTokenTypes(theToken->GetTokenType());
    CITokenHandler* theHandler=GetTokenHandler(theType);

    if(theHandler) {
      result=(*theHandler)(theToken,this);
      if (mDTDDebug)
         mDTDDebug->Verify(this, mParser, mContextStack.mCount, mContextStack.mTags, mFilename);
    }

  }//if
  return result;
}



/**
 *  This method gets called when a start token has been 
 *  encountered in the parse process. If the current container
 *  can contain this tag, then add it. Otherwise, you have
 *  two choices: 1) create an implicit container for this tag
 *                  to be stored in
 *               2) close the top container, and add this to
 *                  whatever container ends up on top.
 *  
 *  @update  gess 3/25/98
 *  @param   aToken -- next (start) token to be handled
 *  @param   aNode -- CParserNode representing this start token
 *  @return  PR_TRUE if all went well; PR_FALSE if error occured
 */
nsresult CNavDTD::HandleDefaultStartToken(CToken* aToken,eHTMLTags aChildTag,nsIParserNode& aNode) {
  NS_PRECONDITION(0!=aToken,kNullToken);

  eHTMLTags theParentTag=GetTopNode();
  nsresult  result=NS_OK;
  PRBool    contains=CanContain(theParentTag,aChildTag);

  if(PR_FALSE==contains){
    if(CanPropagate(theParentTag,aChildTag))
      result=CreateContextStackFor(aChildTag);
    else result=kCantPropagate;
    if(NS_OK!=result) {
      //if you're here, then the new topmost container can't contain aToken.
      //You must determine what container hierarchy you need to hold aToken,
      //and create that on the parsestack.
      result=ReduceContextStackFor(aChildTag);
      if(PR_FALSE==CanContain(GetTopNode(),aChildTag)) {
        //we unwound too far; now we have to recreate a valid context stack.
        result=CreateContextStackFor(aChildTag);
      }
    }
  }

  if(IsContainer(aChildTag)){
    if(PR_TRUE==mContextStack.mBits[mContextStack.mCount-1]) {
      CloseTransientStyles(aChildTag);
    }
    result=OpenContainer(aNode,PR_TRUE);
  }
  else {
    if(PR_FALSE==mContextStack.mBits[mContextStack.mCount-1]) {
      OpenTransientStyles(aChildTag);
    }
    result=AddLeaf(aNode);
  }
  return result;
}

/**
 *  This method gets called when a start token has been 
 *  encountered in the parse process. If the current container
 *  can contain this tag, then add it. Otherwise, you have
 *  two choices: 1) create an implicit container for this tag
 *                  to be stored in
 *               2) close the top container, and add this to
 *                  whatever container ends up on top.
 *  
 *  @update  gess 3/25/98
 *  @param   aToken -- next (start) token to be handled
 *  @param   aNode -- CParserNode representing this start token
 *  @return  PR_TRUE if all went well; PR_FALSE if error occured
 */
nsresult CNavDTD::HandleStartToken(CToken* aToken) {
  NS_PRECONDITION(0!=aToken,kNullToken);

  CStartToken*  st= (CStartToken*)(aToken);
  eHTMLTags     tokenTagType=(eHTMLTags)st->GetTypeID();

  //Begin by gathering up attributes...
  nsCParserNode attrNode((CHTMLToken*)aToken,mLineNumber); 
  PRInt16       attrCount=aToken->GetAttributeCount();
  PRInt32       theCount;
  nsresult      result=(0==attrCount)
    ? NS_OK
    : CollectAttributes(attrNode,attrCount);

  if(NS_OK==result) {
      //now check to see if this token should be omitted...
    if(PR_FALSE==CanOmit(GetTopNode(),tokenTagType)) {
  
      switch(tokenTagType) {

        case eHTMLTag_html:
          result=OpenHTML(attrNode); break;

        case eHTMLTag_title:
          {
            nsCParserNode theNode(st,mLineNumber);
            result=OpenHead(theNode); //open the head...
            if(NS_OK==result) {
              result=CollectSkippedContent(attrNode,theCount);
              mSink->SetTitle(attrNode.GetSkippedContent());
              result=CloseHead(theNode); //close the head...
            }
          }
          break;

        case eHTMLTag_textarea:
          {
            CollectSkippedContent(attrNode,theCount);
            result=AddLeaf(attrNode);
          }
          break;

        case eHTMLTag_form:
          result = OpenForm(attrNode);
          break;

        case eHTMLTag_meta:
        case eHTMLTag_link:
          {
            nsCParserNode theNode((CHTMLToken*)aToken,mLineNumber);
            result=OpenHead(theNode);
            if(NS_OK==result)
              result=AddLeaf(attrNode);
            if(NS_OK==result)
              result=CloseHead(theNode);
          }
          break;

        case eHTMLTag_style:
          {
            nsCParserNode theNode((CHTMLToken*)aToken,mLineNumber);
            result=OpenHead(theNode);
            if(NS_OK==result) {
              CollectSkippedContent(attrNode,theCount);
              if(NS_OK==result) {
                result=AddLeaf(attrNode);
                if(NS_OK==result)
                  result=CloseHead(theNode);
              }
            }
          }
          break;

        case eHTMLTag_script:
          result=HandleScriptToken(st, attrNode); break;

        case eHTMLTag_head:
          break; //ignore head tags...

        case eHTMLTag_base:
          result=OpenHead(attrNode);
          if(NS_OK==result) {
            result=AddLeaf(attrNode);
            if(NS_OK==result)
              result=CloseHead(attrNode);
          }
          break;

        case eHTMLTag_area:
          if (mHasOpenMap) {
            result = mSink->AddLeaf(attrNode);
          }
          break;

        case eHTMLTag_map:
          result = OpenMap(attrNode);
          break;

        default:
          result=HandleDefaultStartToken(aToken,tokenTagType,attrNode);
          break;
      } //switch
    } //if
  } //if

  if(eHTMLTag_newline==tokenTagType)
    mLineNumber++;

  return result;
}


/**
 *  This method gets called when an end token has been 
 *  encountered in the parse process. If the end tag matches
 *  the start tag on the stack, then simply close it. Otherwise,
 *  we have a erroneous state condition. This can be because we
 *  have a close tag with no prior open tag (user error) or because
 *  we screwed something up in the parse process. I'm not sure
 *  yet how to tell the difference.
 *  
 *  @update  gess 3/25/98
 *  @param   aToken -- next (start) token to be handled
 *  @return  PR_TRUE if all went well; PR_FALSE if error occured
 */
nsresult CNavDTD::HandleEndToken(CToken* aToken) {
  NS_PRECONDITION(0!=aToken,kNullToken);

  nsresult    result=NS_OK;
  CEndToken*  et = (CEndToken*)(aToken);
  eHTMLTags   tokenTagType=(eHTMLTags)et->GetTypeID();


  // Here's the hacky part:
  // Because we're trying to be backward compatible with Nav4/5, 
  // we have to handle explicit styles the way it does. That means
  // that we keep an internal style stack.When an EndToken occurs, 
  // we should see if it is an explicit style tag. If so, we can 
  // close the explicit style tag (goofy, huh?)


    //now check to see if this token should be omitted, or 
    //if it's gated from closing by the presence of another tag.
  if(PR_TRUE==CanOmitEndTag(GetTopNode(),tokenTagType)) {
    UpdateStyleStackForCloseTag(tokenTagType,tokenTagType);
    return result;
  }

  nsCParserNode theNode((CHTMLToken*)aToken,mLineNumber);
  switch(tokenTagType) {

    case eHTMLTag_style:
    case eHTMLTag_link:
    case eHTMLTag_meta:
    case eHTMLTag_textarea:
    case eHTMLTag_title:
    case eHTMLTag_head:
    case eHTMLTag_script:
      break;

    case eHTMLTag_map:
    case eHTMLTag_form:
      {
        nsCParserNode aNode((CHTMLToken*)aToken,mLineNumber);
        result=CloseContainer(aNode,tokenTagType,PR_FALSE);
      }
      break;

    case eHTMLTag_td:
    case eHTMLTag_th:
      result=CloseContainersTo(tokenTagType,PR_TRUE); 
      // Empty the transient style stack (we just closed any extra
      // ones off so it's safe to do it now) because they don't carry
      // forward across table cell boundaries.
      mStyleStack->mCount=0;
      break;

    default:
      if(IsContainer(tokenTagType)){
        result=CloseContainersTo(tokenTagType,PR_TRUE); 
      }
      //
      break;
  }
  return result;
}

/**
 *  This method gets called when an entity token has been 
 *  encountered in the parse process. 
 *  
 *  @update  gess 3/25/98
 *  @param   aToken -- next (start) token to be handled
 *  @return  PR_TRUE if all went well; PR_FALSE if error occured
 */
nsresult CNavDTD::HandleEntityToken(CToken* aToken) {
  NS_PRECONDITION(0!=aToken,kNullToken);

  CEntityToken* et = (CEntityToken*)(aToken);
  nsresult      result=NS_OK;
  eHTMLTags     tokenTagType=(eHTMLTags)et->GetTypeID();

  if(PR_FALSE==CanOmit(GetTopNode(),tokenTagType)) {
    nsCParserNode aNode((CHTMLToken*)aToken,mLineNumber);
    result=AddLeaf(aNode);
  }
  return result;
}

/**
 *  This method gets called when a comment token has been 
 *  encountered in the parse process. After making sure
 *  we're somewhere in the body, we handle the comment
 *  in the same code that we use for text.
 *  
 *  @update  gess 3/25/98
 *  @param   aToken -- next (start) token to be handled
 *  @return  PR_TRUE if all went well; PR_FALSE if error occured
 */
nsresult CNavDTD::HandleCommentToken(CToken* aToken) {
  NS_PRECONDITION(0!=aToken,kNullToken);
  return NS_OK;
}

/**
 *  This method gets called when a skippedcontent token has 
 *  been encountered in the parse process. After verifying 
 *  that the topmost container can contain text, we call 
 *  AddLeaf to store this token in the top container.
 *  
 *  @update  gess 3/25/98
 *  @param   aToken -- next (start) token to be handled
 *  @return  PR_TRUE if all went well; PR_FALSE if error occured
 */
nsresult CNavDTD::HandleSkippedContentToken(CToken* aToken) {
  NS_PRECONDITION(0!=aToken,kNullToken);

  nsresult result=NS_OK;

  if(HasOpenContainer(eHTMLTag_body)) {
    nsCParserNode aNode((CHTMLToken*)aToken,mLineNumber);
    result=AddLeaf(aNode);
  }
  return result;
}

/**
 *  This method gets called when an attribute token has been 
 *  encountered in the parse process. This is an error, since
 *  all attributes should have been accounted for in the prior
 *  start or end tokens
 *  
 *  @update  gess 3/25/98
 *  @param   aToken -- next (start) token to be handled
 *  @return  PR_TRUE if all went well; PR_FALSE if error occured
 */
nsresult CNavDTD::HandleAttributeToken(CToken* aToken) {
  NS_PRECONDITION(0!=aToken,kNullToken);
  NS_ERROR("attribute encountered -- this shouldn't happen!");

  return NS_OK;
}

/**
 *  This method gets called when a script token has been 
 *  encountered in the parse process. 
 *  
 *  @update  gess 3/25/98
 *  @param   aToken -- next (start) token to be handled
 *  @return  PR_TRUE if all went well; PR_FALSE if error occured
 */
nsresult CNavDTD::HandleScriptToken(CToken* aToken, nsCParserNode& aNode) {
  NS_PRECONDITION(0!=aToken,kNullToken);
  nsresult result=NS_OK;

  PRInt32 pos=GetTopmostIndexOf(eHTMLTag_body);
  PRInt32 attrCount=aToken->GetAttributeCount();

  if (kNotFound == pos) {
    // We're in the HEAD
    result=OpenHead(aNode);
    if(NS_OK==result) {
      CollectSkippedContent(aNode,attrCount);
      if(NS_OK==result) {
          //Boy is this an evil hack...
        mContextStack.Pop();
        result=AddLeaf(aNode);
          mContextStack.Push(eHTMLTag_head);
          if(NS_OK==result)
          result=CloseHead(aNode);
      }
    }
  }
  else {
    // We're in the BODY
    CollectSkippedContent(aNode,attrCount);
    if(NS_OK==result) {
      result=AddLeaf(aNode);
    }
  }

  return result;
}

/**
 *  This method gets called when a style token has been 
 *  encountered in the parse process. 
 *  
 *  @update  gess 3/25/98
 *  @param   aToken -- next (start) token to be handled
 *  @return  PR_TRUE if all went well; PR_FALSE if error occured
 */
nsresult CNavDTD::HandleStyleToken(CToken* aToken){
  NS_PRECONDITION(0!=aToken,kNullToken);

//  CStyleToken*  st = (CStyleToken*)(aToken);
  return NS_OK;
}


/**
 * Retrieve the attributes for this node, and add then into
 * the node.
 *
 * @update  gess4/22/98
 * @param   aNode is the node you want to collect attributes for
 * @param   aCount is the # of attributes you're expecting
 * @return error code (should be 0)
 */
PRInt32 CNavDTD::CollectAttributes(nsCParserNode& aNode,PRInt32 aCount){
/*
  nsDequeIterator end=mParserContext->mTokenDeque.End();

  int attr=0;
  for(attr=0;attr<aCount;attr++) {
    if(*mParserContext->mCurrentPos<end) {
      CToken* tkn=(CToken*)(++(*mParserContext->mCurrentPos));
      if(tkn){
        if(eToken_attribute==eHTMLTokenTypes(tkn->GetTokenType())){
          aNode.AddAttribute(tkn);
        } 
        else (*mParserContext->mCurrentPos)--;
      }
      else return kInterrupted;
    }
    else return kInterrupted;
  }
*/
  int attr=0;
  for(attr=0;attr<aCount;attr++){
    CToken* theToken=mParser->PeekToken();
    if(theToken)  {
      eHTMLTokenTypes theType=eHTMLTokenTypes(theToken->GetTokenType());
      if(eToken_attribute==theType){
        mParser->PopToken(); //pop it for real...
        aNode.AddAttribute(theToken);
      } 
    }
    else return kInterrupted;
  }
  return kNoError;
}


/**
 * Causes the next skipped-content token (if any) to
 * be consumed by this node.
 * @update  gess5/11/98
 * @param   node to consume skipped-content
 * @param   holds the number of skipped content elements encountered
 * @return  Error condition.
 */
PRInt32 CNavDTD::CollectSkippedContent(nsCParserNode& aNode,PRInt32& aCount) {
  PRInt32         result=kNoError;
  eHTMLTokenTypes theType;
  CToken*         theToken;
  aCount=0;
  do{
    CToken* theToken=mParser->PeekToken();
    if(theToken) {
      theType=eHTMLTokenTypes(theToken->GetTokenType());
      if(eToken_skippedcontent==theType) {
        mParser->PopToken();
        aNode.SetSkippedContent(theToken);
        aCount++;
      } 
    }
  } while(theToken && (eToken_skippedcontent==theType));

/*
  eHTMLTokenTypes   subtype=eToken_attribute;
  nsDequeIterator   end=mParserContext->mTokenDeque.End();

  aCount=0;
  while((*mParserContext->mCurrentPos!=end) && (eToken_attribute==subtype)) {
    CToken* tkn=(CToken*)(++(*mParserContext->mCurrentPos));
    subtype=eHTMLTokenTypes(tkn->GetTokenType());
    if(eToken_skippedcontent==subtype) {
      aNode.SetSkippedContent(tkn);
      aCount++;
    } 
    else (*mParserContext->mCurrentPos)--;
  }
*/
  return result;
}

/**
 *  Finds a tag handler for the given tag type, given in string.
 *  
 *  @update  gess 4/2/98
 *  @param   aString contains name of tag to be handled
 *  @return  valid tag handler (if found) or null
 */
void CNavDTD::DeleteTokenHandlers(void) {
  for(int i=eToken_unknown;i<eToken_last;i++){
    delete mTokenHandlers[i];
    mTokenHandlers[i]=0;
  }
}


/**
 *  Finds a tag handler for the given tag type.
 *  
 *  @update  gess 4/2/98
 *  @param   aTagType type of tag to be handled
 *  @return  valid tag handler (if found) or null
 */
CITokenHandler* CNavDTD::GetTokenHandler(eHTMLTokenTypes aType) const {
  CITokenHandler* result=0;
  if((aType>0) && (aType<eToken_last)) {
    result=mTokenHandlers[aType];
  } 
  else {
  }
  return result;
}


/**
 *  Register a handler.
 *  
 *  @update  gess 4/2/98
 *  @param   
 *  @return  
 */
CITokenHandler* CNavDTD::AddTokenHandler(CITokenHandler* aHandler) {
  NS_ASSERTION(0!=aHandler,"Error: Null handler");
  
  if(aHandler)  {
    eHTMLTokenTypes type=(eHTMLTokenTypes)aHandler->GetTokenType();
    if(type<eToken_last) {
      mTokenHandlers[type]=aHandler;
    }
    else {
      //add code here to handle dynamic tokens...
    }
  }
  return 0;
}

/**
 *  The parser calls this method after it's selected
 *  an constructed a DTD.
 *  
 *  @update  gess 3/25/98
 *  @param   aParser is a ptr to the controlling parser.
 *  @return  nada
 */
void CNavDTD::SetParser(nsIParser* aParser) {
  mParser=(nsParser*)aParser;
  if(aParser)
    mParseMode=aParser->GetParseMode();
//  mParseMode=eParseMode_noquirks;
}


/**
 *  This method gets called in order to set the content
 *  sink for this parser to dump nodes to.
 *  
 *  @update  gess 3/25/98
 *  @param   nsIContentSink interface for node receiver
 *  @return  
 */
nsIContentSink* CNavDTD::SetContentSink(nsIContentSink* aSink) {
  nsIContentSink* old=mSink;
  mSink=(nsIHTMLContentSink*)aSink;
  return old;
}


/**
 *  This method is called to determine whether or not a tag
 *  can contain an explict style tag (font, italic, bold, etc.)
 *  Most can -- but some, like option, cannot. Therefore we
 *  don't bother to open transient styles within these elements.
 *  
 *  @update  gess 4/8/98
 *  @param   aParent -- tag enum of parent container
 *  @param   aChild -- tag enum of child container
 *  @return  PR_TRUE if parent can contain child
 */
PRBool CNavDTD::CanContainStyles(eHTMLTags aParent) const {
  PRBool result=PR_TRUE;
  switch(aParent) {
    case eHTMLTag_option:
      result=PR_FALSE; break;
    default:
      break;
  }
  return result;
}

/**
 *  This method is called to determine whether or not a 
 *  form-type tag can contain a tag of another form-type tag.
 *  
 *  @update  gess 4/8/98
 *  @param   aParent -- tag enum of parent container
 *  @param   aChild -- tag enum of child container
 *  @return  PR_TRUE if parent can contain child
 */
PRBool CNavDTD::CanContainFormElement(eHTMLTags aParent,eHTMLTags aChild) const {
  PRBool result=PR_FALSE;

  if(mParser && HasOpenContainer(eHTMLTag_form)) {
    eHTMLTags topTag=GetTopNode();
    switch(aChild) {
      case eHTMLTag_option:
        result=PRBool(eHTMLTag_select==topTag); break;
      default:
        result=PR_TRUE;
        break;
    } //switch
  }//if
  return result;
}

 /***********************************************************************************
   The following tables determine the set of elements each tag can contain...
  ***********************************************************************************/

static char  gTagSet1[]={ 
  eHTMLTag_a,         eHTMLTag_acronym,   eHTMLTag_address,   eHTMLTag_applet,
  eHTMLTag_blink,     eHTMLTag_b,         eHTMLTag_basefont,  eHTMLTag_bdo,
  eHTMLTag_big,
  eHTMLTag_blockquote,eHTMLTag_br,        eHTMLTag_button,    eHTMLTag_center,
  eHTMLTag_cite,      eHTMLTag_code,      eHTMLTag_dfn,       eHTMLTag_dir,
  eHTMLTag_div,       eHTMLTag_dl,        eHTMLTag_em,        eHTMLTag_fieldset,
  eHTMLTag_embed,
  eHTMLTag_font,      eHTMLTag_form,      eHTMLTag_h1,        eHTMLTag_h2,
  eHTMLTag_h3,        eHTMLTag_h4,        eHTMLTag_h5,        eHTMLTag_h6,
  eHTMLTag_hr,        eHTMLTag_i,         eHTMLTag_iframe,    eHTMLTag_img,
  eHTMLTag_input,     eHTMLTag_isindex,   
  
  eHTMLTag_kbd,       eHTMLTag_label,     eHTMLTag_li,
  eHTMLTag_map,       eHTMLTag_menu,      eHTMLTag_newline,   eHTMLTag_nobr,
  eHTMLTag_noframes,  eHTMLTag_noscript,
  eHTMLTag_object,    eHTMLTag_ol,        eHTMLTag_p,         eHTMLTag_pre,
  eHTMLTag_q,         eHTMLTag_s,         eHTMLTag_strike,    
  eHTMLTag_samp,      eHTMLTag_script,    eHTMLTag_select,    eHTMLTag_small,     
  eHTMLTag_spacer,    eHTMLTag_span,      eHTMLTag_strong,
  eHTMLTag_sub,       eHTMLTag_sup,       eHTMLTag_table,     eHTMLTag_text,
  
  eHTMLTag_textarea,  eHTMLTag_tt,        eHTMLTag_u,         eHTMLTag_ul,        
  eHTMLTag_userdefined,   eHTMLTag_var,   eHTMLTag_wbr,
  eHTMLTag_whitespace,
  0};

static char  gTagSet2[]={ 
  eHTMLTag_a,         eHTMLTag_acronym,   eHTMLTag_applet,    eHTMLTag_blink,
  eHTMLTag_b,
  eHTMLTag_basefont,  eHTMLTag_bdo,       eHTMLTag_big,       eHTMLTag_br,
  eHTMLTag_button,    eHTMLTag_cite,      eHTMLTag_code,      eHTMLTag_dfn,
  eHTMLTag_div,       eHTMLTag_em,        eHTMLTag_font,      eHTMLTag_hr,        
  eHTMLTag_embed,
  eHTMLTag_i,         eHTMLTag_iframe,    eHTMLTag_img,       eHTMLTag_input,     
  eHTMLTag_kbd,       

  eHTMLTag_label,     eHTMLTag_map,       eHTMLTag_newline,   eHTMLTag_nobr,
  eHTMLTag_object,    eHTMLTag_p, 
  eHTMLTag_q,         eHTMLTag_s,         eHTMLTag_strike,    
  eHTMLTag_samp,      eHTMLTag_script,    eHTMLTag_select,    eHTMLTag_small,     
  eHTMLTag_spacer,    eHTMLTag_span,      eHTMLTag_strong,    
  eHTMLTag_sub,       eHTMLTag_sup,       eHTMLTag_text,      eHTMLTag_textarea,  

  eHTMLTag_table,// XXX kipp was here

  eHTMLTag_tt,        eHTMLTag_u,         eHTMLTag_userdefined, eHTMLTag_var,       
  eHTMLTag_wbr,       eHTMLTag_whitespace,
  0};

static char  gTagSet3[]={ 
  eHTMLTag_a,         eHTMLTag_acronym,   eHTMLTag_applet,    eHTMLTag_blink,
  eHTMLTag_b,
  eHTMLTag_bdo,       eHTMLTag_big,       eHTMLTag_br,        eHTMLTag_blockquote,
  eHTMLTag_body,      eHTMLTag_caption,   eHTMLTag_center,    eHTMLTag_cite,
  eHTMLTag_code,      eHTMLTag_dd,        eHTMLTag_del,       eHTMLTag_dfn,        
  eHTMLTag_div,       eHTMLTag_dt,        eHTMLTag_em,        eHTMLTag_fieldset,    
  eHTMLTag_embed,
  eHTMLTag_font,      eHTMLTag_form,      eHTMLTag_h1,        eHTMLTag_h2,
  eHTMLTag_h3,        eHTMLTag_h4,        eHTMLTag_h5,        eHTMLTag_h6,
  eHTMLTag_i,         eHTMLTag_iframe,    eHTMLTag_ins,       eHTMLTag_kbd,       

  eHTMLTag_label,     eHTMLTag_legend,    eHTMLTag_li,        eHTMLTag_newline,
      
  eHTMLTag_noframes,
  eHTMLTag_noscript,  eHTMLTag_object,    eHTMLTag_p,         eHTMLTag_pre,
  eHTMLTag_q,         eHTMLTag_s,         eHTMLTag_strike,    
  eHTMLTag_samp,      eHTMLTag_small,     eHTMLTag_spacer,
  eHTMLTag_span,      eHTMLTag_strong,    eHTMLTag_sub,       eHTMLTag_sup,   
  eHTMLTag_td,        eHTMLTag_text,
  
  eHTMLTag_th,        eHTMLTag_tt,        eHTMLTag_u,         eHTMLTag_userdefined,
  eHTMLTag_var,       eHTMLTag_wbr,       eHTMLTag_whitespace,
  0};

 /***********************************************************************************
   The preceeding tables determine the set of elements each tag can contain...
  ***********************************************************************************/

/**
 *  This method is called to determine whether or not a tag
 *  of one type can contain a tag of another type.
 *  
 *  @update  gess 4/8/98
 *  @param   aParent -- tag enum of parent container
 *  @param   aChild -- tag enum of child container
 *  @return  PR_TRUE if parent can contain child
 */
PRBool CNavDTD::CanContain(PRInt32 aParent,PRInt32 aChild) const {

  PRBool result=PR_FALSE;

    //This hack code is here because we don't yet know what to do
    //with userdefined tags...  XXX Hack
  if(eHTMLTag_userdefined==aChild)  // XXX Hack: For now...
    result=PR_TRUE;

    //handle form elements (this is very much a WIP!!!)
  if(0!=strchr(formElementTags,aChild)){
    return CanContainFormElement((eHTMLTags)aParent,(eHTMLTags)aChild);
  }

  if((aParent) && (0 != strchr(gStyleTags, aParent))) {
    if(eHTMLTag_li == aChild) {
      //This code was added to enforce the rule that listitems
      //autoclose prior listitems.  Stylistic tags (including <A>)
      //that get in the way are simply out of luck.
      result=PR_FALSE;
    }
    else
      result=PRBool(0!=strchr(gTagSet1,aChild));
  }
  else {
    switch((eHTMLTags)aParent) {
    case eHTMLTag_address:
      result=PRBool(0!=strchr(gTagSet2,aChild));
      break;

    case eHTMLTag_applet:
      if(eHTMLTag_param==aChild)
        result=PR_TRUE;
      else
        result=PRBool(0!=strchr(gTagSet2,aChild)); 
      break;

    case eHTMLTag_area:
    case eHTMLTag_base:
    case eHTMLTag_basefont:
    case eHTMLTag_br:
    case eHTMLTag_embed:
    case eHTMLTag_hr:      
    case eHTMLTag_img:
    case eHTMLTag_input:
    case eHTMLTag_isindex:
    case eHTMLTag_meta:
    case eHTMLTag_spacer:
    case eHTMLTag_wbr:
      break;    //singletons can't contain anything...

    case eHTMLTag_blockquote:
    case eHTMLTag_body:
      if(eHTMLTag_userdefined==aChild)
        result=PR_TRUE;
      else
        result=PRBool(0!=strchr(gTagSet1,aChild)); 
      break;

    case eHTMLTag_button:
      result=PRBool(0!=strchr(gTagSet3,aChild));
      break;

    case eHTMLTag_caption:
      result=PRBool(0!=strchr(gTagSet1,aChild));
      break;

    case eHTMLTag_center:
      result=PRBool(0!=strchr(gTagSet1,aChild));
      break;

    case eHTMLTag_col:
    case eHTMLTag_colgroup:
      break;    //singletons can't contain anything...

    case eHTMLTag_dt:
      {
        static char  datalistTags[]={eHTMLTag_dt,eHTMLTag_dd,0};

        if(0!=strchr(datalistTags,aChild)) {
          result=PR_TRUE;
        }
        else
          result=PRBool(0!=strchr(gTagSet1,aChild));
      }
      break;

    case eHTMLTag_dd:
    case eHTMLTag_div:
      result=PRBool(0!=strchr(gTagSet1,aChild)); break;

    case eHTMLTag_dl:
      {
        char okTags[]={eHTMLTag_dd,eHTMLTag_dt,eHTMLTag_whitespace,
                       eHTMLTag_newline,eHTMLTag_p,0};
        result=PRBool(0!=strchr(okTags,aChild));
      }
      break;

    case eHTMLTag_fieldset:
      if(eHTMLTag_legend==aChild)
        result=PR_TRUE;
      else
        result=PRBool(0!=strchr(gTagSet1,aChild)); 
      break;

    case eHTMLTag_form:
      result=PRBool(0!=strchr(gTagSet1,aChild));
      break;

    case eHTMLTag_frame:
      break;  //singletons can't contain other tags

    case eHTMLTag_frameset:
      {
        static char okTags[]={eHTMLTag_frame,eHTMLTag_frameset,
                              eHTMLTag_noframes,
                              eHTMLTag_newline,eHTMLTag_whitespace,0};
        result=PRBool(0!=strchr(okTags,aChild));
      }
      break;

    case eHTMLTag_h1: case eHTMLTag_h2:
    case eHTMLTag_h3: case eHTMLTag_h4:
    case eHTMLTag_h5: case eHTMLTag_h6:
      {
        if(0!=strchr(gHeadingTags,aChild))
          result=PR_FALSE;
        else result=PRBool(0!=strchr(gTagSet1,aChild)); 
      }
      break;

    case eHTMLTag_head:
      {
        static char  okTags[]={
          eHTMLTag_base,    eHTMLTag_isindex, eHTMLTag_link,  eHTMLTag_meta,
          eHTMLTag_script,  eHTMLTag_style,   eHTMLTag_title, 0};
        result=PRBool(0!=strchr(okTags,aChild));
      }
      break;

    case eHTMLTag_html:
      {
        static char okTags[]={eHTMLTag_body,eHTMLTag_frameset,eHTMLTag_head,0};
        result=PRBool(0!=strchr(okTags,aChild));
      }
      break;

    case eHTMLTag_iframe:/* XXX wrong */
      if(eHTMLTag_frame==aChild)
        result=PR_FALSE;
      else
        result=PRBool(0!=strchr(gTagSet1,aChild)); 
      break;

    case eHTMLTag_label:
    case eHTMLTag_legend:/* XXX not sure */
      result=PRBool(0!=strchr(gTagSet1,aChild));
      break;

    case eHTMLTag_layer:
    case eHTMLTag_link:
      break;    //singletons can't contain anything...

    case eHTMLTag_li:
      if (eHTMLTag_li == aChild) {
        result = PR_FALSE;
      }
      else result=PRBool(!strchr(gHeadingTags,aChild));
      break;

    case eHTMLTag_listing:
      result = PR_TRUE;
      break;

    case eHTMLTag_map:
      {
        static char okTags[] = {eHTMLTag_area,
                                eHTMLTag_newline,
                                eHTMLTag_whitespace,
                                0};
        result = PRBool(0 != strchr(okTags, aChild));
      }
      break;

    case eHTMLTag_menu:
    case eHTMLTag_dir:
    case eHTMLTag_ol:
    case eHTMLTag_ul:
      // XXX kipp was here
      result=PRBool(0 != strchr(gTagSet1,aChild));
      break;

    case eHTMLTag_noframes:
      if(eHTMLTag_body==aChild)
        result=PR_TRUE;
      else
        result=PRBool(0!=strchr(gTagSet1,aChild)); 
      break;

    case eHTMLTag_noscript:
      result=PRBool(0!=strchr(gTagSet1,aChild));
      break;

    case eHTMLTag_option:
      //for now, allow an option to contain anything but another option...
      result=PRBool(eHTMLTag_option!=aChild);
      break;

    case eHTMLTag_p:
      {
        static char  datalistTags[]={eHTMLTag_dt,eHTMLTag_dd,0};

        if(eHTMLTag_p==aChild)
          result=PR_FALSE;
        else if(0!=strchr(datalistTags,aChild)) {
          //we now allow DT/DD inside a paragraph, so long as a DL is open...
          if(PR_TRUE==HasOpenContainer(eHTMLTag_dl)) {
            if(PR_TRUE==HasOpenContainer(eHTMLTag_dt))
              result=PR_FALSE;
          } else
            result=PR_TRUE;
        }
        else
          result=PRBool(0!=strchr(gTagSet2,aChild));
      }
      break;

    case eHTMLTag_object:
    case eHTMLTag_pre:
      result=PRBool(0!=strchr(gTagSet2,aChild));
      break;

    case eHTMLTag_param:
      break;  //singletons can't contain other tags

    case eHTMLTag_plaintext:
      break;

    case eHTMLTag_script:
      break; //unadorned script text...

    case eHTMLTag_select:
      result=PR_TRUE; //for now, allow select to contain anything...
      break;

    case eHTMLTag_style:
      break;  //singletons can't contain other tags

    case eHTMLTag_table:
      {
        static char  okTags[]={ 
          eHTMLTag_caption, eHTMLTag_col, eHTMLTag_colgroup,eHTMLTag_tbody,   
          eHTMLTag_tfoot,  /* eHTMLTag_tr,*/  eHTMLTag_thead,   0};
        result=PRBool(0!=strchr(okTags,aChild));
      }
      break;

    case eHTMLTag_tbody:
    case eHTMLTag_tfoot:
    case eHTMLTag_thead:
      result=PRBool(eHTMLTag_tr==aChild);
      break;

    case eHTMLTag_th:
    case eHTMLTag_td:
      {
        static char  extraTags[]={eHTMLTag_newline,0};
        result=PRBool(0!=strchr(extraTags,aChild));
        if(PR_FALSE==result)
          result=PRBool(0!=strchr(gTagSet1,aChild)); 
      }
      break;

    case eHTMLTag_textarea:
    case eHTMLTag_title:
      break; //nothing but plain text...

    case eHTMLTag_tr:
      {
        static char  okTags[]={eHTMLTag_td,eHTMLTag_th,0};
        result=PRBool(0!=strchr(okTags,aChild)); 
      }
      break;

    case eHTMLTag_userdefined:
      result=PR_TRUE; //XXX for now...
      break;

    case eHTMLTag_xmp: 
      break;

    default:
#ifdef NS_DEBUG
      printf("XXX: unhandled tag %s in CanContain switch statement\n",
             NS_EnumToTag((nsHTMLTag)aParent));
#endif
      break;
    } //switch
  } //if

  return result;
}

/**
 *  This method is called to determine whether or not 
 *  the necessary intermediate tags should be propagated
 *  between the given parent and given child.
 *  
 *  @update  gess 4/8/98
 *  @param   aParent -- tag enum of parent container
 *  @param   aChild -- tag enum of child container
 *  @return  PR_TRUE if propagation should occur
 */
PRBool CNavDTD::CanPropagate(eHTMLTags aParent,eHTMLTags aChild) const {
  PRBool result=PR_TRUE;

  switch(aParent) {
    case eHTMLTag_td:
      result=CanContain(aParent,aChild);
      break;

    default:
      break;
  }//switch
  return result;
}

/**
 *  This method is called to determine whether or not a tag
 *  of one type can contain a tag of another type.
 *  
 *  @update  gess 4/8/98
 *  @param   aParent -- tag enum of parent container
 *  @param   aChild -- tag enum of child container
 *  @return  PR_TRUE if parent can contain child
 */
PRBool CNavDTD::CanContainIndirect(eHTMLTags aParent,eHTMLTags aChild) const {
  PRBool result=PR_FALSE;

  switch(aParent) {

    case eHTMLTag_html:
      {
        static char  okTags[]={
          eHTMLTag_head,    eHTMLTag_body, 0
        };
        result=PRBool(0!=strchr(okTags,aChild));
      }

    case eHTMLTag_body:
      result=PR_TRUE; break;

    case eHTMLTag_table:
      {
        static char  okTags[]={
          eHTMLTag_caption, eHTMLTag_colgroup,
          eHTMLTag_tbody,   eHTMLTag_tfoot,
          eHTMLTag_thead,   eHTMLTag_tr,
          eHTMLTag_td,      eHTMLTag_th,
          eHTMLTag_col,     0};
        result=PRBool(0!=strchr(okTags,aChild));
      }
      break;

    default:
      break;
  }
  return result;
}

/**
 *  This method gets called to determine whether a given 
 *  tag can contain newlines. Most do not.
 *  
 *  @update  gess 3/25/98
 *  @param   aTag -- tag to test for containership
 *  @return  PR_TRUE if given tag can contain other tags
 */
PRBool CNavDTD::CanOmit(eHTMLTags aParent,eHTMLTags aChild) const {
  PRBool result=PR_FALSE;

  //begin with some simple (and obvious) cases...
  switch(aParent) {
    case eHTMLTag_table:
      if(eHTMLTag_form==aChild)
        result=PR_FALSE;
      else result=PRBool(!strchr(gTableTags,aChild));
      break;

    case eHTMLTag_tr:
      switch(aChild) {
        case eHTMLTag_td:
        case eHTMLTag_th:
        case eHTMLTag_form:
        case eHTMLTag_tr:
          result=PR_FALSE;
          break;
        default:
          result=PR_TRUE;
      }
      break;

    case eHTMLTag_unknown:
      result=PR_FALSE;
      break;

    default:

        //ok, since no parent claimed it, test based on the child...
      switch(aChild) {

        case eHTMLTag_userdefined:
        case eHTMLTag_comment:
          result=PR_TRUE; 
          break;

        case eHTMLTag_html:
        case eHTMLTag_body:
          result=HasOpenContainer(aChild); //don't bother if they're already open...
          break;

        case eHTMLTag_button:       case eHTMLTag_fieldset:
        case eHTMLTag_input:        case eHTMLTag_isindex:
        case eHTMLTag_label:        case eHTMLTag_legend:
        case eHTMLTag_select:       case eHTMLTag_textarea:
        case eHTMLTag_option:
          if(PR_FALSE==HasOpenContainer(eHTMLTag_form))
            result=PR_TRUE; 
          break;

        case eHTMLTag_newline:    
        case eHTMLTag_whitespace:

          switch(aParent) {
            case eHTMLTag_html:     case eHTMLTag_head:   
            case eHTMLTag_title:    case eHTMLTag_map:    
            case eHTMLTag_tr:       case eHTMLTag_table:  
            case eHTMLTag_thead:    case eHTMLTag_tfoot:  
            case eHTMLTag_tbody:    case eHTMLTag_col:    
            case eHTMLTag_colgroup: case eHTMLTag_unknown:
              result=PR_TRUE;
            default:
              break;
          } //switch
          break;

          //this code prevents table container elements from
          //opening unless a table is actually already opened.
        case eHTMLTag_tr:       case eHTMLTag_thead:    
        case eHTMLTag_tfoot:    case eHTMLTag_tbody:    
        case eHTMLTag_td:       case eHTMLTag_th:
        case eHTMLTag_caption:
          if(PR_FALSE==HasOpenContainer(eHTMLTag_table))
            result=PR_TRUE;
          break;

        case eHTMLTag_entity:
          switch(aParent) {
            case eHTMLTag_tr:       case eHTMLTag_table:  
            case eHTMLTag_thead:    case eHTMLTag_tfoot:  
            case eHTMLTag_tbody:    
              result=PR_TRUE;
            default:
              break;
          } //switch
          break;

        case eHTMLTag_frame:
          if(eHTMLTag_iframe==aParent)
            result=PR_TRUE;
          break;

        default:

          static char kNonStylizedTabletags[]={eHTMLTag_table,eHTMLTag_tr,0};
          if(0!=strchr(gStyleTags,aChild)) {
            if(0!=strchr(kNonStylizedTabletags,aParent))
              return PR_TRUE;
          }

          break;
      } //switch
      break;
  }
  return result;
}


/**
 * This method is called when you want to determine if one tag is
 * synonymous with another. Cases where this are true include style
 * tags (where <i> is allowed to close <b> for example). Another
 * is <H?>, where any open heading tag can be closed by any close heading tag.
 * @update  gess6/16/98
 * @param 
 * @return
 */
PRBool IsCompatibleTag(eHTMLTags aTag1,eHTMLTags aTag2) {
  PRBool result=PR_FALSE;

  if(0!=strchr(gStyleTags,aTag1)) {
    result=PRBool(0!=strchr(gStyleTags,aTag2));
  }
  if(0!=strchr(gHeadingTags,aTag1)) {
    result=PRBool(0!=strchr(gHeadingTags,aTag2));
  }
  return result;
}

/**
 *  This method gets called to determine whether a given
 *  ENDtag can be omitted. Admittedly,this is a gross simplification.
 *  
 *  @update  gess 3/25/98
 *  @param   aTag -- tag to test for containership
 *  @return  PR_TRUE if given tag can contain other tags
 */
PRBool CNavDTD::CanOmitEndTag(eHTMLTags aParent,eHTMLTags aChild) const {
  PRBool  result=PR_FALSE;

  //begin with some simple (and obvious) cases...
  switch((eHTMLTags)aChild) {

    case eHTMLTag_userdefined:
    case eHTMLTag_comment:
      result=PR_TRUE; 
      break;

    case eHTMLTag_a:
      result=!HasOpenContainer(aChild);
      break;

    case eHTMLTag_html:
    case eHTMLTag_body:
      result=HasOpenContainer(aChild); //don't bother if they're already open...
      break;
      
    case eHTMLTag_newline:    
    case eHTMLTag_whitespace:

      switch(aParent) {
        case eHTMLTag_html:     case eHTMLTag_head:   
        case eHTMLTag_title:    case eHTMLTag_map:    
        case eHTMLTag_tr:       case eHTMLTag_table:  
        case eHTMLTag_thead:    case eHTMLTag_tfoot:  
        case eHTMLTag_tbody:    case eHTMLTag_col:    
        case eHTMLTag_colgroup: case eHTMLTag_unknown:
          result=PR_TRUE;
        default:
          break;
      } //switch
      break;

      //It turns out that a <Hn> can be closed by any other <H?>
      //This code makes them all seem compatible.
    case eHTMLTag_h1:           case eHTMLTag_h2:
    case eHTMLTag_h3:           case eHTMLTag_h4:
    case eHTMLTag_h5:           case eHTMLTag_h6:
      if(0!=strchr(gHeadingTags,aParent)) {
        result=PR_FALSE;
        break;
      }
      //Otherwise, IT's OK TO FALL THROUGH HERE...


    default:
      if(IsGatedFromClosing(aChild))
        result=PR_TRUE;
      else if(IsCompatibleTag(aChild,GetTopNode()))
        result=PR_FALSE;
      else result=(!HasOpenContainer(aChild));
      break;
  } //switch
  return result;
}

/**
 *  This method gets called to determine whether a given 
 *  tag is itself a container
 *  
 *  @update  gess 4/8/98
 *  @param   aTag -- tag to test for containership
 *  @return  PR_TRUE if given tag can contain other tags
 */
PRBool CNavDTD::IsContainer(eHTMLTags aTag) const {
  PRBool result=PR_FALSE;

  switch(aTag){
    case eHTMLTag_area:       case eHTMLTag_base:
    case eHTMLTag_basefont:   case eHTMLTag_br:
    case eHTMLTag_col:        case eHTMLTag_colgroup:
    case eHTMLTag_embed:
    case eHTMLTag_frame:
    case eHTMLTag_hr:         case eHTMLTag_img:
    case eHTMLTag_input:      case eHTMLTag_isindex:
    case eHTMLTag_link:
    case eHTMLTag_meta:
//    case eHTMLTag_option:     
    case eHTMLTag_param:
    case eHTMLTag_style:      
    case eHTMLTag_spacer:
    case eHTMLTag_wbr:
    case eHTMLTag_form:
    case eHTMLTag_newline:
    case eHTMLTag_whitespace:
    case eHTMLTag_text: 
    case eHTMLTag_unknown:
      result=PR_FALSE;
      break;

    default:
      result=PR_TRUE;
  }
  return result;
}

/**
 * This method does two things: 1st, help construct
 * our own internal model of the content-stack; and
 * 2nd, pass this message on to the sink.
 * @update  gess4/6/98
 * @param   aNode -- next node to be added to model
 * @return  TRUE if ok, FALSE if error
 */
eHTMLTags CNavDTD::GetDefaultParentTagFor(eHTMLTags aTag) const{
  eHTMLTags result=eHTMLTag_unknown;
  switch(aTag) {

    case eHTMLTag_text:
      result=eHTMLTag_p; break;

    case eHTMLTag_html:
      result=eHTMLTag_unknown; break;

    case eHTMLTag_body:
    case eHTMLTag_head:
    case eHTMLTag_frameset:
      result=eHTMLTag_html; break;

      //These tags are head specific...
    case eHTMLTag_style:
    case eHTMLTag_meta:
    case eHTMLTag_title:
    case eHTMLTag_base:
    case eHTMLTag_link:
      result=eHTMLTag_head; break;

      //These tags are table specific...
    case eHTMLTag_caption:
    case eHTMLTag_colgroup:
    case eHTMLTag_tbody:
    case eHTMLTag_tfoot:
    case eHTMLTag_thead:
      result=eHTMLTag_table; break;

    case eHTMLTag_tr:
      result=eHTMLTag_tbody; break;

    case eHTMLTag_td:
    case eHTMLTag_th:
      result=eHTMLTag_tr; break;

    case eHTMLTag_col:
      result=eHTMLTag_colgroup; break;    

    case eHTMLTag_dd:
    case eHTMLTag_dt:
      result=eHTMLTag_dl; break;    

    case eHTMLTag_option:
      result=eHTMLTag_select; break;    

      //These have to do with image maps...
    case eHTMLTag_area:
      result=eHTMLTag_map; break;    

      //These have to do with applets...
    case eHTMLTag_param:
      result=eHTMLTag_applet; break;    

      //These have to do with frames...
    case eHTMLTag_frame:
      result=eHTMLTag_frameset; break;    

    default:
      result=eHTMLTag_body; //XXX Hack! Just for now.
      break;
  }
  return result;
}


/**
 * This method tries to design a context vector (without actually
 * changing our parser state) from the parent down to the
 * child. 
 *
 * @update  gess4/6/98
 * @param   aVector is the string where we store our output vector
 *          in bottom-up order.
 * @param   aParent -- tag type of parent
 * @param   aChild -- tag type of child
 * @return  TRUE if propagation closes; false otherwise
 */
PRBool CNavDTD::ForwardPropagate(CTagStack& aStack,eHTMLTags aParentTag,eHTMLTags aChildTag)  {
  PRBool result=PR_FALSE;

  switch(aParentTag) {
    case eHTMLTag_table:
      {
        static char  tableTags[]={eHTMLTag_tr,eHTMLTag_td,0};
        if(strchr(tableTags,aChildTag)) {
          //if you're here, we know we can correctly backward propagate.
          return BackwardPropagate(aStack,aParentTag,aChildTag);
        }
      }
      //otherwise, intentionally fall through...

    case eHTMLTag_tr:
      if(PR_TRUE==CanContain((PRInt32)eHTMLTag_td,(PRInt32)aChildTag)) {
        aStack.Push(eHTMLTag_td);
        result=BackwardPropagate(aStack,aParentTag,eHTMLTag_td);
//        result=PR_TRUE;
      }

      break;

    case eHTMLTag_th:
      break;

    default:
      break;
  }//switch
  return result;
}


/**
 * This method tries to design a context map (without actually
 * changing our parser state) from the child up to the parent.
 *
 * @update  gess4/6/98
 * @param   aVector is the string where we store our output vector
 *          in bottom-up order.
 * @param   aParent -- tag type of parent
 * @param   aChild -- tag type of child
 * @return  TRUE if propagation closes; false otherwise
 */
PRBool CNavDTD::BackwardPropagate(CTagStack& aStack,eHTMLTags aParentTag,eHTMLTags aChildTag) const {

  eHTMLTags theParentTag=aChildTag;

  do {
    theParentTag=(eHTMLTags)GetDefaultParentTagFor(theParentTag);
    if(theParentTag!=eHTMLTag_unknown) {
      aStack.Push(theParentTag);
    }
    if(CanContain(aParentTag,theParentTag)) {
      //we've found a complete sequence, so push the parent...
      theParentTag=aParentTag;
      aStack.Push(theParentTag);
    }
  } while((theParentTag!=eHTMLTag_unknown) && (theParentTag!=aParentTag));
  
  return PRBool(aParentTag==theParentTag);
}



/**
 *  This method allows the caller to determine if a form
 *  element is currently open.
 *  
 *  @update  gess 4/2/98
 *  @param   
 *  @return  
 */
PRBool CNavDTD::HasOpenContainer(eHTMLTags aContainer) const {
  PRBool result=PR_FALSE;

  switch(aContainer) {
    case eHTMLTag_form:
      result=mHasOpenForm; break;

    default:
      result=(kNotFound!=GetTopmostIndexOf(aContainer)); break;
  }
  return result;
}

/**
 *  Call this method when you want to determine whether a
 *  given tag should be prevented (gated) from closing.
 *
 *  @update  gess 7/15/96
 *  @param   aTag the tag to be tested for autoclosure
 *  @return  TRUE if tag is gated.
 */
PRBool CNavDTD::IsGatedFromClosing(eHTMLTags aChildTag) const {
  PRBool  result=PR_FALSE;
  PRInt32 tagPos=GetTopmostIndexOf(aChildTag);
  PRInt32 theGatePos=kNotFound;

  if (0 != strchr(gStyleTags, aChildTag)) {
    static char theGateTags[]={ 
      eHTMLTag_caption, eHTMLTag_col, eHTMLTag_colgroup,  eHTMLTag_tbody,   
      eHTMLTag_tfoot,   eHTMLTag_tr,  eHTMLTag_thead,     eHTMLTag_td, 
      eHTMLTag_body,0};
    theGatePos=GetTopmostIndexOf(theGateTags);
  }
  else {
    switch(aChildTag) {
    case eHTMLTag_li:
      {
        static char theGateTags[]={eHTMLTag_ol,eHTMLTag_ul,0};
        theGatePos=GetTopmostIndexOf(theGateTags);
      }
      break;

    case eHTMLTag_td:
    case eHTMLTag_tr:
      {      
          /***************************************************************************
           * This code may seem confusing, so let me explain it.
           * We're trying to handle the case where a child tag may be prevented from
           * closing a tag already on the stack. Here's a case:
           *
           *  <table>
           *    <tr>
           *      <td>
           *        <table>
           *          </tr>
           *    
           * In this case, the bottom </tr> is "gated" from closing the bottommost
           * table.
           *
           * HOW THIS CODE WORKS:
           *
           *  This block of code compares all the members of the default parent 
           *  hierarchy for the childtag against the open containers on our stack.
           *  If they ever match, then the given child should be gated.
           ***************************************************************************/

        int theIndex; 
        int theTopIndex=mContextStack.mCount-1;

        eHTMLTags theParent=GetDefaultParentTagFor(aChildTag);
        while(eHTMLTag_unknown!=theParent) {
          for(theIndex=theTopIndex;theIndex>tagPos;theIndex--){
            if(mContextStack.mTags[theIndex]==theParent){
              theGatePos=theIndex;
              break;
            }
          }
          theParent=GetDefaultParentTagFor(theParent);
        }
      }
      break;

    default:
      break;
    }
  }
  if(kNotFound!=theGatePos)
    if(kNotFound!=tagPos)
      result=PRBool(theGatePos>tagPos);
  return result;
}

/**
 *  This method retrieves the HTMLTag type of the topmost
 *  container on the stack.
 *  
 *  @update  gess 4/2/98
 *  @return  tag id of topmost node in contextstack
 */
eHTMLTags CNavDTD::GetTopNode() const {
  return mContextStack.Last();
}

/**
 *  Determine whether the given tag is open anywhere
 *  in our context stack.
 *  
 *  @update  gess 4/2/98
 *  @param   eHTMLTags tag to be searched for in stack
 *  @return  topmost index of tag on stack
 */
PRInt32 CNavDTD::GetTopmostIndexOf(char aTagSet[]) const {
  int i=0;
  for(i=mContextStack.mCount-1;i>=0;i--){
    if(0!=strchr(aTagSet,mContextStack.mTags[i]))
      return i;
  }
  return kNotFound;
}

/**
 *  Determine whether the given tag is open anywhere
 *  in our context stack.
 *  
 *  @update  gess 4/2/98
 *  @param   eHTMLTags tag to be searched for in stack
 *  @return  topmost index of tag on stack
 */
PRInt32 CNavDTD::GetTopmostIndexOf(eHTMLTags aTag) const {
  int i=0;
  for(i=mContextStack.mCount-1;i>=0;i--){
    if(mContextStack.mTags[i]==aTag)
      return i;
  }
  return kNotFound;
}

/*********************************************
  Here comes code that handles the interface
  to our content sink.
 *********************************************/


/**
 * It is with great trepidation that I offer this method (privately of course).
 * The gets called whenever a container gets opened. This methods job is to 
 * take a look at the (transient) style stack, and open any style containers that
 * are there. Of course, we shouldn't bother to open styles that are incompatible
 * with our parent container.
 *
 * @update  gess6/4/98
 * @param   tag of the container just opened
 * @return  0 (for now)
 */
nsresult CNavDTD::OpenTransientStyles(eHTMLTags aTag){
  nsresult result=NS_OK;

  if(0==strchr(gWhitespaceTags,aTag)){
    PRInt32 pos=0;

    eHTMLTags parentTag=GetTopNode();
    if(CanContainStyles(parentTag)) {
      for(pos=0;pos<mStyleStack->mCount;pos++) {
        eHTMLTags theTag=mStyleStack->mTags[pos]; 
        if(PR_FALSE==HasOpenContainer(theTag)) {

          CStartToken   token(theTag);
          nsCParserNode theNode(&token,mLineNumber);

          switch(theTag) {
            case eHTMLTag_secret_h1style: case eHTMLTag_secret_h2style: 
            case eHTMLTag_secret_h3style: case eHTMLTag_secret_h4style:
            case eHTMLTag_secret_h5style: case eHTMLTag_secret_h6style:
              break;
            default:
              token.SetTypeID(theTag);  //open the html container...
              result=OpenContainer(theNode,PR_FALSE);
              mContextStack.mBits[mContextStack.mCount-1]=PR_TRUE;
          } //switch
        }
        if(NS_OK!=result)
          break;
      }//for
    }
  }//if
  return result;
}

/**
 * It is with great trepidation that I offer this method (privately of course).
 * The gets called just prior when a container gets opened. This methods job is to 
 * take a look at the (transient) style stack, and <i>close</i> any style containers 
 * that are there. Of course, we shouldn't bother to open styles that are incompatible
 * with our parent container.
 * SEE THE TOP OF THIS FILE for more information about how the transient style stack works.
 *
 * @update  gess6/4/98
 * @param   tag of the container just opened
 * @return  0 (for now)
 */
nsresult CNavDTD::CloseTransientStyles(eHTMLTags aTag){
  nsresult result=NS_OK;

  if((mStyleStack->mCount>0) && (mContextStack.mBits[mContextStack.mCount-1])) {
    if(0==strchr(gWhitespaceTags,aTag)){

      result=CloseContainersTo(mStyleStack->mTags[0],PR_FALSE);
      mContextStack.mBits[mContextStack.mCount-1]=PR_FALSE;

    }//if
  }//if
  return result;
}

/**
 * This method does two things: 1st, help construct
 * our own internal model of the content-stack; and
 * 2nd, pass this message on to the sink.
 * 
 * @update  gess4/22/98
 * @param   aNode -- next node to be added to model
 */
nsresult CNavDTD::OpenHTML(const nsIParserNode& aNode){
  NS_PRECONDITION(mContextStack.mCount >= 0, kInvalidTagStackPos);

  nsresult result=mSink->OpenHTML(aNode); 
  mContextStack.Push((eHTMLTags)aNode.GetNodeType());
  return result;
}

/**
 * This method does two things: 1st, help construct
 * our own internal model of the content-stack; and
 * 2nd, pass this message on to the sink.
 *
 * @update  gess4/6/98
 * @param   aNode -- next node to be removed from our model
 * @return  TRUE if ok, FALSE if error
 */
nsresult CNavDTD::CloseHTML(const nsIParserNode& aNode){
  NS_PRECONDITION(mContextStack.mCount > 0, kInvalidTagStackPos);
  nsresult result=mSink->CloseHTML(aNode); 
  mContextStack.Pop();
  return result;
}


/**
 * This method does two things: 1st, help construct
 * our own internal model of the content-stack; and
 * 2nd, pass this message on to the sink.
 * @update  gess4/6/98
 * @param   aNode -- next node to be added to model
 * @return  TRUE if ok, FALSE if error
 */
nsresult CNavDTD::OpenHead(const nsIParserNode& aNode){
  mContextStack.Push(eHTMLTag_head);
  nsresult result=mSink->OpenHead(aNode); 
  return result;
}

/**
 * This method does two things: 1st, help construct
 * our own internal model of the content-stack; and
 * 2nd, pass this message on to the sink.
 * @update  gess4/6/98
 * @param   aNode -- next node to be removed from our model
 * @return  TRUE if ok, FALSE if error
 */
nsresult CNavDTD::CloseHead(const nsIParserNode& aNode){
  nsresult result=mSink->CloseHead(aNode); 
  mContextStack.Pop();
  return result;
}

/**
 * This method does two things: 1st, help construct
 * our own internal model of the content-stack; and
 * 2nd, pass this message on to the sink.
 * @update  gess4/6/98
 * @param   aNode -- next node to be added to model
 * @return  TRUE if ok, FALSE if error
 */
nsresult CNavDTD::OpenBody(const nsIParserNode& aNode){
  NS_PRECONDITION(mContextStack.mCount >= 0, kInvalidTagStackPos);

  nsresult result=NS_OK;
  eHTMLTags topTag=GetTopNode();

  if(eHTMLTag_html!=topTag) {
    
    //ok, there are two cases:
    //  1. Nobody opened the html container
    //  2. Someone left the head (or other) open
    PRInt32 pos=GetTopmostIndexOf(eHTMLTag_html);
    if(kNotFound!=pos) {
      //if you're here, it means html is open, 
      //but some other tag(s) are in the way.
      //So close other tag(s).
      result=CloseContainersTo(pos+1,eHTMLTag_body,PR_TRUE);
    } else {
      //if you're here, it means that there is
      //no HTML tag in document. Let's open it.

      result=CloseContainersTo(0,eHTMLTag_html,PR_TRUE);  //close current stack containers.

      CHTMLToken    token(gEmpty,eHTMLTag_html);
      nsCParserNode htmlNode(&token,mLineNumber);
      result=OpenHTML(htmlNode);  //open the html container...
    }
  }

  if(NS_OK==result) {
    result=mSink->OpenBody(aNode); 
    mContextStack.Push((eHTMLTags)aNode.GetNodeType());
  }
  return result;
}

/**
 * This method does two things: 1st, help close
 * our own internal model of the content-stack; and
 * 2nd, pass this message on to the sink.
 * @update  gess4/6/98
 * @param   aNode -- next node to be removed from our model
 * @return  TRUE if ok, FALSE if error
 */
nsresult CNavDTD::CloseBody(const nsIParserNode& aNode){
  NS_PRECONDITION(mContextStack.mCount >= 0, kInvalidTagStackPos);
  nsresult result=mSink->CloseBody(aNode); 
  mContextStack.Pop();
  return result;
}

/**
 * This method does two things: 1st, help construct
 * our own internal model of the content-stack; and
 * 2nd, pass this message on to the sink.
 * @update  gess4/6/98
 * @param   aNode -- next node to be added to model
 * @return  TRUE if ok, FALSE if error
 */
nsresult CNavDTD::OpenForm(const nsIParserNode& aNode){
  if(mHasOpenForm)
    CloseForm(aNode);
  nsresult result=mSink->OpenForm(aNode);
  if(NS_OK==result)
    mHasOpenForm=PR_TRUE;
  return result;
}

/**
 * This method does two things: 1st, help construct
 * our own internal model of the content-stack; and
 * 2nd, pass this message on to the sink.
 * @update  gess4/6/98
 * @param   aNode -- next node to be removed from our model
 * @return  TRUE if ok, FALSE if error
 */
nsresult CNavDTD::CloseForm(const nsIParserNode& aNode){
  nsresult result=NS_OK;
  if(mHasOpenForm) {
    mHasOpenForm=PR_FALSE;
    result=mSink->CloseForm(aNode); 
  }
  return result;
}

/**
 * This method does two things: 1st, help construct
 * our own internal model of the content-stack; and
 * 2nd, pass this message on to the sink.
 * @update  gess4/6/98
 * @param   aNode -- next node to be added to model
 * @return  TRUE if ok, FALSE if error
 */
nsresult CNavDTD::OpenMap(const nsIParserNode& aNode){
  if(mHasOpenMap)
    CloseMap(aNode);
  nsresult result=mSink->OpenMap(aNode);
  if(NS_OK==result)
    mHasOpenMap=PR_TRUE;
  return result;
}

/**
 * This method does two things: 1st, help construct
 * our own internal model of the content-stack; and
 * 2nd, pass this message on to the sink.
 * @update  gess4/6/98
 * @param   aNode -- next node to be removed from our model
 * @return  TRUE if ok, FALSE if error
 */
nsresult CNavDTD::CloseMap(const nsIParserNode& aNode){
  nsresult result=NS_OK;
  if(mHasOpenMap) {
    mHasOpenMap=PR_FALSE;
    result=mSink->CloseMap(aNode); 
  }
  return result;
}

/**
 * This method does two things: 1st, help construct
 * our own internal model of the content-stack; and
 * 2nd, pass this message on to the sink.
 * @update  gess4/6/98
 * @param   aNode -- next node to be added to model
 * @return  TRUE if ok, FALSE if error
 */
nsresult CNavDTD::OpenFrameset(const nsIParserNode& aNode){
  NS_PRECONDITION(mContextStack.mCount >= 0, kInvalidTagStackPos);
  nsresult result=mSink->OpenFrameset(aNode); 
  mContextStack.Push((eHTMLTags)aNode.GetNodeType());
  return result;
}

/**
 * This method does two things: 1st, help construct
 * our own internal model of the content-stack; and
 * 2nd, pass this message on to the sink.
 * @update  gess4/6/98
 * @param   aNode -- next node to be removed from our model
 * @return  TRUE if ok, FALSE if error
 */
nsresult CNavDTD::CloseFrameset(const nsIParserNode& aNode){
  NS_PRECONDITION(mContextStack.mCount > 0, kInvalidTagStackPos);
  nsresult result=mSink->CloseFrameset(aNode); 
  mContextStack.Pop();
  return result;
}


/**
 * This method does two things: 1st, help construct
 * our own internal model of the content-stack; and
 * 2nd, pass this message on to the sink.
 * @update  gess4/6/98
 * @param   aNode -- next node to be added to model
 * @return  TRUE if ok, FALSE if error
 */
nsresult
CNavDTD::OpenContainer(const nsIParserNode& aNode,PRBool aUpdateStyleStack){
  NS_PRECONDITION(mContextStack.mCount > 0, kInvalidTagStackPos);
  
  nsresult   result=NS_OK; 
  eHTMLTags nodeType=(eHTMLTags)aNode.GetNodeType();

  switch(nodeType) {

    case eHTMLTag_html:
      result=OpenHTML(aNode); break;

    case eHTMLTag_body:
      result=OpenBody(aNode); break;

    case eHTMLTag_style:
    case eHTMLTag_textarea:
    case eHTMLTag_head:
    case eHTMLTag_title:
      break;

    case eHTMLTag_map:
      result=OpenMap(aNode);
      break;

    case eHTMLTag_form:
      result=OpenForm(aNode); break;

    default:
      result=mSink->OpenContainer(aNode); 
      mContextStack.Push((eHTMLTags)aNode.GetNodeType());
      break;
  }

  if((NS_OK==result) && (PR_TRUE==aUpdateStyleStack)){
    UpdateStyleStackForOpenTag(nodeType,nodeType);
  }

  return result;
}

/**
 * This method does two things: 1st, help construct
 * our own internal model of the content-stack; and
 * 2nd, pass this message on to the sink.
 * @update  gess4/6/98
 * @param   aNode -- next node to be removed from our model
 * @return  TRUE if ok, FALSE if error
 */
nsresult
CNavDTD::CloseContainer(const nsIParserNode& aNode,eHTMLTags aTag,
                        PRBool aUpdateStyles){
  NS_PRECONDITION(mContextStack.mCount > 0, kInvalidTagStackPos);
  nsresult   result=NS_OK;
  eHTMLTags nodeType=(eHTMLTags)aNode.GetNodeType();
  
  //XXX Hack! We know this is wrong, but it works
  //for the general case until we get it right.
  switch(nodeType) {

    case eHTMLTag_html:
      result=CloseHTML(aNode); break;

    case eHTMLTag_style:
    case eHTMLTag_textarea:
      break;

    case eHTMLTag_head:
      //result=CloseHead(aNode); 
      break;

    case eHTMLTag_body:
      result=CloseBody(aNode); break;

    case eHTMLTag_map:
      result=CloseMap(aNode);
      break;

    case eHTMLTag_form:
      result=CloseForm(aNode); break;

    case eHTMLTag_title:
    default:
      result=mSink->CloseContainer(aNode); 
      mContextStack.Pop();
      break;
  }

  mContextStack.mBits[mContextStack.mCount]=PR_FALSE;
  if((NS_OK==result) && (PR_TRUE==aUpdateStyles)){
    UpdateStyleStackForCloseTag(nodeType,aTag);
  }

  return result;
}

/**
 * This method does two things: 1st, help construct
 * our own internal model of the content-stack; and
 * 2nd, pass this message on to the sink.
 * @update  gess4/6/98
 * @param   
 * @return  TRUE if ok, FALSE if error
 */
nsresult
CNavDTD::CloseContainersTo(PRInt32 anIndex,eHTMLTags aTag,
                           PRBool aUpdateStyles){
  NS_PRECONDITION(mContextStack.mCount > 0, kInvalidTagStackPos);
  nsresult result=NS_OK;

  static CEndToken aToken(gEmpty);
  nsCParserNode theNode(&aToken,mLineNumber);

  if((anIndex<mContextStack.mCount) && (anIndex>=0)) {
    while(mContextStack.mCount>anIndex) {
      eHTMLTags theTag=mContextStack.Last();
      aToken.SetTypeID(theTag);
      result=CloseContainer(theNode,aTag,aUpdateStyles);
    }
  }
  return result;
}

/**
 * This method does two things: 1st, help construct
 * our own internal model of the content-stack; and
 * 2nd, pass this message on to the sink.
 * @update  gess4/6/98
 * @param   
 * @return  TRUE if ok, FALSE if error
 */
nsresult CNavDTD::CloseContainersTo(eHTMLTags aTag,PRBool aUpdateStyles){
  NS_PRECONDITION(mContextStack.mCount > 0, kInvalidTagStackPos);

  PRInt32 pos=GetTopmostIndexOf(aTag);

  if(kNotFound!=pos) {
    //the tag is indeed open, so close it.
    return CloseContainersTo(pos,aTag,aUpdateStyles);
  }

  eHTMLTags theTopTag=GetTopNode();
  if(IsCompatibleTag(aTag,theTopTag)) {
    //if you're here, it's because we're trying to close one style tag,
    //but a different one is actually open. Because this is NAV4x
    //compatibililty mode, we must close the one that's really open.
    aTag=theTopTag;    
    pos=GetTopmostIndexOf(aTag);
    if(kNotFound!=pos) {
      //the tag is indeed open, so close it.
      return CloseContainersTo(pos,aTag,aUpdateStyles);
    }
  }
  
  nsresult result=NS_OK;
  eHTMLTags theParentTag=GetDefaultParentTagFor(aTag);
  pos=GetTopmostIndexOf(theParentTag);
  if(kNotFound!=pos) {
    //the parent container is open, so close it instead
    result=CloseContainersTo(pos+1,aTag,aUpdateStyles);
  }
  return result;
}

/**
 * This method causes the topmost container on the stack
 * to be closed. 
 * @update  gess4/6/98
 * @see     CloseContainer()
 * @param   
 * @return  TRUE if ok, FALSE if error
 */
nsresult CNavDTD::CloseTopmostContainer(){
  NS_PRECONDITION(mContextStack.mCount > 0, kInvalidTagStackPos);

  CEndToken aToken(gEmpty);
  eHTMLTags theTag=mContextStack.Last();
  aToken.SetTypeID(theTag);
  nsCParserNode theNode(&aToken,mLineNumber);
  return CloseContainer(theNode,theTag,PR_TRUE);
}

/**
 * This method does two things: 1st, help construct
 * our own internal model of the content-stack; and
 * 2nd, pass this message on to the sink.
 * @update  gess4/6/98
 * @param   aNode -- next node to be added to model
 * @return  TRUE if ok, FALSE if error
 */
nsresult CNavDTD::AddLeaf(const nsIParserNode& aNode){
  nsresult result=mSink->AddLeaf(aNode); 
  return result;
}

CTagStack kPropagationStack;

/**
 *  This method gets called to create a valid context stack
 *  for the given child. We compare the current stack to the
 *  default needs of the child, and push new guys onto the
 *  stack until the child can be properly placed.
 *
 *  @update  gess 4/8/98
 *  @param   aChildTag is the child for whom we need to 
 *           create a new context vector
 *  @return  true if we succeeded, otherwise false
 */
nsresult CNavDTD::CreateContextStackFor(eHTMLTags aChildTag){
  
  kPropagationStack.Empty();
  
  nsresult  result=(nsresult)kContextMismatch;
  PRInt32   cnt=0;
  eHTMLTags theTop=GetTopNode();
  PRBool    bResult=ForwardPropagate(kPropagationStack,theTop,aChildTag);
  
  if(PR_FALSE==bResult){

    if(eHTMLTag_unknown!=theTop) {
      if(theTop!=aChildTag) //dont even bother if we're already inside a similar element...
        bResult=BackwardPropagate(kPropagationStack,theTop,aChildTag);

     /*****************************************************************************
        OH NOOOO!...

        We found a pretty fundamental flaw in the backward propagation code.
        The previous version propagated from a child to a target parent, and
        then again from the target parent to the root. 
        Only thing is, that won't work in cases where a container exists that's 
        not in the usual hiearchy:
          
          <html><body>
            <div>
              <table>
                <!--missing TR>
                  <td>cell text</td>
                </tr>
              </table> ...etc...

        In this case, we'd propagate fine from the <TD> to the <TABLE>, and
        then from the table to the <html>. Unfortunately, the <DIV> won't show
        up in the propagated form, and then we get out of sync with the actual
        context stack when it comes to autogenerate containers.
      ******************************************************************************/
      
    } //if
    else bResult=BackwardPropagate(kPropagationStack,eHTMLTag_html,aChildTag);
  } //elseif

  if((0==mContextStack.mCount) || (mContextStack.Last()==kPropagationStack.Pop()))
    result=NS_OK;

  //now, build up the stack according to the tags 
  //you have that aren't in the stack...
  static CStartToken theToken(gEmpty);
  if(PR_TRUE==bResult){
    while(kPropagationStack.mCount>0) {
      eHTMLTags theTag=kPropagationStack.Pop();
      theToken.SetTypeID(theTag);  //open the container...
      HandleStartToken(&theToken);
    }
    result=NS_OK;
  }
  return result;
}


/**
 *  This method gets called to ensure that the context
 *  stack is properly set up for the given child. 
 *  We pop containers off the stack (all the way down 
 *  html) until we get a container that can contain
 *  the given child.
 *  
 *  @update  gess 4/8/98
 *  @param   
 *  @return  
 */
nsresult CNavDTD::ReduceContextStackFor(eHTMLTags aChildTag){
  nsresult   result=NS_OK;
  eHTMLTags topTag=GetTopNode();

  while( (topTag!=kNotFound) && 
         (PR_FALSE==CanContain(topTag,aChildTag)) &&
         (PR_FALSE==CanContainIndirect(topTag,aChildTag))) {
    CloseTopmostContainer();
    topTag=GetTopNode();
  }
  return result;
}


/**
 * This method causes all explicit style-tag containers that
 * are opened to be reflected on our internal style-stack.
 *
 * @update  gess6/4/98
 * @param   aTag is the id of the html container being opened
 * @return  0 if all is well.
 */
nsresult
CNavDTD::UpdateStyleStackForOpenTag(eHTMLTags aTag,eHTMLTags anActualTag){
  nsresult   result=0;
  
  if (0 != strchr(gStyleTags, aTag)) {
    mStyleStack->Push(aTag);
  }
  else {
    switch (aTag) {
    case eHTMLTag_h1: case eHTMLTag_h2:
    case eHTMLTag_h3: case eHTMLTag_h4:
    case eHTMLTag_h5: case eHTMLTag_h6:
      break;

    default:
      break;
    }
  }
  return result;
} //update...


/**
 * This method gets called when an explicit style close-tag is encountered.
 * It results in the style tag id being popped from our internal style stack.
 *
 * @update  gess6/4/98
 * @param 
 * @return  0 if all went well (which it always does)
 */
nsresult
CNavDTD::UpdateStyleStackForCloseTag(eHTMLTags aTag,eHTMLTags anActualTag){
  nsresult result=0;
  
  if(mStyleStack->mCount>0) {
    if (0 != strchr(gStyleTags, aTag)) {
      if(aTag==anActualTag)
        mStyleStack->Pop();
    }
    else {
      switch (aTag) {
      case eHTMLTag_h1: case eHTMLTag_h2:
      case eHTMLTag_h3: case eHTMLTag_h4:
      case eHTMLTag_h5: case eHTMLTag_h6:
        break;

      default:
        break;
      }//switch
    }
  }//if
  return result;
}


/*******************************************************************
  These methods used to be hidden in the tokenizer-delegate. 
  That file merged with the DTD, since the separation wasn't really
  buying us anything.
 *******************************************************************/

/**
 *  This method is called just after a "<" has been consumed 
 *  and we know we're at the start of some kind of tagged 
 *  element. We don't know yet if it's a tag or a comment.
 *  
 *  @update  gess 5/12/98
 *  @param   aChar is the last char read
 *  @param   aScanner is represents our input source
 *  @param   aToken is the out arg holding our new token
 *  @return  error code (may return kInterrupted).
 */
nsresult
CNavDTD::ConsumeTag(PRUnichar aChar,CScanner& aScanner,CToken*& aToken) {

  nsresult result=aScanner.GetChar(aChar);

  if(NS_OK==result) {

    switch(aChar) {
      case kForwardSlash:
        PRUnichar ch; 
        result=aScanner.Peek(ch);
        if(NS_OK==result) {
          if(nsString::IsAlpha(ch))
            aToken=gTokenRecycler.CreateTokenOfType(eToken_end,eHTMLTag_unknown,gEmpty);
          else aToken=gTokenRecycler.CreateTokenOfType(eToken_comment,eHTMLTag_unknown,gEmpty);
        }//if
        break;
      case kExclamation:
        aToken=gTokenRecycler.CreateTokenOfType(eToken_comment,eHTMLTag_unknown,gEmpty);
        break;
      default:
        if(nsString::IsAlpha(aChar))
          return ConsumeStartTag(aChar,aScanner,aToken);
        else if(kEOF!=aChar) {
          nsAutoString temp("<");
          return ConsumeText(temp,aScanner,aToken);
        }
    } //switch

    if((0!=aToken) && (NS_OK==result)) {
      result= aToken->Consume(aChar,aScanner);  //tell new token to finish consuming text...    
      if(result) {
        delete aToken;
        aToken=0;
      }
    } //if
  } //if
  return result;
}

/**
 *  This method is called just after we've consumed a start
 *  tag, and we now have to consume its attributes.
 *  
 *  @update  gess 3/25/98
 *  @param   aChar: last char read
 *  @param   aScanner: see nsScanner.h
 *  @return  
 */
nsresult
CNavDTD::ConsumeAttributes(PRUnichar aChar,CScanner& aScanner,CStartToken* aToken) {
  PRBool done=PR_FALSE;
  nsresult result=NS_OK;
  PRInt16 theAttrCount=0;

  while((!done) && (result==NS_OK)) {
    CAttributeToken* theToken= (CAttributeToken*)gTokenRecycler.CreateTokenOfType(eToken_attribute,eHTMLTag_unknown,gEmpty);
    if(theToken){
      result=theToken->Consume(aChar,aScanner);  //tell new token to finish consuming text...    

      //Much as I hate to do this, here's some special case code.
      //This handles the case of empty-tags in XML. Our last
      //attribute token will come through with a text value of ""
      //and a textkey of "/". We should destroy it, and tell the 
      //start token it was empty.
      nsString& key=theToken->GetKey();
      nsString& text=theToken->GetStringValueXXX();
      if((key[0]==kForwardSlash) && (0==text.Length())){
        //tada! our special case! Treat it like an empty start tag...
        aToken->SetEmpty(PR_TRUE);
        delete theToken;
      }
      else if(NS_OK==result){
        theAttrCount++;
        mTokenDeque.Push(theToken);
      }//if
      else delete theToken; //we can't keep it...
    }//if
    
    if(NS_OK==result){
      result=aScanner.Peek(aChar);
      if(aChar==kGreaterThan) { //you just ate the '>'
        aScanner.GetChar(aChar); //skip the '>'
        done=PR_TRUE;
      }//if
    }//if
  }//while

  aToken->SetAttributeCount(theAttrCount);
  return result;
}

/**
 *  This is a special case method. It's job is to consume 
 *  all of the given tag up to an including the end tag.
 *
 *  @param  aChar: last char read
 *  @param  aScanner: see nsScanner.h
 *  @param  anErrorCode: arg that will hold error condition
 *  @return new token or null
 */
nsresult
CNavDTD::ConsumeContentToEndTag(const nsString& aString,
                                PRUnichar aChar,
                                eHTMLTags aChildTag,
                                CScanner& aScanner,
                                CToken*& aToken){
  
  //In the case that we just read the given tag, we should go and
  //consume all the input until we find a matching end tag.

  nsAutoString endTag("</");
  endTag.Append(aString);
  endTag.Append(">");
  aToken=gTokenRecycler.CreateTokenOfType(eToken_skippedcontent,aChildTag,endTag);
  return aToken->Consume(aChar,aScanner);  //tell new token to finish consuming text...    
}

/**
 *  This method is called just after a "<" has been consumed 
 *  and we know we're at the start of a tag.  
 *  
 *  @update gess 3/25/98
 *  @param  aChar: last char read
 *  @param  aScanner: see nsScanner.h
 *  @param  anErrorCode: arg that will hold error condition
 *  @return new token or null 
 */

static char gSkippedContentTags[]={ eHTMLTag_script, eHTMLTag_style,  eHTMLTag_title,  eHTMLTag_textarea, 0};

nsresult
CNavDTD::ConsumeStartTag(PRUnichar aChar,CScanner& aScanner,CToken*& aToken) {
  PRInt32 theDequeSize=mTokenDeque.GetSize();
  nsresult result=NS_OK;

  aToken=gTokenRecycler.CreateTokenOfType(eToken_start,eHTMLTag_unknown,gEmpty);
  
  if(aToken) {
    result= aToken->Consume(aChar,aScanner);  //tell new token to finish consuming text...    
    if(NS_OK==result) {
      if(((CStartToken*)aToken)->IsAttributed()) {
        result=ConsumeAttributes(aChar,aScanner,(CStartToken*)aToken);
      }
      //now that that's over with, we have one more problem to solve.
      //In the case that we just read a <SCRIPT> or <STYLE> tags, we should go and
      //consume all the content itself.
      if(NS_OK==result) {

        eHTMLTags theTag=(eHTMLTags)aToken->GetTypeID();

        if(0!=strchr(gSkippedContentTags,theTag)){

            //Do special case handling for <script>, <style>, <title> or <textarea>...
          CToken*   skippedToken=0;
          nsString& str=aToken->GetStringValueXXX();
          result=ConsumeContentToEndTag(str,aChar,theTag,aScanner,skippedToken);
    
          if((NS_OK==result) && skippedToken){
              //now we strip the ending sequence from our new SkippedContent token...
            //PRInt32 slen=str.Length()+3;
            nsString& skippedText=((CSkippedContentToken*)skippedToken)->GetKey();    
            mTokenDeque.Push(skippedToken);
  
            //In the case that we just read a given tag, we should go and
            //consume all the tag content itself (and throw it all away).

            CToken* endtoken=gTokenRecycler.CreateTokenOfType(eToken_end,theTag,skippedToken->GetStringValueXXX());
            mTokenDeque.Push(endtoken);
          } //if
        } //if
      } //if

      //EEEEECCCCKKKK!!! 
      //This code is confusing, so pay attention.
      //If you're here, it's because we were in the midst of consuming a start
      //tag but ran out of data (not in the stream, but in this *part* of the stream.
      //For simplicity, we have to unwind our input. Therefore, we pop and discard
      //any new tokens we've cued this round. Later we can get smarter about this.
      if(NS_OK!=result) {
        while(mTokenDeque.GetSize()>theDequeSize) {
          delete mTokenDeque.PopBack();
        }
      }


    } //if
  } //if
  return result;
}

/**
 *  This method is called just after a "&" has been consumed 
 *  and we know we're at the start of an entity.  
 *  
 *  @update gess 3/25/98
 *  @param  aChar: last char read
 *  @param  aScanner: see nsScanner.h
 *  @param  anErrorCode: arg that will hold error condition
 *  @return new token or null 
 */
nsresult
CNavDTD::ConsumeEntity(PRUnichar aChar,CScanner& aScanner,CToken*& aToken) {
   PRUnichar  ch;
   nsresult result=aScanner.GetChar(ch);

   if(NS_OK==result) {
     if(nsString::IsAlpha(ch)) { //handle common enity references &xxx; or &#000.
       aToken = gTokenRecycler.CreateTokenOfType(eToken_entity,eHTMLTag_unknown,gEmpty);
       result = aToken->Consume(ch,aScanner);  //tell new token to finish consuming text...    
     }
     else if(kHashsign==ch) {
       aToken = gTokenRecycler.CreateTokenOfType(eToken_entity,eHTMLTag_unknown,gEmpty);
       result=aToken->Consume(0,aScanner);
     }
     else {
       //oops, we're actually looking at plain text...
       nsAutoString temp("&");
       temp.Append(ch);
       result=ConsumeText(temp,aScanner,aToken);
     }
   }//if
   return result;
}

/**
 *  This method is called just after whitespace has been 
 *  consumed and we know we're at the start a whitespace run.  
 *  
 *  @update gess 3/25/98
 *  @param  aChar: last char read
 *  @param  aScanner: see nsScanner.h
 *  @param  anErrorCode: arg that will hold error condition
 *  @return new token or null 
 */
nsresult
CNavDTD::ConsumeWhitespace(PRUnichar aChar,
                           CScanner& aScanner,
                           CToken*& aToken) {
  aToken = gTokenRecycler.CreateTokenOfType(eToken_whitespace,eHTMLTag_whitespace,gEmpty);
  nsresult result=kNoError;
  if(aToken) {
     result=aToken->Consume(aChar,aScanner);
  }
  return kNoError;
}

/**
 *  This method is called just after a "<!" has been consumed 
 *  and we know we're at the start of a comment.  
 *  
 *  @update gess 3/25/98
 *  @param  aChar: last char read
 *  @param  aScanner: see nsScanner.h
 *  @param  anErrorCode: arg that will hold error condition
 *  @return new token or null 
 */
nsresult CNavDTD::ConsumeComment(PRUnichar aChar,CScanner& aScanner,CToken*& aToken){
  aToken = gTokenRecycler.CreateTokenOfType(eToken_comment,eHTMLTag_unknown,gEmpty);
  nsresult result=NS_OK;
  if(aToken) {
     result=aToken->Consume(aChar,aScanner);
  }
  return result;
}

/**
 *  This method is called just after a known text char has
 *  been consumed and we should read a text run.
 *  
 *  @update gess 3/25/98
 *  @param  aChar: last char read
 *  @param  aScanner: see nsScanner.h
 *  @param  anErrorCode: arg that will hold error condition
 *  @return new token or null 
 */
nsresult CNavDTD::ConsumeText(const nsString& aString,CScanner& aScanner,CToken*& aToken){
  nsresult result=NS_OK;
  aToken=gTokenRecycler.CreateTokenOfType(eToken_text,eHTMLTag_text,aString);
  if(aToken) {
    PRUnichar ch=0;
    result=aToken->Consume(ch,aScanner);
    if(result) {
      nsString& temp=aToken->GetStringValueXXX();
      if(0==temp.Length()){
        delete aToken;
      }
      else result=kNoError;
    }
  }
  return result;
}

/**
 *  This method is called just after a newline has been consumed. 
 *  
 *  @update gess 3/25/98
 *  @param  aChar: last char read
 *  @param  aScanner: see nsScanner.h
 *  @param  aToken is the newly created newline token that is parsing
 *  @return error code
 */
nsresult CNavDTD::ConsumeNewline(PRUnichar aChar,CScanner& aScanner,CToken*& aToken){
  aToken=gTokenRecycler.CreateTokenOfType(eToken_newline,eHTMLTag_newline,gEmpty);
  nsresult result=NS_OK;
  if(aToken) {
    result=aToken->Consume(aChar,aScanner);
  }
  return kNoError;
}

/**
 *  This method repeatedly called by the tokenizer. 
 *  Each time, we determine the kind of token were about to 
 *  read, and then we call the appropriate method to handle
 *  that token type.
 *  
 *  @update gess 3/25/98
 *  @param  aChar: last char read
 *  @param  aScanner: see nsScanner.h
 *  @param  anErrorCode: arg that will hold error condition
 *  @return new token or null 
 */
nsresult CNavDTD::ConsumeToken(CToken*& aToken){
  aToken=0;
  if(mTokenDeque.GetSize()>0) {
    aToken=(CToken*)mTokenDeque.Pop();
    return NS_OK;
  }

  nsresult   result=NS_OK;
  CScanner* theScanner=mParser->GetScanner();
  if(NS_OK==result){
    PRUnichar theChar;
    result=theScanner->GetChar(theChar);
    switch(result) {
      case kEOF:
          //We convert from eof to complete here, because we never really tried to get data.
          //All we did was try to see if data was available, which it wasn't.
          //It's important to return process complete, so that controlling logic can know that
          //everything went well, but we're done with token processing.
        result=kProcessComplete;
        break;

      case kInterrupted:
        theScanner->RewindToMark();
        break; 

      case NS_OK:
      default:
        switch(theChar) {
          case kLessThan:
            result=ConsumeTag(theChar,*theScanner,aToken);
            break;

          case kAmpersand:
            result=ConsumeEntity(theChar,*theScanner,aToken);
            break;
          
          case kCR: case kLF:
            result=ConsumeNewline(theChar,*theScanner,aToken);
            break;
          
          case kNotFound:
            break;
          
          default:
            if(!nsString::IsSpace(theChar)) {
              nsAutoString temp(theChar);
              result=ConsumeText(temp,*theScanner,aToken);
              break;
            }
            result=ConsumeWhitespace(theChar,*theScanner,aToken);
            break;
        } //switch
        break; 
    } //switch
//    if(NS_OK==result)
//      result=theScanner->Eof(); 
  } //if
  return result;
}


/**
 * 
 * @update  gess5/18/98
 * @param 
 * @return
 */
nsresult CNavDTD::WillResumeParse(void){
  nsresult result = NS_OK;
  if(mSink) {
    result = mSink->WillResume();
  }
  return result;
}

/**
 * This method gets called when the parsing process is interrupted
 * due to lack of data (waiting for netlib).
 * @update  gess5/18/98
 * @return  error code
 */
nsresult CNavDTD::WillInterruptParse(void){
  nsresult result = NS_OK;
  if(mSink) {
    result = mSink->WillInterrupt();
  }
  return result;
}


