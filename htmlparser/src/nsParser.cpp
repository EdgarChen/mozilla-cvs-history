/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 *   Pierre Phaneuf <pp@ludusdesign.com>
 */
  
#define DEBUG_XMLENCODING
#define XMLENCODING_PEEKBYTES 64

#include "nsParser.h"
#include "nsIContentSink.h" 
#include "nsString.h"
#include "nsCRT.h" 
#include "nsScanner.h"
#include "prenv.h"  //this is here for debug reasons...
#include "plstr.h"
#include "nsIParserFilter.h"
#include "nshtmlpars.h"
#include "CNavDTD.h"
#include "nsWellFormedDTD.h"
#include "nsViewSourceHTML.h" 
#include "nsHTMLContentSinkStream.h" //this is here so we can get a null sink, which really should be gotten from nsICOntentSink.h
#include "nsIStringStream.h"
#include "nsIChannel.h"
#include "nsIProgressEventSink.h"
#include "nsIBufferInputStream.h"
#include "CRtfDTD.h"

//#define rickgdebug 

 
static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);                 
static NS_DEFINE_IID(kClassIID, NS_PARSER_IID); 
static NS_DEFINE_IID(kIParserIID, NS_IPARSER_IID);
static NS_DEFINE_IID(kIStreamListenerIID, NS_ISTREAMLISTENER_IID);

static const char* kNullURL = "Error: Null URL given";
static const char* kOnStartNotCalled = "Error: OnStartRequest() must be called before OnDataAvailable()";
static const char* kBadListenerInit  = "Error: Parser's IStreamListener API was not setup correctly in constructor.";

//-------------------------------------------------------------------
 

class CDTDDeallocator: public nsDequeFunctor{
public:
  virtual void* operator()(void* anObject) {
    nsIDTD* aDTD =(nsIDTD*)anObject;
    NS_RELEASE(aDTD);
    return 0;
  }
};

//-------------------------------------------------------------------

class CDTDFinder: public nsDequeFunctor{
public:
  CDTDFinder(nsIDTD* aDTD) {
    mTargetDTD=aDTD;
  }
  virtual ~CDTDFinder() {
  }
  virtual void* operator()(void* anObject) {
    nsIDTD* theDTD=(nsIDTD*)anObject;
    if(theDTD->GetMostDerivedIID().Equals(mTargetDTD->GetMostDerivedIID()))
      return anObject;
    return 0;
  }
  nsIDTD* mTargetDTD;
};

//-------------------------------------------------------------------

class CSharedParserObjects {
public:

  CSharedParserObjects() : mDTDDeque(0) {

    //Note: To cut down on startup time/overhead, we defer the construction of non-html DTD's. 

    nsIDTD* theDTD;
    NS_NewNavHTMLDTD(&theDTD);    //do this as the default HTML DTD...
    mDTDDeque.Push(theDTD);

    mHasViewSourceDTD=PR_FALSE;
    mHasRTFDTD=mHasXMLDTD=PR_FALSE;
  }

  ~CSharedParserObjects() {
    CDTDDeallocator theDeallocator;
    mDTDDeque.ForEach(theDeallocator);  //release all the DTD's
  }

  void RegisterDTD(nsIDTD* aDTD){
    if(aDTD) {
      NS_ADDREF(aDTD);
      CDTDFinder theFinder(aDTD);
      if(!mDTDDeque.FirstThat(theFinder)) {
        nsIDTD* theDTD;
        aDTD->CreateNewInstance(&theDTD);
        mDTDDeque.Push(theDTD);
      }
      NS_RELEASE(aDTD);
    }
  }
  
  nsDeque mDTDDeque;
  PRBool  mHasViewSourceDTD;  //this allows us to defer construction of this object.
  PRBool  mHasXMLDTD;         //also defer XML dtd construction
  PRBool  mHasRTFDTD;         //also defer RTF dtd construction
};

static CSharedParserObjects* gSharedParserObjects=0;


//-------------------------------------------------------------------------

/**********************************************************************************
  This class is used as an interface between an external agent (like the DOM) and
  the parser. It will contain a stack full of tagnames, which is used in our
  parser/paste API's.
 **********************************************************************************/

class nsTagStack : public nsITagStack {
public:
  nsTagStack() : nsITagStack(), mTags(0) {
  }

  virtual ~nsTagStack() {
  }

  virtual void Push(PRUnichar* aTag){
    mTags.Push(aTag);
  }
  
  virtual PRUnichar*  Pop(void){
    PRUnichar* result=(PRUnichar*)mTags.Pop();
    return result;
  }
  
  virtual PRUnichar*  TagAt(PRUint32 anIndex){
    PRUnichar* result=0;
    if(anIndex<(PRUint32)mTags.GetSize())
      result=(PRUnichar*)mTags.ObjectAt(anIndex);
    return result;
  }

  virtual PRUint32    GetSize(void){
    return mTags.GetSize();
  }

  nsDeque mTags;  //will hold a deque of prunichars...
};

CSharedParserObjects& GetSharedObjects() {
  if (!gSharedParserObjects) {
    gSharedParserObjects = new CSharedParserObjects();
  }
  return *gSharedParserObjects;
}

/** 
 *  This gets called when the htmlparser module is shutdown.
 *   
 *  @update  gess 01/04/99
 */
void nsParser::FreeSharedObjects(void) {
  if (gSharedParserObjects) {
    delete gSharedParserObjects;
    gSharedParserObjects=0;
  }
}

/** 
 *  default constructor
 *   
 *  @update  gess 01/04/99
 *  @param   
 *  @return   
 */
nsParser::nsParser(nsITokenObserver* anObserver) : mUnusedInput("") , mCharset("ISO-8859-1") {
  NS_INIT_REFCNT();
  mParserFilter = 0;
  mObserver = 0;
  mProgressEventSink = nsnull;
  mSink=0;
  mParserContext=0;
  mTokenObserver=anObserver;
  mStreamStatus=0;
  mDTDVerification=PR_FALSE;
  mCharsetSource=kCharsetUninitialized;
  mInternalState=NS_OK;
  mObserversEnabled=PR_TRUE;
  mCommand=eViewNormal;

  MOZ_TIMER_DEBUGLOG(("Reset: Parse Time: nsParser::nsParser(), this=%p\n", this));
  MOZ_TIMER_RESET(mParseTime);  
  MOZ_TIMER_RESET(mDTDTime);  
  MOZ_TIMER_RESET(mTokenizeTime);
}

 
/**
 *  Default destructor
 *  
 *  @update  gess 01/04/99
 *  @param   
 *  @return  
 */
nsParser::~nsParser() {
  NS_IF_RELEASE(mObserver);
  NS_IF_RELEASE(mProgressEventSink);
  NS_IF_RELEASE(mSink);

  //don't forget to add code here to delete 
  //what may be several contexts...
  delete mParserContext;
}


NS_IMPL_ADDREF(nsParser)
NS_IMPL_RELEASE(nsParser)
//NS_IMPL_ISUPPORTS(nsParser,NS_IHTML_HTMLPARSER_IID)


/**
 *  This method gets called as part of our COM-like interfaces.
 *  Its purpose is to create an interface to parser object
 *  of some type.
 *  
 *  @update   gess 01/04/99
 *  @param    nsIID  id of object to discover
 *  @param    aInstancePtr ptr to newly discovered interface
 *  @return   NS_xxx result code
 */
nsresult nsParser::QueryInterface(const nsIID& aIID, void** aInstancePtr)  
{                                                                        
  if (NULL == aInstancePtr) {                                            
    return NS_ERROR_NULL_POINTER;                                        
  }                                                                      

  if(aIID.Equals(kISupportsIID))    {  //do IUnknown...
    *aInstancePtr = (nsIParser*)(this);                                        
  }
  else if(aIID.Equals(kIParserIID)) {  //do IParser base class...
    *aInstancePtr = (nsIParser*)(this);                                        
  }
  else if(aIID.Equals(NS_GET_IID(nsIProgressEventSink))) {
    *aInstancePtr = (nsIStreamListener*)(this);                                        
  }
  else if(aIID.Equals(NS_GET_IID(nsIStreamObserver))) {
    *aInstancePtr = (nsIStreamObserver*)(this);                                        
  }
  else if(aIID.Equals(NS_GET_IID(nsIStreamListener))) {
    *aInstancePtr = (nsIStreamListener*)(this);                                        
  }
  else if(aIID.Equals(kClassIID)) {  //do this class...
    *aInstancePtr = (nsParser*)(this);                                        
  }                 
  else {
    *aInstancePtr=0;
    return NS_NOINTERFACE;
  }
  NS_ADDREF_THIS();
  return NS_OK;                                                        
}


