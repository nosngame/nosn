#pragma once
static int static_setText( lua_State* lua );

BEGIN_EXPORT( Static )
	EXPORT_FUNC_EX( "setText",		static_setText )
END_EXPORT

static int static_setText( lua_State* lua )
{
	DECLARE_BEGIN_PROTECTED
	HWND wnd			= lua_tohwnd( lua, 1, WC_STATIC );
	const char* text	= luaL_checkstring( lua, 3 );

	SetWindowText( wnd, CNSString::ToTChar( text ) );
	DECLARE_END_PROTECTED
	return 0;
}
