#include <nsbase.h>
#include "protocol.h"
#include "interface.h"
#include "server.h"

namespace ChargeServer
{
	static int getServerList( lua_State* lua );
	static int reloadServerList( lua_State* lua );
	static int send2ChargeClient( lua_State* lua );

	BEGIN_EXPORT( Nosn )
		EXPORT_FUNC( getServerList )
		EXPORT_FUNC( reloadServerList )
		EXPORT_FUNC( send2ChargeClient )
	END_EXPORT

	void regLuaLib( )
	{
		NSBase::CNSLuaStack& luaStack = NSBase::CNSLuaStack::getLuaStack( );
		luaStack.regLib( "NSCharge", ChargeServer::Nosn );
		NSLog::log( _UTF8( "注册lua函数成功" ) );
	}

	static int getServerList( lua_State* lua )
	{
		DECLARE_BEGIN_PROTECTED
		CChargeLogic* logic = NSWin32::CConsoleApp< CChargeLogic >::getLogic( );
		luaStack << logic->mRegions;
		DECLARE_END_PROTECTED
	}

	static int reloadServerList( lua_State* lua )
	{
		DECLARE_BEGIN_PROTECTED
		NSWin32::CConsoleApp< CChargeLogic >::getLogic( )->reloadServerList( );
		DECLARE_END_PROTECTED
	}

	static int send2ChargeClient( lua_State* lua )
	{
		DECLARE_BEGIN_PROTECTED
		TSessionID sessionID = 0;
		luaStack >> sessionID;
		CChargeLogic* logic = (CChargeLogic*) NSWin32::CConsoleApp< CChargeLogic >::getLogic( );

		NSChargeProto::CProtocolScript scriptProto( &luaStack );
		logic->send2ChargeClient( sessionID, &scriptProto );
		DECLARE_END_PROTECTED
	}
}