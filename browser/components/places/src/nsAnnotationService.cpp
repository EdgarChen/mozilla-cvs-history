//* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Mozilla Annotation Service
 *
 * The Initial Developer of the Original Code is
 * Google Inc.
 * Portions created by the Initial Developer are Copyright (C) 2005
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Brett Wilson <brettw@gmail.com> (original author)
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "nsAnnotationService.h"
#include "mozStorageCID.h"
#include "nsNavHistory.h"
#include "nsNetUtil.h"
#include "mozIStorageValueArray.h"
#include "mozIStorageStatement.h"
#include "mozIStorageFunction.h"
#include "mozStorageHelper.h"
#include "nsIServiceManager.h"
#include "nsIVariant.h"
#include "nsString.h"
#include "nsVariant.h"

const PRInt32 nsAnnotationService::kAnnoIndex_ID = 0;
const PRInt32 nsAnnotationService::kAnnoIndex_Page = 1;
const PRInt32 nsAnnotationService::kAnnoIndex_Name = 2;
const PRInt32 nsAnnotationService::kAnnoIndex_MimeType = 3;
const PRInt32 nsAnnotationService::kAnnoIndex_Content = 4;
const PRInt32 nsAnnotationService::kAnnoIndex_Flags = 5;
const PRInt32 nsAnnotationService::kAnnoIndex_Expiration = 6;

static NS_DEFINE_CID(kmozStorageServiceCID, MOZ_STORAGE_SERVICE_CID);
static NS_DEFINE_CID(kmozStorageConnectionCID, MOZ_STORAGE_CONNECTION_CID);

NS_IMPL_ISUPPORTS1(nsAnnotationService,
                   nsIAnnotationService)

// nsAnnotationService::nsAnnotationService

nsAnnotationService::nsAnnotationService()
{

}


// nsAnnotationService::~nsAnnotationService

nsAnnotationService::~nsAnnotationService()
{

}


// nsAnnotationService::Init

nsresult
nsAnnotationService::Init()
{
  nsresult rv;

  // The history service will normally already be created and will call our
  // static InitTables function. It will get autocreated here if it hasn't
  // already been created.
  nsNavHistory* history = nsNavHistory::GetHistoryService();
  if (! history)
    return NS_ERROR_FAILURE;
  mDBConn = history->GetStorageConnection();

  // annotation statements
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING("UPDATE moz_anno SET mime_type = ?4, content = ?5, flags = ?6, expiration = ?7 WHERE anno_id = ?1"),
                                getter_AddRefs(mDBSetAnnotation));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING("SELECT * FROM moz_anno WHERE page = ?1 AND name = ?2"),
                                getter_AddRefs(mDBGetAnnotation));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING("SELECT name FROM moz_anno WHERE page = ?1"),
                                getter_AddRefs(mDBGetAnnotationNames));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING("SELECT a.anno_id, a.page, a.name, a.mime_type, a.content, a.flags, a.expiration FROM moz_history h JOIN moz_anno a ON h.id = a.page WHERE h.url = ?1 AND a.name = ?2"),
                                getter_AddRefs(mDBGetAnnotationFromURI));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING("INSERT INTO moz_anno (page, name, mime_type, content, flags, expiration) VALUES (?2, ?3, ?4, ?5, ?6, ?7)"),
                                getter_AddRefs(mDBAddAnnotation));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING("DELETE FROM moz_anno WHERE page = ?1 AND name = ?2"),
                                getter_AddRefs(mDBRemoveAnnotation));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


// nsAnnotationService::InitTables
//
//    All commands that initialize the schema of the DB go in here. This is
//    called from history init before the dummy DB connection is started that
//    will prevent us from modifying the schema.
//
//    The history service will always be created before us (we get it at the
//    beginning of the init function which covers us if it's not).