/**
 * 
 * @update	gess 01/04/99
 * @param 
 * @return
 */
nsIParserFilter * nsParser::SetParserFilter(nsIParserFilter * aFilter)
{
  nsIParserFilter* old=mParserFilter;
  if(old)
    NS_RELEASE(old);
  if(aFilter) {
    mParserFilter=aFilter;
    NS_ADDREF(aFilter);
  }
  return old;
}


/**
 *  Call this method once you've created a parser, and want to instruct it
 *  about the command which caused the parser to be constructed. For example,
 *  this allows us to select a DTD which can do, say, view-source.
 *  
 *  @update  gess 01/04/99
 *  @param   aContentSink -- ptr to content sink that will receive output
 *  @return	 ptr to previously set contentsink (usually null)  
 */
void nsParser::SetCommand(const char* aCommand){
  nsAutoString theCommand(aCommand);
  if(theCommand.Equals(kViewSourceCommand))
    mCommand=eViewSource;
  else mCommand=eViewNormal;
}

/**
 *  Call this method once you've created a parser, and want to instruct it
 *  about the command which caused the parser to be constructed. For example,
 *  this allows us to select a DTD which can do, say, view-source.
 *  
 *  @update  gess 01/04/99
 *  @param   aContentSink -- ptr to content sink that will receive output
 *  @return	 ptr to previously set contentsink (usually null)  
 */
void nsParser::SetCommand(eParserCommands aParserCommand){
  mCommand=aParserCommand;
}


/**
 *  Call this method once you've created a parser, and want to instruct it
 *  about what charset to load
 *  
 *  @update  ftang 4/23/99
 *  @param   aCharset- the charest of a document
 *  @param   aCharsetSource- the soure of the chares
 *  @return	 nada
 */
void nsParser::SetDocumentCharset(nsString& aCharset, nsCharsetSource aCharsetSource){
  mCharset = aCharset;
  mCharsetSource = aCharsetSource; 
  if(mParserContext && mParserContext->mScanner)
     mParserContext->mScanner->SetDocumentCharset(aCharset, aCharsetSource);
}

/**
 *  This method gets called in order to set the content
 *  sink for this parser to dump nodes to.
 *  
 *  @update  gess 01/04/99
 *  @param   nsIContentSink interface for node receiver
 *  @return  
 */
nsIContentSink* nsParser::SetContentSink(nsIContentSink* aSink) {
  NS_PRECONDITION(0!=aSink,"sink cannot be null!");
  nsIContentSink* old=mSink;

  NS_IF_RELEASE(old);
  if(aSink) {
    mSink=aSink;
    NS_ADDREF(aSink);
    mSink->SetParser(this);
  }
  return old;
}

/**
 * retrive the sink set into the parser 
 * @update	gess5/11/98
 * @param   aSink is the new sink to be used by parser
 * @return  old sink, or NULL
 */
nsIContentSink* nsParser::GetContentSink(void){
  return mSink;
}

/**
 *  Call this method when you want to
 *  register your dynamic DTD's with the parser.
 *  
 *  @update  gess 01/04/99
 *  @param   aDTD  is the object to be registered.
 *  @return  nothing.
 */
void nsParser::RegisterDTD(nsIDTD* aDTD){
  CSharedParserObjects& theShare=GetSharedObjects();
  theShare.RegisterDTD(aDTD);
}

/**
 *  Retrieve scanner from topmost parsecontext
 *  
 *  @update  gess 01/04/99
 *  @return  ptr to internal scanner
 */
nsScanner* nsParser::GetScanner(void){
  if(mParserContext)
    return mParserContext->mScanner;
  return 0;
}


/**
 *  Retrieve parsemode from topmost parser context
 *  
 *  @update  gess 01/04/99
 *  @return  parsemode
 */
eParseMode nsParser::GetParseMode(void){
  if(mParserContext)
    return mParserContext->mParseMode;
  return eParseMode_unknown;
}



/**
 *  
 *  
 *  @update  gess 5/13/98
 *  @param   
 *  @return  
 */
static
PRBool FindSuitableDTD( CParserContext& aParserContext,nsString& aBuffer) {
  
  //Let's start by trying the defaultDTD, if one exists...
  if(aParserContext.mDTD)
    if(aParserContext.mDTD->CanParse(aParserContext,aBuffer,0))
      return PR_TRUE;

  CSharedParserObjects& gSharedObjects=GetSharedObjects();


  aParserContext.mAutoDetectStatus=eUnknownDetect;
  PRInt32 theDTDIndex=0;
  nsIDTD* theBestDTD=0;
  nsIDTD* theDTD=0;
  PRBool  thePrimaryFound=PR_FALSE;

  while((theDTDIndex<=gSharedObjects.mDTDDeque.GetSize()) && (aParserContext.mAutoDetectStatus!=ePrimaryDetect)){
    theDTD=(nsIDTD*)gSharedObjects.mDTDDeque.ObjectAt(theDTDIndex++);
    if(theDTD) {
      aParserContext.mAutoDetectStatus=theDTD->CanParse(aParserContext,aBuffer,0);
      if(eValidDetect==aParserContext.mAutoDetectStatus){
        theBestDTD=theDTD;
      }
      else if(ePrimaryDetect==aParserContext.mAutoDetectStatus) {  
        theBestDTD=theDTD;
        thePrimaryFound=PR_TRUE;
      }
    }
    if((theDTDIndex==gSharedObjects.mDTDDeque.GetSize()) && (!thePrimaryFound)) {
      if(!gSharedObjects.mHasXMLDTD) {
        NS_NewWellFormed_DTD(&theDTD);  //do this to view XML files...
        gSharedObjects.mDTDDeque.Push(theDTD);
        gSharedObjects.mHasXMLDTD=PR_TRUE;
      }
      else if(!gSharedObjects.mHasViewSourceDTD) {
        NS_NewViewSourceHTML(&theDTD);  //do this so all non-html files can be viewed...
        gSharedObjects.mDTDDeque.Push(theDTD);
        gSharedObjects.mHasViewSourceDTD=PR_TRUE;
      }
      else if(!gSharedObjects.mHasRTFDTD) {
        NS_NewRTF_DTD(&theDTD);  //do this so all non-html files can be viewed...
        gSharedObjects.mDTDDeque.Push(theDTD);
        gSharedObjects.mHasRTFDTD=PR_TRUE;
      }
    }
  }

  if(theBestDTD) {
    theBestDTD->CreateNewInstance(&aParserContext.mDTD);
    return PR_TRUE;
  }
  return PR_FALSE;
}

