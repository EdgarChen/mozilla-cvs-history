/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 4 -*-
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

#ifndef nsStdURL_h__
#define nsStdURL_h__

#include "nsIURL.h"
#include "nsAgg.h"
#include "nsCRT.h"
#include "nsString.h" // REMOVE Later!!

#define NS_THIS_STANDARDURL_IMPLEMENTATION_CID       \
{ /* e3939dc8-29ab-11d3-8cce-0060b0fc14a3 */         \
    0xe3939dc8,                                      \
    0x29ab,                                          \
    0x11d3,                                          \
    {0x8c, 0xce, 0x00, 0x60, 0xb0, 0xfc, 0x14, 0xa3} \
}

class nsStdURL : public nsIURL
{
public:
    ////////////////////////////////////////////////////////////////////////////
    // nsStdURL methods:

    nsStdURL(const char* i_Spec, nsISupports* outer=nsnull);
    virtual ~nsStdURL();

    static NS_METHOD
    Create(nsISupports *aOuter, REFNSIID aIID, void **aResult);

    NS_DECL_AGGREGATED

    ////////////////////////////////////////////////////////////////////////////
    // nsIURI methods:

    /* attribute string Spec; */
    NS_IMETHOD GetSpec(char * *aSpec);
    NS_IMETHOD SetSpec(char * aSpec);

    /* attribute string Scheme; */
    NS_IMETHOD GetScheme(char * *aScheme);
    NS_IMETHOD SetScheme(char * aScheme);

    /* attribute string PreHost; */
    NS_IMETHOD GetPreHost(char * *aPreHost);
    NS_IMETHOD SetPreHost(char * aPreHost);

    /* attribute string Host; */
    NS_IMETHOD GetHost(char * *aHost);
    NS_IMETHOD SetHost(char * aHost);

    /* attribute long Port; */
    NS_IMETHOD GetPort(PRInt32 *aPort);
    NS_IMETHOD SetPort(PRInt32 aPort);

    /* attribute string Path; */
    NS_IMETHOD GetPath(char * *aPath);
    NS_IMETHOD SetPath(char * aPath);

    /* boolean Equals (in nsIURI other); */
    NS_IMETHOD Equals(nsIURI *other, PRBool *_retval);

    /* nsIURI Clone (); */
    NS_IMETHOD Clone(nsIURI **_retval);

    /* void SetRelativePath (in string i_RelativePath); */
    NS_IMETHOD SetRelativePath(const char *i_RelativePath);

    ////////////////////////////////////////////////////////////////////////////
    // nsIURL methods:

    /* attribute string Directory; */
    NS_IMETHOD GetDirectory(char * *aDirectory);
    NS_IMETHOD SetDirectory(char * aDirectory);

    /* attribute string FileName; */
    NS_IMETHOD GetFileName(char * *aFileName);
    NS_IMETHOD SetFileName(char * aFileName);

    /* attribute string Query; */
    NS_IMETHOD GetQuery(char * *aQuery);
    NS_IMETHOD SetQuery(char * aQuery);

    /* attribute string Ref; */
    NS_IMETHOD GetRef(char * *aRef);
    NS_IMETHOD SetRef(char * aRef);

    /* void DirFile (out string o_DirFile); */
    NS_IMETHOD DirFile(char **o_DirFile);

	/* todo move this to protected later */
	nsresult ParsePath(void);
protected:
    nsresult Parse(void);
    nsresult ReconstructPath(void);
    nsresult ReconstructSpec(void);

    // Some handy functions 
	nsresult DupString(char* *o_Destination, char* i_Source);
	nsresult ExtractString(char* i_Source, char* *o_Destination, PRUint32 start, PRUint32 length);
protected:

    char*       mScheme;
    char*       mPreHost;
    char*       mHost;
    PRInt32     mPort;
    char*       mPath;

	char*		mDirectory;
	char*		mFileName;
    char*       mQuery;
    char*       mRef;

    char*       mSpec; 

private:
	nsStdURL(const nsStdURL&); // no-op
	nsStdURL& operator =(const nsStdURL&); // no-op

};

inline NS_METHOD
nsStdURL::GetSpec(char* *o_Spec)
{
	return DupString(o_Spec, mSpec);
}

inline NS_METHOD
nsStdURL::GetScheme(char* *o_Scheme)
{
	return DupString(o_Scheme, mScheme);
}

inline NS_METHOD
nsStdURL::GetPreHost(char* *o_PreHost)
{
	return DupString(o_PreHost, mPreHost);
}

inline NS_METHOD
nsStdURL::GetHost(char* *o_Host)
{
	return DupString(o_Host, mHost);
}

inline NS_METHOD
nsStdURL::GetPath(char* *o_Path)
{
	return DupString(o_Path, mPath);
}

inline NS_METHOD
nsStdURL::GetDirectory(char* *o_Directory)
{
	return DupString(o_Directory, mDirectory);
}

inline NS_METHOD
nsStdURL::GetFileName(char* *o_FileName)
{
	return DupString(o_FileName, mFileName);
}

inline NS_METHOD
nsStdURL::GetRef(char* *o_Ref)
{
	return DupString(o_Ref, mRef);
}

inline NS_METHOD
nsStdURL::GetQuery(char* *o_Query)
{
	return DupString(o_Query, mQuery);
}

inline NS_METHOD
nsStdURL::SetScheme(char* i_Scheme)
{
    if (mScheme) nsCRT::free(mScheme);
	nsresult status = DupString(&mScheme, i_Scheme);
	ReconstructSpec();
	return status;
}

inline NS_METHOD
nsStdURL::SetPreHost(char* i_PreHost)
{
    if (mPreHost) nsCRT::free(mPreHost);
	nsresult status = DupString(&mPreHost, i_PreHost);
	ReconstructSpec();
	return status;
}

inline NS_METHOD
nsStdURL::SetHost(char* i_Host)
{
    if (mHost) nsCRT::free(mHost);
	nsresult status = DupString(&mHost, i_Host);
	ReconstructSpec();
	return status;
}

inline NS_METHOD
nsStdURL::GetPort(PRInt32 *aPort)
{
	if (aPort)
	{
		*aPort = mPort;
		return NS_OK;
	}
	return NS_ERROR_NULL_POINTER;
}

inline NS_METHOD
nsStdURL::SetPort(PRInt32 aPort)
{
	mPort = aPort;
	ReconstructSpec();
	return NS_OK;
}

#endif // nsStdURL_h__
