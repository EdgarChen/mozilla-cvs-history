/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
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
#ifndef nsIFrame_h___
#define nsIFrame_h___

#include <stdio.h>
#include "nslayout.h"
#include "nsISupports.h"
#include "nsSize.h"
#include "nsGUIEvent.h"
#include "nsStyleStruct.h"

class nsIAtom;
class nsIContent;
class nsIFrame;
class nsIPresContext;
class nsIPresShell;
class nsIRenderingContext;
class nsISizeOfHandler;
class nsISpaceManager;
class nsIStyleContext;
class nsIView;
class nsIWidget;
class nsIReflowCommand;
class nsAutoString;
class nsString;
class nsIFocusTracker; 
class nsStyleChangeList;

struct nsPeekOffsetStruct;
struct nsPoint;
struct nsRect;
struct nsStyleStruct;
class  nsIDOMRange;
class  nsICaret;
struct PRLogModuleInfo;

// IID for the nsIFrame interface 
// a6cf9050-15b3-11d2-932e-00805f8add32
#define NS_IFRAME_IID \
 { 0xa6cf9050, 0x15b3, 0x11d2,{0x93, 0x2e, 0x00, 0x80, 0x5f, 0x8a, 0xdd, 0x32}}

/**
 * Indication of how the frame can be split. This is used when doing runaround
 * of floaters, and when pulling up child frames from a next-in-flow.
 *
 * The choices are splittable, not splittable at all, and splittable in
 * a non-rectangular fashion. This last type only applies to block-level
 * elements, and indicates whether splitting can be used when doing runaround.
 * If you can split across page boundaries, but you expect each continuing
 * frame to be the same width then return frSplittable and not
 * frSplittableNonRectangular.
 *
 * @see #IsSplittable()
 */
typedef PRUint32 nsSplittableType;

#define NS_FRAME_NOT_SPLITTABLE             0   // Note: not a bit!
#define NS_FRAME_SPLITTABLE                 0x1
#define NS_FRAME_SPLITTABLE_NON_RECTANGULAR 0x3

#define NS_FRAME_IS_SPLITTABLE(type)\
  (0 != ((type) & NS_FRAME_SPLITTABLE))

#define NS_FRAME_IS_NOT_SPLITTABLE(type)\
  (0 == ((type) & NS_FRAME_SPLITTABLE))

//----------------------------------------------------------------------

/**
 * Frame state bits. Any bits not listed here are reserved for future
 * extensions, but must be stored by the frames.
 */
typedef PRUint32 nsFrameState;

#define NS_FRAME_IN_REFLOW 0x00000001

// This bit is set when a frame is created. After it has been reflowed
// once (during the DidReflow with a finished state) the bit is
// cleared.
#define NS_FRAME_FIRST_REFLOW 0x00000002

// If this bit is is set, then the view position and size should be
// kept in sync with the frame position and size. If the bit is not
// set then it's the responsibility of the frame itself (or whoever
// created the view) to position and size its associated view
#define NS_FRAME_SYNC_FRAME_AND_VIEW 0x00000004

// If this bit is set, then there is a child frame in the frame that
// extends outside this frame's bounding box. The implication is that
// the frames rect does not completely cover its children and
// therefore operations like rendering and hit testing (for example)
// must operate differently.
#define NS_FRAME_OUTSIDE_CHILDREN 0x00000008

// If this bit is set, then a reference to the frame is being held
// elsewhere.  The frame may want to send a notification when it is
// destroyed to allow these references to be cleared.
#define NS_FRAME_EXTERNAL_REFERENCE 0x00000010

// If this bit is set, then the frame is a replaced element. For example,
// a frame displaying an image
#define NS_FRAME_REPLACED_ELEMENT 0x00000020

// If this bit is set, then the frame corresponds to generated content
#define NS_FRAME_GENERATED_CONTENT 0x00000040