char* doctypes[] = {
  "<!DOCTYPE \"-//W3O//DTD W3 HTML 3.0//EN//\">", 

  "<!DOCTYPE \"-//W3C//DTD HTML 5.0//EN\">",

  "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">",
  "<!DOCTYPE \"-//W3C//DTD HTML 4.0 STRICT//EN\">",
  "<!DOCTYPE \"-//W3C//DTD HTML 4.01//EN\">",
  "<!DOCTYPE \"-//W3C//DTD HTML 4.0//EN\">",
  "<!DOCTYPE \"-//W3C//DTD XHTML 1.0 Strict//EN\">",
  "<!DOCTYPE \"-//W3C//DTD XHTML 1.0 Transitional//EN\">",
  "<!DOCTYPE \"-//W3C//DTD XHTML 1.0 Frameset//EN\">",
  "<!DOCTYPE \"ISO/IEC 15445:1999//DTD HyperText Markup Language//EN\">",
  "<!DOCTYPE \"ISO/IEC 15445:1999//DTD HTML//EN\">",
  
  "<!DOCTYPE \"-//W3C//DTD HTML 4.01 Transitional//EN\">",
  "<!DOCTYPE \"-//W3C//DTD HTML 4.1 Frameset//EN\">", 
  "<!DOCTYPE \"-//W3C//DTD HTML 4.0 Transitional//EN\">", 
  "<!DOCTYPE \"-//W3C//DTD HTML 4.0 Frameset//EN\">", 
  "<!DOCTYPE \"-//SoftQuad Software//DTD HoTMetaL PRO 6.::19990601::extensions to HTML 4.//EN\">", 
  "<!DOCTYPE \"-//IETF//DTD HTML//EN//3.\">", 
  "<!DOCTYPE \"-//W3C//DTD W3 HTML 3.0//EN//\">", 
  "<!DOCTYPE \"-//W3C//DTD W3 HTML 3.0//EN\">", 
  "<!DOCTYPE \"-//W3C//DTD HTML 3.0 1995-03-24//EN\">", 
  "<!DOCTYPE \"-//IETF//DTD HTML 3.0//EN\">", 
  "<!DOCTYPE \"-//IETF//DTD HTML 3.0//EN//\">", 
  "<!DOCTYPE \"-//IETF//DTD HTML Level 3//EN\">", 
  "<!DOCTYPE \"-//IETF//DTD HTML Level 3//EN//3.0\">", 
  "<!DOCTYPE \"-//AS//DTD HTML 3 asWedit + extensions//EN\">", 
  "<!DOCTYPE \"-//AdvaSoft Ltd//DTD HTML 3.0 asWedit + extensions//EN\">", 
  "<!DOCTYPE \"-//IETF//DTD HTML Strict//EN//3.0\">", 
  "<!DOCTYPE \"-//W3C//DTD W3 HTML Strict 3//EN//\">", 
  "<!DOCTYPE \"-//IETF//DTD HTML Strict Level 3//EN\">", 
  "<!DOCTYPE \"-//IETF//DTD HTML Strict Level 3//EN//3.0\">", 

  "<!DOCTYPE \"HTML\">", 
  "<!DOCTYPE \"-//IETF//DTD HTML//EN\">", 
  "<!DOCTYPE \"-//IETF//DTD HTML//EN//2\">", 
  "<!DOCTYPE \"-//IETF//DTD HTML 2.0//EN\">", 
  "<!DOCTYPE \"-//IETF//DTD HTML Level 2//EN\">", 
  "<!DOCTYPE \"-//IETF//DTD HTML Level 2//EN//2.0\">", 
  "<!DOCTYPE \"-//IETF//DTD HTML 2.0 Level 2//EN\">", 
  "<!DOCTYPE \"-//IETF//DTD HTML Level 1//EN\">", 
  "<!DOCTYPE \"-//IETF//DTD HTML Level 1//EN//2.0\">", 
  "<!DOCTYPE \"-//IETF//DTD HTML 2.0 Level 1//EN\">", 
  "<!DOCTYPE \"-//IETF//DTD HTML Level 0//EN\">", 
  "<!DOCTYPE \"-//IETF//DTD HTML Level 0//EN//2.0\">", 
  "<!DOCTYPE \"-//IETF//DTD HTML Strict//EN\">", 
  "<!DOCTYPE \"-//IETF//DTD HTML Strict//EN//2\">", 
  "<!DOCTYPE \"-//IETF//DTD HTML Strict Level 2//EN\">", 
  "<!DOCTYPE \"-//IETF//DTD HTML Strict Level 2//EN//2.0\">", 
  "<!DOCTYPE \"-//IETF//DTD HTML 2.0 Strict//EN\">", 
  "<!DOCTYPE \"-//IETF//DTD HTML 2.0 Strict Level 2//EN\">", 
  "<!DOCTYPE \"-//IETF//DTD HTML Strict Level 1//EN\">", 
  "<!DOCTYPE \"-//IETF//DTD HTML Strict Level 1//EN//2\">", 
  "<!DOCTYPE \"-//IETF//DTD HTML 2.0 Strict Level 1//EN\">", 
  "<!DOCTYPE \"-//IETF//DTD HTML Strict Level 0//EN\">", 
  "<!DOCTYPE \"-//IETF//DTD HTML Strict Level 0//EN//2.0\">", 
  "<!DOCTYPE \"-//WebTechs//DTD Mozilla HTML//EN\">", 
  "<!DOCTYPE \"-//WebTechs//DTD Mozilla HTML 2//EN\">", 
  "<!DOCTYPE \"-//Netscape Comm Corp //DTD HTML//EN\">", 
  "<!DOCTYPE \"-//Netscape Comm Corp //DTD HTML//EN\">", 
  "<!DOCTYPE \"-//Netscape Comm Corp //DTD Strict HTML//EN\">", 
  "<!DOCTYPE \"-//Microsoft//DTD Internet Explorer 2.0 HTML//EN\">", 
  "<!DOCTYPE \"-//Microsoft//DTD Internet Explorer 2.0 HTML Strict//EN\">", 
  "<!DOCTYPE \"-//Microsoft//DTD Internet Explorer 2.0 Tables//EN\">", 
  "<!DOCTYPE \"-//Microsoft//DTD Internet Explorer 3.0 HTML//EN\">", 
  "<!DOCTYPE \"-//Microsoft//DTD Internet Explorer 3.0 HTML Strict//EN\">", 
  "<!DOCTYPE \"-//Microsoft//DTD Internet Explorer 3.0 Tables//EN\">", 
  "<!DOCTYPE \"-//Sun Microsystems Corp DTD HotJava HTML//EN\">", 
  "<!DOCTYPE \"-//Sun Microsystems Corp //DTD HotJava Strict HTML//EN\">", 
  "<!DOCTYPE \"-//IETF//DTD HTML 2.1E//EN\">", 
  "<!DOCTYPE \"-//O'Reilly and Associates//DTD HTML Extended 1.0//EN\">", 
  "<!DOCTYPE \"-//O'Reilly and Associates//DTD HTML Extended Relaxed 1.0//EN\">", 
  "<!DOCTYPE \"-//O'Reilly and Associates//DTD HTML 2.0//EN\">", 
  "<!DOCTYPE \"-//SQ//DTD HTML 2. HoTMetaL + extensions//EN\">", 
  "<!DOCTYPE \"-//Spyglass//DTD HTML 2.0 Extended//EN\">", 
  "<!DOCTYPE \"+//Silmaril//DTD HTML Pro v0r11 19970101//EN\">", 
  "<!DOCTYPE \"-//W3C//DTD HTML Experimental 19960712//EN\">", 
  "<!DOCTYPE \"-//W3C//DTD HTML 3.2//EN\">", 
  "<!DOCTYPE \"-//W3C//DTD HTML 3.2 Final//EN\">", 
  "<!DOCTYPE \"-//W3C//DTD HTML 3.2 Draft//EN\">", 
  "<!DOCTYPE \"-//W3C//DTD HTML Experimental 970421//EN\">", 
  "<!DOCTYPE \"-//W3C//DTD HTML 3.2S Draft//EN\">", 
  "<!DOCTYPE \"-//IETF//DTD HTML i18n//EN\">",
  0
  };


/**
 *  This is called when it's time to find out 
 *  what mode the parser/DTD should run for this document.
 *  (Each parsercontext can have it's own mode).
 *  
 *  @update  gess 02/17/00
 *  @return  parsermode (define in nsIParser.h)
 */
