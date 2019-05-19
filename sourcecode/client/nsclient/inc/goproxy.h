#pragma once
namespace NSProxy
{
	class CNSGoProxy : public CNSLuaMarshal
	{
	public:
		CNSString mFileName;
		int mType;
		int mParentID;

	public:
		CNSGoProxy( const CNSString& uiFile, int parentID, int type ) : mFileName( uiFile ), mParentID( parentID ), mType( type )
		{
		}

	public:
		virtual CNSLuaStack& marshal( CNSLuaStack& luaStack ) const;
		virtual const CNSLuaStack& unmarshal( const CNSLuaStack& luaStack );
	};
	
	class CGoProxyComponent;
	class CGoProxyObject : public CNSLuaWeakRef
	{
	public:
		CNSMap< CNSString, CGoProxyComponent* > mUIProxyComp;
		int mInstanceID;
		int mType;	// 0 - UI 1 - GameObject

	public:
		CGoProxyObject( int type, int id, const CNSString& objName ) : mType( type ), mInstanceID( id )
		{
		}

	public:
		virtual ~CGoProxyObject( )
		{
		}

	public:
		static void pushUserData( CNSLuaStack& luaStack, CGoProxyObject* ref );
		static void popUserData( const CNSLuaStack& luaStack, CGoProxyObject*& ref );
	};

	class CGoProxyComponent : public CNSLuaWeakRef
	{
	public:
		// ∂‘œÛ√˚
		CNSString mCompName;
		CGoProxyObject* mProxyObject = NULL;
		CNSLuaFunction onEventFunc;

	public:
		CGoProxyComponent( const CNSString& objName, CGoProxyObject* proxyObject ) : mCompName( objName ), mProxyObject( proxyObject )
		{
		}

	public:
		virtual ~CGoProxyComponent( )
		{
		}

	public:
		static void pushUserData( CNSLuaStack& luaStack, CGoProxyComponent* ref );
		static void popUserData( const CNSLuaStack& luaStack, CGoProxyComponent*& ref );
	};
}