nsresult // static
nsAnnotationService::InitTables(mozIStorageConnection* aDBConn)
{
  nsresult rv;
  PRBool exists;
  rv = aDBConn->TableExists(NS_LITERAL_CSTRING("moz_anno"), &exists);
  NS_ENSURE_SUCCESS(rv, rv);
  if (! exists) {
    rv = aDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING("CREATE TABLE moz_anno ("
        "anno_id INTEGER PRIMARY KEY,"
        "page INTEGER NOT NULL,"
        "name VARCHAR(32) NOT NULL,"
        "mime_type VARCHAR(32) DEFAULT NULL,"
        "content LONGVARCHAR, flags INTEGER DEFAULT 0,"
        "expiration INTEGER DEFAULT 0)"));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = aDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
        "CREATE INDEX moz_anno_pageindex ON moz_anno (page)"));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = aDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
        "CREATE INDEX moz_anno_nameindex ON moz_anno (name)"));
    NS_ENSURE_SUCCESS(rv, rv);
  }
  return NS_OK;
}


// nsAnnotationService::SetAnnotationString

NS_IMETHODIMP
nsAnnotationService::SetAnnotationString(nsIURI* aURI,
                                         const nsACString& aName,
                                         const nsAString& aValue,
                                         PRInt32 aFlags, PRInt32 aExpiration)
{
  mozStorageTransaction transaction(mDBConn, PR_FALSE);
  mozIStorageStatement* statement; // class var, not owned by this function
  nsresult rv = StartSetAnnotation(aURI, aName, aFlags, aExpiration, &statement);
  NS_ENSURE_SUCCESS(rv, rv);
  mozStorageStatementScoper statementResetter(statement);

  rv = statement->BindStringParameter(kAnnoIndex_Content, aValue);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = statement->BindNullParameter(kAnnoIndex_MimeType);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = statement->Execute();
  NS_ENSURE_SUCCESS(rv, rv);
  transaction.Commit();

  // should reset the statement; observers may call our service back to get
  // annotation values!
  statement->Reset();
  statementResetter.Abandon();
  CallSetObservers(aURI, aName);
  return NS_OK;
}


// nsAnnotationService::SetAnnotationInt32

NS_IMETHODIMP
nsAnnotationService::SetAnnotationInt32(nsIURI* aURI,
                                        const nsACString& aName,
                                        PRInt32 aValue,
                                        PRInt32 aFlags, PRInt32 aExpiration)
{
  mozStorageTransaction transaction(mDBConn, PR_FALSE);
  mozIStorageStatement* statement; // class var, not owned by this function
  nsresult rv = StartSetAnnotation(aURI, aName, aFlags, aExpiration, &statement);
  NS_ENSURE_SUCCESS(rv, rv);
  mozStorageStatementScoper statementResetter(statement);

  rv = statement->BindInt32Parameter(kAnnoIndex_Content, aValue);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = statement->BindNullParameter(kAnnoIndex_MimeType);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = statement->Execute();
  NS_ENSURE_SUCCESS(rv, rv);
  transaction.Commit();

  // should reset the statement; observers may call our service back to get
  // annotation values!
  statement->Reset();
  statementResetter.Abandon();
  CallSetObservers(aURI, aName);
  return NS_OK;
}


// nsAnnotationService::SetAnnotationInt64

NS_IMETHODIMP
nsAnnotationService::SetAnnotationInt64(nsIURI* aURI,
                                        const nsACString& aName,
                                        PRInt64 aValue,
                                        PRInt32 aFlags, PRInt32 aExpiration)
{
  mozStorageTransaction transaction(mDBConn, PR_FALSE);
  mozIStorageStatement* statement; // class var, not owned by this function
  nsresult rv = StartSetAnnotation(aURI, aName, aFlags, aExpiration, &statement);
  NS_ENSURE_SUCCESS(rv, rv);
  mozStorageStatementScoper statementResetter(statement);

  rv = statement->BindInt64Parameter(kAnnoIndex_Content, aValue);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = statement->BindNullParameter(kAnnoIndex_MimeType);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = statement->Execute();
  NS_ENSURE_SUCCESS(rv, rv);
  transaction.Commit();

  // should reset the statement; observers may call our service back to get
  // annotation values!
  statement->Reset();
  statementResetter.Abandon();
  CallSetObservers(aURI, aName);
  return NS_OK;
}