// If this bit is set, then the frame has requested one or more image
// loads via the nsIPresContext.StartLoadImage API at some time during
// its lifetime.
#define NS_FRAME_HAS_LOADED_IMAGES 0x00000080

// If this bit is set, then the frame is has been moved out of the flow,
// e.g., it is absolutely positioned or floated
#define NS_FRAME_OUT_OF_FLOW 0x00000100

// If this bit is set, then the frame reflects content that may be selected
#define NS_FRAME_SELECTED_CONTENT 0x00000200

// If this bit is set, then the frame is dirty and needs to be reflowed.
// This bit is set when the frame is first created
#define NS_FRAME_IS_DIRTY 0x00000400

// If this bit is set then the frame is unflowable.
#define NS_FRAME_IS_UNFLOWABLE 0x00000800

// The low 16 bits of the frame state word are reserved by this API.
#define NS_FRAME_RESERVED 0x0000FFFF

// The upper 16 bits of the frame state word are reserved for frame
// implementations.
#define NS_FRAME_IMPL_RESERVED 0xFFFF0000

//----------------------------------------------------------------------

enum nsFramePaintLayer {
  eFramePaintLayer_Underlay = 0,
  eFramePaintLayer_Content = 1,
  eFramePaintLayer_Overlay = 2
};

enum nsSelectionAmount {
  eSelectCharacter = 0,
  eSelectWord      = 1,
  eSelectLine      = 2,  //previous drawn line in flow.
  eSelectBeginLine = 3,
  eSelectEndLine   = 4,
  eSelectNoAmount  = 5,   //just bounce back current offset.
  eSelectDir       = 6    //select next/previous frame based on direction
};

enum nsDirection {
  eDirNext    = 0,
  eDirPrevious= 1
};

enum nsSpread {
  eSpreadNone   = 0,
  eSpreadAcross = 1,
  eSpreadDown   = 2
};

//----------------------------------------------------------------------

/**
 * A frame in the layout model. This interface is supported by all frame
 * objects.
 *
 * Frames can have multiple child lists: the default unnamed child list
 * (referred to as the <i>principal</i> child list, and additional named
 * child lists. There is an ordering of frames within a child list, but
 * there is no order defined between frames in different child lists of
 * the same parent frame.
 *
 * Frames are NOT reference counted. Use the Destroy() member function
 * to destroy a frame. The lifetime of the frame hierarchy is bounded by the
 * lifetime of the presentation shell which owns the frames.
 */
class nsIFrame : public nsISupports
{
public:
  /**
   * Called to initialize the frame. This is called immediately after creating
   * the frame.
   *
   * If the frame is a continuing frame, then aPrevInFlow indicates the previous
   * frame (the frame that was split). You should connect the continuing frame to
   * its prev-in-flow, e.g. by using the AppendToFlow() function
   *
   * If you want a view associated with your frame, you should create the view
   * now.
   *
   * @param   aContent the content object associated with the frame
   * @param   aGeometricParent  the geometric parent frame
   * @param   aContentParent  the content parent frame
   * @param   aContext the style context associated with the frame
   * @param   aPrevInFlow the prev-in-flow frame
   * @see #AppendToFlow()
   */
  NS_IMETHOD  Init(nsIPresContext&  aPresContext,
                   nsIContent*      aContent,
                   nsIFrame*        aParent,
                   nsIStyleContext* aContext,
                   nsIFrame*        aPrevInFlow) = 0;

  /**
   * Destroys this frame and each of its child frames (recursively calls
   * Destroy() for each child)
   */
  NS_IMETHOD  Destroy(nsIPresContext& aPresContext) = 0;

