#include <nsbase.h>
#include <protocol.h>
#include "gateclient.h"
#include "server.h"
#include "interface.h"

namespace OperServer
{
	static int getServerList( lua_State* lua );
	static int reloadServerList( lua_State* lua );
	static int send2OperClient( lua_State* lua );
	static int send2User( lua_State* lua );
	static int enableIPOpen( lua_State* lua );
	static int verifyResult( lua_State* lua );

	BEGIN_EXPORT( Nosn )
		EXPORT_FUNC( getServerList )
		EXPORT_FUNC( reloadServerList )
		EXPORT_FUNC( send2OperClient )
		EXPORT_FUNC( send2User )
		EXPORT_FUNC( enableIPOpen )
		EXPORT_FUNC( verifyResult )
	END_EXPORT

	void regLuaLib( )
	{
		NSBase::CNSLuaStack& luaStack = NSBase::CNSLuaStack::getLuaStack( );
		luaStack.regLib( "NSOper", OperServer::Nosn );

		luaStack.newTable( );
		luaStack.pushField( "LOGIN_SUCCESS", NSClientProto::CProtocolLoginResult::LOGIN_SUCCESS );
		luaStack.pushField( "LOGIN_PSWERROR", NSClientProto::CProtocolLoginResult::LOGIN_PSWERROR );
		luaStack.pushField( "LOGIN_TOKENINVALID", NSClientProto::CProtocolLoginResult::LOGIN_TOKENINVALID );
		luaStack.setGlobalTable( "loginResult" );
		NSLog::log( _UTF8( "注册lua函数成功" ) );
	}

	static int getServerList( lua_State* lua )
	{
		DECLARE_BEGIN_PROTECTED
		COperLogic*	logic = NSWin32::CConsoleApp< COperLogic >::getLogic( );
		luaStack << logic->mRegions;
		DECLARE_END_PROTECTED
	}

	static int reloadServerList( lua_State* lua )
	{
		DECLARE_BEGIN_PROTECTED
		OperServer::COperLogic*	logic = NSWin32::CConsoleApp< OperServer::COperLogic >::getLogic( );
		logic->reloadServerList( );
		DECLARE_END_PROTECTED
	}

	static int send2OperClient( lua_State* lua )
	{
		DECLARE_BEGIN_PROTECTED
		TSessionID sessionID = 0;
		luaStack >> sessionID;
		OperServer::COperLogic*	logic = NSWin32::CConsoleApp< OperServer::COperLogic >::getLogic( );

		NSOperProto::CProtocolScript scriptProto( &luaStack );
		logic->send2OperClient( sessionID, &scriptProto );
		DECLARE_END_PROTECTED
	}

	static int send2User( lua_State* lua )
	{
		DECLARE_BEGIN_PROTECTED
		long long userID = 0L;
		luaStack >> userID;
		OperServer::COperLogic*	logic = NSWin32::CConsoleApp< OperServer::COperLogic >::getLogic( );

		NSClientProto::CProtocolScript scriptProto( &luaStack );
		logic->send2User( userID, &scriptProto );
		DECLARE_END_PROTECTED
	}

	static int enableIPOpen( lua_State* lua )
	{
		DECLARE_BEGIN_PROTECTED
		bool enable = false;
		luaStack >> enable;
		OperServer::COperLogic*	logic = NSWin32::CConsoleApp< OperServer::COperLogic >::getLogic( );
		logic->enableIPOpen( enable );
		DECLARE_END_PROTECTED
	}

	static int verifyResult( lua_State* lua )
	{
		DECLARE_BEGIN_PROTECTED
		static CNSString gateName;
		TSessionID sessionID = 0;
		long long nosnUserID = 0;
		unsigned int loginResult = 0;
		luaStack >> gateName;
		luaStack >> sessionID;
		luaStack >> nosnUserID;
		luaStack >> loginResult;
		OperServer::COperLogic*	logic = NSWin32::CConsoleApp< OperServer::COperLogic >::getLogic( );
		logic->verifyResult( gateName, sessionID, nosnUserID, ( NSClientProto::CProtocolLoginResult::ELoginResult ) loginResult );
		DECLARE_END_PROTECTED
	}
}
