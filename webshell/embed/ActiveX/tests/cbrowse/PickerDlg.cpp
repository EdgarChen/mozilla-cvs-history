// PickerDlg.cpp : implementation file
//

#include "stdafx.h"
#include "cbrowse.h"
#include "PickerDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// CLSIDs for the Mozilla and IE browser controls

static const CLSID CLSID_Mozilla =
{ 0x1339B54C, 0x3453, 0x11D2, { 0x93, 0xB9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } };

static const CLSID CLSID_InternetExplorer =
{ 0x8856F961, 0x340A, 0x11D0, { 0xA9, 0x6B, 0x00, 0xC0, 0x4F, 0xD7, 0x05, 0xA2 } };

struct BrowserControl
{
	TCHAR *szName;
	const CLSID *clsid;
};

BrowserControl aControls[] =
{
	{ _T("Mozilla Control"), &CLSID_Mozilla },
	{ _T("Internet Explorer Control"), &CLSID_InternetExplorer }
};


/////////////////////////////////////////////////////////////////////////////
// CPickerDlg dialog


CPickerDlg::CPickerDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CPickerDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CPickerDlg)
	m_szTestURL = _T("");
	m_szTestCGI = _T("");
	//}}AFX_DATA_INIT
	m_clsid = CLSID_NULL;

	CWinApp *pApp = AfxGetApp();
	m_szTestURL = pApp->GetProfileString(SECTION_TEST, KEY_TESTURL, KEY_TESTURL_DEFAULTVALUE);
	m_szTestCGI = pApp->GetProfileString(SECTION_TEST, KEY_TESTCGI, KEY_TESTCGI_DEFAULTVALUE);
}


void CPickerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPickerDlg)
	DDX_Control(pDX, IDC_LISTBROWSER, m_lbPicker);
	DDX_Text(pDX, IDC_TESTURL, m_szTestURL);
	DDX_Text(pDX, IDC_TESTCGI, m_szTestCGI);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPickerDlg, CDialog)
	//{{AFX_MSG_MAP(CPickerDlg)
	ON_BN_CLICKED(IDOK, OnOk)
	ON_LBN_DBLCLK(IDC_LISTBROWSER, OnDblclkListbrowser)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPickerDlg message handlers

BOOL CPickerDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

	for (int i = 0; i < sizeof(aControls) / sizeof(aControls[0]); i++)
	{
		m_lbPicker.AddString(aControls[i].szName);
	}
	m_lbPicker.SetCurSel(0);
		
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


void CPickerDlg::OnOk() 
{
	UpdateData();

	int nItem = m_lbPicker.GetCurSel();
	if (nItem == LB_ERR)
	{
		AfxMessageBox(IDS_CHOOSEBROWSER);
		return;
	}

	m_clsid = *aControls[nItem].clsid;

	CWinApp *pApp = AfxGetApp();
	pApp->WriteProfileString(SECTION_TEST, KEY_TESTURL, m_szTestURL);
	pApp->WriteProfileString(SECTION_TEST, KEY_TESTCGI, m_szTestCGI);

	EndDialog(IDOK);
}


void CPickerDlg::OnDblclkListbrowser() 
{
	OnOk();
}
