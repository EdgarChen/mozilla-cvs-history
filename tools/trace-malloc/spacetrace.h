/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is spacetrace.h/spacetrace.c code, released
 * Nov 6, 2001.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 2001 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s):
 *    Garrett Arch Blythe, 31-October-2001
 *    Suresh Duddi <dp@netscape.com>
 *
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU Public License (the "GPL"), in which case the
 * provisions of the GPL are applicable instead of those above.
 * If you wish to allow use of your version of this file only
 * under the terms of the GPL and not to allow others to use your
 * version of this file under the MPL, indicate your decision by
 * deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL.  If you do not delete
 * the provisions above, a recipient may use your version of this
 * file under either the MPL or the GPL.
 */

#ifndef spacetrace_h__
#define spacetrace_h__

/*
** spacetrace.h
**
** SpaceTrace is meant to take the output of trace-malloc and present
**   a picture of allocations over the run of the application.
*/

/*
** Required includes.
*/
#include "nspr.h"
#include "nsTraceMalloc.h"
#include "tmreader.h"

/*
** REPORT_ERROR
** REPORT_INFO
**
** Just report errors and stuff in a consistent manner.
*/
#define REPORT_ERROR(code, function) \
        PR_fprintf(PR_STDERR, "error(%d):\t%s\n", code, #function)
#define REPORT_INFO(msg) \
        PR_fprintf(PR_STDOUT, "%s: %s\n", globals.mOptions.mProgramName, (msg))

/*
** CALLSITE_RUN
**
** How to get a callsite run.
** Allows for further indirection if needed later.
*/
#define CALLSITE_RUN(callsite) \
        ((STRun*)((callsite)->data))

/*
** ST_PERMS
** ST_FLAGS
**
** File permissions we desire.
** 0644
*/
#define ST_PERMS (PR_IRUSR | PR_IWUSR | PR_IRGRP | PR_IROTH)
#define ST_FLAGS (PR_WRONLY | PR_CREATE_FILE | PR_TRUNCATE) 

/*
** Sorting order
*/
#define ST_WEIGHT   0 /* size * timeval */
#define ST_SIZE     1
#define ST_TIMEVAL  2
#define ST_COUNT    3
#define ST_HEAPCOST 4

/*
** Callsite loop direction flags.
*/
#define ST_FOLLOW_SIBLINGS   0
#define ST_FOLLOW_PARENTS    1

/*
** Graph data.
*/
#define STGD_WIDTH           640
#define STGD_HEIGHT          480
#define STGD_MARGIN          75
#define STGD_SPACE_X         (STGD_WIDTH - (2 * STGD_MARGIN))
#define STGD_SPACE_Y         (STGD_HEIGHT - (2 * STGD_MARGIN))

/*
** Minimum lifetime default, in seconds.
*/
#define ST_DEFAULT_LIFETIME_MIN 10

/*
** Allocations fall to this boundry size by default.
*/
#define ST_DEFAULT_ALIGNMENT_SIZE 16
#define ST_DEFAULT_OVERHEAD_SIZE 8

/*
** Numer of substring match specifications to allow.
*/
#define ST_SUBSTRING_MATCH_MAX 5

/*
** Max Number of patterns per rule
*/
#define ST_MAX_PATTERNS_PER_RULE 16

/*
** Rule pointers and child pointers are allocated in steps of ST_ALLOC_STEP
*/
#define ST_ALLOC_STEP 16

/*
** Name of the root category. Appears in UI.
*/
#define ST_ROOT_CATEGORY_NAME "All"

/*
**  Size of our option string buffers.
*/
#define ST_OPTION_STRING_MAX 256

/*
** Set the desired resolution of the timevals.
** The resolution is just mimicking what is recorded in the trace-malloc
**  output, and that is currently milliseconds.
*/
#define ST_TIMEVAL_RESOLUTION 1000
#define ST_TIMEVAL_FORMAT "%.3f"
#define ST_TIMEVAL_PRINTABLE(timeval) ((PRFloat64)(timeval) / (PRFloat64)ST_TIMEVAL_RESOLUTION)
#define ST_TIMEVAL_PRINTABLE64(timeval) ((PRFloat64)((PRInt64)(timeval)) / (PRFloat64)ST_TIMEVAL_RESOLUTION)
#define ST_TIMEVAL_MAX ((PRUint32)-1 - ((PRUint32)-1 % ST_TIMEVAL_RESOLUTION))

#define ST_MICROVAL_RESOLUTION 1000000
#define ST_MICROVAL_FORMAT "%.6f"
#define ST_MICROVAL_PRINTABLE(timeval) ((PRFloat64)(timeval) / (PRFloat64)ST_MICROVAL_RESOLUTION)
#define ST_MICROVAL_PRINTABLE64(timeval) ((PRFloat64)((PRInt64)(timeval)) / (PRFloat64)ST_MICROVAL_RESOLUTION)
#define ST_MICROVAL_MAX ((PRUint32)-1 - ((PRUint32)-1 % ST_MICROVAL_RESOLUTION))

/*
** Forward Declaration
*/
typedef struct __struct_STCategoryNode STCategoryNode;
typedef struct __struct_STCategoryRule STCategoryRule;


/*
** STAllocEvent
**
** An event that happens to an allocation (malloc, free, et. al.)
*/
typedef struct __struct_STAllocEvent
{
        /*
        ** The type of allocation event.
        ** This maps directly to the trace malloc events (i.e. TM_EVENT_MALLOC)
        */
        char mEventType;
        
        /*
        ** Each event, foremost, has a chronologically increasing ID in
        **  relation to other allocation events.  This is a time stamp
        **  of sorts.
        */
        PRUint32 mTimeval;
        
        /*
        ** Every event has a heap ID (pointer).
        ** In the event of a realloc, this is the new heap ID.
        ** In the event of a free, this is the previous heap ID value.
        */
        PRUint32 mHeapID;
        
        /*
        ** Every event, along with the heap ID, tells of the size.
        ** In the event of a realloc, this is the new size.
        ** In th event of a free, this is the previous size.
        */
        PRUint32 mHeapSize;

        /*
        ** Every event has a callsite/stack backtrace.
        ** In the event of a realloc, this is the new callsite.
        ** In the event of a free, this is the previous call site.
        */
        tmcallsite* mCallsite;
} STAllocEvent;

/*
** STAllocation
**
** An allocation is a temporal entity in the heap.
** It possibly lives under different heap IDs (pointers) and different
**  sizes during its given time.
** An allocation is defined by the events during its lifetime.
** An allocation's lifetime is defined by the range of event IDs it holds.
*/
typedef struct __struct_STAllocation
{
        /*
        ** The array of events.
        */
        PRUint32 mEventCount;
        STAllocEvent* mEvents;
        
        /*
        ** The lifetime/lifespan of the allocation.
        */
        PRUint32 mMinTimeval;
        PRUint32 mMaxTimeval;

        /*
        ** Index of this allocation in the global run.
        */
        PRUint32 mRunIndex;

        /*
        ** The runtime cost of heap events in this allocation.
        ** The cost is defined as the number of time units recorded as being
        **  spent in heap code (time of malloc, free, et al.).
        ** We do not track individual event cost in order to save space.
        */
        PRUint32 mHeapRuntimeCost;
} STAllocation;

/*
** STCallsiteStats
**
** Stats regarding a run, kept mainly for callsite runs.
*/
typedef struct __struct_STCallsiteStats
{
        /*
        ** Sum timeval of the allocations.
        ** Callsite runs total all allocations below the callsite.
        */
        PRUint64 mTimeval64;

        /*
        ** Sum weight of the allocations.
        ** Callsite runs total all allocations below the callsite.
        */
        PRUint64 mWeight64;

        /*
        ** Sum size of the allocations.
        ** Callsite runs total all allocations below the callsite.
        */
        PRUint32 mSize;

        /*
        ** A stamp, indicated the relevance of the run.
        ** If the stamp does not match the origin value, the
        **  data contained here-in is considered invalid.
        */
        PRUint32 mStamp;

        /*
        ** A sum total of allocations (note, not sizes)  below the callsite.
        ** This is NOT the same as STRun::mAllocationCount which
        **  tracks the STRun::mAllocations array size.
        */
        PRUint32 mCompositeCount;

        /*
        ** A sum total runtime cost of heap operations below the calliste.
        ** The cost is defined as the number of time units recorded as being
        **  spent in heap code (time of malloc, free, et al.).
        */
        PRUint32 mHeapRuntimeCost;
} STCallsiteStats;

/*
** STRun
**
** A run is a closed set of allocations.
** Given a run, we can deduce information about the contained allocations.
** We can also determine if an allocation lives beyond a run (leak).
**
** A run might be used to represent allocations for an entire application.
** A run might also be used to represent allocations from a single callstack.
*/
typedef struct __struct_STRun
{
        /*
        ** The array of allocations.
        */
        PRUint32 mAllocationCount;
        STAllocation** mAllocations;

        /*
        ** Callsites like to keep some information.
        */
        STCallsiteStats mStats;

} STRun;

/*
** Categorize allocations
**
** The objective is to have a tree of categories with each leaf node of the tree
** matching a set of callsites that belong to the category. Each category can
** signify a functional area like say css and hence the user can browse this
** tree looking for how much of each of these are live at an instant.
*/

/*
** STCategoryNode
*/

struct __struct_STCategoryNode
{
        /*
        ** Category name
        */
        const char *categoryName;

        /*
        ** Pointer to parent node. NULL for Root.
        */
        STCategoryNode *parent;

        /*
        ** For non-leaf nodes, an array of children node pointers.
        ** NULL if leaf node.
        */
        STCategoryNode** children;
        PRUint32 nchildren;

        /*
        ** The Run. Valid for both leaf and parent nodes.
        */
        STRun *run;
};


struct __struct_STCategoryRule
{
        /*
        ** The pattern for the rule. Patterns are an array of strings.
        ** A callsite needs to pass substring match for all the strings.
        */
        char* pats[ST_MAX_PATTERNS_PER_RULE];
        PRUint32 patlen[ST_MAX_PATTERNS_PER_RULE];
        PRUint32 npats;

        /*
        ** Category name that this rule belongs to
        */
        const char* categoryName;

        /*
        ** The node this should be categorized into
        */
        STCategoryNode* node;

};


/*
** CategoryName to Node mapping table
*/
typedef struct __struct_STCategoryMapEntry {
    STCategoryNode* node;
    const char * categoryName;
} STCategoryMapEntry;


/*
** STOptions
**
** Structure containing the various options for this code.
*/
typedef struct __struct_STOptions
{
        /*
        ** The string which identifies this program.
        */
        char mProgramName[ST_OPTION_STRING_MAX];

        /*
        ** File from which we retrieve the input.
        ** The input should have been generated from a trace malloc run.
        */
        char mFileName[ST_OPTION_STRING_MAX];

        /*
        ** Which directory we will take over and write our output.
        */
        char mOutputDir[ST_OPTION_STRING_MAX];

        /*
        ** The various batch mode requests we've received.
        */
        const char** mBatchRequests;
        PRUint32 mBatchRequestCount;

        /*
        ** Httpd port, where we accept queries.
        */
        PRUint32 mHttpdPort;

        /*
        ** Wether or not we should show help.
        */
        int mShowHelp;

        /*
        ** Maximum number of items to put in a list.
        */
        PRUint32 mListItemMax;

        /*
        ** Sort order control.
        */
        PRUint32 mOrderBy;

        /*
        ** Memory alignment size.
        ** Overhead, taken after alignment.
        **
        ** The msvcrt malloc has an alignment of 16 with an overhead of 8.
        ** The win32 HeapAlloc has an alignment of 8 with an overhead of 8.
        */
        PRUint32 mAlignBy;
        PRUint32 mOverhead;

        /*
        ** Timeval control.
        */
        PRUint32 mTimevalMin;
        PRUint32 mTimevalMax;

        /*
        ** Allocation timeval control.
        */
        PRUint32 mAllocationTimevalMin;
        PRUint32 mAllocationTimevalMax;

        /*
        ** Size control.
        */
        PRUint32 mSizeMin;
        PRUint32 mSizeMax;

        /*
        ** Lifetime timeval control.
        */
        PRUint32 mLifetimeMin;
        PRUint32 mLifetimeMax;

        /*
        ** Weight control.
        */
        PRUint64 mWeightMin64;
        PRUint64 mWeightMax64;

        /*
        ** Graph timeval control.
        */
        PRUint32 mGraphTimevalMin;
        PRUint32 mGraphTimevalMax;

        /*
        ** Restrict callsite backtraces to those containing text.
        */
        char mRestrictText[ST_SUBSTRING_MATCH_MAX][ST_OPTION_STRING_MAX];

        /*
        ** File containing rules to categorize allocations
        */
        char mCategoryFile[ST_OPTION_STRING_MAX];

        /*
        ** Category to focus report on. '\0' if not focussing on a category.
        */
        char mCategoryName[ST_OPTION_STRING_MAX];

} STOptions;

/*
**  STOptionChange
**
**  Generalized way to determine what options changed.
**  Useful for flushing caches, et. al.
*/
typedef struct __struct_STOptionChange
{
    int mSet;
    int mOrder;
    int mGraph;
    int mCategory;
}
STOptionChange;

/*
** STRequest
**
** Things specific to a request.
*/
typedef struct __struct_STRequest
{
        /*
        ** Sink/where to output.
        */
        PRFileDesc* mFD;

        /*
        ** The filename requested.
        */
        const char* mGetFileName;

        /*
        ** The GET form data, if any.
        */
        const char* mGetData;

        /*
        **  Options specific to this request.
        */
        STOptions mOptions;
} STRequest;

/*
** STCache
**
** Things we cache when the options get set.
** We can avoid some heavy duty processing should the options remain
**  constant by caching them here.
*/
typedef struct __struct_STCache
{
        /*
        ** Pre sorted run.
        */
        STRun* mSortedRun;
   
        /*
        ** Category the mSortedRun belongs to. NULL if not to any category.
        */
        char mCategoryName[ST_OPTION_STRING_MAX];

        /*
        ** Footprint graph cache.
        */
        int mFootprintCached;
        PRUint32 mFootprintYData[STGD_SPACE_X];

        /*
        ** Timeval graph cache.
        */
        int mTimevalCached;
        PRUint32 mTimevalYData[STGD_SPACE_X];

        /*
        ** Lifespan graph cache.
        */
        int mLifespanCached;
        PRUint32 mLifespanYData[STGD_SPACE_X];

        /*
        ** Weight graph cache.
        */
        int mWeightCached;
        PRUint64 mWeightYData64[STGD_SPACE_X];
} STCache;

/*
** STGlobals
**
** Various globals we keep around.
*/
typedef struct __struct_STGlobals
{
        /*
        ** Options derived from the command line.
        */
        STOptions mOptions;

        /*
        ** Cached data, generally reset by the options.
        */
        STCache mCache;

        /*
        ** Various counters for different types of events.
        */
        PRUint32 mMallocCount;
        PRUint32 mCallocCount;
        PRUint32 mReallocCount;
        PRUint32 mFreeCount;

        /*
        ** Total events, operation counter.
        */
        PRUint32 mOperationCount;

        /*
        ** The "run" of the input.
        */
        STRun mRun;

        /*
        ** Operation minimum/maximum timevals.
        ** So that we can determine the overall timeval of the run.
        ** NOTE:  These are NOT the options to control the data set.
        */
        PRUint32 mMinTimeval;
        PRUint32 mMaxTimeval;

        /*
        ** Calculates peak allocation overall for all allocations.
        */
        PRUint32 mPeakMemoryUsed;
        PRUint32 mMemoryUsed;

        /*
        ** A list of rules for categorization read in from the mCategoryFile
        */
       STCategoryRule** mCategoryRules;
       PRUint32 mNRules;

       /*
       ** CategoryName to Node mapping table
       */
       STCategoryMapEntry** mCategoryMap;
       PRUint32 mNCategoryMap;

       /*
       ** Categorized allocations. For now we support only one tree.
       */
       STCategoryNode mCategoryRoot;

       /*
       **   tmreader hash tables.
       **   Moved into globals since we need to destroy these only after all
       **       client threads are finishes (after PR_Cleanup).
       */
       tmreader* mTMR;
} STGlobals;


/*
** Function prototypes
*/
extern STRun* createRun(PRUint32 aStamp);
extern void freeRun(STRun* aRun);
extern int initCategories(STGlobals* g);
extern int categorizeRun(const STRun* aRun, STGlobals* g);
extern STCategoryNode* findCategoryNode(const char *catName, STGlobals *g);
extern int freeCategories(STGlobals* g);
extern int displayCategoryReport(STRequest* inRequest, STCategoryNode *root, int depth);

extern int recalculateAllocationCost(STRun* aRun, STAllocation* aAllocation, PRBool updateParent);
extern void htmlHeader(STRequest* inRequest, const char* aTitle);
extern void htmlFooter(STRequest* inRequest);
extern void htmlAnchor(STRequest* inRequest, const char* aHref, const char* aText, const char* aTarget);
extern char *FormatNumber(PRInt32 num);

/*
** shared globals
*/
extern STGlobals globals;

#endif /* spacetrace_h__ */