// nsAnnotationService::SetAnnotationDouble

NS_IMETHODIMP
nsAnnotationService::SetAnnotationDouble(nsIURI* aURI,
                                         const nsACString& aName,
                                         double aValue,
                                         PRInt32 aFlags, PRInt32 aExpiration)
{
  mozStorageTransaction transaction(mDBConn, PR_FALSE);
  mozIStorageStatement* statement; // class var, not owned by this function
  nsresult rv = StartSetAnnotation(aURI, aName, aFlags, aExpiration, &statement);
  NS_ENSURE_SUCCESS(rv, rv);
  mozStorageStatementScoper statementResetter(statement);

  rv = statement->BindDoubleParameter(kAnnoIndex_Content, aValue);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = statement->BindNullParameter(kAnnoIndex_MimeType);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = statement->Execute();
  NS_ENSURE_SUCCESS(rv, rv);
  transaction.Commit();

  // should reset the statement; observers may call our service back to get
  // annotation values!
  statement->Reset();
  statementResetter.Abandon();
  CallSetObservers(aURI, aName);
  return NS_OK;
}


// nsAnnotationService::SetAnnotationBinary

NS_IMETHODIMP
nsAnnotationService::SetAnnotationBinary(nsIURI* aURI,
                                         const nsACString& aName,
                                         const PRUint8 *aData,
                                         PRUint32 aDataLen,
                                         const nsACString& aMimeType,
                                         PRInt32 aFlags, PRInt32 aExpiration)
{
  if (aMimeType.Length() == 0)
    return NS_ERROR_INVALID_ARG;

  mozStorageTransaction transaction(mDBConn, PR_FALSE);
  mozIStorageStatement* statement; // class var, not owned by this function
  nsresult rv = StartSetAnnotation(aURI, aName, aFlags, aExpiration, &statement);
  NS_ENSURE_SUCCESS(rv, rv);
  mozStorageStatementScoper statementResetter(statement);

  rv = statement->BindBlobParameter(kAnnoIndex_Content, aData, aDataLen);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = statement->BindUTF8StringParameter(kAnnoIndex_MimeType, aMimeType);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = statement->Execute();
  NS_ENSURE_SUCCESS(rv, rv);
  transaction.Commit();

  // should reset the statement; observers may call our service back to get
  // annotation values!
  statement->Reset();
  statementResetter.Abandon();
  CallSetObservers(aURI, aName);
  return NS_OK;
}


// nsAnnotationService::GetAnnotationString

NS_IMETHODIMP
nsAnnotationService::GetAnnotationString(nsIURI* aURI,
                                         const nsACString& aName,
                                         nsAString& _retval)
{
  nsresult rv = StartGetAnnotationFromURI(aURI, aName);
  if (NS_FAILED(rv))
    return rv;
  rv = mDBGetAnnotationFromURI->GetString(kAnnoIndex_Content, _retval);
  mDBGetAnnotationFromURI->Reset();
  return rv;
}


// nsAnnotationService::GetAnnotationInt32

NS_IMETHODIMP
nsAnnotationService::GetAnnotationInt32(nsIURI* aURI,
                                        const nsACString& aName,
                                        PRInt32 *_retval)
{
  nsresult rv = StartGetAnnotationFromURI(aURI, aName);
  if (NS_FAILED(rv))
    return rv;
  *_retval = mDBGetAnnotationFromURI->AsInt32(kAnnoIndex_Content);
  mDBGetAnnotationFromURI->Reset();
  return NS_OK;
}


// nsAnnotationService::GetAnnotationInt64

NS_IMETHODIMP
nsAnnotationService::GetAnnotationInt64(nsIURI* aURI,
                                        const nsACString& aName,
                                        PRInt64 *_retval)
{
  nsresult rv = StartGetAnnotationFromURI(aURI, aName);
  if (NS_FAILED(rv))
    return rv;
  *_retval = mDBGetAnnotationFromURI->AsInt64(kAnnoIndex_Content);
  mDBGetAnnotationFromURI->Reset();
  return NS_OK;
}


