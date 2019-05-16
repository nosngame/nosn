#pragma once

namespace NSWin32
{
	static int setBNClickedEvent( lua_State* lua );
	static const struct luaL_Reg NSVsBtn[] =
	{
		{ "setBNClickedEvent", setBNClickedEvent },
		{ NULL, NULL },
	};

	static bool onNSLuaBNClicked( NSWin32::CNSWindow* child, WPARAM wParam, LPARAM lParam, LRESULT* result )
	{
		const CNSLuaFunction& func = child->getLuaEventHandler( NSWin32::CNSVsBtn::EventClicked );
		NSBase::CNSLuaStack& luaStack = NSBase::CNSLuaStack::getLuaStack( );
		try
		{
			luaStack.preCall( func );
			bool retValue = true;
			luaStack.call<bool>( retValue );
			return retValue;
		}
		catch ( CNSException& e )
		{
			NSException( _UTF8( "函数[setBNClickedEvent] handler没有返回boolean值" ) );
		}

		return true;
	}

	static int setBNClickedEvent( lua_State* lua )
	{
		DECLARE_BEGIN_PROTECTED
		NSWin32::CNSWindow* window = NULL;
		CNSLuaFunction func( __FUNCTION__ );

		luaStack >> window;
		luaStack >> func;
		if ( func.isValid( ) == false )
			NSException( _UTF8( "Lua函数[setBNClickedEvent]参数1 不是一个lua函数" ) )
			
		window->setLuaEventHandler( NSWin32::CNSVsBtn::EventClicked, func );
		window->registerEvent( NSWin32::CNSVsBtn::EventClicked, onNSLuaBNClicked );
		DECLARE_END_PROTECTED
	}
}