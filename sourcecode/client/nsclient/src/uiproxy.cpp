#include "precomplie.h"
namespace NSProxy
{
	CNSMap< int, CUIProxyObject* > gUIProxyObject;
	CNSLuaStack* gpLuaStack = NULL;
	static int loadLayout( lua_State* lua );

	BEGIN_EXPORT( NosnUI )
		EXPORT_FUNC( loadLayout )
	END_EXPORT

	static int loadLayout( lua_State* lua )
	{
		DECLARE_BEGIN_PROTECTED
		CNSString uiFile;
		luaStack >> uiFile;
		CNSUIProxy uiProxy( uiFile );
		luaStack << uiProxy;
		DECLARE_END_PROTECTED
	}
	
	static int runAction( lua_State* lua )
	{
		DECLARE_BEGIN_PROTECTED
		DECLARE_END_PROTECTED
	}

	static int destroy( lua_State* lua )
	{
		DECLARE_BEGIN_PROTECTED
		CUIProxyObject* proxy = NULL;
		luaStack >> proxy;

		// 销毁unity对象
		NSClient::gUIDestroyProc( proxy->mInstanceID );

		// 销毁组件的lua事件函数
		HLISTINDEX beginIndex = proxy->mUIProxyComp.getHead( );
		for ( ; beginIndex != NULL; proxy->mUIProxyComp.getNext( beginIndex ) )
		{
			NSProxy::CUIProxyComponent* proxyComp = proxy->mUIProxyComp.getValue( beginIndex );
			if ( proxyComp->onEventFunc.isValid( ) == true )
				luaStack.clearFunc( proxyComp->onEventFunc );
			
			proxyComp->cleanUpRef( );
			delete proxyComp;
		}

		// 清理所有引用
		proxy->cleanUpRef( );

		// 删除对象
		HMAPINDEX mapIndex = gUIProxyObject.findNode( proxy->mInstanceID );
		if ( mapIndex != NULL )
		{
			NSProxy::CUIProxyObject* proxyObjectRef = gUIProxyObject.getValueEx( mapIndex );
			delete proxyObjectRef;
			gUIProxyObject.eraseNode( mapIndex );
		}
		DECLARE_END_PROTECTED
	}

	CNSLuaStack& CNSUIProxy::marshal( CNSLuaStack& luaStack ) const
	{
		gpLuaStack = &luaStack;
		// key 为第一个对象的名称，value为数据代理
		NSClient::gUILoadProc( mUIFile.getBuffer( ) );
		return luaStack;
	}

	const CNSLuaStack& CNSUIProxy::unmarshal( const CNSLuaStack& luaStack )
	{
		return luaStack;
	}

	void regLuaLib( )
	{
		NSBase::CNSLuaStack& luaStack = NSBase::CNSLuaStack::getLuaStack( );
		luaStack.regLib( "NosnUI", NSProxy::NosnUI );
	}

	static int proxyGetField( lua_State* lua )
	{
		DECLARE_BEGIN_PROTECTED
		CUIProxyComponent* proxyComp = NULL;
		luaStack >> proxyComp;

		static CNSString fieldName;
		luaStack >> fieldName;

		int dataType = EDataType::TYPE_NONE;
		void* buf = NULL;
		if ( NSClient::gUIGetValue( proxyComp->mProxyObject->mInstanceID, proxyComp->mCompName, fieldName, buf, dataType ) == false )
			NSException( NSClient::gUIGetLastError( ) );

		if ( dataType == EDataType::TYPE_BOOLEAN )
			luaStack << *(bool*) buf;
		else if ( dataType == EDataType::TYPE_CHAR )
			luaStack << *(char*) buf;
		else if ( dataType == EDataType::TYPE_UCHAR )
			luaStack << *(unsigned char*) buf;
		else if ( dataType == EDataType::TYPE_SHORT )
			luaStack << *(short*) buf;
		else if ( dataType == EDataType::TYPE_USHORT )
			luaStack << *(unsigned short*) buf;
		else if ( dataType == EDataType::TYPE_INT )
			luaStack << *(int*) buf;
		else if ( dataType == EDataType::TYPE_UINT )
			luaStack << *(unsigned int*) buf;
		else if ( dataType == EDataType::TYPE_INT64 )
			luaStack << ( double ) *(long long*) buf;
		else if ( dataType == EDataType::TYPE_UINT64 )
			luaStack << ( double ) *( unsigned long long* ) buf;
		else if ( dataType == EDataType::TYPE_FLOAT )
			luaStack << ( double ) *(float*) buf;
		else if ( dataType == EDataType::TYPE_DOUBLE )
			luaStack << ( double ) *(double*) buf;
		else if ( dataType == EDataType::TYPE_STRING )
		{
			static CNSString value;
			value = (char*) buf;
			luaStack << value;
		}
		DECLARE_END_PROTECTED
	}