  /**
   * Called to set the initial list of frames. This happens after the frame
   * has been initialized.
   *
   * This is only called once for a given child list, and won't be called
   * at all for child lists with no initial list of frames.
   *
   * @param   aListName the name of the child list. A NULL pointer for the atom
   *            name means the unnamed principal child list
   * @param   aChildList list of child frames. Each of the frames has its
   *            NS_FRAME_IS_DIRTY bit set
   * @return  NS_ERROR_INVALID_ARG if there is no child list with the specified
   *            name,
   *          NS_ERROR_UNEXPECTED if the frame is an atomic frame or if the
   *            initial list of frames has already been set for that child list,
   *          NS_OK otherwise
   * @see     #Init()
   */
  NS_IMETHOD  SetInitialChildList(nsIPresContext& aPresContext,
                                  nsIAtom*        aListName,
                                  nsIFrame*       aChildList) = 0;

  /**
   * This method is responsible for appending frames to the frame
   * list.  The implementation should append the frames to the specified
   * child list and then generate a reflow command.
   *
   * @param   aListName the name of the child list. A NULL pointer for the atom
   *            name means the unnamed principal child list
   * @param   aFrameList list of child frames to append. Each of the frames has
   *            its NS_FRAME_IS_DIRTY bit set
   * @return  NS_ERROR_INVALID_ARG if there is no child list with the specified
   *            name,
   *          NS_ERROR_UNEXPECTED if the frame is an atomic frame,
   *          NS_OK otherwise
   */
  NS_IMETHOD AppendFrames(nsIPresContext& aPresContext,
                          nsIPresShell&   aPresShell,
                          nsIAtom*        aListName,
                          nsIFrame*       aFrameList) = 0;

  /**
   * This method is responsible for inserting frames into the frame
   * list.  The implementation should insert the new frames into the specified
   * child list and then generate a reflow command.
   *
   * @param   aListName the name of the child list. A NULL pointer for the atom
   *            name means the unnamed principal child list
   * @param   aPrevFrame the frame to insert frames <b>after</b>
   * @param   aFrameList list of child frames to insert <b>after</b> aPrevFrame.
   *            Each of the frames has its NS_FRAME_IS_DIRTY bit set
   * @return  NS_ERROR_INVALID_ARG if there is no child list with the specified
   *            name,
   *          NS_ERROR_UNEXPECTED if the frame is an atomic frame,
   *          NS_OK otherwise
   */
  NS_IMETHOD InsertFrames(nsIPresContext& aPresContext,
                          nsIPresShell&   aPresShell,
                          nsIAtom*        aListName,
                          nsIFrame*       aPrevFrame,
                          nsIFrame*       aFrameList) = 0;

  /**
   * This method is responsible for removing a frame in the frame
   * list.  The implementation should do something with the removed frame
   * and then generate a reflow command. The implementation is responsible
   * for destroying aOldFrame (the caller mustn't destroy aOldFrame).
   *
   * @param   aListName the name of the child list. A NULL pointer for the atom
   *            name means the unnamed principal child list
   * @param   aOldFrame the frame to remove
   * @return  NS_ERROR_INVALID_ARG if there is no child list with the specified
   *            name,
   *          NS_ERROR_FAILURE if the child frame is not in the specified
   *            child list,
   *          NS_ERROR_UNEXPECTED if the frame is an atomic frame,
   *          NS_OK otherwise
   */
  NS_IMETHOD RemoveFrame(nsIPresContext& aPresContext,
                         nsIPresShell&   aPresShell,
                         nsIAtom*        aListName,
                         nsIFrame*       aOldFrame) = 0;

  /**
   * This method is responsible for replacing the old frame with the
   * new frame. The old frame should be destroyed and the new frame inserted
   * in its place in the specified child list.
   *
   * @param   aListName the name of the child list. A NULL pointer for the atom
   *            name means the unnamed principal child list
   * @param   aOldFrame the frame to remove
   * @param   aNewFrame the frame to replace it with. The new frame has its
   *            NS_FRAME_IS_DIRTY bit set
   * @return  NS_ERROR_INVALID_ARG if there is no child list with the specified
   *            name,
   *          NS_ERROR_FAILURE if the old child frame is not in the specified
   *            child list,
   *          NS_ERROR_UNEXPECTED if the frame is an atomic frame,
   *          NS_OK otherwise
   */
  NS_IMETHOD ReplaceFrame(nsIPresContext& aPresContext,
                          nsIPresShell&   aPresShell,
                          nsIAtom*        aListName,
                          nsIFrame*       aOldFrame,
                          nsIFrame*       aNewFrame) = 0;

