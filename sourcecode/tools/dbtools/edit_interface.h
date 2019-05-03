#pragma once
static int edit_getText( lua_State* lua );
static int edit_setText( lua_State* lua );
static int edit_addStyle( lua_State* lua );
static int edit_removeStyle( lua_State* lua );

BEGIN_EXPORT( Edit )
	EXPORT_FUNC_EX( "getText",		edit_getText )
	EXPORT_FUNC_EX( "setText",		edit_setText )
	EXPORT_FUNC_EX( "addStyle",		edit_addStyle )
	EXPORT_FUNC_EX( "removeStyle",	edit_removeStyle )
END_EXPORT

static int edit_removeStyle( lua_State* lua )
{
	DECLARE_BEGIN_PROTECTED
	HWND edit					= lua_tohwnd( lua, 1, WC_EDIT );
	unsigned long long style	= (unsigned long long) luaL_checkunsigned( lua, 3 );
	LONG_PTR curStyle			= GetWindowLongPtr( edit, GWL_STYLE );

	if ( SetWindowLongPtr( edit, GWL_STYLE, curStyle & ~style ) == 0 )
	{
		static CNSString errorDesc;
		errorDesc.Format( _UTF8( "函数[SetWindowLongPtr]函数调用失败，错误码:%d" ), GetLastError( ) );
		FBException( errorDesc );
	}

	DECLARE_END_PROTECTED
	return 0;
}

static int edit_addStyle( lua_State* lua )
{
	DECLARE_BEGIN_PROTECTED
	HWND edit				= lua_tohwnd( lua, 1, WC_EDIT );
	unsigned int style		= (unsigned int) luaL_checkunsigned( lua, 3 );
	LONG_PTR curStyle		= GetWindowLongPtr( edit, GWL_STYLE );
	if ( SetWindowLongPtr( edit, GWL_STYLE, curStyle | style ) == 0 )
	{
		static CNSString errorDesc;
		errorDesc.Format( _UTF8( "函数[SetWindowLongPtr]调用失败，错误码:%d" ), GetLastError( ) );
		FBException( errorDesc );
	}
	DECLARE_END_PROTECTED
	return 0;
}

// 一会获得utf8
static int edit_getText( lua_State* lua )
{
	DECLARE_BEGIN_PROTECTED
	HWND	edit	= lua_tohwnd( lua, 1, WC_EDIT );
	TCHAR text[ 256 ];
	windowText( edit, text, 256 );
	lua_pushstring( lua, CNSString::FromTChar( text ).GetBuffer( ) );
	DECLARE_END_PROTECTED
	return 1;
}

static int edit_setText( lua_State* lua )
{
	DECLARE_BEGIN_PROTECTED
	HWND edit			= lua_tohwnd( lua, 1, WC_EDIT );
	const char* text	= (const char*) luaL_checkstring( lua, 3 );
	SetWindowText( edit, CNSString::ToTChar( text ) );
	DECLARE_END_PROTECTED
	return 0;
}