static
eParseMode DetermineParseMode(nsParser& aParser) {
  const char* theModeStr= PR_GetEnv("PARSE_MODE");
  const char* other="other";

  eParseMode result=eParseMode_unknown;
  nsScanner* theScanner=aParser.GetScanner();

  if(theScanner){

    nsString& theBuffer=theScanner->GetBuffer();
    
    PRInt32 theIndex=theBuffer.Find("<!",PR_FALSE,-1);
    if(kNotFound<theIndex)
      theIndex=theBuffer.Find("DOCTYPE",PR_TRUE,theIndex+1,10);

    if(kNotFound<theIndex) {
    
      //good, we found "DOCTYPE" -- now go find it's end delimiter '>'
      PRInt32 theGTPos=theBuffer.FindChar(kGreaterThan,theIndex+1);
      PRInt32 theEnd=(kNotFound==theGTPos) ? 512 : MinInt(512,theGTPos);
      PRInt32 theSubIndex=theBuffer.Find("-//W3C//DTD",PR_TRUE,theIndex+8,theEnd-(theIndex+8));

      //note that if we don't find '>', then we just scan the first 512 bytes.

      PRInt32 theStartPos=theSubIndex+11;
      PRInt32 theCount=theEnd-theStartPos;

      if(kNotFound<theSubIndex) {

        if(kNotFound<(theSubIndex=theBuffer.Find("XHTML",PR_TRUE,theStartPos,theCount))) {

            //this logic has been changed so that ALL XHTML doc's are no quirks.
          result=eParseMode_noquirks;
        }

        else if(kNotFound<(theSubIndex=theBuffer.Find("HTML",PR_TRUE,theStartPos,theCount))) {

          theStartPos=theSubIndex+5;
          theCount=theEnd-theStartPos;
          nsAutoString theNum;
          theBuffer.Mid(theNum,theStartPos-1,3);
          
          //get the next substring from the buffer, which should be a number.
          //now see what the version number is...

          PRInt32 theErr;
          PRInt32 theMajorVersion=theNum.ToInteger(&theErr);

          switch(theMajorVersion) {

            case 4:
              theStartPos=theSubIndex+3;
              theCount=theEnd-theStartPos;

              if(kNotFound<theBuffer.Find("STRICT",PR_TRUE,theStartPos,theCount)) {
                result=eParseMode_noquirks;
              }
              else            
              if((theBuffer.Find("TRANSITIONAL",PR_TRUE,theStartPos,theCount)>kNotFound)||
                 (theBuffer.Find("FRAMESET",PR_TRUE,theStartPos,theCount)>kNotFound)    ||
                 (theBuffer.Find("LATIN1", PR_TRUE,theStartPos,theCount) >kNotFound)    ||
                 (theBuffer.Find("SYMBOLS",PR_TRUE,theStartPos,theCount) >kNotFound)    ||
                 (theBuffer.Find("SPECIAL",PR_TRUE,theStartPos,theCount) >kNotFound)) {
                result=eParseMode_quirks; // XXX -HACK- Set the appropriate mode.
              }
              break;

            default:
              result= (theMajorVersion<4) ? eParseMode_quirks : eParseMode_noquirks;
              break;
          }

        }
        else if(kNotFound<(theSubIndex=theBuffer.Find("HYPERTEXT MARKUP LANGUAGE",PR_TRUE,theStartPos,theCount))) {
          result=eParseMode_quirks;
        }
      }
      else if(kNotFound<(theSubIndex=theBuffer.Find("ISO/IEC 15445:1999",PR_TRUE,theIndex+8,theEnd-(theIndex+8)))) {
        result=eParseMode_noquirks;        
      }
    }
    else if(kNotFound<(theIndex=theBuffer.Find("?XML",PR_TRUE,0,128))) {
        result=eParseMode_noquirks;
    }
    else {
        //this is debug only, and will go away by the time we ship...
      theIndex=theBuffer.Find("NOQUIRKS",PR_TRUE,0,128);
      if(kNotFound<theIndex) {
        result=eParseMode_noquirks;
      }
    }
  }

  if(theModeStr) 
    if(0==nsCRT::strcasecmp(other,theModeStr))
      return eParseMode_other;    
    return (eParseMode_unknown==result)? eParseMode_quirks:result;

}


/**
 * This gets called just prior to the model actually
 * being constructed. It's important to make this the
 * last thing that happens right before parsing, so we
 * can delay until the last moment the resolution of
 * which DTD to use (unless of course we're assigned one).
 *
 * @update	gess5/18/98
 * @param 
 * @return  error code -- 0 if ok, non-zero if error.
 */
nsresult nsParser::WillBuildModel(nsString& aFilename){

  nsresult result=NS_OK;

  if(mParserContext){
    if(eUnknownDetect==mParserContext->mAutoDetectStatus) {  
      mMajorIteration=-1; 
      mMinorIteration=-1; 
      if(PR_TRUE==FindSuitableDTD(*mParserContext,mParserContext->mScanner->GetBuffer())) {
        mParserContext->mParseMode=DetermineParseMode(*this);  
       // mParserContext->mStreamListenerState=eOnDataAvail;
        mParserContext->mDTD->WillBuildModel( *mParserContext,mSink);
      }//if        
    }//if
  } 
  else result=kInvalidParserContext;    
  return result;
}

/**
 * This gets called when the parser is done with its input.
 * Note that the parser may have been called recursively, so we
 * have to check for a prev. context before closing out the DTD/sink.
 * @update	gess5/18/98
 * @param 
 * @return  error code -- 0 if ok, non-zero if error.
 */
nsresult nsParser::DidBuildModel(nsresult anErrorCode) {
  //One last thing...close any open containers.
  nsresult result=anErrorCode;

  if((mParserContext) && mParserContext->mParserEnabled) {
    if((!mParserContext->mPrevContext) && (mParserContext->mDTD)) {
      result=mParserContext->mDTD->DidBuildModel(anErrorCode,PRBool(0==mParserContext->mPrevContext),this,mSink);
    }
  }//if

  return result;
}


/**
 * This method adds a new parser context to the list,
 * pushing the current one to the next position.
 * @update	gess7/22/98
 * @param   ptr to new context
 * @return  nada
 */
void nsParser::PushContext(CParserContext& aContext) {
  aContext.mPrevContext=mParserContext;  
  if (mParserContext) { 
     aContext.mParserEnabled = mParserContext->mParserEnabled; 
  } 
  mParserContext=&aContext;
}

/**
 * This method pops the topmost context off the stack,
 * returning it to the user. The next context  (if any)
 * becomes the current context.
 * @update	gess7/22/98
 * @return  prev. context
 */
CParserContext* nsParser::PopContext() {
  CParserContext* oldContext=mParserContext;
  if(oldContext) {
    mParserContext=oldContext->mPrevContext;
    // If the old context was blocked, propogate the blocked state
    // back to the newe one.
    if (mParserContext) {
      mParserContext->mParserEnabled = oldContext->mParserEnabled;
    }
  }
  return oldContext;
}

/**
 *  Call this when you want control whether or not the parser will parse
 *  and tokenize input (TRUE), or whether it just caches input to be 
 *  parsed later (FALSE).
 *  
 *  @update  gess 1/29/99
 *  @param   aState determines whether we parse/tokenize or just cache.
 *  @return  current state
 */
void nsParser::SetUnusedInput(nsString& aBuffer) {
  mUnusedInput=aBuffer;
}

/**
 *  Call this when you want to *force* the parser to terminate the
 *  parsing process altogether. This is binary -- so once you terminate
 *  you can't resume without restarting altogether.
 *  
 *  @update  gess 7/4/99
 *  @return  should return NS_OK once implemented
 */
nsresult nsParser::Terminate(void){
  nsresult result=NS_OK;
  if(mParserContext && mParserContext->mDTD)
    result=mParserContext->mDTD->Terminate();
  mInternalState=result;
  return result;
}

/**
 *  Call this when you want control whether or not the parser will parse
 *  and tokenize input (TRUE), or whether it just caches input to be 
 *  parsed later (FALSE).
 *  
 *  @update  gess 1/29/99
 *  @param   aState determines whether we parse/tokenize or just cache.
 *  @return  current state
 */
nsresult nsParser::EnableParser(PRBool aState){

  // If the stream has already finished, there's a good chance
  // that we might start closing things down when the parser
  // is reenabled. To make sure that we're not deleted across
  // the reenabling process, hold a reference to ourselves.
  nsresult result=NS_OK;
  nsIParser* me = this;
  NS_ADDREF(me);

  // If we're reenabling the parser
  if(mParserContext) {
    mParserContext->mParserEnabled=aState;
    if(aState) {

      //printf("  Re-enable parser\n");

      result=ResumeParse();
      if(result!=NS_OK) 
        result=mInternalState;
    }
    else {
      MOZ_TIMER_DEBUGLOG(("Stop: Parse Time: nsParser::EnableParser(), this=%p\n", this));
      MOZ_TIMER_STOP(mParseTime);
    }  
  }

  // Release reference if we added one at the top of this routine
  NS_IF_RELEASE(me);
  return result;
}

/**
 * Call this to query whether the parser is enabled or not.
 *
 *  @update  vidur 4/12/99
 *  @return  current state
 */
PRBool nsParser::IsParserEnabled() {
  return mParserContext->mParserEnabled;
}


/**
 *  This is the main controlling routine in the parsing process. 
 *  Note that it may get called multiple times for the same scanner, 
 *  since this is a pushed based system, and all the tokens may 
 *  not have been consumed by the scanner during a given invocation 
 *  of this method. 
 *
 *  @update  gess 01/04/99
 *  @param   aFilename -- const char* containing file to be parsed.
 *  @return  error code -- 0 if ok, non-zero if error.
 */