  /**
   * Get the content object associated with this frame. Adds a reference to
   * the content object so the caller must do a release.
   *
   * @see nsISupports#Release()
   */
  NS_IMETHOD  GetContent(nsIContent** aContent) const = 0;

  /**
   * Get the offsets of the frame. most will be 0,0
   *
   */
  NS_IMETHOD GetOffsets(PRInt32 &start, PRInt32 &end) const = 0;

  /**
   * Get the style context associated with this frame. Note that GetStyleContext()
   * adds a reference to the style context so the caller must do a release.
   *
   * @see nsISupports#Release()
   */
  NS_IMETHOD  GetStyleContext(nsIStyleContext** aStyleContext) const = 0;
  NS_IMETHOD  SetStyleContext(nsIPresContext*  aPresContext,
                              nsIStyleContext* aContext) = 0;

  /**
   * Get the style data associated with this frame.
   */
  NS_IMETHOD  GetStyleData(nsStyleStructID       aSID,
                           const nsStyleStruct*& aStyleStruct) const = 0;

  /**
   * These methods are to access any additional style contexts that
   * the frame may be holding. These are contexts that are children
   * of the frame's primary context and are NOT used as style contexts
   * for any child frames. These contexts also MUST NOT have any child 
   * contexts whatsoever. If you need to insert style contexts into the
   * style tree, then you should create pseudo element frames to own them
   * The indicies must be consecutive and implementations MUST return an 
   * NS_ERROR_INVALID_ARG if asked for an index that is out of range.
   */
  NS_IMETHOD  GetAdditionalStyleContext(PRInt32 aIndex, 
                                        nsIStyleContext** aStyleContext) const = 0;
  NS_IMETHOD  SetAdditionalStyleContext(PRInt32 aIndex, 
                                        nsIStyleContext* aStyleContext) = 0;

  /**
   * Accessor functions for geometric parent
   */
  NS_IMETHOD  GetParent(nsIFrame** aParent) const = 0;
  NS_IMETHOD  SetParent(const nsIFrame* aParent) = 0;

  /**
   * Bounding rect of the frame. The values are in twips, and the origin is
   * relative to the upper-left of the geometric parent. The size includes the
   * content area, borders, and padding.
   */
  NS_IMETHOD  GetRect(nsRect& aRect) const = 0;
  NS_IMETHOD  GetOrigin(nsPoint& aPoint) const = 0;
  NS_IMETHOD  GetSize(nsSize& aSize) const = 0;
  NS_IMETHOD  SetRect(nsIPresContext* aPresContext,
                      const nsRect&   aRect) = 0;
  NS_IMETHOD  MoveTo(nsIPresContext* aPresContext,
                     nscoord         aX,
                     nscoord         aY) = 0;
  NS_IMETHOD  SizeTo(nsIPresContext* aPresContext,
                     nscoord         aWidth,
                     nscoord         aHeight) = 0;

  /**
   * Used to iterate the list of additional child list names. Returns the atom
   * name for the additional child list at the specified 0-based index, or a
   * NULL pointer if there are no more named child lists.
   *
   * Note that the list is only the additional named child lists and does not
   * include the unnamed principal child list.
   *
   * @return NS_ERROR_INVALID_ARG if the index is < 0 and NS_OK otherwise
   */
  NS_IMETHOD  GetAdditionalChildListName(PRInt32   aIndex,
                                         nsIAtom** aListName) const = 0;

