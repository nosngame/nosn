#include <nsbase.h>
#include "protocol.h"
#include "gateclient.h"
#include "interface.h"
#include "server.h"

namespace LogicServer
{
	static int GetWorkPath( lua_State* lua );
	static int GetOperAddress( lua_State* lua );
	static int getDBServer( lua_State* lua );
	static int send2LogicServer( lua_State* lua );
	static int send2LogicClient( lua_State* lua );
	static int send2OperServer( lua_State* lua );
	static int send2ChargeServer( lua_State* lua );
	static int send2User( lua_State* lua );

	BEGIN_EXPORT( Nosn )
		EXPORT_FUNC( GetWorkPath )
		EXPORT_FUNC( GetOperAddress )
		EXPORT_FUNC( getDBServer )
		EXPORT_FUNC( send2LogicServer )
		EXPORT_FUNC( send2LogicClient )
		EXPORT_FUNC( send2OperServer )
		EXPORT_FUNC( send2ChargeServer )
		EXPORT_FUNC( send2User )
	END_EXPORT

	void regLuaLib( )
	{
		NSBase::CNSLuaStack& luaStack = NSBase::CNSLuaStack::getLuaStack( );
		lua_State* lua = luaStack.getLuaState( );

		luaL_newlib( lua, LogicServer::Nosn );
		lua_setglobal( lua, "NSLogic" );
		NSLog::log( _UTF8( "注册lua函数成功" ) );
	}
	
	static int GetWorkPath( lua_State* lua )
	{
		DECLARE_BEGIN_PROTECTED
		CLogic* logic = (CLogic*) NSWin32::CConsoleApp< CLogic >::getLogic( );
		luaStack << logic->mWorkPath;
		DECLARE_END_PROTECTED
	}

	static int GetOperAddress( lua_State* lua )
	{
		DECLARE_BEGIN_PROTECTED
		CLogic* logic = (CLogic*) NSWin32::CConsoleApp< CLogic >::getLogic( );
		luaStack << logic->mpOperClient->mAddress;
		DECLARE_END_PROTECTED
	}

	static int getDBServer( lua_State* lua )
	{
		DECLARE_BEGIN_PROTECTED
		CLogic* logic = (CLogic*) NSWin32::CConsoleApp< CLogic >::getLogic( );
		luaStack << logic->mDBConn;
		DECLARE_END_PROTECTED
	}

	static int send2LogicServer( lua_State* lua )
	{
		DECLARE_BEGIN_PROTECTED
		static CNSString name;
		luaStack >> name;
		CLogic* logic = (CLogic*) NSWin32::CConsoleApp< CLogic >::getLogic( );

		static CNSString netName;
		netName = name;
		NSLogicProto::CProtocolScript scriptProto( &luaStack );
		logic->send2LogicServer( netName, &scriptProto );
		DECLARE_END_PROTECTED
	}

	static int send2LogicClient(lua_State* lua)
	{
		DECLARE_BEGIN_PROTECTED
		static CNSString name;
		TSessionID sessionID = 0;
		luaStack >> name;
		luaStack >> sessionID;
		CLogic* logic = (CLogic*) NSWin32::CConsoleApp< CLogic >::getLogic( );

		static CNSString netName;
		netName = name;
	
		NSLogicProto::CProtocolScript scriptProto( &luaStack );
		logic->send2LogicClient( netName, sessionID, &scriptProto );
		DECLARE_END_PROTECTED
	}
	
	static int send2OperServer(lua_State* lua)
	{
		DECLARE_BEGIN_PROTECTED
		CLogic* logic = (CLogic*) NSWin32::CConsoleApp< CLogic >::getLogic( );

		NSOperProto::CProtocolScript scriptProto( &luaStack );
		logic->send2OperServer( &scriptProto );
		DECLARE_END_PROTECTED
	}
	
	static int send2ChargeServer(lua_State* lua)
	{
		DECLARE_BEGIN_PROTECTED
		CLogic* logic = (CLogic*) NSWin32::CConsoleApp< CLogic >::getLogic( );

		NSChargeProto::CProtocolScript scriptProto( &luaStack );
		logic->send2ChargeServer( &scriptProto );
		DECLARE_END_PROTECTED
	}

	static int send2User( lua_State* lua )
	{
		DECLARE_BEGIN_PROTECTED
		long long nosnUserID = 0;
		luaStack >> nosnUserID;
		LogicServer::CLogic* logic = NSWin32::CConsoleApp< LogicServer::CLogic >::getLogic( );

		NSClientProto::CProtocolScript scriptProto( &luaStack );
		logic->send2User( nosnUserID, &scriptProto );
		DECLARE_END_PROTECTED
	}
}