nsresult nsParser::Parse(nsIURI* aURL,nsIStreamObserver* aListener,PRBool aVerifyEnabled, void* aKey,eParseMode aMode) {  

  NS_PRECONDITION(0!=aURL,kNullURL);

  nsresult result=kBadURL;
  mObserver = aListener;
  NS_IF_ADDREF(mObserver);
  mDTDVerification=aVerifyEnabled;
  if(aURL) {
    char* spec;
    nsresult rv = aURL->GetSpec(&spec);
    if (rv != NS_OK) {      
      return rv;
    }
    nsAutoString theName(spec);
    nsCRT::free(spec);

    nsScanner* theScanner=new nsScanner(theName,PR_FALSE,mCharset,mCharsetSource);
    CParserContext* pc=new CParserContext(theScanner,aKey,mCommand,aListener);
    if(pc && theScanner) {
      pc->mMultipart=PR_TRUE;
      pc->mContextType=CParserContext::eCTURL;
      PushContext(*pc);
      result=NS_OK;
    }
    else{
      result=mInternalState=NS_ERROR_HTMLPARSER_BADCONTEXT;
    }
  }  
  return result;
}


/**
 * Cause parser to parse input from given stream 
 * @update	vidur 12/11/98
 * @param   aStream is the i/o source
 * @return  error code -- 0 if ok, non-zero if error.
 */
nsresult nsParser::Parse(nsIInputStream& aStream,const nsString& aMimeType,PRBool aVerifyEnabled, void* aKey,eParseMode aMode){

  mDTDVerification=aVerifyEnabled;
  nsresult  result=NS_ERROR_OUT_OF_MEMORY;

  //ok, time to create our tokenizer and begin the process
  nsAutoString theUnknownFilename("unknown");

  nsInputStream input(&aStream);
    
  nsScanner* theScanner=new nsScanner(theUnknownFilename,input,mCharset,mCharsetSource);
  CParserContext* pc=new CParserContext(theScanner,aKey,mCommand,0);
  if(pc && theScanner) {
    PushContext(*pc);
    pc->SetMimeType(aMimeType);
    pc->mStreamListenerState=eOnStart;  
    pc->mMultipart=PR_FALSE;
    pc->mContextType=CParserContext::eCTStream;
    mParserContext->mScanner->Eof();
    result=ResumeParse();
    pc=PopContext();
    delete pc;
  }
  else{
    result=mInternalState=NS_ERROR_HTMLPARSER_BADCONTEXT;
  }  
  return result;
}


/**
 * Call this method if all you want to do is parse 1 string full of HTML text.
 * In particular, this method should be called by the DOM when it has an HTML
 * string to feed to the parser in real-time.
 *
 * @update	gess5/11/98
 * @param   aSourceBuffer contains a string-full of real content
 * @param   aMimeType tells us what type of content to expect in the given string
 * @return  error code -- 0 if ok, non-zero if error.
 */
nsresult nsParser::Parse(const nsString& aSourceBuffer,void* aKey,const nsString&
aMimeType,PRBool aVerifyEnabled,PRBool aLastCall,eParseMode aMode){ 
  
  //NOTE: Make sure that updates to this method don't cause 
  //      bug #2361 to break again! 

#if 0
    //this is only for debug purposes 
  aSourceBuffer.DebugDump(); 
#endif 

  nsresult result=NS_OK; 
  nsParser* me = this; 
  // Maintain a reference to ourselves so we don't go away 
  // till we're completely done. 
  NS_ADDREF(me); 

  if(aSourceBuffer.Length() || mUnusedInput.Length()) { 
    mDTDVerification=aVerifyEnabled; 
    CParserContext* pc=0; 

    if((!mParserContext) || (mParserContext->mKey!=aKey))  { 
      //only make a new context if we dont have one, OR if we do, but has a different context key... 
  
      nsScanner* theScanner=new nsScanner(mUnusedInput,mCharset,mCharsetSource); 
      nsIDTD *theDTD=0; 
      eAutoDetectResult theStatus=eUnknownDetect; 

      if(mParserContext && (mParserContext->mMimeType==aMimeType)) { 
        mParserContext->mDTD->CreateNewInstance(&theDTD); // To fix 32263
        theStatus=mParserContext->mAutoDetectStatus; 

        //added this to fix bug 32022.
      } 

      pc=new CParserContext(theScanner,aKey, mCommand,0,theDTD,theStatus,aLastCall); 

      if(pc && theScanner) { 
        PushContext(*pc); 

        pc->mMultipart=!aLastCall; //by default 
        if (pc->mPrevContext) { 
          pc->mMultipart |= pc->mPrevContext->mMultipart;  //if available 
        } 

        pc->mStreamListenerState = (pc->mMultipart) ? eOnDataAvail : eOnStop; 
        pc->mContextType=CParserContext::eCTString; 
        pc->SetMimeType(aMimeType);
        mUnusedInput.Truncate(0); 

        //printf("Parse(string) iterate: %i",PR_FALSE); 
        pc->mScanner->Append(aSourceBuffer); 
        result=ResumeParse(PR_FALSE); 

      } 
      else { 
        NS_RELEASE(me); 
        return NS_ERROR_OUT_OF_MEMORY; 
      } 

    } 
    else { 
      mParserContext->mScanner->Append(aSourceBuffer); 
      if(!mParserContext->mPrevContext) {
        ResumeParse(PR_FALSE);
      }
    } 
  }//if 
  NS_RELEASE(me); 
  return result; 
}  


/**
 *  Call this method to test whether a given fragment is valid within a given context-stack.
 *  @update  gess 04/01/99
 *  @param   aSourceBuffer contains the content blob you're trying to insert
 *  @param   aInsertPos tells us where in the context stack you're trying to do the insertion
 *  @param   aMimeType tells us what kind of stuff you're inserting
 *  @return  TRUE if valid, otherwise FALSE
 */
PRBool nsParser::IsValidFragment(const nsString& aSourceBuffer,nsITagStack& aStack,PRUint32 anInsertPos,const nsString& aMimeType,eParseMode aMode){

  /************************************************************************************
    This method works like this:
      1. Convert aStack to a markup string
      2. Append a "sentinel" tag to markup string so we know where new content is inserted
      3. Append new context to markup stack
      4. Call the normal parse() methods for a string, using an HTMLContentSink.
         The output of this call is stored in an outputstring
      5. Scan the output string looking for markup inside our sentinel. If non-empty
         then we have to assume that the fragment is valid (at least in part)
   ************************************************************************************/

  nsAutoString  theContext;
  PRUint32 theCount=aStack.GetSize();
  PRUint32 theIndex=0;
  while(theIndex++<theCount){
    theContext.Append("<");
    theContext.Append(aStack.TagAt(theCount-theIndex));
    theContext.Append(">");
  }
  theContext.Append("<endnote>");       //XXXHack! I'll make this better later.
  nsAutoString theBuffer(theContext);
  theBuffer.Append(aSourceBuffer);
  
  PRBool result=PR_FALSE;
  if(theBuffer.Length()){
    //now it's time to try to build the model from this fragment

    nsString theOutput("");
    nsIHTMLContentSink*  theSink=0;
    nsresult theResult=NS_New_HTML_ContentSinkStream(&theSink,&theOutput,0);
    SetContentSink(theSink);
    theResult=Parse(theBuffer,(void*)&theBuffer,aMimeType,PR_FALSE,PR_TRUE);
    theOutput.StripWhitespace();
    if(NS_OK==theResult){
      theOutput.Cut(0,theContext.Length());
      PRInt32 aPos=theOutput.RFind("</endnote>");
      if(-1<aPos)
        theOutput.Truncate(aPos);
      result=PRBool(0<theOutput.Length());
    }
  }
  return result;
}


/**
 *
 *  @update  gess 04/01/99
 *  @param   
 *  @return  
 */
