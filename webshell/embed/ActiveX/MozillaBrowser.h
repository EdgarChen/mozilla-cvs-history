// MozillaBrowser.h : Declaration of the CMozillaBrowser

#ifndef __MOZILLABROWSER_H_
#define __MOZILLABROWSER_H_

// This file is autogenerated by the ATL proxy wizard
// so don't edit it!
#include "CPMozillaControl.h"

// DEVNOTE: Property and PropertyList should be defined inside CMozillaBrowser
//          but were moved outside due to a bug with templates on nested classes
//          in VC++ 5.0 which is not at SP3 level.

// Property is a name,variant pair held by the browser. In IE, properties
// offer a primitive way for DHTML elements to talk back and forth with
// the host app. The Mozilla app currently just implements them for
// compatibility reasons
struct Property
{
  CComBSTR szName;
  CComVariant vValue;
};

// A list of properties
typedef std::vector<Property> PropertyList;


// DEVNOTE: These operators are required since the unpatched VC++ 5.0
//          generates code even for unreferenced template methods in
//          the file <vector>  and will give compiler errors without
//          them. Service Pack 1 and above fixes this problem

int operator <(const Property&, const Property&);
int operator ==(const Property&, const Property&); 

class CWebShellContainer;

// Some definitions which are used to make firing events easier
#define CDWebBrowserEvents1 CProxyDWebBrowserEvents<CMozillaBrowser>
#define CDWebBrowserEvents2 CProxyDWebBrowserEvents2<CMozillaBrowser>

/////////////////////////////////////////////////////////////////////////////
// CMozillaBrowser
class ATL_NO_VTABLE CMozillaBrowser : 
	public CComObjectRootEx<CComSingleThreadModel>,
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
	COM_INTERFACE_ENTRY(IProvideClassInfo)
	COM_INTERFACE_ENTRY(IProvideClassInfo2)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
	COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
END_COM_MAP()

BEGIN_PROPERTY_MAP(CMozillaBrowser)
	// Example entries
	// PROP_ENTRY("Property Description", dispid, clsid)
	PROP_PAGE(CLSID_StockColorPage)
END_PROPERTY_MAP()


BEGIN_CONNECTION_POINT_MAP(CMozillaBrowser)
	// Fires IE events
	CONNECTION_POINT_ENTRY(DIID_DWebBrowserEvents)
	CONNECTION_POINT_ENTRY(DIID_DWebBrowserEvents2)
END_CONNECTION_POINT_MAP()


BEGIN_MSG_MAP(CMozillaBrowser)
	MESSAGE_HANDLER(WM_CREATE, OnCreate)
	MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
	MESSAGE_HANDLER(WM_SIZE, OnSize)
	MESSAGE_HANDLER(WM_PAINT, OnPaint)
	MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocus)
	MESSAGE_HANDLER(WM_KILLFOCUS, OnKillFocus)
END_MSG_MAP()

// Windows message handlers
	LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

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
	CWebShellContainer	*	m_pWebShellContainer;

	// Mozilla interfaces
    nsIWebShell			*	m_pIWebShell;
	nsIPref             *   m_pIPref;
	
	// Indicates the browser is busy doing something
	BOOL					m_bBusy;

	// Property list
	PropertyList m_PropertyList;
	
	virtual HRESULT CreateWebShell();
	virtual BOOL IsValid();

public:
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
