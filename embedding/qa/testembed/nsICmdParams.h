#if !defined(AFX_NSICMDPARAMS_H__E2105F5B_B953_11D6_9BE4_00C04FA02BE6__INCLUDED_)
#define AFX_NSICMDPARAMS_H__E2105F5B_B953_11D6_9BE4_00C04FA02BE6__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// nsICmdParams.h : header file
//

#include "QaUtils.h"
#include "BrowserFrm.h"
#include "BrowserImpl.h"
#include "BrowserView.h"
#include "Tests.h"
#include "nsICommandMgr.h"

/////////////////////////////////////////////////////////////////////////////
// nsICmdParams window

class CnsICommandMgr;

class CnsICmdParams
{
// Construction
public:
	CnsICmdParams(nsIWebBrowser *mWebBrowser);

	nsCOMPtr<nsIWebBrowser> qaWebBrowser;
	nsCOMPtr<nsICommandManager> cmdMgrObj;
	nsCOMPtr<nsICommandParams> cmdParamObj;
// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CnsICmdParams)
	//}}AFX_VIRTUAL

// Implementation
	static nsICommandParams * GetCommandParamObject();
	void GetValueTypeTest(const char *, const char *);
	void GetBooleanValueTest(const char *, const char *);
	void GetLongValueTest(PRInt32, const char *);
	void GetDoubleValueTest(double, const char *);
	void GetStringValueTest(char *, const char *);
	void GetCStringValueTest(const char *, const char *);

	void SetBooleanValueTest(PRBool, const char *);
	void SetLongValueTest(PRInt32, const char *);
	void SetDoubleValueTest(double, const char *);
	void SetStringValueTest(char *, const char *);
	void SetCStringValueTest(char *, const char *);

	void OnStartTests(UINT nMenuID);
	void RunAllTests();

public:
	virtual ~CnsICmdParams();

	// Generated message map functions
protected:

};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_NSICMDPARAMS_H__E2105F5B_B953_11D6_9BE4_00C04FA02BE6__INCLUDED_)
