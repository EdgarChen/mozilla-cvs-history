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
// MozillaBrowser.h : Declaration of the CMozillaBrowser

#ifndef __MOZILLABROWSER_H_
#define __MOZILLABROWSER_H_

// This file is autogenerated by the ATL proxy wizard
// so don't edit it!
#include "CPMozillaControl.h"

// Commands sent via WM_COMMAND
#define ID_PRINT     1
#define ID_PAGESETUP 2
#define ID_VIEWSOURCE 3

// Command group and IDs exposed through IOleCommandTarget
extern const GUID CGID_IWebBrowser;
extern const GUID CGID_MSHTML;

#define HTMLID_FIND 1
#define HTMLID_VIEWSOURCE 2
#define HTMLID_OPTIONS 3

// Some definitions which are used to make firing events easier
#define CDWebBrowserEvents1 CProxyDWebBrowserEvents<CMozillaBrowser>
#define CDWebBrowserEvents2 CProxyDWebBrowserEvents2<CMozillaBrowser>

// A list of objects
typedef CComPtr<IUnknown> CComUnkPtr;
typedef std::vector<CComUnkPtr> ObjectList;

class CWebShellContainer;

/////////////////////////////////////////////////////////////////////////////
// CMozillaBrowser
class ATL_NO_VTABLE CMozillaBrowser : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CMozillaBrowser, &CLSID_MozillaBrowser>,
	public CComControl<CMozillaBrowser>,
	public CDWebBrowserEvents1,
	public CDWebBrowserEvents2,
	public CStockPropImpl<CMozillaBrowser, IWebBrowser2, &IID_IWebBrowser2, &LIBID_MOZILLACONTROLLib>,
	public IProvideClassInfo2Impl<&CLSID_MozillaBrowser, &DIID_DWebBrowserEvents2, &LIBID_MOZILLACONTROLLib>,
	public IPersistStreamInitImpl<CMozillaBrowser>,
	public IPersistStorageImpl<CMozillaBrowser>,
	public IQuickActivateImpl<CMozillaBrowser>,
	public IOleControlImpl<CMozillaBrowser>,
	public IOleObjectImpl<CMozillaBrowser>,
	public IOleInPlaceActiveObjectImpl<CMozillaBrowser>,
	public IViewObjectExImpl<CMozillaBrowser>,
	public IOleInPlaceObjectWindowlessImpl<CMozillaBrowser>,
	public IDataObjectImpl<CMozillaBrowser>,
	public ISupportErrorInfo,
	public IOleCommandTargetImpl<CMozillaBrowser>,
	public IConnectionPointContainerImpl<CMozillaBrowser>,
	public ISpecifyPropertyPagesImpl<CMozillaBrowser>
{
	friend CWebShellContainer;
public:
	CMozillaBrowser();
	virtual ~CMozillaBrowser();

DECLARE_REGISTRY_RESOURCEID(IDR_MOZILLABROWSER)

BEGIN_COM_MAP(CMozillaBrowser)
	// IE web browser interface
	COM_INTERFACE_ENTRY(IWebBrowser2)
	COM_INTERFACE_ENTRY_IID(IID_IDispatch, IWebBrowser2)
	COM_INTERFACE_ENTRY_IID(IID_IWebBrowser, IWebBrowser2)
	COM_INTERFACE_ENTRY_IID(IID_IWebBrowserApp, IWebBrowser2)
	COM_INTERFACE_ENTRY_IMPL(IViewObjectEx)
	COM_INTERFACE_ENTRY_IMPL_IID(IID_IViewObject2, IViewObjectEx)
	COM_INTERFACE_ENTRY_IMPL_IID(IID_IViewObject, IViewObjectEx)
	COM_INTERFACE_ENTRY_IMPL(IOleInPlaceObjectWindowless)
	COM_INTERFACE_ENTRY_IMPL_IID(IID_IOleInPlaceObject, IOleInPlaceObjectWindowless)
	COM_INTERFACE_ENTRY_IMPL_IID(IID_IOleWindow, IOleInPlaceObjectWindowless)
	COM_INTERFACE_ENTRY_IMPL(IOleInPlaceActiveObject)
	COM_INTERFACE_ENTRY_IMPL(IOleControl)
	COM_INTERFACE_ENTRY_IMPL(IOleObject)
//	COM_INTERFACE_ENTRY_IMPL(IQuickActivate) // This causes size assertion in ATL
	COM_INTERFACE_ENTRY_IMPL(IPersistStorage)
	COM_INTERFACE_ENTRY_IMPL(IPersistStreamInit)
	COM_INTERFACE_ENTRY_IMPL(ISpecifyPropertyPages)
	COM_INTERFACE_ENTRY_IMPL(IDataObject)
	COM_INTERFACE_ENTRY(IOleCommandTarget)
	COM_INTERFACE_ENTRY(IProvideClassInfo)
	COM_INTERFACE_ENTRY(IProvideClassInfo2)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
	COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
	COM_INTERFACE_ENTRY_IID(DIID_DWebBrowserEvents,  CDWebBrowserEvents1)
	COM_INTERFACE_ENTRY_IID(DIID_DWebBrowserEvents2, CDWebBrowserEvents2)
END_COM_MAP()

BEGIN_PROPERTY_MAP(CMozillaBrowser)
	// Example entries
	// PROP_ENTRY("Property Description", dispid, clsid)
	PROP_PAGE(CLSID_StockColorPage)
END_PROPERTY_MAP()


BEGIN_CONNECTION_POINT_MAP(CMozillaBrowser)
	// Fires IE events
	CONNECTION_POINT_ENTRY(DIID_DWebBrowserEvents2)
	CONNECTION_POINT_ENTRY(DIID_DWebBrowserEvents)
END_CONNECTION_POINT_MAP()


BEGIN_MSG_MAP(CMozillaBrowser)
	MESSAGE_HANDLER(WM_CREATE, OnCreate)
	MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
	MESSAGE_HANDLER(WM_SIZE, OnSize)
	MESSAGE_HANDLER(WM_PAINT, OnPaint)
	MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocus)
	MESSAGE_HANDLER(WM_KILLFOCUS, OnKillFocus)
	COMMAND_ID_HANDLER(ID_PRINT, OnPrint)
	COMMAND_ID_HANDLER(ID_PAGESETUP, OnPageSetup)
