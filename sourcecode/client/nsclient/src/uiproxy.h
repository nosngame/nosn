#pragma once
namespace NSProxy
{
	class CNSUIProxy : public CNSLuaMarshal
	{
	public:
		CNSString mUIFile;

	public:
		CNSUIProxy( const CNSString& uiFile ) : mUIFile( uiFile )
		{
		}

	public:
		virtual CNSLuaStack& marshal( CNSLuaStack& luaStack ) const;
		virtual const CNSLuaStack& unmarshal( const CNSLuaStack& luaStack );
	};
	
	class CUIProxyComponent;
	class CUIProxyObject : public CNSLuaWeakRef
	{
	public:
		CNSMap< CNSString, CUIProxyComponent* > mUIProxyComp;
		int mInstanceID;

	public:
		CUIProxyObject( int id, const CNSString& objName ) : mInstanceID( id )
		{
		}

	public:
		virtual ~CUIProxyObject( )
		{
		}

	public:
		static void pushUserData( CNSLuaStack& luaStack, CUIProxyObject* ref );
		static void popUserData( const CNSLuaStack& luaStack, CUIProxyObject*& ref );
	};

	class CUIProxyComponent : public CNSLuaWeakRef
	{
	public:
		// ∂‘œÛ√˚
		CNSString mCompName;
		CUIProxyObject* mProxyObject = NULL;
		CNSLuaFunction onEventFunc;

	public:
		CUIProxyComponent( const CNSString& objName, CUIProxyObject* proxyObject ) : mCompName( objName ), mProxyObject( proxyObject )
		{
		}

	public:
		virtual ~CUIProxyComponent( )
		{
		}

	public:
		static void pushUserData( CNSLuaStack& luaStack, CUIProxyComponent* ref );
		static void popUserData( const CNSLuaStack& luaStack, CUIProxyComponent*& ref );
	};

	void regLuaLib( );
}
