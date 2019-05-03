#pragma once

namespace NSWin32
{
	static int setBkColor( lua_State* lua );
	static int setTextColor( lua_State* lua );

	static const struct luaL_Reg NSVsStatic[] =
	{
		{ "setBkColor", setBkColor },
		{ "setTextColor", setTextColor },
		{ NULL, NULL },
	};

	static int setBkColor( lua_State* lua )
	{
		DECLARE_BEGIN_PROTECTED
		DECLARE_END_PROTECTED
	}

	static int setTextColor( lua_State* lua )
	{
		DECLARE_BEGIN_PROTECTED
		const char* windowID = luaL_checkstring( lua, 1 );
		const char* text = luaL_checkstring( lua, 2 );
		NSWin32::CNSWindow* window = NSWin32::CNSWindow::getWindow( windowID );
		if (window == NULL)
		{
			static CNSString errorDesc;
			errorDesc.format( _UTF8( "Lua函数[setText]指定窗口不存在, windowID - %s" ), windowID );
			NSException( errorDesc )
		}

		window->setText( text );
		DECLARE_END_PROTECTED
	}
}