// nsAnnotationService::GetAnnotationDouble

NS_IMETHODIMP
nsAnnotationService::GetAnnotationDouble(nsIURI* aURI,
                                         const nsACString& aName,
                                         double *_retval)
{
  nsresult rv = StartGetAnnotationFromURI(aURI, aName);
  if (NS_FAILED(rv))
    return rv;
  *_retval = mDBGetAnnotationFromURI->AsDouble(kAnnoIndex_Content);
  mDBGetAnnotationFromURI->Reset();
  return NS_OK;
}


// nsAnnotationService::GetAnnotationBinary

NS_IMETHODIMP
nsAnnotationService::GetAnnotationBinary(nsIURI* aURI,
                                         const nsACString& aName,
                                         PRUint8** aData, PRUint32* aDataLen,
                                         nsACString& aMimeType)
{
  nsresult rv = StartGetAnnotationFromURI(aURI, aName);
  if (NS_FAILED(rv))
    return rv;
  rv = mDBGetAnnotationFromURI->GetBlob(kAnnoIndex_Content, aDataLen, aData);
  if (NS_FAILED(rv)) {
    mDBGetAnnotationFromURI->Reset();
    return rv;
  }
  rv = mDBGetAnnotationFromURI->GetUTF8String(kAnnoIndex_MimeType, aMimeType);
  mDBGetAnnotationFromURI->Reset();
  return rv;
}


// nsAnnotationService::GetAnnotationInfo

NS_IMETHODIMP
nsAnnotationService::GetAnnotationInfo(nsIURI* aURI,
                                       const nsACString& aName,
                                       PRInt32 *aFlags, PRInt32 *aExpiration,
                                       nsACString& aMimeType,
                                       PRInt32 *aStorageType)
{
  nsresult rv = StartGetAnnotationFromURI(aURI, aName);
  if (NS_FAILED(rv))
    return rv;
  mozStorageStatementScoper resetter(mDBGetAnnotationFromURI);

  *aFlags = mDBGetAnnotationFromURI->AsInt32(kAnnoIndex_Flags);
  *aExpiration = mDBGetAnnotationFromURI->AsInt32(kAnnoIndex_Expiration);
  rv = mDBGetAnnotationFromURI->GetUTF8String(kAnnoIndex_MimeType, aMimeType);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mDBGetAnnotationFromURI->GetTypeOfIndex(kAnnoIndex_Content, aStorageType);
  return rv;
}


// nsAnnotationService::GetPagesWithAnnotation

NS_IMETHODIMP
nsAnnotationService::GetPagesWithAnnotation(const nsACString& aName,
                                            PRUint32* aResultCount,
                                            nsIURI*** aResults)
{
  if (aName.IsEmpty() || ! aResultCount || ! aResults)
    return NS_ERROR_INVALID_ARG;
  *aResultCount = 0;
  *aResults = nsnull;

  // this probably isn't a common operation, so we don't have a precompiled
  // statement. Perhaps this should change.
  nsCOMPtr<mozIStorageStatement> statement;
  nsresult rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
    "SELECT h.url FROM moz_anno a JOIN moz_history h ON a.page = h.id WHERE a.name = ?1"),
    getter_AddRefs(statement));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = statement->BindUTF8StringParameter(0, aName);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool hasMore = PR_FALSE;
  nsCOMArray<nsIURI> results;
  while (NS_SUCCEEDED(rv = statement->ExecuteStep(&hasMore)) && hasMore) {
    nsCAutoString uristring;
    rv = statement->GetUTF8String(0, uristring);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIURI> uri;
    rv = NS_NewURI(getter_AddRefs(uri), uristring);
    NS_ENSURE_SUCCESS(rv, rv);
    PRBool added = results.AppendObject(uri);
    NS_ENSURE_TRUE(added, NS_ERROR_OUT_OF_MEMORY);
  }

  // convert to raw array
  if (results.Count() == 0)
    return NS_OK;
  *aResults = NS_STATIC_CAST(nsIURI**,
                             nsMemory::Alloc(results.Count() * sizeof(nsIURI*)));
  if (! aResults)
    return NS_ERROR_OUT_OF_MEMORY;
  *aResultCount = results.Count();
  for (PRUint32 i = 0; i < *aResultCount; i ++) {
    (*aResults)[i] = results[i];
    NS_ADDREF((*aResults)[i]);
  }
  return NS_OK;
}


