#include "precomplie.h"

namespace NSClient
{
	static int createNSClient( lua_State* lua );
	static int login( lua_State* lua );
	static int send( lua_State* lua );
	static int send2Oper( lua_State* lua );
	static int getAuthName( lua_State* lua );

	BEGIN_EXPORT( Nosn )
		EXPORT_FUNC( createNSClient )
		EXPORT_FUNC( login )
		EXPORT_FUNC( send )
		EXPORT_FUNC( send2Oper )
		EXPORT_FUNC( getAuthName )
	END_EXPORT

	void regLuaLib( )
	{
		NSBase::CNSLuaStack& luaStack = NSBase::CNSLuaStack::getLuaStack( );
		luaStack.regLib( "NSClient", NSClient::Nosn );

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
}
