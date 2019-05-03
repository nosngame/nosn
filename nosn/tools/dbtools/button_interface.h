#pragma once
static int button_setText( lua_State* lua );

BEGIN_EXPORT( Button )
	EXPORT_FUNC_EX( "setText",		button_setText )
END_EXPORT

static int button_setText( lua_State* lua )
{
	DECLARE_BEGIN_PROTECTED
	HWND wnd			= lua_tohwnd( lua, 1, WC_BUTTON );
	const char* text	= luaL_checkstring( lua, 3 );

	SetWindowText( wnd, CNSString::ToTChar( text ) );
	DECLARE_END_PROTECTED
	return 0;
}
