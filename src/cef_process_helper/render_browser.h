#ifndef RENDER_BROWSER_H
#define RENDER_BROWSER_H
#ifdef _WIN32
#pragma once
#endif

#include "include/cef_app.h"
#include "include/cef_v8.h"
#include "warscef/wars_cef_shared.h"

class RenderBrowser;
class ClientApp;

#define INVALID_IDENTIFIER -1

class WarsCefUserData : public CefBaseRefCounted 
{
public:
	WarsCefUserData() {}
	CefString function_uuid;
	IMPLEMENT_REFCOUNTING(WarsCefUserData);
};

class FunctionV8Handler : public CefV8Handler
{
public:
	FunctionV8Handler( CefRefPtr<RenderBrowser> renderBrowser );

	virtual void SetFunc( CefRefPtr<CefV8Value> func );

	virtual bool Execute(const CefString& name,
						CefRefPtr<CefV8Value> object,
						const CefV8ValueList& arguments,
						CefRefPtr<CefV8Value>& retval,
						CefString& exception) OVERRIDE;

protected:
	CefRefPtr<RenderBrowser> m_RenderBrowser;
	CefRefPtr<CefV8Value> m_Func;

	// Provide the reference counting implementation for this class.
	IMPLEMENT_REFCOUNTING(FunctionV8Handler);
};

class FunctionWithCallbackV8Handler : public FunctionV8Handler
{
public:
	FunctionWithCallbackV8Handler( CefRefPtr<RenderBrowser> renderBrowser ) : FunctionV8Handler( renderBrowser ) {}

	virtual bool Execute(const CefString& name,
						CefRefPtr<CefV8Value> object,
						const CefV8ValueList& arguments,
						CefRefPtr<CefV8Value>& retval,
						CefString& exception) OVERRIDE;
};

// Browsers representation on render process (maintains js objects)
class RenderBrowser : public CefBaseRefCounted
{
public:
	RenderBrowser( CefRefPtr<CefBrowser> browser, CefRefPtr<ClientApp> clientApp );

	void OnDestroyed();

	CefRefPtr<CefBrowser> GetBrowser();

	void SetV8Context( CefRefPtr<CefV8Context> context );
	CefRefPtr<CefV8Context> GetV8Context();

	void Clear();

	// Creating new objects
	bool RegisterObject( CefRefPtr<CefV8Value> object, WarsCefJSObject_t &data );
	bool RegisterObject( CefString identifier, CefRefPtr<CefV8Value> object );

	CefRefPtr<CefV8Value> FindObjectForUUID( CefString uuid );

	bool CreateGlobalObject( CefString identifier, CefString name );

	bool CreateFunction( CefString identifier, CefString name, CefString parentIdentifier = "", bool bCallback = false );



	// Function calling with "result"
	bool ExecuteJavascriptWithResult( CefString identifier, CefString code );

	// Function handlers
	void CallFunction(	CefRefPtr<CefV8Value> object, 
						const CefV8ValueList& arguments,
						CefRefPtr<CefV8Value>& retval,
						CefString& exception,
						CefRefPtr<CefV8Value> callback = NULL );

	bool DoCallback( int iCallbackID, CefRefPtr<CefListValue> methodargs );
	bool Invoke( CefString identifier, CefString methodname, CefRefPtr<CefListValue> methodargs );
	bool InvokeWithResult( CefString resultIdentifier, CefString identifier, CefString methodname, CefRefPtr<CefListValue> methodargs );

	bool ObjectSetAttr( CefString identifier, CefString attrname,  CefRefPtr<CefV8Value> value );
	bool ObjectGetAttr( CefString identifier, CefString attrname, CefString resultIdentifier );

private:
	CefRefPtr<CefBrowser> m_Browser;
	CefRefPtr<ClientApp> m_ClientApp;
	CefRefPtr<CefV8Context> m_Context;

	std::map< CefString, CefRefPtr<CefV8Value>> m_Objects;
	std::map< CefString, CefRefPtr<CefV8Value>> m_GlobalObjects;

	typedef struct jscallback_t {
		int callbackid;
		CefRefPtr<CefV8Value> callback;
		CefRefPtr<CefV8Value> thisobject;
	} jscallback_t;
	std::vector< jscallback_t > m_Callbacks;

	IMPLEMENT_REFCOUNTING( RenderBrowser );
};

inline CefRefPtr<CefBrowser> RenderBrowser::GetBrowser()
{
	return m_Browser;
}

inline CefRefPtr<CefV8Context> RenderBrowser::GetV8Context()
{
	return m_Context;
}

#endif // RENDER_BROWSER_H