// nsAnnotationService::GetPageAnnotationNames

NS_IMETHODIMP
nsAnnotationService::GetPageAnnotationNames(nsIURI* aURI, PRUint32* aCount,
                                            nsIVariant*** _result)
{
  *aCount = 0;
  *_result = nsnull;

  nsTArray<nsCString> names;
  nsresult rv = GetPageAnnotationNamesTArray(aURI, &names);
  NS_ENSURE_SUCCESS(rv, rv);
  if (names.Length() == 0)
    return NS_OK;

  *_result = NS_STATIC_CAST(nsIVariant**,
      nsMemory::Alloc(sizeof(nsIVariant*) * names.Length()));
  NS_ENSURE_TRUE(*_result, NS_ERROR_OUT_OF_MEMORY);

  for (PRUint32 i = 0; i < names.Length(); i ++) {
    nsCOMPtr<nsIWritableVariant> var = new nsVariant;
    if (! var) {
      // need to release all the variants we've already created
      for (PRUint32 j = 0; j < i; j ++)
        NS_RELEASE((*_result)[j]);
      nsMemory::Free(*_result);
      *_result = nsnull;
      return rv;
    }
    var->SetAsAUTF8String(names[i]);
    NS_ADDREF((*_result)[i] = var);
  }
  *aCount = names.Length();

  return NS_OK;
}


// nsAnnotationService::GetPageAnnotationNamesTArray

NS_IMETHODIMP
nsAnnotationService::GetPageAnnotationNamesTArray(nsIURI* aURI,
                                                  nsTArray<nsCString>* aResult)
{
  aResult->Clear();

  nsNavHistory* history = nsNavHistory::GetHistoryService();
  NS_ENSURE_TRUE(history, NS_ERROR_FAILURE);

  PRInt64 uriID;
  nsresult rv = history->GetUrlIdFor(aURI, &uriID, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);
  if (uriID == 0) // Check if URI exists.
    return NS_OK;

  mozStorageStatementScoper scoper(mDBGetAnnotationNames);
  rv = mDBGetAnnotationNames->BindInt64Parameter(0, uriID);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool hasResult;
  nsCAutoString name;
  while (NS_SUCCEEDED(mDBGetAnnotationNames->ExecuteStep(&hasResult)) &&
         hasResult) {
    rv = mDBGetAnnotationNames->GetUTF8String(0, name);
    NS_ENSURE_SUCCESS(rv, rv);
    if (! aResult->AppendElement(name))
      return NS_ERROR_OUT_OF_MEMORY;
  }

  return NS_OK;
}


// nsAnnotationService::HasAnnotation

NS_IMETHODIMP
nsAnnotationService::HasAnnotation(nsIURI* aURI,
                                   const nsACString& aName,
                                   PRBool *_retval)
{
  nsresult rv = StartGetAnnotationFromURI(aURI, aName);
  if (rv == NS_ERROR_NOT_AVAILABLE) {
    *_retval = PR_FALSE;
    rv = NS_OK;
  } else if (NS_SUCCEEDED(rv)) {
    *_retval = PR_TRUE;
  }
  mDBGetAnnotationFromURI->Reset();
  return rv;
}


// nsAnnotationService::RemoveAnnotation