	static int proxySetField( lua_State* lua )
	{
		DECLARE_BEGIN_PROTECTED
		CUIProxyComponent* proxyComp = NULL;
		luaStack >> proxyComp;

		static CNSString fieldName;
		luaStack >> fieldName;
		int type = lua_type( lua, 3 );
		if ( type == LUA_TNUMBER )
		{
			double value;
			luaStack >> value;
			if ( NSClient::gUISetValue( proxyComp->mProxyObject->mInstanceID, proxyComp->mCompName, fieldName, &value, EDataType::TYPE_DOUBLE ) == false )
				NSException( NSClient::gUIGetLastError( ) );
		}
		else if ( type == LUA_TBOOLEAN )
		{
			bool value;
			luaStack >> value;
			if ( NSClient::gUISetValue( proxyComp->mProxyObject->mInstanceID, proxyComp->mCompName, fieldName, &value, EDataType::TYPE_BOOLEAN ) == false )
				NSException( NSClient::gUIGetLastError( ) );
		}
		else if ( type == LUA_TSTRING )
		{
			static CNSString value;
			luaStack >> value;
			if ( NSClient::gUISetValue( proxyComp->mProxyObject->mInstanceID, proxyComp->mCompName, fieldName, (char*) value.getBuffer( ), EDataType::TYPE_STRING ) == false )
				NSException( NSClient::gUIGetLastError( ) );
		}
		else if ( type == LUA_TFUNCTION )
		{
			CNSLuaFunction func( __FUNCTION__ );
			luaStack >> func;
			if ( func.isValid( ) == false )
				NSException( _UTF8( "Lua函数[proxySetField]参数3 不是一个lua函数" ) )

			proxyComp->onEventFunc = func;
			if ( NSClient::gUISetValue( proxyComp->mProxyObject->mInstanceID, proxyComp->mCompName, fieldName, NULL, EDataType::TYPE_NONE ) == false )
				NSException( NSClient::gUIGetLastError( ) );
		}
		DECLARE_END_PROTECTED
	}

	static int proxyGetComponent( lua_State* lua )
	{
		DECLARE_BEGIN_PROTECTED
		CUIProxyObject* proxy = NULL;
		luaStack >> proxy;

		static CNSString compName;
		luaStack >> compName;

		if ( compName == "destroy" )
		{
			lua_pushcfunction( lua, destroy );
			return 1;
		}
		else if ( compName == "runAction" )
		{
			lua_pushcfunction( lua, runAction );
			return 1;
		}

		CUIProxyComponent** uiComponentRef = NULL;
		CUIProxyComponent* uiComponent = NULL;
		uiComponentRef = proxy->mUIProxyComp.get( compName );
		if ( uiComponentRef != NULL )
			uiComponent = *uiComponentRef;
		
		if ( uiComponent == NULL )
		{
			uiComponent = new CUIProxyComponent( compName, proxy );
			proxy->mUIProxyComp.insert( compName, uiComponent );
		}

		luaStack << uiComponent;
		DECLARE_END_PROTECTED
	}

	void CUIProxyObject::pushUserData( CNSLuaStack& luaStack, CUIProxyObject* ref )
	{
		luaStack.setIndexFunc( NSProxy::proxyGetComponent );
		static CNSVector< const luaL_Reg* > reg;
		luaStack.pushNSWeakRef( ref, "nsUIProxy", reg );
	}

	void CUIProxyObject::popUserData( const CNSLuaStack& luaStack, CUIProxyObject*& ref )
	{
		ref = (CUIProxyObject*) luaStack.popNSWeakRef( "nsUIProxy" );
	}

	void CUIProxyComponent::pushUserData( CNSLuaStack& luaStack, CUIProxyComponent* ref )
	{
		luaStack.setIndexFunc( proxyGetField );
		luaStack.setNewIndexFunc( proxySetField );
		static CNSVector< const luaL_Reg* > reg;
		luaStack.pushNSWeakRef( ref, "nsUIProxyComponent", reg );
	}

	void CUIProxyComponent::popUserData( const CNSLuaStack& luaStack, CUIProxyComponent*& ref )
	{
		ref = (CUIProxyComponent*) luaStack.popNSWeakRef( "nsUIProxyComponent" );
	}
}

void nsCreateUIProxy( int instanceID, const char* objName )
{
	CNSLuaStack& luaStack = *NSProxy::gpLuaStack;

	NSProxy::CUIProxyObject* uiProxyObject = new NSProxy::CUIProxyObject( instanceID, objName );
	luaStack.pushField( objName, uiProxyObject );

	NSProxy::gUIProxyObject.insert( instanceID, uiProxyObject );
}

void nsSetUILoadProc( FUILoadLayout loadProc )
{
	NSClient::gUILoadProc = loadProc;
}

void nsSetUIDestroyProc( FUIDestroy destroyProc )
{
	NSClient::gUIDestroyProc = destroyProc;
}

void nsSetUISetProc( FUISetValue setProc )
{
	NSClient::gUISetValue = setProc;
}

void nsSetUIGetProc( FUIGetValue getProc )
{
	NSClient::gUIGetValue = getProc;
}

void nsSetUIGetLastError( FUIGetLastError lastErrorProc )
{
	NSClient::gUIGetLastError = lastErrorProc;
}

void nsUIFireEvent( int instanceID, const char* compName )
{
	try
	{
		NSProxy::CUIProxyObject** proxyObjectRef = NSProxy::gUIProxyObject.get( instanceID );
		if ( proxyObjectRef == NULL )
			return;

		NSProxy::CUIProxyObject* uiProxyObject = *proxyObjectRef;
		NSProxy::CUIProxyComponent** nsCompRef = uiProxyObject->mUIProxyComp.get( compName );
		if ( nsCompRef == NULL )
			return;

		NSProxy::CUIProxyComponent* nsComp = *nsCompRef;
		CNSLuaStack& luaStack = CNSLuaStack::getLuaStack( );
		if ( nsComp->onEventFunc.isValid( ) == false )
			return;

		luaStack.preCall( nsComp->onEventFunc );
		luaStack << uiProxyObject;
		luaStack.call( );
	}
	catch ( CNSException& e )
	{
		NSLog::exception( _UTF8( "函数[nsUIFireEvent]发生异常\n错误描述: \n\t%s\nC++调用堆栈: \n%s" ), e.mErrorDesc, NSBase::NSFunction::getStackInfo( ).getBuffer( ) );
	}
}