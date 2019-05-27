#include "precomplie.h"
namespace NSProxy
{
	CNSMap< int, CGoProxyObject* > gGoProxyObject;
	CNSLuaStack& CNSGoProxy::marshal( CNSLuaStack& luaStack ) const
	{
		NSClient::gpLuaStack = &luaStack;
		// key 为第一个对象的名称，value为数据代理
		if ( NSClient::gGoLoadProc( mFileName.getBuffer( ), mParentID, mType ) == false )
			NSException( NSClient::gGoGetLastError( ) );

		return luaStack;
	}

	const CNSLuaStack& CNSGoProxy::unmarshal( const CNSLuaStack& luaStack )
	{
		return luaStack;
	}

	static int invokeMethod( lua_State* lua )
	{
		DECLARE_BEGIN_PROTECTED
		CGoProxyComponent* proxyComp = NULL;
		luaStack >> proxyComp;
		if ( proxyComp == NULL )
			NSException( _UTF8( "组件方法调用错误" ) );

		CNSOctetsStream stream;
		while ( luaStack.isEnd( ) == false )
			luaStack >> stream;

		int dataType = EDataType::TYPE_NONE;
			void* buf = NULL;
		if ( NSClient::gGoInvoke( proxyComp->mProxyObject->mType, proxyComp->mProxyObject->mInstanceID, proxyComp->mCompName, stream.begin( ), stream.length( ), buf, dataType ) == false )
			NSException( NSClient::gGoGetLastError( ) );

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

	static int destroy( lua_State* lua )
	{
		DECLARE_BEGIN_PROTECTED
		CGoProxyObject* proxy = NULL;
		luaStack >> proxy;
		if ( proxy == NULL )
			NSException( _UTF8( "destroy方法调用错误" ) );

		// 销毁unity对象
		if ( NSClient::gGoDestroyProc( proxy->mInstanceID ) == false )
			NSException( NSClient::gGoGetLastError( ) );

		// 销毁组件的lua事件函数
		HLISTINDEX beginIndex = proxy->mUIProxyComp.getHead( );
		for ( ; beginIndex != NULL; proxy->mUIProxyComp.getNext( beginIndex ) )
		{
			NSProxy::CGoProxyComponent* proxyComp = proxy->mUIProxyComp.getValue( beginIndex );
			if ( proxyComp->onEventFunc.isValid( ) == true )
				luaStack.clearFunc( proxyComp->onEventFunc );
			
			proxyComp->cleanUpRef( );
			delete proxyComp;
		}

		// 清理所有引用
		proxy->cleanUpRef( );

		// 删除对象
		HMAPINDEX mapIndex = gGoProxyObject.findNode( proxy->mInstanceID );
		if ( mapIndex != NULL )
		{
			NSProxy::CGoProxyObject* proxyObjectRef = gGoProxyObject.getValueEx( mapIndex );
			delete proxyObjectRef;
			gGoProxyObject.eraseNode( mapIndex );
		}
		DECLARE_END_PROTECTED
	}

	static int proxyGetField( lua_State* lua )
	{
		DECLARE_BEGIN_PROTECTED
		CGoProxyComponent* proxyComp = NULL;
		luaStack >> proxyComp;

		static CNSString fieldName;
		luaStack >> fieldName;

		bool isMethod = false;
		if ( NSClient::gGoQueryMethod( proxyComp->mProxyObject->mType, proxyComp->mProxyObject->mInstanceID, proxyComp->mCompName, fieldName, isMethod ) == false )
			NSException( NSClient::gGoGetLastError( ) );

		if ( isMethod == true )
		{
			lua_pushcfunction( lua, invokeMethod );
			return 1;
		}

		int dataType = EDataType::TYPE_NONE;
		void* buf = NULL;
		if ( NSClient::gGoGetValue( proxyComp->mProxyObject->mType, proxyComp->mProxyObject->mInstanceID, proxyComp->mCompName, fieldName, buf, dataType ) == false )
			NSException( NSClient::gGoGetLastError( ) );

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
		CGoProxyComponent* proxyComp = NULL;
		luaStack >> proxyComp;

		static CNSString fieldName;
		luaStack >> fieldName;

		int type = lua_type( lua, 3 );
		if ( type == LUA_TNUMBER )
		{
			double value;
			luaStack >> value;
			if ( NSClient::gGoSetValue( proxyComp->mProxyObject->mType, proxyComp->mProxyObject->mInstanceID, proxyComp->mCompName, fieldName, &value, EDataType::TYPE_DOUBLE ) == false )
				NSException( NSClient::gGoGetLastError( ) );
		}
		else if ( type == LUA_TBOOLEAN )
		{
			bool value;
			luaStack >> value;
			if ( NSClient::gGoSetValue( proxyComp->mProxyObject->mType, proxyComp->mProxyObject->mInstanceID, proxyComp->mCompName, fieldName, &value, EDataType::TYPE_BOOLEAN ) == false )
				NSException( NSClient::gGoGetLastError( ) );
		}
		else if ( type == LUA_TSTRING )
		{
			static CNSString value;
			luaStack >> value;
			if ( NSClient::gGoSetValue( proxyComp->mProxyObject->mType, proxyComp->mProxyObject->mInstanceID, proxyComp->mCompName, fieldName, (char*) value.getBuffer( ), EDataType::TYPE_STRING ) == false )
				NSException( NSClient::gGoGetLastError( ) );
		}
		else if ( type == LUA_TFUNCTION )
		{
			CNSLuaFunction func( __FUNCTION__ );
			luaStack >> func;
			if ( func.isValid( ) == false )
				NSException( _UTF8( "函数[proxySetField]参数3 不是一个lua函数" ) )

			proxyComp->onEventFunc = func;
			if ( NSClient::gGoSetValue( proxyComp->mProxyObject->mType, proxyComp->mProxyObject->mInstanceID, proxyComp->mCompName, fieldName, NULL, EDataType::TYPE_NONE ) == false )
				NSException( NSClient::gGoGetLastError( ) );
		}
		DECLARE_END_PROTECTED
	}

	static int proxyGetComponent( lua_State* lua )
	{
		DECLARE_BEGIN_PROTECTED
		CGoProxyObject* proxy = NULL;
		luaStack >> proxy;

		static CNSString compName;
		luaStack >> compName;

		// 对象方法写在这里
		if ( compName == "destroy" )
		{
			lua_pushcfunction( lua, destroy );
			return 1;
		}

		if ( compName == "id" )
		{
			lua_pushinteger( lua, proxy->mInstanceID );
			return 1;
		}

		if ( compName == "layer" )
		{
			const char* layer = NULL;
			if ( NSClient::gGoGetLayer( proxy->mType, proxy->mInstanceID, layer ) == false )
				NSException( NSClient::gGoGetLastError( ) );

			static CNSString value;
			value = (char*) layer;
			luaStack << value;
			return 1;
		}

		if ( compName == "tag" )
		{
			const char* tag = NULL;
			if ( NSClient::gGoGetTag( proxy->mType, proxy->mInstanceID, tag ) == false )
				NSException( NSClient::gGoGetLastError( ) );

			static CNSString value;
			value = (char*) tag;
			luaStack << value;
			return 1;
		}

		CGoProxyComponent** goCompRef = NULL;
		CGoProxyComponent* goComp = NULL;
		goCompRef = proxy->mUIProxyComp.get( compName );
		if ( goCompRef != NULL )
			goComp = *goCompRef;
		
		// 如果组件不存在，代理层添加
		// 脚本不能主动添加组件，是不是考虑加一个对象方法添加组件？
		if ( goComp == NULL )
		{
			goComp = new CGoProxyComponent( compName, proxy );
			proxy->mUIProxyComp.insert( compName, goComp );
		}

		luaStack << goComp;
		DECLARE_END_PROTECTED
	}

	static int proxySetComponent( lua_State* lua )
	{
		DECLARE_BEGIN_PROTECTED
		CGoProxyObject* proxy = NULL;
		luaStack >> proxy;

		static CNSString compName;
		luaStack >> compName;

		if ( compName == "layer" )
		{
			static CNSString layerName;
			luaStack >> layerName;

			if ( NSClient::gGoSetLayer( proxy->mType, proxy->mInstanceID, layerName.getBuffer( ) ) == false )
				NSException( NSClient::gGoGetLastError( ) );

			return 0;
		}

		if ( compName == "tag" )
		{
			static CNSString tagName;
			luaStack >> tagName;

			if ( NSClient::gGoSetTag( proxy->mType, proxy->mInstanceID, tagName.getBuffer( ) ) == false )
				NSException( NSClient::gGoGetLastError( ) );

			return 0;
		}

		DECLARE_END_PROTECTED
	}

	void CGoProxyObject::pushUserData( CNSLuaStack& luaStack, CGoProxyObject* ref )
	{
		luaStack.setIndexFunc( NSProxy::proxyGetComponent );
		luaStack.setNewIndexFunc( NSProxy::proxySetComponent );
		static CNSVector< const luaL_Reg* > reg;
		luaStack.pushNSWeakRef( ref, "nsGoProxy", reg );
	}

	void CGoProxyObject::popUserData( const CNSLuaStack& luaStack, CGoProxyObject*& ref )
	{
		ref = (CGoProxyObject*) luaStack.popNSWeakRef( "nsGoProxy" );
	}

	void CGoProxyComponent::pushUserData( CNSLuaStack& luaStack, CGoProxyComponent* ref )
	{
		luaStack.setIndexFunc( proxyGetField );
		luaStack.setNewIndexFunc( proxySetField );
		static CNSVector< const luaL_Reg* > reg;
		luaStack.pushNSWeakRef( ref, "nsGoProxyComponent", reg );
	}

	void CGoProxyComponent::popUserData( const CNSLuaStack& luaStack, CGoProxyComponent*& ref )
	{
		ref = (CGoProxyComponent*) luaStack.popNSWeakRef( "nsGoProxyComponent" );
	}
}

void nsCreateGoProxy( int type, int instanceID, const char* objName )
{
	CNSLuaStack& luaStack = *NSClient::gpLuaStack;

	NSProxy::CGoProxyObject* goProxyObject = new NSProxy::CGoProxyObject( type, instanceID, objName );
	luaStack.pushField( objName, goProxyObject );

	NSProxy::gGoProxyObject.insert( instanceID, goProxyObject );
}

void nsSetGoLoadProc( FGoLoadProc loadProc )
{
	NSClient::gGoLoadProc = loadProc;
}

void nsSetGoDestroyProc( FGoDestroyProc destroyProc )
{
	NSClient::gGoDestroyProc = destroyProc;
}

void nsSetGoSetProc( FGoSetValue setProc )
{
	NSClient::gGoSetValue = setProc;
}

void nsSetGoGetProc( FGoGetValue getProc )
{
	NSClient::gGoGetValue = getProc;
}

void nsSetGoGetLayer( FGoGetLayer getLayer )
{
	NSClient::gGoGetLayer = getLayer;
}

void nsSetGoGetTag( FGoGetTag getTag )
{
	NSClient::gGoGetTag = getTag;
}

void nsSetGoSetLayer( FGoSetLayer setLayer )
{
	NSClient::gGoSetLayer = setLayer;
}

void nsSetGoSetTag( FGoSetTag setTag )
{
	NSClient::gGoSetTag = setTag;
}
void nsSetGoQueryMethod( FGoQueryMethod queryMethod )
{
	NSClient::gGoQueryMethod = queryMethod;
}

void nsSetGoInvoke( FGoInvoke invoke )
{
	NSClient::gGoInvoke = invoke;
}

void nsSetGoGetLastError( FGoGetLastError lastErrorProc )
{
	NSClient::gGoGetLastError = lastErrorProc;
}

void nsGoFireEvent( int instanceID, const char* compName )
{
	try
	{
		NSProxy::CGoProxyObject** proxyObjectRef = NSProxy::gGoProxyObject.get( instanceID );
		if ( proxyObjectRef == NULL )
			return;

		NSProxy::CGoProxyObject* uiProxyObject = *proxyObjectRef;
		NSProxy::CGoProxyComponent** nsCompRef = uiProxyObject->mUIProxyComp.get( compName );
		if ( nsCompRef == NULL )
			return;

		NSProxy::CGoProxyComponent* nsComp = *nsCompRef;
		CNSLuaStack& luaStack = CNSLuaStack::getLuaStack( );
		if ( nsComp->onEventFunc.isValid( ) == false )
			return;

		luaStack.preCall( nsComp->onEventFunc );
		luaStack << uiProxyObject;
		luaStack.call( );
	}
	catch ( CNSException& e )
	{
		NSLog::exception( _UTF8( "函数[nsGoFireEvent]发生异常\n错误描述: \n\t%s\nC++调用堆栈: \n%s" ), e.mErrorDesc, NSBase::NSFunction::getStackInfo( ).getBuffer( ) );
	}
}