END_MSG_MAP()

	static HRESULT _stdcall EditModeHandler(CMozillaBrowser *pThis, const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut);
	static HRESULT _stdcall EditCommandHandler(CMozillaBrowser *pThis, const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut);

BEGIN_OLECOMMAND_TABLE()
	// Standard "common" commands
	OLECOMMAND_MESSAGE(OLECMDID_PRINT, NULL, ID_PRINT, L"Print", L"Print the page")
	OLECOMMAND_MESSAGE(OLECMDID_SAVEAS, NULL, 0, L"SaveAs", L"Save the page")
	OLECOMMAND_MESSAGE(OLECMDID_PAGESETUP, NULL, ID_PAGESETUP, L"Page Setup", L"Page Setup")
	OLECOMMAND_MESSAGE(OLECMDID_PROPERTIES, NULL, 0, L"Properties", L"Show page properties")
	OLECOMMAND_MESSAGE(OLECMDID_CUT, NULL, 0, L"Cut", L"Cut selection")
	OLECOMMAND_MESSAGE(OLECMDID_COPY, NULL, 0, L"Copy", L"Copy selection")
	OLECOMMAND_MESSAGE(OLECMDID_PASTE, NULL, 0, L"Paste", L"Paste as selection")
	OLECOMMAND_MESSAGE(OLECMDID_UNDO, NULL, 0, L"Undo", L"Undo")
	OLECOMMAND_MESSAGE(OLECMDID_REDO, NULL, 0, L"Redo", L"Redo")
	OLECOMMAND_MESSAGE(OLECMDID_SELECTALL, NULL, 0, L"SelectAll", L"Select all")
	OLECOMMAND_MESSAGE(OLECMDID_REFRESH, NULL, 0, L"Refresh", L"Refresh")
	OLECOMMAND_MESSAGE(OLECMDID_STOP, NULL, 0, L"Stop", L"Stop")
	OLECOMMAND_MESSAGE(OLECMDID_ONUNLOAD, NULL, 0, L"OnUnload", L"OnUnload")
	// Unsupported IE 4.x command group
	OLECOMMAND_MESSAGE(HTMLID_FIND, &CGID_IWebBrowser, 0, L"Find", L"Find")
	OLECOMMAND_MESSAGE(HTMLID_VIEWSOURCE, &CGID_IWebBrowser, 0, L"ViewSource", L"View Source")
	OLECOMMAND_MESSAGE(HTMLID_OPTIONS, &CGID_IWebBrowser, 0, L"Options", L"Options")
	// DHTML editor command group
	OLECOMMAND_HANDLER(IDM_EDITMODE, &CGID_MSHTML, EditModeHandler, L"EditMode", L"Switch to edit mode")
	OLECOMMAND_HANDLER(IDM_BROWSEMODE, &CGID_MSHTML, EditModeHandler, L"UserMode", L"Switch to user mode")
	OLECOMMAND_HANDLER(IDM_BOLD, &CGID_MSHTML, EditCommandHandler, L"Bold", L"Toggle Bold")
	OLECOMMAND_HANDLER(IDM_ITALIC, &CGID_MSHTML, EditCommandHandler, L"Italic", L"Toggle Italic")
	OLECOMMAND_HANDLER(IDM_UNDERLINE, &CGID_MSHTML, EditCommandHandler, L"Underline", L"Toggle Underline")
	OLECOMMAND_HANDLER(IDM_UNKNOWN, &CGID_MSHTML, NULL, L"", L"")
	OLECOMMAND_HANDLER(IDM_ALIGNBOTTOM, &CGID_MSHTML, NULL, L"", L"")
	OLECOMMAND_HANDLER(IDM_ALIGNHORIZONTALCENTERS, &CGID_MSHTML, NULL, L"", L"")
	OLECOMMAND_HANDLER(IDM_ALIGNLEFT, &CGID_MSHTML, NULL, L"", L"")
	OLECOMMAND_HANDLER(IDM_ALIGNRIGHT, &CGID_MSHTML, NULL, L"", L"")
	OLECOMMAND_HANDLER(IDM_ALIGNTOGRID, &CGID_MSHTML, NULL, L"", L"")
	OLECOMMAND_HANDLER(IDM_ALIGNTOP, &CGID_MSHTML, NULL, L"", L"")
	OLECOMMAND_HANDLER(IDM_ALIGNVERTICALCENTERS, &CGID_MSHTML, NULL, L"", L"")
	OLECOMMAND_HANDLER(IDM_ARRANGEBOTTOM, &CGID_MSHTML, NULL, L"", L"")
	OLECOMMAND_HANDLER(IDM_ARRANGERIGHT, &CGID_MSHTML, NULL, L"", L"")
	OLECOMMAND_HANDLER(IDM_BRINGFORWARD, &CGID_MSHTML, NULL, L"", L"")
	OLECOMMAND_HANDLER(IDM_BRINGTOFRONT, &CGID_MSHTML, NULL, L"", L"")
	OLECOMMAND_HANDLER(IDM_CENTERHORIZONTALLY, &CGID_MSHTML, NULL, L"", L"")
	OLECOMMAND_HANDLER(IDM_CENTERVERTICALLY, &CGID_MSHTML, NULL, L"", L"")
	OLECOMMAND_HANDLER(IDM_CODE, &CGID_MSHTML, NULL, L"", L"")
	OLECOMMAND_HANDLER(IDM_DELETE, &CGID_MSHTML, NULL, L"", L"")
	OLECOMMAND_HANDLER(IDM_FONTNAME, &CGID_MSHTML, NULL, L"", L"")
	OLECOMMAND_HANDLER(IDM_FONTSIZE, &CGID_MSHTML, NULL, L"", L"")
	OLECOMMAND_HANDLER(IDM_GROUP, &CGID_MSHTML, NULL, L"", L"")
	OLECOMMAND_HANDLER(IDM_HORIZSPACECONCATENATE, &CGID_MSHTML, NULL, L"", L"")
	OLECOMMAND_HANDLER(IDM_HORIZSPACEDECREASE, &CGID_MSHTML, NULL, L"", L"")
	OLECOMMAND_HANDLER(IDM_HORIZSPACEINCREASE, &CGID_MSHTML, NULL, L"", L"")
	OLECOMMAND_HANDLER(IDM_HORIZSPACEMAKEEQUAL, &CGID_MSHTML, NULL, L"", L"")
	OLECOMMAND_HANDLER(IDM_INSERTOBJECT, &CGID_MSHTML, NULL, L"", L"")
	OLECOMMAND_HANDLER(IDM_MULTILEVELREDO, &CGID_MSHTML, NULL, L"", L"")
	OLECOMMAND_HANDLER(IDM_SENDBACKWARD, &CGID_MSHTML, NULL, L"", L"")
	OLECOMMAND_HANDLER(IDM_SENDTOBACK, &CGID_MSHTML, NULL, L"", L"")
	OLECOMMAND_HANDLER(IDM_SHOWTABLE, &CGID_MSHTML, NULL, L"", L"")
	OLECOMMAND_HANDLER(IDM_SIZETOCONTROL, &CGID_MSHTML, NULL, L"", L"")
	OLECOMMAND_HANDLER(IDM_SIZETOCONTROLHEIGHT, &CGID_MSHTML, NULL, L"", L"")
	OLECOMMAND_HANDLER(IDM_SIZETOCONTROLWIDTH, &CGID_MSHTML, NULL, L"", L"")
	OLECOMMAND_HANDLER(IDM_SIZETOFIT, &CGID_MSHTML, NULL, L"", L"")
	OLECOMMAND_HANDLER(IDM_SIZETOGRID, &CGID_MSHTML, NULL, L"", L"")
	OLECOMMAND_HANDLER(IDM_SNAPTOGRID, &CGID_MSHTML, NULL, L"", L"")
	OLECOMMAND_HANDLER(IDM_TABORDER, &CGID_MSHTML, NULL, L"", L"")
	OLECOMMAND_HANDLER(IDM_TOOLBOX, &CGID_MSHTML, NULL, L"", L"")
	OLECOMMAND_HANDLER(IDM_MULTILEVELUNDO, &CGID_MSHTML, NULL, L"", L"")
	OLECOMMAND_HANDLER(IDM_UNGROUP, &CGID_MSHTML, NULL, L"", L"")
	OLECOMMAND_HANDLER(IDM_VERTSPACECONCATENATE, &CGID_MSHTML, NULL, L"", L"")
	OLECOMMAND_HANDLER(IDM_VERTSPACEDECREASE, &CGID_MSHTML, NULL, L"", L"")
	OLECOMMAND_HANDLER(IDM_VERTSPACEINCREASE, &CGID_MSHTML, NULL, L"", L"")
	OLECOMMAND_HANDLER(IDM_VERTSPACEMAKEEQUAL, &CGID_MSHTML, NULL, L"", L"")
	OLECOMMAND_HANDLER(IDM_JUSTIFYFULL, &CGID_MSHTML, NULL, L"", L"")
	OLECOMMAND_HANDLER(IDM_BACKCOLOR, &CGID_MSHTML, NULL, L"", L"")
	OLECOMMAND_HANDLER(IDM_BORDERCOLOR, &CGID_MSHTML, NULL, L"", L"")
	OLECOMMAND_HANDLER(IDM_FLAT, &CGID_MSHTML, NULL, L"", L"")
	OLECOMMAND_HANDLER(IDM_FORECOLOR, &CGID_MSHTML, NULL, L"", L"")
	OLECOMMAND_HANDLER(IDM_JUSTIFYCENTER, &CGID_MSHTML, NULL, L"", L"")
	OLECOMMAND_HANDLER(IDM_JUSTIFYGENERAL, &CGID_MSHTML, NULL, L"", L"")
	OLECOMMAND_HANDLER(IDM_JUSTIFYLEFT, &CGID_MSHTML, NULL, L"", L"")
	OLECOMMAND_HANDLER(IDM_JUSTIFYRIGHT, &CGID_MSHTML, NULL, L"", L"")
	OLECOMMAND_HANDLER(IDM_RAISED, &CGID_MSHTML, NULL, L"", L"")
	OLECOMMAND_HANDLER(IDM_SUNKEN, &CGID_MSHTML, NULL, L"", L"")
	OLECOMMAND_HANDLER(IDM_CHISELED, &CGID_MSHTML, NULL, L"", L"")
	OLECOMMAND_HANDLER(IDM_ETCHED, &CGID_MSHTML, NULL, L"", L"")
	OLECOMMAND_HANDLER(IDM_SHADOWED, &CGID_MSHTML, NULL, L"", L"")
	OLECOMMAND_HANDLER(IDM_FIND, &CGID_MSHTML, NULL, L"", L"")
	OLECOMMAND_HANDLER(IDM_SHOWGRID, &CGID_MSHTML, NULL, L"", L"")
	OLECOMMAND_HANDLER(IDM_OBJECTVERBLIST0, &CGID_MSHTML, NULL, L"", L"")
	OLECOMMAND_HANDLER(IDM_OBJECTVERBLIST1, &CGID_MSHTML, NULL, L"", L"")
	OLECOMMAND_HANDLER(IDM_OBJECTVERBLIST2, &CGID_MSHTML, NULL, L"", L"")
	OLECOMMAND_HANDLER(IDM_OBJECTVERBLIST3, &CGID_MSHTML, NULL, L"", L"")
	OLECOMMAND_HANDLER(IDM_OBJECTVERBLIST4, &CGID_MSHTML, NULL, L"", L"")
	OLECOMMAND_HANDLER(IDM_OBJECTVERBLIST5, &CGID_MSHTML, NULL, L"", L"")
	OLECOMMAND_HANDLER(IDM_OBJECTVERBLIST6, &CGID_MSHTML, NULL, L"", L"")
	OLECOMMAND_HANDLER(IDM_OBJECTVERBLIST7, &CGID_MSHTML, NULL, L"", L"")
	OLECOMMAND_HANDLER(IDM_OBJECTVERBLIST8, &CGID_MSHTML, NULL, L"", L"")
	OLECOMMAND_HANDLER(IDM_OBJECTVERBLIST9, &CGID_MSHTML, NULL, L"", L"")
	OLECOMMAND_HANDLER(IDM_OBJECTVERBLISTLAST, &CGID_MSHTML, NULL, L"", L"")
	OLECOMMAND_HANDLER(IDM_CONVERTOBJECT, &CGID_MSHTML, NULL, L"", L"")
	OLECOMMAND_HANDLER(IDM_CUSTOMCONTROL, &CGID_MSHTML, NULL, L"", L"")
	OLECOMMAND_HANDLER(IDM_CUSTOMIZEITEM, &CGID_MSHTML, NULL, L"", L"")
	OLECOMMAND_HANDLER(IDM_RENAME, &CGID_MSHTML, NULL, L"", L"")
	OLECOMMAND_HANDLER(IDM_IMPORT, &CGID_MSHTML, NULL, L"", L"")
	OLECOMMAND_HANDLER(IDM_NEWPAGE, &CGID_MSHTML, NULL, L"", L"")
	OLECOMMAND_HANDLER(IDM_MOVE, &CGID_MSHTML, NULL, L"", L"")
	OLECOMMAND_HANDLER(IDM_CANCEL, &CGID_MSHTML, NULL, L"", L"")
	OLECOMMAND_HANDLER(IDM_FONT, &CGID_MSHTML, NULL, L"", L"")
	OLECOMMAND_HANDLER(IDM_STRIKETHROUGH, &CGID_MSHTML, NULL, L"", L"")
	OLECOMMAND_HANDLER(IDM_DELETEWORD, &CGID_MSHTML, NULL, L"", L"")
	OLECOMMAND_HANDLER(IDM_EXECPRINT, &CGID_MSHTML, NULL, L"", L"")
	OLECOMMAND_HANDLER(IDM_JUSTIFYNONE, &CGID_MSHTML, NULL, L"", L"")
