#pragma once
static int comboBox_appendItem( lua_State* lua );
static int comboBox_setCurSel( lua_State* lua );
static int comboBox_getCurSel( lua_State* lua );

BEGIN_EXPORT( ComboBox )
	EXPORT_FUNC_EX( "appendItem", comboBox_appendItem )
	EXPORT_FUNC_EX( "setCurSel", comboBox_setCurSel )
	EXPORT_FUNC_EX( "getCurSel", comboBox_getCurSel )
END_EXPORT

static int comboBox_setCurSel( lua_State* lua )
{
	DECLARE_BEGIN_PROTECTED
	HWND	combo	= (HWND) lua_tohwnd( lua, 1, WC_COMBOBOX );
	int		index	= (unsigned int) luaL_checknumber( lua, 3 );

	ComboBox_SetCurSel( combo, index );
	DECLARE_END_PROTECTED
	return 0;
}

static int comboBox_getCurSel( lua_State* lua )
{
	DECLARE_BEGIN_PROTECTED
	HWND	combo	= (HWND) lua_tohwnd( lua, 1, WC_COMBOBOX );
	int		index	= ComboBox_GetCurSel( combo );
	lua_pushnumber( lua, index );
	DECLARE_END_PROTECTED
	return 1;
}

static int comboBox_appendItem( lua_State* lua )
{
	DECLARE_BEGIN_PROTECTED
	HWND		combo	= (HWND) lua_tohwnd( lua, 1, WC_COMBOBOX );
	const char* text	= luaL_checkstring( lua, 3 );
	int			index	= ComboBox_AddString( combo, CNSString::ToTChar( text ) );
	lua_pushnumber( lua, index );
	DECLARE_END_PROTECTED
	return 1;
}