  /**
   * Get the first child frame from the specified child list.
   *
   * @param   aListName the name of the child list. A NULL pointer for the atom
   *            name means the unnamed principal child list
   * @return  NS_ERROR_INVALID_ARG if there is no child list with the specified name
   * @see     #GetAdditionalListName()
   */
  NS_IMETHOD  FirstChild(nsIAtom* aListName, nsIFrame** aFirstChild) const = 0;

  /**
   * Child frames are linked together in a singly-linked
   */
  NS_IMETHOD  GetNextSibling(nsIFrame** aNextSibling) const = 0;
  NS_IMETHOD  SetNextSibling(nsIFrame* aNextSibling) = 0;

  /**
   * Paint is responsible for painting the a frame. The aWhichLayer
   * argument indicates which layer of painting should be done during
   * the call.
   */
  NS_IMETHOD  Paint(nsIPresContext&      aPresContext,
                    nsIRenderingContext& aRenderingContext,
                    const nsRect&        aDirtyRect,
                    nsFramePaintLayer    aWhichLayer) = 0;

  /**
   * Event handling of GUI events.
   *
   * @param   aEvent event structure describing the type of event and rge widget
   *            where the event originated
   * @param   aEventStatus a return value indicating whether the event was handled
   *            and whether default processing should be done
   *
   * XXX From a frame's perspective it's unclear what the effect of the event status
   * is. Does it cause the event to continue propagating through the frame hierarchy
   * or is it just returned to the widgets?
   *
   * @see     nsGUIEvent
   * @see     nsEventStatus
   */
  NS_IMETHOD  HandleEvent(nsIPresContext& aPresContext,
                          nsGUIEvent*     aEvent,
                          nsEventStatus&  aEventStatus) = 0;

  NS_IMETHOD GetContentAndOffsetsFromPoint(nsIPresContext& aCX,
                                           const nsPoint&  aPoint,
                                           nsIContent **   aNewContent,
                                           PRInt32&        aContentOffset,
                                           PRInt32&        aContentOffsetEnd,
                                           PRBool&         aBeginFrameContent) = 0;


  /**
   * Get the cursor for a given frame.
   */
  NS_IMETHOD  GetCursor(nsIPresContext& aPresContext,
                        nsPoint&        aPoint,
                        PRInt32&        aCursor) = 0;

  NS_IMETHOD  GetFrameForPoint(nsIPresContext* aPresContext,
                               const nsPoint& aPoint, 
                               nsIFrame**     aFrame) = 0;
  
  
  /**
   * Get a point (in the frame's coordinate space) given an offset into
   * the content. This point should be on the baseline of text with
   * the correct horizontal offset
   */
  NS_IMETHOD  GetPointFromOffset(nsIPresContext*          inPresContext,
															   nsIRenderingContext*     inRendContext,
                                 PRInt32                  inOffset,
                                 nsPoint*                 outPoint) = 0;
  
  /**
   * Get the child frame of this frame which contains the given
   * content offset. outChildFrame may be this frame, or nsnull on return.
   * outContentOffset returns the content offset relative to the start
   * of the returned node. You can also pass a hint which tells the method
   * to stick to the end of the first found frame or the beginning of the 
   * next in case the offset falls on a boundary.
   */
  NS_IMETHOD  GetChildFrameContainingOffset(PRInt32       inContentOffset,
                                 PRBool                   inHint,//false stick left
                                 PRInt32*                 outFrameContentOffset,
                                 nsIFrame*                *outChildFrame) = 0;

 /**
   * Get the current frame-state value for this frame. aResult is
   * filled in with the state bits. The return value has no
   * meaning.
   */
  NS_IMETHOD  GetFrameState(nsFrameState* aResult) = 0;

  /**
   * Set the current frame-state value for this frame. The return
   * value has no meaning.
   */
  NS_IMETHOD  SetFrameState(nsFrameState aNewState) = 0;

