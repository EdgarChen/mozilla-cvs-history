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
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation. Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 */


/*

  The converts a filesystem directory into an "HTTP index" stream.

 */

#include "nsEscape.h"
#include "nsDirectoryIndexStream.h"
#include "prio.h"


nsDirectoryIndexStream::nsDirectoryIndexStream()
    : mOffset(0)
{
    NS_INIT_REFCNT();
}

nsresult
nsDirectoryIndexStream::Init(nsIFile* aDir)
{
    nsresult rv;
    PRBool isDir;
    rv = aDir->IsDirectory(&isDir);
    if (NS_FAILED(rv)) return rv;
    NS_PRECONDITION(isDir, "not a directory");
    if (!isDir)
        return NS_ERROR_ILLEGAL_VALUE;

    mDir = aDir;

    // Sigh. We have to allocate on the heap because there are no
    // assignment operators defined.
    rv = mDir->GetDirectoryEntries(getter_AddRefs(mIter));
    if (NS_FAILED(rv)) return rv;

    mBuf = "200: filename content-length last-modified file-type\n";
    return NS_OK;
}

nsDirectoryIndexStream::~nsDirectoryIndexStream()
{
}

nsresult
nsDirectoryIndexStream::Create(nsIFile* aDir, nsIInputStream** aResult)
{
    nsDirectoryIndexStream* result = new nsDirectoryIndexStream();
    if (! result)
        return NS_ERROR_OUT_OF_MEMORY;

    nsresult rv;
    rv = result->Init(aDir);
    if (NS_FAILED(rv)) {
        delete result;
        return rv;
    }

    *aResult = result;
    NS_ADDREF(*aResult);
    return NS_OK;
}

NS_IMPL_ISUPPORTS2(nsDirectoryIndexStream,
                   nsIInputStream,
                   nsIBaseStream)

NS_IMETHODIMP
nsDirectoryIndexStream::Close()
{
    return NS_OK;
}

NS_IMETHODIMP
nsDirectoryIndexStream::Available(PRUint32* aLength)
{
    // Lie, and tell the caller that the stream is endless (until we
    // actually don't have anything left).
    PRBool more;
    nsresult rv = mIter->HasMoreElements(&more);
    if (NS_FAILED(rv)) return rv; 
    if (more) {
        *aLength = PRUint32(-1);
        return NS_OK;
    }
    else {
        *aLength = 0;
        return NS_OK;
    }
}

NS_IMETHODIMP
nsDirectoryIndexStream::Read(char* aBuf, PRUint32 aCount, PRUint32* aReadCount)
{
    PRUint32 nread = 0;

    // If anything is enqueued (or left-over) in mBuf, then feed it to
    // the reader first.
    while (mOffset < mBuf.Length() && aCount != 0) {
        *(aBuf++) = char(mBuf.CharAt(mOffset++));
        --aCount;
        ++nread;
    }

    // Room left?
    if (aCount > 0) {
        mOffset = 0;
        mBuf.Truncate();

        // Okay, now we'll suck stuff off of our iterator into the mBuf...
        while (PRUint32(mBuf.Length()) < aCount) {
            PRBool more;
            nsresult rv = mIter->HasMoreElements(&more);
            if (NS_FAILED(rv)) return rv; 
            if (!more) break;
            
            nsCOMPtr<nsISupports> cur;
            rv = mIter->GetNext(getter_AddRefs(cur));
            nsCOMPtr<nsIFile> current = do_QueryInterface(cur, &rv);
            if (NS_FAILED(rv)) return rv; 

            // rjc: don't return hidden files/directories!
            PRBool hidden;
            rv = current->IsHidden(&hidden);
            if (NS_FAILED(rv)) return rv; 
            if (hidden) continue;

            char* path;
            rv = current->GetPath(&path);
            if (NS_FAILED(rv)) return rv; 
            PRFileInfo fileinfo;
            PRStatus status = PR_GetFileInfo(path, &fileinfo);
            nsCRT::free(path);
            if (status != PR_SUCCESS)
                continue;

            mBuf += "201: ";

            // The "filename" field
            {
                char* leafname;
                rv = current->GetLeafName(&leafname);
                if (NS_FAILED(rv)) return rv; 
                if (leafname) {
                    char* escaped = nsEscape(leafname, url_Path);
                    if (escaped) {
                        mBuf += escaped;
                        mBuf.Append(' ');
                        nsCRT::free(escaped);
                    }
                    nsCRT::free(leafname);
                }
            }

            // The "content-length" field
            mBuf.Append(fileinfo.size, 10);
            mBuf.Append(' ');

            // The "last-modified" field
            PRExplodedTime tm;
            PR_ExplodeTime(fileinfo.modifyTime, PR_GMTParameters, &tm);
            {
                char buf[64];
                PR_FormatTimeUSEnglish(buf, sizeof(buf), "%a,%%20%d%%20%b%%20%Y%%20%H:%M:%S%%20GMT ", &tm);
                mBuf.Append(buf);
            }

            // The "file-type" field
            PRBool isFile;
            rv = current->IsFile(&isFile);
            if (NS_FAILED(rv)) return rv; 
            if (isFile) {
                mBuf += "FILE ";
            }
            else {
                PRBool isDir;
                rv = current->IsDirectory(&isDir);
                if (NS_FAILED(rv)) return rv; 
                if (isDir) {
                    mBuf += "DIRECTORY ";
                }
                else {
                    PRBool isLink;
                    rv = current->IsSymlink(&isLink);
                    if (NS_FAILED(rv)) return rv; 
                    if (isLink) {
                        mBuf += "SYMBOLIC-LINK ";
                    }
                }
            }

            mBuf.Append('\n');
        }

        // ...and once we've either run out of directory entries, or
        // filled up the buffer, then we'll push it to the reader.
        while (mOffset < mBuf.Length() && aCount != 0) {
            *(aBuf++) = char(mBuf.CharAt(mOffset++));
            --aCount;
            ++nread;
        }
    }

    *aReadCount = nread;
    return NS_OK;
}