nsresult nsParser::ParseFragment(const nsString& aSourceBuffer,void* aKey,nsITagStack& aStack,PRUint32 anInsertPos,const nsString& aMimeType,eParseMode aMode){

  nsresult result=NS_OK;
  nsAutoString  theContext;
  PRUint32 theCount=aStack.GetSize();
  PRUint32 theIndex=0;
  while(theIndex++<theCount){
    theContext.Append("<");
    theContext.Append(aStack.TagAt(theCount-theIndex));
    theContext.Append(">");
  }
  theContext.Append("<endnote>");       //XXXHack! I'll make this better later.
  nsAutoString theBuffer(theContext);

#if 0
      //use this to force a buffer-full of content as part of a paste operation...
    theBuffer.Append("<title>title</title><a href=\"one\">link</a>");
#else

//#define USEFILE
#ifdef USEFILE

  const char* theFile="c:/temp/rhp.html";
  fstream input(theFile,ios::in);
  char buffer[1024];
  int count=1;
  while(count) {
    input.getline(buffer,sizeof(buffer));
    count=input.gcount();
    if(0<count) {
      buffer[count-1]=0;
      theBuffer.Append(buffer,count-1);
    }
  }

#else
      //this is the normal code path for paste...
    theBuffer.Append(aSourceBuffer); 
#endif

#endif

  if(theBuffer.Length()){
    //now it's time to try to build the model from this fragment

    mObserversEnabled=PR_FALSE; //disable observers for fragments
    result=Parse(theBuffer,(void*)&theBuffer,aMimeType,PR_FALSE,PR_TRUE);
    mObserversEnabled=PR_TRUE; //now reenable.
  }

  return result;
}

 
/**
 *  This routine is called to cause the parser to continue parsing it's underlying stream. 
 *  This call allows the parse process to happen in chunks, such as when the content is push 
 *  based, and we need to parse in pieces.
 *  
 *  An interesting change in how the parser gets used has led us to add extra processing to this method. 
 *  The case occurs when the parser is blocked in one context, and gets a parse(string) call in another context.
 *  In this case, the parserContexts are linked. No problem.
 *
 *  The problem is that Parse(string) assumes that it can proceed unabated, but if the parser is already
 *  blocked that assumption is false. So we needed to add a mechanism here to allow the parser to continue
 *  to process (the pop and free) contexts until 1) it get's blocked again; 2) it runs out of contexts.
 *
 *
 *  @update  rickg 03.10.2000 
 *  @param   allowItertion : set to true if non-script resumption is requested
 *  @param   aIsFinalChunk : tells us when the last chunk of data is provided.
 *  @return  error code -- 0 if ok, non-zero if error.
 */
nsresult nsParser::ResumeParse(PRBool allowIteration, PRBool aIsFinalChunk) {

  //printf("  Resume %i, prev-context: %p\n",allowIteration,mParserContext->mPrevContext);
  

  nsresult result=NS_OK;

  if(mParserContext->mParserEnabled && mInternalState!=NS_ERROR_HTMLPARSER_STOPPARSING) {


    MOZ_TIMER_DEBUGLOG(("Start: Parse Time: nsParser::ResumeParse(), this=%p\n", this));
    MOZ_TIMER_START(mParseTime);

    result=WillBuildModel(mParserContext->mScanner->GetFilename());
    if(mParserContext->mDTD) {

      mParserContext->mDTD->WillResumeParse();
      PRBool theFirstTime=PR_TRUE;
      PRBool theIterationIsOk=(theFirstTime || allowIteration||(!mParserContext->mPrevContext));
       
      while((result==NS_OK) && (theIterationIsOk)) {
        theFirstTime=PR_FALSE;
        if(mUnusedInput.Length()>0) {
          if(mParserContext->mScanner) {
            // -- Ref: Bug# 22485 --
            // Insert the unused input into the source buffer 
            // as if it was read from the input stream. 
            // Adding Insert() per vidur!!
            mParserContext->mScanner->Insert(mUnusedInput);
            mUnusedInput.Truncate(0);
          }
        }

        nsresult theTokenizerResult=Tokenize(aIsFinalChunk);   // kEOF==2152596456
        result=BuildModel(); 

        theIterationIsOk=PRBool(kEOF!=theTokenizerResult);
      
       // Make sure not to stop parsing too early. Therefore, before shutting down the 
        // parser, it's important to check whether the input buffer has been scanned to 
        // completion ( theTokenizerResult should be kEOF ). kEOF -> End of buffer.

        // If we're told to block the parser, we disable all further parsing 
        // (and cache any data coming in) until the parser is re-enabled.

        if(NS_ERROR_HTMLPARSER_BLOCK==result) {
              //BLOCK == 2152596464
           mParserContext->mDTD->WillInterruptParse();
           result=EnableParser(PR_FALSE);
           return result;
        }
        
        else if (NS_ERROR_HTMLPARSER_STOPPARSING==result) {
          mInternalState=result;
          DidBuildModel(mStreamStatus);
          return mInternalState;
        }
                  
        else if((NS_OK==result) && (theTokenizerResult==kEOF)){

          PRBool theContextIsStringBased=PRBool(CParserContext::eCTString==mParserContext->mContextType);
          if( (eOnStop==mParserContext->mStreamListenerState) || 
              (!mParserContext->mMultipart) || theContextIsStringBased) {

            if(!mParserContext->mPrevContext) {
              if(eOnStop==mParserContext->mStreamListenerState) {

                DidBuildModel(mStreamStatus);          

                MOZ_TIMER_DEBUGLOG(("Stop: Parse Time: nsParser::ResumeParse(), this=%p\n", this));
                MOZ_TIMER_STOP(mParseTime);

                MOZ_TIMER_LOG(("Parse Time (this=%p): ", this));
                MOZ_TIMER_PRINT(mParseTime);

                MOZ_TIMER_LOG(("DTD Time: "));
                MOZ_TIMER_PRINT(mDTDTime);

                MOZ_TIMER_LOG(("Tokenize Time: "));
                MOZ_TIMER_PRINT(mTokenizeTime);

                return result;
              }

            }
            else {

              CParserContext* theContext=PopContext();
              if(theContext) {
                theIterationIsOk=PRBool(allowIteration && theContextIsStringBased);
                if(theContext->mCopyUnused) {
                  theContext->mScanner->CopyUnusedData(mUnusedInput);
                }
                delete theContext;
              }
              result = mInternalState;  
                //...then intentionally fall through to WillInterruptParse()...
            }

          }             

        }

        if(kEOF==theTokenizerResult) {
          mParserContext->mDTD->WillInterruptParse();
        }

      }//while
    }//if
    else {
      mInternalState=result=NS_ERROR_HTMLPARSER_UNRESOLVEDDTD;
    }
  }//if

  MOZ_TIMER_DEBUGLOG(("Stop: Parse Time: nsParser::ResumeParse(), this=%p\n", this));
  MOZ_TIMER_STOP(mParseTime);

  return result;
}

/**
 *  This is where we loop over the tokens created in the 
 *  tokenization phase, and try to make sense out of them. 
 *
 *  @update  gess 01/04/99
 *  @param   
 *  @return  error code -- 0 if ok, non-zero if error.
 */
nsresult nsParser::BuildModel() {
  
  //nsDequeIterator e=mParserContext->mTokenDeque.End(); 

//  if(!mParserContext->mCurrentPos)
//    mParserContext->mCurrentPos=new nsDequeIterator(mParserContext->mTokenDeque.Begin());

    //Get the root DTD for use in model building...

  CParserContext* theRootContext=mParserContext;
  nsITokenizer*   theTokenizer=0;

  nsresult result=mParserContext->mDTD->GetTokenizer(theTokenizer);
  if(theTokenizer){

    while(theRootContext->mPrevContext) {
      theRootContext=theRootContext->mPrevContext;
    }

    nsIDTD* theRootDTD=theRootContext->mDTD;
    if(theRootDTD) {      
      MOZ_TIMER_START(mDTDTime);
      nsresult rv=theRootDTD->BuildModel(this,theTokenizer,mTokenObserver,mSink);  
      result=(mInternalState==NS_ERROR_HTMLPARSER_STOPPARSING)? mInternalState:rv; // Fix for 32527
      MOZ_TIMER_STOP(mDTDTime);
    }
  }
  else{
    mInternalState=result=NS_ERROR_HTMLPARSER_BADTOKENIZER;
  }
  return result;
}


/**
 * 
 * @update	gess1/22/99
 * @param 
 * @return
 */
nsITokenizer* nsParser::GetTokenizer(void) {
  nsITokenizer* theTokenizer=0;
  if(mParserContext && mParserContext->mDTD) {
    mParserContext->mDTD->GetTokenizer(theTokenizer);
  }
  return theTokenizer;
}

/*******************************************************************
  These methods are used to talk to the netlib system...
 *******************************************************************/

/**
 *  
 *  
 *  @update  gess 5/12/98
 *  @param   
 *  @return  error code -- 0 if ok, non-zero if error.
 */
nsresult
nsParser::OnProgress(nsIChannel* channel, nsISupports* aContext, PRUint32 aProgress, PRUint32 aProgressMax)
{
  nsresult result=0;
  if (nsnull != mProgressEventSink) {
    mProgressEventSink->OnProgress(channel, aContext, aProgress, aProgressMax);
  }
  return result;
}

/**
 *  
 *  
 *  @update  gess 5/12/98
 *  @param   
 *  @return  error code -- 0 if ok, non-zero if error.
 */