  /**
   * This call is invoked when content is changed in the content tree.
   * The first frame that maps that content is asked to deal with the
   * change by generating an incremental reflow command.
   *
   * @param aIndexInParent the index in the content container where
   *          the new content was deleted.
   */
  NS_IMETHOD  ContentChanged(nsIPresContext* aPresContext,
                             nsIContent*     aChild,
                             nsISupports*    aSubContent) = 0;

  /**
   * This call is invoked when the value of a content objects's attribute
   * is changed. 
   * The first frame that maps that content is asked to deal
   * with the change by doing whatever is appropriate.
   *
   * @param aChild the content object
   * @param aAttribute the attribute whose value changed
   * @param aHint the level of change that has already been dealt with
   */
  NS_IMETHOD  AttributeChanged(nsIPresContext* aPresContext,
                               nsIContent*     aChild,
                               PRInt32         aNameSpaceID,
                               nsIAtom*        aAttribute,
                               PRInt32         aHint) = 0;

  /**
   * This call is invoked when the value of a content object's state
   * is changed. 
   * The first frame that maps that content is asked to deal
   * with the change by doing whatever is appropriate.
   *
   * @param aChild the content object
   * @param aHint the level of change that has already been dealt with
   */
  NS_IMETHOD  ContentStateChanged(nsIPresContext* aPresContext,
                                  nsIContent*     aChild,
                                  PRInt32         aHint) = 0;

  /**
   * Return how your frame can be split.
   */
  NS_IMETHOD  IsSplittable(nsSplittableType& aIsSplittable) const = 0;

  /**
   * Flow member functions
   */
  NS_IMETHOD  GetPrevInFlow(nsIFrame** aPrevInFlow) const = 0;
  NS_IMETHOD  SetPrevInFlow(nsIFrame*) = 0;
  NS_IMETHOD  GetNextInFlow(nsIFrame** aNextInFlow) const = 0;
  NS_IMETHOD  SetNextInFlow(nsIFrame*) = 0;

  /**
   * Accessor functions to get/set the associated view object
   */
  NS_IMETHOD  GetView(nsIPresContext* aPresContext,
                      nsIView**       aView) const = 0;  // may be null
  NS_IMETHOD  SetView(nsIPresContext* aPresContext,
                      nsIView*        aView) = 0;

  /**
   * Find the first geometric parent that has a view
   */
  NS_IMETHOD  GetParentWithView(nsIPresContext* aPresContext,
                                nsIFrame**      aParent) const = 0;

  /**
   * Returns the offset from this frame to the closest geometric parent that
   * has a view. Also returns the containing view or null in case of error
   */
  NS_IMETHOD  GetOffsetFromView(nsIPresContext* aPresContext,
                                nsPoint&        aOffset,
                                nsIView**       aView) const = 0;

  /**
   * Returns the window that contains this frame. If this frame has a
   * view and the view has a window, then this frames window is
   * returned, otherwise this frame's geometric parent is checked
   * recursively upwards.
   */
  NS_IMETHOD  GetWindow(nsIPresContext* aPresContext,
                        nsIWidget**     aWidget) const = 0;

  /**
   * Get the "type" of the frame. May return a NULL atom pointer
   *
   * @see nsLayoutAtoms
   */
  NS_IMETHOD  GetFrameType(nsIAtom** aType) const = 0;
  
  /**
   * Is this frame a "containing block"?
   */
  NS_IMETHOD  IsPercentageBase(PRBool& aBase) const = 0;

  /**
   * called when the frame has been scrolled to a new
   * position. only called for frames with views.
   */
  NS_IMETHOD  Scrolled(nsIView *aView) = 0;

  // Debugging
  NS_IMETHOD  List(nsIPresContext* aPresContext, FILE* out, PRInt32 aIndent) const = 0;

  /**
   * Get a printable from of the name of the frame type.
   */
  NS_IMETHOD  GetFrameName(nsString& aResult) const = 0;

