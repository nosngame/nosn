#pragma once
static int listView_insertColumn( lua_State* lua );
static int listView_insertItem( lua_State* lua );
static int listView_getSelectList( lua_State* lua );
static int listView_addStyleEx( lua_State* lua );
static int listView_removeStyleEx( lua_State* lua );
static int listView_getCount( lua_State* lua );
static int listView_clear( lua_State* lua );
//static int listView_setItemData( lua_State* lua );
//static int listView_getItemData( lua_State* lua );
BEGIN_EXPORT( ListView )
	EXPORT_FUNC_EX( "insertColumn",		listView_insertColumn )
	EXPORT_FUNC_EX( "insertItem",		listView_insertItem )
	EXPORT_FUNC_EX( "getSelectList",	listView_getSelectList )
	EXPORT_FUNC_EX( "addStyleEx",		listView_addStyleEx )
	EXPORT_FUNC_EX( "removeStyleEx",	listView_removeStyleEx )
	EXPORT_FUNC_EX( "getCount",			listView_getCount )
	EXPORT_FUNC_EX( "clear",			listView_clear )
	//EXPORT_FUNC_EX( "setItemData",		listView_setItem )
	//EXPORT_FUNC_EX( "getItemData",		listView_clear )
END_EXPORT

static int listView_clear( lua_State* lua )
{
	DECLARE_BEGIN_PROTECTED
	HWND list = lua_tohwnd( lua, 1, WC_LISTVIEW );
	ListView_DeleteAllItems( list );
	DECLARE_END_PROTECTED
	return 0;
}

static int listView_getCount( lua_State* lua )
{
	DECLARE_BEGIN_PROTECTED
	HWND list = lua_tohwnd( lua, 1, WC_LISTVIEW );
	int count = ListView_GetItemCount( list );
	lua_pushnumber( lua, count );
	DECLARE_END_PROTECTED
	return 1;
}

static int listView_removeStyleEx( lua_State* lua )
{
	DECLARE_BEGIN_PROTECTED
	HWND			list		= lua_tohwnd( lua, 1, WC_LISTVIEW );
	unsigned int	styleEx		= (unsigned int) luaL_checknumber( lua, 3 );
	LONG			curStyle	= ListView_GetExtendedListViewStyle( list );
	ListView_SetExtendedListViewStyle( list, curStyle & ~styleEx );
	DECLARE_END_PROTECTED
	return 0;
}

static int listView_addStyleEx( lua_State* lua )
{
	DECLARE_BEGIN_PROTECTED
	HWND			list		= lua_tohwnd( lua, 1, WC_LISTVIEW );
	unsigned int	styleEx		= (unsigned int) luaL_checknumber( lua, 3 );
	LONG			curStyle	= ListView_GetExtendedListViewStyle( list );
	ListView_SetExtendedListViewStyle( list, curStyle | styleEx );
	DECLARE_END_PROTECTED
	return 0;
}

static int listView_getSelectList( lua_State* lua )
{
	DECLARE_BEGIN_PROTECTED
	HWND			list		= lua_tohwnd( lua, 1, WC_LISTVIEW );
	CNSLuaStack*	luaStack	= CNSLuaStack::FromState( lua );
	luaStack->BeginTable( );

	int index = 1;
	int rowIndex = ListView_GetNextItem( list, -1, LVNI_SELECTED );
	while ( rowIndex != -1 )
	{
		luaStack->LuaPushField( index, CFBDataType( rowIndex ) );
		rowIndex = ListView_GetNextItem( list, rowIndex, LVNI_SELECTED );
		index ++;
	}
	DECLARE_END_PROTECTED
	return 1;
}

static int listView_insertColumn( lua_State* lua )
{
	DECLARE_BEGIN_PROTECTED
	HWND			list	= lua_tohwnd( lua, 1, WC_LISTVIEW );
	unsigned int	iCol	= (unsigned int) luaL_checknumber( lua, 3 );
	const char*		text	= luaL_checkstring( lua, 4 );
	unsigned int	width	= (unsigned int) luaL_checknumber( lua, 5 );
	CString			unicode	= CNSString::ToTChar( text );
	LV_COLUMN column;
	column.mask		= LVCF_TEXT | LVCF_WIDTH;
	column.pszText	= unicode.GetBuffer( );
    column.cx		= width;
	ListView_InsertColumn( list, iCol, &column );
	DECLARE_END_PROTECTED
	return 0;
}

static int listView_insertItem( lua_State* lua )
{
	DECLARE_BEGIN_PROTECTED
	HWND	list	= lua_tohwnd( lua, 1, WC_LISTVIEW );
	int		iRow	= ListView_GetItemCount( list );
	int		top		= lua_gettop( lua );
	int		index	= 0;
	for ( int i = 3; i <= top; i ++, index ++ )
	{
		const char* text	= luaL_checkstring( lua, i );
		CString unicode		= CNSString::ToTChar( text );
		LVITEM item;
		item.iItem			= iRow;
		item.iSubItem		= index;
		item.mask			= LVIF_TEXT;
		item.pszText 		= unicode.GetBuffer( );
		if ( index == 0 )
			ListView_InsertItem( list, &item );
		else
			ListView_SetItem( list, &item );
	}

	lua_pushnumber( lua, iRow );
	DECLARE_END_PROTECTED
	return 1;
}