nsresult
nsParser::OnStatus(nsIChannel* channel, nsISupports* aContext, const PRUnichar* aMsg)
{
  nsresult result=0;
  if (nsnull != mProgressEventSink) {
    mProgressEventSink->OnStatus(channel, aContext, aMsg);
  }
  return result;
}

#ifdef rickgdebug
#include <fstream.h>
  fstream* gOutFile;
#endif

/**
 *  
 *  
 *  @update  gess 5/12/98
 *  @param   
 *  @return  error code -- 0 if ok, non-zero if error.
 */
nsresult nsParser::OnStartRequest(nsIChannel* channel, nsISupports* aContext)
{
  NS_PRECONDITION((eNone==mParserContext->mStreamListenerState),kBadListenerInit);

  if (nsnull != mObserver) {
    mObserver->OnStartRequest(channel, aContext);
  }
  mParserContext->mStreamListenerState=eOnStart;
  mParserContext->mAutoDetectStatus=eUnknownDetect;
  mParserContext->mChannel=channel;
  mParserContext->mDTD=0;
  nsresult rv;
  char* contentType = nsnull;
  rv = channel->GetContentType(&contentType);
  if (NS_SUCCEEDED(rv))
  {
    mParserContext->SetMimeType(contentType);
	  nsCRT::free(contentType);
  }
  else
    NS_ASSERTION(contentType, "parser needs a content type to find a dtd");

#ifdef rickgdebug
  gOutFile= new fstream("c:/temp/out.file",ios::trunc);
#endif

  return NS_OK;
}


#define UCS2_BE "UTF-16BE"
#define UCS2_LE "UTF-16LE"
#define UCS4_BE "UTF-32BE"
#define UCS4_LE "UTF-32LE"
#define UCS4_2143 "X-ISO-10646-UCS-4-2143"
#define UCS4_3412 "X-ISO-10646-UCS-4-3412"

static PRBool detectByteOrderMark(const unsigned char* aBytes, PRInt32 aLen, nsString& oCharset, nsCharsetSource& oCharsetSource) {
 oCharsetSource= kCharsetFromAutoDetection;
 oCharset = "";
 // see http://www.w3.org/TR/1998/REC-xml-19980210#sec-oCharseting
 // for details
 switch(aBytes[0])
	 {
   case 0x00:
     if(0x00==aBytes[1]) {
        // 00 00
        if((0x00==aBytes[2]) && (0x3C==aBytes[3])) {
           // 00 00 00 3C UCS-4, big-endian machine (1234 order)
           oCharset = UCS4_BE;
        } else if((0x3C==aBytes[2]) && (0x00==aBytes[3])) {
           // 00 00 3C 00 UCS-4, unusual octet order (2143)
           oCharset = UCS4_2143;
        } 
     } else if(0x3C==aBytes[1]) {
        // 00 3C
        if((0x00==aBytes[2]) && (0x00==aBytes[3])) {
           // 00 3C 00 00 UCS-4, unusual octet order (3412)
           oCharset = UCS4_3412;
        } else if((0x3C==aBytes[2]) && (0x3F==aBytes[3])) {
           // 00 3C 00 3F UTF-16, big-endian, no Byte Order Mark
           oCharset = UCS2_BE; // should change to UTF-16BE
        } 
     }
   break;
   case 0x3C:
     if(0x00==aBytes[1]) {
        // 3C 00
        if((0x00==aBytes[2]) && (0x00==aBytes[3])) {
           // 3C 00 00 00 UCS-4, little-endian machine (4321 order)
           oCharset = UCS4_LE;
        } else if((0x3F==aBytes[2]) && (0x00==aBytes[3])) {
           // 3C 00 3F 00 UTF-16, little-endian, no Byte Order Mark
           oCharset = UCS2_LE; // should change to UTF-16LE
        } 
     } else if((0x3C==aBytes[0]) && (0x3F==aBytes[1]) &&
               (0x78==aBytes[2]) && (0x6D==aBytes[3]) &&
               (0 == PL_strncmp("<?xml version", (char*)aBytes, 13 ))) {
       // 3C 3F 78 6D
       nsAutoString firstXbytes("");
       firstXbytes.Append((const char*)aBytes, (PRInt32)
                       ((aLen > XMLENCODING_PEEKBYTES)?
                       XMLENCODING_PEEKBYTES:
                       aLen)); 
       PRInt32 xmlDeclEnd = firstXbytes.Find("?>", PR_FALSE, 13);
	   // 27 == strlen("<xml? version="1" encoding=");
       if((kNotFound != xmlDeclEnd) &&(xmlDeclEnd > 27 )){
           firstXbytes.Cut(xmlDeclEnd, firstXbytes.Length()-xmlDeclEnd);
           PRInt32 encStart = firstXbytes.Find("encoding", PR_FALSE,13);
           if(kNotFound != encStart) {
             encStart = firstXbytes.FindCharInSet("\"'", encStart+8);
                              // 8 == strlen("encoding")
             if(kNotFound != encStart) {
                PRUnichar q = firstXbytes.CharAt(encStart); 
                PRInt32 encEnd = firstXbytes.FindChar(q, PR_FALSE, encStart+1);
                if(kNotFound != encEnd) {
                   PRInt32 count = encEnd - encStart -1;
                   if(count >0) {
                      firstXbytes.Mid(oCharset,(encStart+1), count);
                      oCharsetSource= kCharsetFromMetaTag;
                   }
                }
             }
           }
       }
     }
   break;
   case 0xFE:
     if(0xFF==aBytes[1]) {
        // FE FF
        // UTF-16, big-endian 
        oCharset = UCS2_BE; // should change to UTF-16BE
        oCharsetSource= kCharsetFromByteOrderMark;
     }
   break;
   case 0xFF:
     if(0xFE==aBytes[1]) {
        // FF FE
        // UTF-16, little-endian 
        oCharset = UCS2_LE; // should change to UTF-16LE
        oCharsetSource= kCharsetFromByteOrderMark;
     }
   break;
   // case 0x4C: if((0x6F==aBytes[1]) && ((0xA7==aBytes[2] && (0x94==aBytes[3])) {
   //   We do not care EBCIDIC here....
   // }
   // break;
 }  // switch
 return oCharset.Length() > 0;
}


/**
 *  
 *  
 *  @update  gess 1/4/99
 *  @param   pIStream contains the input chars
 *  @param   length is the number of bytes waiting input
 *  @return  error code (usually 0)
 */

nsresult nsParser::OnDataAvailable(nsIChannel* channel, nsISupports* aContext, 
                                   nsIInputStream *pIStream, PRUint32 sourceOffset, PRUint32 aLength) 
{ 

 
NS_PRECONDITION(((eOnStart==mParserContext->mStreamListenerState)||(eOnDataAvail==mParserContext->mStreamListenerState)),kOnStartNotCalled);

  nsresult result=NS_OK; 

  CParserContext *theContext=mParserContext; 
  
  while(theContext) { 
    if(theContext->mChannel!=channel && theContext->mPrevContext) 
      theContext=theContext->mPrevContext; 
    else break; 
  } 

  if(theContext && theContext->mChannel==channel) { 

    theContext->mStreamListenerState=eOnDataAvail; 

    if(eInvalidDetect==theContext->mAutoDetectStatus) { 
      if(theContext->mScanner) { 
        theContext->mScanner->GetBuffer().Truncate(); 
      } 
    } 

    PRInt32 newLength=(aLength>theContext->mTransferBufferSize) ? aLength :
theContext->mTransferBufferSize; 
    if(!theContext->mTransferBuffer) { 
      theContext->mTransferBufferSize=newLength; 
      theContext->mTransferBuffer=new char[newLength+20]; 
    } 
    else if(aLength>theContext->mTransferBufferSize){ 
      delete [] theContext->mTransferBuffer; 
      theContext->mTransferBufferSize=newLength; 
      theContext->mTransferBuffer=new char[newLength+20]; 
    } 

    if(theContext->mTransferBuffer) { 

        //We need to add code to defensively deal with the case where the transfer buffer is null. 

      PRUint32  theTotalRead=0; 
      PRUint32  theNumRead=1;   //init to a non-zero value 
      int       theStartPos=0; 

      PRBool needCheckFirst4Bytes = 
              ((0 == sourceOffset) && (mCharsetSource<kCharsetFromAutoDetection)); 
      while ((theNumRead>0) && (aLength>theTotalRead) && (NS_OK==result)) { 
        result = pIStream->Read(theContext->mTransferBuffer, aLength, &theNumRead); 
        if(NS_SUCCEEDED(result) && (theNumRead>0)) { 
          if(needCheckFirst4Bytes && (theNumRead >= 4)) { 
             nsCharsetSource guessSource; 
             nsAutoString guess(""); 
  
             needCheckFirst4Bytes = PR_FALSE; 
             if(detectByteOrderMark((const unsigned char*)theContext->mTransferBuffer, 
                                    theNumRead, guess, guessSource)) 
             { 
    #ifdef DEBUG_XMLENCODING 
                printf("xmlencoding detect- %s\n", guess.ToNewCString()); 
    #endif 
                this->SetDocumentCharset(guess, guessSource); 
             } 
          } 
          theTotalRead+=theNumRead; 
          if(mParserFilter) 
             mParserFilter->RawBuffer(theContext->mTransferBuffer, &theNumRead); 

    #ifdef NS_DEBUG 
          unsigned int index=0; 
          for(index=0;index<theNumRead;index++) { 
            if(0==theContext->mTransferBuffer[index]){ 
              printf("\nNull found at buffer[%i] provided by netlib...\n",index); 
              break; 
            } 
          } 
    #endif 

          theContext->mScanner->Append(theContext->mTransferBuffer,theNumRead); 

#if 0
          static int dump=0;
          if(dump) {
            theContext->mTransferBuffer[theNumRead]=0;
            printf("\n-----------------\n%s",theContext->mTransferBuffer);
          }
#endif

    #ifdef rickgdebug 
          theContext->mTransferBuffer[theNumRead]=0; 
          theContext->mTransferBuffer[theNumRead+1]=0; 
          theContext->mTransferBuffer[theNumRead+2]=0; 
          cout << theContext->mTransferBuffer; 
    #endif 

        } //if 
        theStartPos+=theNumRead; 
      }//while 

      result=ResumeParse(); 
    } //if 

  } //if 

  return result; 
} 