NS_IMETHODIMP
nsAnnotationService::RemoveAnnotation(nsIURI* aURI,
                                      const nsACString& aName)
{
  nsresult rv;
  nsNavHistory* history = nsNavHistory::GetHistoryService();
  NS_ENSURE_TRUE(history, NS_ERROR_FAILURE);

  PRInt64 uriID;
  rv = history->GetUrlIdFor(aURI, &uriID, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);
  if (uriID == 0) // Check if URI exists.
    return NS_OK;

  mozStorageStatementScoper resetter(mDBRemoveAnnotation);

  rv = mDBRemoveAnnotation->BindInt64Parameter(0, uriID);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDBRemoveAnnotation->BindUTF8StringParameter(1, aName);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDBRemoveAnnotation->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  resetter.Abandon();

  // Update observers
  for (PRInt32 i = 0; i < mObservers.Count(); i ++)
    mObservers[i]->OnAnnotationRemoved(aURI, aName);

  return NS_OK;
}


// nsAnnotationSerivce::RemovePageAnnotations
//
//    I don't believe this is a common operation, so am not using a precompiled
//    statement. If this ends up being used a lot, the statement should be
//    precompiled and added to the class.

NS_IMETHODIMP
nsAnnotationService::RemovePageAnnotations(nsIURI* aURI)
{
  nsresult rv;
  nsNavHistory* history = nsNavHistory::GetHistoryService();
  NS_ENSURE_TRUE(history, NS_ERROR_UNEXPECTED);

  PRInt64 uriID;
  rv = history->GetUrlIdFor(aURI, &uriID, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);
  if (uriID == 0)
    return NS_OK; // URI doesn't exist, nothing to delete

  nsCOMPtr<mozIStorageStatement> statement;
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "DELETE FROM moz_anno WHERE page = ?1"),
    getter_AddRefs(statement));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = statement->BindInt64Parameter(0, uriID);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = statement->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  // Update observers
  for (PRInt32 i = 0; i < mObservers.Count(); i ++)
    mObservers[i]->OnAnnotationRemoved(aURI, EmptyCString());
  return NS_OK;
}


// nsAnnotationService::AddObserver

NS_IMETHODIMP
nsAnnotationService::AddObserver(nsIAnnotationObserver* aObserver)
{
  if (mObservers.IndexOfObject(aObserver) >= 0)
    return NS_ERROR_INVALID_ARG; // already registered
  if (!mObservers.AppendObject(aObserver))
    return NS_ERROR_OUT_OF_MEMORY;
  return NS_OK;
}


// nsAnnotationService::RemoveObserver

NS_IMETHODIMP
nsAnnotationService::RemoveObserver(nsIAnnotationObserver* aObserver)
{
  if (!mObservers.RemoveObject(aObserver))
    return NS_ERROR_INVALID_ARG;
  return NS_OK;
}


// nsAnnotationService::GetAnnotationURI

NS_IMETHODIMP
nsAnnotationService::GetAnnotationURI(nsIURI* aURI, const nsACString& aName,
                                      nsIURI** _result)
{
  if (aName.IsEmpty())
    return NS_ERROR_INVALID_ARG;

  nsCAutoString annoSpec;
  nsresult rv = aURI->GetSpec(annoSpec);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCAutoString spec;
  spec.AssignLiteral("moz-anno:");
  spec += aName;
  spec += NS_LITERAL_CSTRING(":");
  spec += annoSpec;

  return NS_NewURI(_result, spec);
}


// nsAnnotationService::HasAnnotationInternal
//
//    This is just like HasAnnotation but takes a URL ID. It will also give
//    you the ID of the annotation, if it exists. This value can be NULL and
//    it won't retrieve the annotation ID. If it doesn't exist, annotationID
//    is not touched.

nsresult
nsAnnotationService::HasAnnotationInternal(PRInt64 aURLID,
                                           const nsACString& aName,
                                           PRBool* hasAnnotation,
                                           PRInt64* annotationID)
{
  mozStorageStatementScoper resetter(mDBGetAnnotation);
  nsresult rv;

  rv = mDBGetAnnotation->BindInt64Parameter(0, aURLID);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDBGetAnnotation->BindUTF8StringParameter(1, aName);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDBGetAnnotation->ExecuteStep(hasAnnotation);
  NS_ENSURE_SUCCESS(rv, rv);
  if (! annotationID || ! *hasAnnotation)
    return NS_OK;

  return mDBGetAnnotation->GetInt64(0, annotationID);
}


