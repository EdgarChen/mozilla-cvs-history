/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
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
 * The Initial Developer of the Original Code is Sun
 * Microsystems, Inc.  Portions created by Sun are
 * Copyright (C) 2001 Sun Microsystems, Inc. All
 * Rights Reserved.
 *
 * Created by: Paul Sandoz   <paul.sandoz@sun.com> 
 *
 * Contributor(s): 
 */

#ifndef nsAbDirSearchListener_h__
#define nsAbDirSearchListener_h__

#include "nsIAbDirectoryQuery.h"
#include "nsIAbCard.h"

class nsAbDirSearchListenerContext
{
public:
    nsAbDirSearchListenerContext () {}
    virtual ~nsAbDirSearchListenerContext () {}

    virtual nsresult OnSearchFinished (PRInt32 result) = 0;
    virtual nsresult OnSearchFoundCard (nsIAbCard* card) = 0;
};


class nsAbDirSearchListener : public nsIAbDirectoryQueryResultListener
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIABDIRECTORYQUERYRESULTLISTENER

    nsAbDirSearchListener(nsAbDirSearchListenerContext* directory);
    virtual ~nsAbDirSearchListener();

protected:
    nsAbDirSearchListenerContext* mSearchContext;
};

#endif