  /**
   * Called to dump out regression data that describes the layout
   * of the frame and it's children, and so on. The format of the
   * data is dictated to be XML (using a specific DTD); the
   * specific kind of data dumped is up to the frame itself, with
   * the caveat that some base types are defined.
   * For more information, see XXX.
   */
  NS_IMETHOD  DumpRegressionData(nsIPresContext* aPresContext, FILE* out, PRInt32 aIndent) = 0;

  /**
   * Get the size of the frame object. The size value should include
   * all subordinate data referenced by the frame that is not
   * accounted for by child frames. However, this value should not
   * include the content objects, style contexts, views or other data
   * that lies logically outside the frame system.
   *
   * If the implementation so chooses, instead of returning the total
   * subordinate data it may instead use the sizeof handler to store
   * away subordinate data under its own key so that the subordinate
   * data may be tabulated independently of the frame itself.
   *
   * The caller is responsible for recursing over all child-lists that
   * the frame supports.
   */
  NS_IMETHOD  SizeOf(nsISizeOfHandler* aSizer, PRUint32* aResult) const = 0;

  NS_IMETHOD  VerifyTree() const = 0;

  /** Selection related calls
   */
  /** 
   *  Called to set the selection of the frame based on frame offsets.  you can FORCE the frame
   *  to redraw event if aSelected == the frame selection with the last parameter.
   *  data in struct may be changed when passed in.
   *  @param aRange is the range that will dictate if the frames need to be redrawn null means the whole content needs to be redrawn
   *  @param aSelected is it selected
   *  @param aSpread should is spread selection to flow elements around it? or go down to its children?
   */
  NS_IMETHOD  SetSelected(nsIPresContext* aPresContext,
                          nsIDOMRange*    aRange,
                          PRBool          aSelected,
                          nsSpread        aSpread) = 0;

  NS_IMETHOD  GetSelected(PRBool *aSelected) const = 0;

  /** EndSelection related calls
   */

  /**
   *  called to find the previous/next character, word, or line  returns the actual 
   *  nsIFrame and the frame offset.  THIS DOES NOT CHANGE SELECTION STATE
   *  uses frame's begin selection state to start. if no selection on this frame will 
   *  return NS_ERROR_FAILURE
   *  @param aPOS is defined in nsIFrameSelection
   */
  NS_IMETHOD  PeekOffset(nsIPresContext* aPresContext, nsPeekOffsetStruct *aPos) = 0;

  /**
   * See if tree verification is enabled. To enable tree verification add
   * "frameverifytree:1" to your NSPR_LOG_MODULES environment variable
   * (any non-zero debug level will work). Or, call SetVerifyTreeEnable
   * with PR_TRUE.
   */
  static NS_LAYOUT PRBool GetVerifyTreeEnable();

  /**
   * Set the verify-tree enable flag.
   */
  static NS_LAYOUT void SetVerifyTreeEnable(PRBool aEnabled);

  /**
   * See if style tree verification is enabled. To enable style tree 
   * verification add "styleverifytree:1" to your NSPR_LOG_MODULES 
   * environment variable (any non-zero debug level will work). Or, 
   * call SetVerifyStyleTreeEnable with PR_TRUE.
   */
  static NS_LAYOUT PRBool GetVerifyStyleTreeEnable();

  /**
   * Set the verify-style-tree enable flag.
   */
  static NS_LAYOUT void SetVerifyStyleTreeEnable(PRBool aEnabled);

  /**
   * The frame class and related classes share an nspr log module
   * for logging frame activity.
   *
   * Note: the log module is created during library initialization which
   * means that you cannot perform logging before then.
   */
  static NS_LAYOUT PRLogModuleInfo* GetLogModuleInfo();

  // Show frame borders when rendering
  static NS_LAYOUT void ShowFrameBorders(PRBool aEnable);
  static NS_LAYOUT PRBool GetShowFrameBorders();

private:
  NS_IMETHOD_(nsrefcnt) AddRef(void) = 0;
  NS_IMETHOD_(nsrefcnt) Release(void) = 0;
};

#endif /* nsIFrame_h___ */