// nsAnnotationService::StartGetAnnotationFromURI
//
//    This loads the statement GetAnnotationFromURI and steps it once so you
//    can get data out of it. YOU NEED TO RESET THIS STATEMENT WHEN YOU ARE
//    DONE! Returns error if the annotation is not found, in which case you
//    don't need to reset anything.

nsresult
nsAnnotationService::StartGetAnnotationFromURI(nsIURI* aURI,
                                               const nsACString& aName)
{
  mozStorageStatementScoper statementResetter(mDBGetAnnotationFromURI);
  nsresult rv;

  rv = BindStatementURI(mDBGetAnnotationFromURI, 0, aURI);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mDBGetAnnotationFromURI->BindUTF8StringParameter(1, aName);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool hasResult = PR_FALSE;
  rv = mDBGetAnnotationFromURI->ExecuteStep(&hasResult);
  if (NS_FAILED(rv) || ! hasResult)
    return NS_ERROR_NOT_AVAILABLE;

  // on success, DON'T reset the statement, the caller needs to read from it,
  // and it is the caller's job to do the reseting.
  statementResetter.Abandon();
  return NS_OK;
}


// nsAnnotationService::StartSetAnnotation
//
//    This does most of the work of setting an annotation, except for setting
//    the actual value and MIME type and executing the statement. It will
//    create a URL entry if necessary. It will either update an existing
//    annotation or insert a new one, and aStatement will be set to either
//    mDBAddAnnotation or mDBSetAnnotation. The aStatement RESULT IS NOT
//    ADDREFED. This is just one of the class vars, which control its scope.
//    DO NOT RELEASE.
//
//    The caller must make sure the statement is reset. On error, the
//    statement will not need reseting.

nsresult
nsAnnotationService::StartSetAnnotation(nsIURI* aURI,
                                        const nsACString& aName,
                                        PRInt32 aFlags, PRInt32 aExpiration,
                                        mozIStorageStatement** aStatement)
{
  nsresult rv;
  nsNavHistory* history = nsNavHistory::GetHistoryService();
  NS_ENSURE_TRUE(history, NS_ERROR_FAILURE);

  PRInt64 uriID;
  rv = history->GetUrlIdFor(aURI, &uriID, PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool hasAnnotation;
  PRInt64 annotationID;
  rv = HasAnnotationInternal(uriID, aName, &hasAnnotation, &annotationID);
  NS_ENSURE_SUCCESS(rv, rv);

  // either update the existing annotation (using the old annotation's ID)
  // or insert a new one tied to the URI.
  if (hasAnnotation) {
    *aStatement = mDBSetAnnotation;
    rv = (*aStatement)->BindInt64Parameter(kAnnoIndex_ID, annotationID);
  } else {
    *aStatement = mDBAddAnnotation;
    rv = (*aStatement)->BindInt64Parameter(kAnnoIndex_Page, uriID);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = (*aStatement)->BindUTF8StringParameter(kAnnoIndex_Name, aName);
  }
  mozStorageStatementScoper statementResetter(*aStatement);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = (*aStatement)->BindInt32Parameter(kAnnoIndex_Flags, aFlags);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = (*aStatement)->BindInt32Parameter(kAnnoIndex_Expiration, aExpiration);
  NS_ENSURE_SUCCESS(rv, rv);

  // on success, leave the statement open, the caller will set the value
  // and MIME type and execute the statement
  statementResetter.Abandon();
  return NS_OK;
}


// nsAnnotationService::CallSetObservers

void
nsAnnotationService::CallSetObservers(nsIURI* aURI, const nsACString& aName)
{
  for (PRInt32 i = 0; i < mObservers.Count(); i ++)
    mObservers[i]->OnAnnotationSet(aURI, aName);
}