END_OLECOMMAND_TABLE()



	HWND GetCommandTargetWindow()
	{
		return m_hWnd;
	}

// Windows message handlers
	LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnPrint(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnPageSetup(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT OnViewSource(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// IViewObjectEx
	STDMETHOD(GetViewStatus)(DWORD* pdwStatus)
	{
		ATLTRACE(_T("IViewObjectExImpl::GetViewStatus\n"));
		*pdwStatus = VIEWSTATUS_SOLIDBKGND | VIEWSTATUS_OPAQUE;
		return S_OK;
	}

// Protected members
protected:
	// Flag to prevent multiple object registrations
	static BOOL m_bRegistryInitialized;

	// Pointer to web shell manager
	CWebShellContainer	*	m_pWebShellContainer;
	// CComObject to IHTMLDocument implementer
	CIEHtmlDocumentInstance * m_pDocument;

	// Mozilla interfaces
    nsIWebShell			*	m_pIWebShell;
	nsIPref             *   m_pIPref;
	nsIEditor			*	m_pEditor;

	// Indicates the browser is busy doing something
	BOOL					m_bBusy;
	// Flag to indicate if browser is in edit mode or not
	BOOL					m_bEditorMode;
	// Flag to indicate if the browser has a drop target
	BOOL                    m_bDropTarget;
	// Contains an error message if startup went wrong
	tstring					m_sErrorMessage;
	// Property list
	PropertyList			m_PropertyList;
	// Ready status of control
	READYSTATE				m_nBrowserReadyState;
	// List of registered browser helper objects
	ObjectList				m_cBrowserHelperList;

	virtual HRESULT SetErrorInfo(LPCTSTR lpszDesc, HRESULT hr);
	virtual HRESULT CreateWebShell();
	virtual HRESULT GetPresShell(nsIPresShell **pPresShell);
	virtual HRESULT GetDOMDocument(nsIDOMDocument **pDocument);
	virtual HRESULT SetEditorMode(BOOL bEnabled);
	virtual HRESULT OnEditorCommand(DWORD nCmdID);
	virtual BOOL IsValid();
	virtual int MessageBox(LPCTSTR lpszText, LPCTSTR lpszCaption = _T(""), UINT nType = MB_OK);

	virtual HRESULT LoadBrowserHelpers();
	virtual HRESULT UnloadBrowserHelpers();

public:
// IOleObjectImpl overrides
	HRESULT InPlaceActivate(LONG iVerb, const RECT* prcPosRect);

// IOleObject overrides
	virtual HRESULT STDMETHODCALLTYPE CMozillaBrowser::GetClientSite(IOleClientSite **ppClientSite);


// IWebBrowser implementation
    virtual HRESULT STDMETHODCALLTYPE GoBack(void);
    virtual HRESULT STDMETHODCALLTYPE GoForward(void);
    virtual HRESULT STDMETHODCALLTYPE GoHome(void);
    virtual HRESULT STDMETHODCALLTYPE GoSearch(void);
    virtual HRESULT STDMETHODCALLTYPE Navigate(BSTR URL, VARIANT __RPC_FAR *Flags, VARIANT __RPC_FAR *TargetFrameName, VARIANT __RPC_FAR *PostData, VARIANT __RPC_FAR *Headers);
    virtual HRESULT STDMETHODCALLTYPE Refresh(void);
    virtual HRESULT STDMETHODCALLTYPE Refresh2(VARIANT __RPC_FAR *Level);
    virtual HRESULT STDMETHODCALLTYPE Stop( void);
    virtual HRESULT STDMETHODCALLTYPE get_Application(IDispatch __RPC_FAR *__RPC_FAR *ppDisp);
    virtual HRESULT STDMETHODCALLTYPE get_Parent(IDispatch __RPC_FAR *__RPC_FAR *ppDisp);
    virtual HRESULT STDMETHODCALLTYPE get_Container(IDispatch __RPC_FAR *__RPC_FAR *ppDisp);
    virtual HRESULT STDMETHODCALLTYPE get_Document(IDispatch __RPC_FAR *__RPC_FAR *ppDisp);
    virtual HRESULT STDMETHODCALLTYPE get_TopLevelContainer(VARIANT_BOOL __RPC_FAR *pBool);
    virtual HRESULT STDMETHODCALLTYPE get_Type(BSTR __RPC_FAR *Type);
    virtual HRESULT STDMETHODCALLTYPE get_Left(long __RPC_FAR *pl);
    virtual HRESULT STDMETHODCALLTYPE put_Left(long Left);
    virtual HRESULT STDMETHODCALLTYPE get_Top(long __RPC_FAR *pl);
    virtual HRESULT STDMETHODCALLTYPE put_Top(long Top);
    virtual HRESULT STDMETHODCALLTYPE get_Width(long __RPC_FAR *pl);
    virtual HRESULT STDMETHODCALLTYPE put_Width(long Width);
    virtual HRESULT STDMETHODCALLTYPE get_Height(long __RPC_FAR *pl);
    virtual HRESULT STDMETHODCALLTYPE put_Height(long Height);
    virtual HRESULT STDMETHODCALLTYPE get_LocationName(BSTR __RPC_FAR *LocationName);
    virtual HRESULT STDMETHODCALLTYPE get_LocationURL(BSTR __RPC_FAR *LocationURL);
    virtual HRESULT STDMETHODCALLTYPE get_Busy(VARIANT_BOOL __RPC_FAR *pBool);

// IWebBrowserApp implementation
	virtual HRESULT STDMETHODCALLTYPE Quit(void);
	virtual HRESULT STDMETHODCALLTYPE ClientToWindow(int __RPC_FAR *pcx, int __RPC_FAR *pcy);
	virtual HRESULT STDMETHODCALLTYPE PutProperty(BSTR Property, VARIANT vtValue);
	virtual HRESULT STDMETHODCALLTYPE GetProperty(BSTR Property, VARIANT __RPC_FAR *pvtValue);
	virtual HRESULT STDMETHODCALLTYPE get_Name(BSTR __RPC_FAR *Name);
	virtual HRESULT STDMETHODCALLTYPE get_HWND(long __RPC_FAR *pHWND);
	virtual HRESULT STDMETHODCALLTYPE get_FullName(BSTR __RPC_FAR *FullName);
	virtual HRESULT STDMETHODCALLTYPE get_Path(BSTR __RPC_FAR *Path);
	virtual HRESULT STDMETHODCALLTYPE get_Visible(VARIANT_BOOL __RPC_FAR *pBool);
	virtual HRESULT STDMETHODCALLTYPE put_Visible(VARIANT_BOOL Value);
	virtual HRESULT STDMETHODCALLTYPE get_StatusBar(VARIANT_BOOL __RPC_FAR *pBool);
	virtual HRESULT STDMETHODCALLTYPE put_StatusBar(VARIANT_BOOL Value);
	virtual HRESULT STDMETHODCALLTYPE get_StatusText(BSTR __RPC_FAR *StatusText);
	virtual HRESULT STDMETHODCALLTYPE put_StatusText(BSTR StatusText);
	virtual HRESULT STDMETHODCALLTYPE get_ToolBar(int __RPC_FAR *Value);
	virtual HRESULT STDMETHODCALLTYPE put_ToolBar(int Value);
	virtual HRESULT STDMETHODCALLTYPE get_MenuBar(VARIANT_BOOL __RPC_FAR *Value);
	virtual HRESULT STDMETHODCALLTYPE put_MenuBar(VARIANT_BOOL Value);
	virtual HRESULT STDMETHODCALLTYPE get_FullScreen(VARIANT_BOOL __RPC_FAR *pbFullScreen);
	virtual HRESULT STDMETHODCALLTYPE put_FullScreen(VARIANT_BOOL bFullScreen);

// IWebBrowser2 implementation
	virtual HRESULT STDMETHODCALLTYPE Navigate2(VARIANT __RPC_FAR *URL, VARIANT __RPC_FAR *Flags, VARIANT __RPC_FAR *TargetFrameName, VARIANT __RPC_FAR *PostData, VARIANT __RPC_FAR *Headers);
	virtual HRESULT STDMETHODCALLTYPE QueryStatusWB(OLECMDID cmdID, OLECMDF __RPC_FAR *pcmdf);
	virtual HRESULT STDMETHODCALLTYPE ExecWB(OLECMDID cmdID, OLECMDEXECOPT cmdexecopt, VARIANT __RPC_FAR *pvaIn, VARIANT __RPC_FAR *pvaOut);
	virtual HRESULT STDMETHODCALLTYPE ShowBrowserBar(VARIANT __RPC_FAR *pvaClsid, VARIANT __RPC_FAR *pvarShow, VARIANT __RPC_FAR *pvarSize);
	virtual HRESULT STDMETHODCALLTYPE get_ReadyState(READYSTATE __RPC_FAR *plReadyState);
	virtual HRESULT STDMETHODCALLTYPE get_Offline(VARIANT_BOOL __RPC_FAR *pbOffline);
	virtual HRESULT STDMETHODCALLTYPE put_Offline(VARIANT_BOOL bOffline);
	virtual HRESULT STDMETHODCALLTYPE get_Silent(VARIANT_BOOL __RPC_FAR *pbSilent);
	virtual HRESULT STDMETHODCALLTYPE put_Silent(VARIANT_BOOL bSilent);
	virtual HRESULT STDMETHODCALLTYPE get_RegisterAsBrowser(VARIANT_BOOL __RPC_FAR *pbRegister);
	virtual HRESULT STDMETHODCALLTYPE put_RegisterAsBrowser(VARIANT_BOOL bRegister);
	virtual HRESULT STDMETHODCALLTYPE get_RegisterAsDropTarget(VARIANT_BOOL __RPC_FAR *pbRegister);
	virtual HRESULT STDMETHODCALLTYPE put_RegisterAsDropTarget(VARIANT_BOOL bRegister);
	virtual HRESULT STDMETHODCALLTYPE get_TheaterMode(VARIANT_BOOL __RPC_FAR *pbRegister);
	virtual HRESULT STDMETHODCALLTYPE put_TheaterMode(VARIANT_BOOL bRegister);
	virtual HRESULT STDMETHODCALLTYPE get_AddressBar(VARIANT_BOOL __RPC_FAR *Value);
	virtual HRESULT STDMETHODCALLTYPE put_AddressBar(VARIANT_BOOL Value);
	virtual HRESULT STDMETHODCALLTYPE get_Resizable(VARIANT_BOOL __RPC_FAR *Value);
	virtual HRESULT STDMETHODCALLTYPE put_Resizable(VARIANT_BOOL Value);

public:
	HRESULT OnDraw(ATL_DRAWINFO& di);

};

#endif //__MOZILLABROWSER_H_
