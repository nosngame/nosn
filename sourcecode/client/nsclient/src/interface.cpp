#include "precomplie.h"

namespace NSClient
{
	CNSLuaStack* gpLuaStack = NULL;
	CNSLuaFunction mTouchProc;
	static int createNSClient( lua_State* lua );
	static int login( lua_State* lua );
	static int send( lua_State* lua );
	static int send2Oper( lua_State* lua );
	static int getAuthName( lua_State* lua );
	static int loadUILayout( lua_State* lua );
	static int loadObject( lua_State* lua );
	static int touch( lua_State* lua );

	BEGIN_EXPORT( Nosn )
		EXPORT_FUNC( createNSClient )
		EXPORT_FUNC( login )
		EXPORT_FUNC( send )
		EXPORT_FUNC( send2Oper )
		EXPORT_FUNC( getAuthName )
		EXPORT_FUNC( loadUILayout )
		EXPORT_FUNC( loadObject )
		EXPORT_FUNC( touch )
	END_EXPORT

	void regLuaLib( )
	{
		NSBase::CNSLuaStack& luaStack = NSBase::CNSLuaStack::getLuaStack( );
		luaStack.regLib( "Nosn", NSClient::Nosn );

		luaStack.newTable( );
		luaStack.pushField( "LOGIN_SUCCESS", NSClientProto::CProtocolLoginResult::LOGIN_SUCCESS );
		luaStack.pushField( "LOGIN_PSWERROR", NSClientProto::CProtocolLoginResult::LOGIN_PSWERROR );
		luaStack.pushField( "LOGIN_TOKENINVALID", NSClientProto::CProtocolLoginResult::LOGIN_TOKENINVALID );
		luaStack.setGlobalTable( "loginResult" );
		NSLog::log( _UTF8( "注册lua函数成功" ) );
	}

	static int createNSClient( lua_State* lua )
	{
		DECLARE_BEGIN_PROTECTED
		CNSClient* client = CNSClient::createNSClient( );
		int clientID = client->getClientID( );
		luaStack << clientID;
		DECLARE_END_PROTECTED
	}

	static int login( lua_State* lua )
	{
		DECLARE_BEGIN_PROTECTED
		int clientID = 0;
		static CNSString address;
		static CNSString authName;
		static CNSString userName;
		static CNSString token;
		luaStack >> clientID;
		luaStack >> address;
		luaStack >> authName;
		luaStack >> userName;
		luaStack >> token;
		CNSClient* client = CNSClient::getNSClient( clientID );
		if ( client == NULL )
		{
			static CNSString errorDesc;
			errorDesc.format( _UTF8( "ClientID - %d, 没有找到客户端对象" ), clientID );
			NSException( errorDesc );
		}

		client->login( address, authName, userName, token );
		DECLARE_END_PROTECTED
	}

	static int send( lua_State* lua )
	{
		DECLARE_BEGIN_PROTECTED
		int clientID = 0;
		luaStack >> clientID;

		NSClientProto::CProtocolScript scriptProto( &luaStack );
		CNSClient* client = CNSClient::getNSClient( clientID );
		if ( client == NULL )
		{
			static CNSString errorDesc;
			errorDesc.format( _UTF8( "ClientID - %d, 没有找到客户端对象" ), clientID );
			NSException( errorDesc );
		}

		client->send( &scriptProto );
		DECLARE_END_PROTECTED
	}

	static int send2Oper( lua_State* lua )
	{
		DECLARE_BEGIN_PROTECTED
		int clientID = 0;
		luaStack >> clientID;

		NSClientProto::CProtocolScript scriptProto( &luaStack );
		CNSClient* client = CNSClient::getNSClient( clientID );
		if ( client == NULL )
		{
			static CNSString errorDesc;
			errorDesc.format( _UTF8( "ClientID - %d, 没有找到客户端对象" ), clientID );
			NSException( errorDesc );
		}

		client->send2Oper( &scriptProto );
		DECLARE_END_PROTECTED
	}

	static int getAuthName( lua_State* lua )
	{
		DECLARE_BEGIN_PROTECTED
		luaStack << NSClient::CNSClient::sAuthName;
		DECLARE_END_PROTECTED
	}
	static int loadUILayout( lua_State* lua )
	{
		DECLARE_BEGIN_PROTECTED
		CNSString uiFile;
		luaStack >> uiFile;

		int parentID = 0;
		luaStack >> parentID;

		NSProxy::CNSGoProxy uiProxy( uiFile, parentID, 0 );
		luaStack << uiProxy;
		DECLARE_END_PROTECTED
	}

	static int loadObject( lua_State* lua )
	{
		DECLARE_BEGIN_PROTECTED
		CNSString goFile;
		luaStack >> goFile;

		int parentID = 0;
		luaStack >> parentID;

		NSProxy::CNSGoProxy goProxy( goFile, parentID, 1 );
		luaStack << goProxy;
		DECLARE_END_PROTECTED
	}

	static int touch( lua_State* lua )
	{
		DECLARE_BEGIN_PROTECTED
		CNSVector< CNSString > bundles;
		luaStack >> bundles;

		bool permanent = false;
		luaStack >> permanent;

		CNSLuaFunction loadFunc( __FUNCTION__ );
		luaStack >> loadFunc;
		if ( loadFunc.isValid( ) == false )
			NSException( _UTF8( "函数[touch]参数2 不是一个lua函数" ) )

			mTouchProc = loadFunc;
		for ( unsigned int i = 0; i < bundles.getCount( ); i ++ )
			NSClient::gTouchProc( bundles[ i ], permanent, false );

		NSClient::gTouchProc( "", permanent, true );
		DECLARE_END_PROTECTED
	}
}

void nsTouchNotify( bool isDone, float progress )
{
	CNSLuaStack& luaStack = CNSLuaStack::getLuaStack( );
	if ( NSClient::mTouchProc.isValid( ) == false )
		return;

	luaStack.preCall( NSClient::mTouchProc );
	luaStack << isDone;
	luaStack << progress;
	luaStack.call( );
	if ( isDone == true )
		luaStack.clearFunc( NSClient::mTouchProc );
}