/**
 *  This is called by the networking library once the last block of data
 *  has been collected from the net.
 *  
 *  @update  gess 04/01/99
 *  @param   
 *  @return  
 */
nsresult nsParser::OnStopRequest(nsIChannel* channel, nsISupports* aContext,
                                 nsresult status, const PRUnichar* aMsg)
{  

  nsresult result=NS_OK;
  
  if(eOnStart==mParserContext->mStreamListenerState) {

    //If you're here, then OnDataAvailable() never got called. 
    //Prior to necko, we never dealt with this case, but the problem may have existed.
    //What we'll do (for now at least) is construct a blank HTML document.
    nsAutoString  temp("<html><body></body></html>");
    mParserContext->mScanner->Append(temp);
    result=ResumeParse(PR_TRUE,PR_TRUE);    
  }

  mParserContext->mStreamListenerState=eOnStop;
  mStreamStatus=status;

  if(mParserFilter)
     mParserFilter->Finish();

  mParserContext->mScanner->SetIncremental(PR_FALSE);
  result=ResumeParse(PR_TRUE,PR_TRUE);
  
  // If the parser isn't enabled, we don't finish parsing till
  // it is reenabled.


  // XXX Should we wait to notify our observers as well if the
  // parser isn't yet enabled?
  if (nsnull != mObserver) {
    mObserver->OnStopRequest(channel, aContext, status, aMsg);
  }

#ifdef rickgdebug
  if(gOutFile){
    gOutFile->close();
    delete gOutFile;
    gOutFile=0;
  }
#endif

  return result;
}


/*******************************************************************
  Here comes the tokenization methods...
 *******************************************************************/


/**
 *  Part of the code sandwich, this gets called right before
 *  the tokenization process begins. The main reason for
 *  this call is to allow the delegate to do initialization.
 *  
 *  @update  gess 01/04/99
 *  @param   
 *  @return  TRUE if it's ok to proceed
 */
PRBool nsParser::WillTokenize(PRBool aIsFinalChunk){
  nsITokenizer* theTokenizer=0;
  nsresult result=mParserContext->mDTD->GetTokenizer(theTokenizer);
  if (theTokenizer) {
    result = theTokenizer->WillTokenize(aIsFinalChunk);
  }  
  return result;
}


/**
 *  This is the primary control routine to consume tokens. 
 *	It iteratively consumes tokens until an error occurs or 
 *	you run out of data.
 *  
 *  @update  gess 01/04/99
 *  @return  error code -- 0 if ok, non-zero if error.
 */
nsresult nsParser::Tokenize(PRBool aIsFinalChunk){

  ++mMajorIteration; 

  nsITokenizer* theTokenizer=0;
  nsresult result=mParserContext->mDTD->GetTokenizer(theTokenizer);

  if(theTokenizer){    
    PRBool flushTokens=PR_FALSE;

    MOZ_TIMER_START(mTokenizeTime);

    WillTokenize(aIsFinalChunk);
    while(NS_SUCCEEDED(result)) {
      mParserContext->mScanner->Mark();
      ++mMinorIteration;
      result=theTokenizer->ConsumeToken(*mParserContext->mScanner,flushTokens);
      if(NS_FAILED(result)) {
        mParserContext->mScanner->RewindToMark();
        if(kEOF==result){
          break;
        }
        else if(NS_ERROR_HTMLPARSER_STOPPARSING==result)
          return Terminate();
      }
      else if(flushTokens && mObserversEnabled) {
        // I added the extra test of mObserversEnabled to fix Bug# 23931.
        // Flush tokens on seeing </SCRIPT> -- Ref: Bug# 22485 --
        // Also remember to update the marked position.
        mParserContext->mScanner->Mark();
        break; 
      }
    } 
    DidTokenize(aIsFinalChunk);

    MOZ_TIMER_STOP(mTokenizeTime);
  }  
  else{
    result=mInternalState=NS_ERROR_HTMLPARSER_BADTOKENIZER;
  }
  return result;
}

/**
 *  This is the tail-end of the code sandwich for the
 *  tokenization process. It gets called once tokenziation
 *  has completed for each phase.
 *  
 *  @update  gess 01/04/99
 *  @param   
 *  @return  TRUE if all went well
 */
PRBool nsParser::DidTokenize(PRBool aIsFinalChunk){
  PRBool result=PR_TRUE;

  nsITokenizer* theTokenizer=0;
  nsresult rv=mParserContext->mDTD->GetTokenizer(theTokenizer);

  if (NS_SUCCEEDED(rv) && theTokenizer) {
    result = theTokenizer->DidTokenize(aIsFinalChunk);
    if(mTokenObserver) {
      PRInt32 theCount=theTokenizer->GetCount();
      PRInt32 theIndex;
      for(theIndex=0;theIndex<theCount;theIndex++){
        if((*mTokenObserver)(theTokenizer->GetTokenAt(theIndex))){
          //add code here to pull unwanted tokens out of the stack...
        }
      }//for      
    }//if
  }
  return result;
}

void nsParser::DebugDumpSource(nsOutputStream& aStream) {
  PRInt32 theIndex=-1;

  nsITokenizer* theTokenizer=0;
  if(NS_SUCCEEDED(mParserContext->mDTD->GetTokenizer(theTokenizer))){
    CToken* theToken;
    while(nsnull != (theToken=theTokenizer->GetTokenAt(++theIndex))) {
      // theToken->DebugDumpToken(out);
      theToken->DebugDumpSource(aStream);
    }
  }
}


/**
 * Call this to get a newly constructed tagstack
 * @update	gess 5/05/99
 * @param   aTagStack is an out parm that will contain your result
 * @return  NS_OK if successful, or NS_HTMLPARSER_MEMORY_ERROR on error
 */
nsresult nsParser::CreateTagStack(nsITagStack** aTagStack){
  *aTagStack=new nsTagStack();
  if(*aTagStack)
    return NS_OK;
  return NS_ERROR_OUT_OF_MEMORY;
}

/** 
 * Get the DTD associated with this parser
 * @update vidur 9/29/99
 * @param aDTD out param that will contain the result
 * @return NS_OK if successful, NS_ERROR_FAILURE for runtime error
 */
NS_IMETHODIMP 
nsParser::GetDTD(nsIDTD** aDTD)
{
  if (mParserContext) {
    *aDTD = mParserContext->mDTD;
    NS_IF_ADDREF(mParserContext->mDTD);
  }
  
  return NS_OK;
}


/** 
 * Get the observer service
 *
 * @update rickg 11/22/99
 * @return ptr to server or NULL
 */
CObserverService* nsParser::GetObserverService(void) { 
    //XXX Hack! this should be XPCOM based!
  if(mObserversEnabled)
    return &mObserverService; 
  return 0;
}

