#pragma once
static int treeView_insertItem( lua_State* lua );
static int treeView_getCurSel( lua_State* lua );
static int treeView_clear( lua_State* lua );
static int treeView_addImage( lua_State* lua );
static int treeView_setItemImage( lua_State* lua );
static int treeView_setItemText( lua_State* lua );
static int treeView_getInplaceEdit( lua_State* lua );
static int treeView_setInplaceText( lua_State* lua );
static int treeView_getInplaceText( lua_State* lua );
static int treeView_getItemRect( lua_State* lua );

BEGIN_EXPORT( TreeView )
	EXPORT_FUNC_EX( "insertItem",		treeView_insertItem )
	EXPORT_FUNC_EX( "setItemImage",		treeView_setItemImage )
	EXPORT_FUNC_EX( "setItemText",		treeView_setItemText )
	EXPORT_FUNC_EX( "getCurSel",		treeView_getCurSel )
	EXPORT_FUNC_EX( "addImage",			treeView_addImage )
	EXPORT_FUNC_EX( "clear",			treeView_clear )
	EXPORT_FUNC_EX( "getInplaceEdit",	treeView_getInplaceEdit )
	EXPORT_FUNC_EX( "setInplaceText",	treeView_setInplaceText )
	EXPORT_FUNC_EX( "getInplaceText",	treeView_getInplaceText )
	EXPORT_FUNC_EX( "getItemRect",		treeView_getItemRect )
END_EXPORT

static int treeView_setInplaceText( lua_State* lua )
{
	DECLARE_BEGIN_PROTECTED
	HWND		tree	= lua_tohwnd( lua, 1, WC_TREEVIEW );
	const char* text	= luaL_checkstring( lua, 3 );
	HWND		inplace	= TreeView_GetEditControl( tree );
	CString		unicode	= CNSString::ToTChar( text );
	SetWindowText( inplace, unicode );
	DECLARE_END_PROTECTED
	return 0;
}

static int treeView_getInplaceText( lua_State* lua )
{
	DECLARE_BEGIN_PROTECTED
	HWND tree		= lua_tohwnd( lua, 1, WC_TREEVIEW );
	HWND inplace	= TreeView_GetEditControl( tree );

	TCHAR title[ 256 ];
	windowText( inplace, title, 256 );
	lua_pushstring( lua, CNSString::FromTChar( title ).GetBuffer( ) );
	DECLARE_END_PROTECTED
	return 1;
}

static int treeView_getInplaceEdit( lua_State* lua )
{
	DECLARE_BEGIN_PROTECTED
	HWND	tree		= lua_tohwnd( lua, 1, WC_TREEVIEW );
	HWND	inplace		= TreeView_GetEditControl( tree );
	lua_pushlightuserdata( lua, (void*) inplace );
	DECLARE_END_PROTECTED
	return 1;
}

static int treeView_getItemRect( lua_State* lua )
{
	DECLARE_BEGIN_PROTECTED
	HWND		tree	= lua_tohwnd( lua, 1, WC_TREEVIEW );
	HTREEITEM	item	= (HTREEITEM) (ptrdiff_t) luaL_checkunsigned( lua, 3 );

	CNSLuaStack* luaStack = CNSLuaStack::FromState( lua );
	CRect rc;
	TreeView_GetItemRect( tree, item, &rc, TRUE );
	luaStack->BeginTable( );
	luaStack->LuaPushField( "x", CFBDataType( (int) rc.left ) );
	luaStack->LuaPushField( "y", CFBDataType( (int) rc.top ) );
	luaStack->LuaPushField( "width", CFBDataType( (int) rc.Width( ) ) );
	luaStack->LuaPushField( "height", CFBDataType( (int) rc.Height( ) ) );
	DECLARE_END_PROTECTED
	return 1;
}

static int treeView_addImage( lua_State* lua )
{
	DECLARE_BEGIN_PROTECTED
	HWND			tree		= lua_tohwnd( lua, 1, WC_TREEVIEW );
	const char*		image		= luaL_checkstring( lua, 3 );

	HIMAGELIST		imageList	= theApp.GetImageList( image );
	TreeView_SetImageList( tree, imageList, TVSIL_NORMAL );
	DECLARE_END_PROTECTED
	return 0;
}

static int treeView_clear( lua_State* lua )
{
	DECLARE_BEGIN_PROTECTED
	HWND tree = lua_tohwnd( lua, 1, WC_TREEVIEW );
	TreeView_DeleteAllItems( tree );
	DECLARE_END_PROTECTED
	return 0;
}

static int treeView_getCurSel( lua_State* lua )
{
	DECLARE_BEGIN_PROTECTED
	HWND		tree	= lua_tohwnd( lua, 1, WC_TREEVIEW );
	HTREEITEM	curSel	= (HTREEITEM) TreeView_GetSelection( tree );
	lua_pushunsigned( lua, (size_t) curSel );
	DECLARE_END_PROTECTED
	return 1;
}

static int treeView_insertItem( lua_State* lua )
{
	DECLARE_BEGIN_PROTECTED
	HWND		tree		= lua_tohwnd( lua, 1, WC_TREEVIEW );
	HTREEITEM	parentItem	= (HTREEITEM) (ptrdiff_t) luaL_checkunsigned( lua, 3 );
	HTREEITEM	afterItem	= (HTREEITEM) (ptrdiff_t) luaL_checkunsigned( lua, 4 );
	const char* text		= luaL_checkstring( lua, 5 );
	unsigned int imageIndex	= luaL_checkunsigned( lua, 6 );

	CString		unicode			= CNSString::ToTChar( text );
	TVINSERTSTRUCT item;
	item.itemex.mask			= TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
	item.itemex.pszText			= unicode.GetBuffer( );
	item.itemex.cchTextMax		= unicode.GetLength( );
	item.itemex.iImage			= imageIndex;
	item.itemex.iSelectedImage	= imageIndex;
	item.hInsertAfter			= afterItem;
	item.hParent				= parentItem;

	TreeView_SortChildren( tree, parentItem, true );
	HTREEITEM itemHandle		= (HTREEITEM) TreeView_InsertItem( tree, &item );
	lua_pushunsigned( lua, (size_t) itemHandle );
	DECLARE_END_PROTECTED
	return 1;
}

static int treeView_setItemImage( lua_State* lua )
{
	DECLARE_BEGIN_PROTECTED
	HWND			tree		= lua_tohwnd( lua, 1, WC_TREEVIEW );
	HTREEITEM		treeItem	= (HTREEITEM)(ptrdiff_t) luaL_checkunsigned( lua, 3 );
	unsigned int	imageIndex	= luaL_checkunsigned( lua, 4 );

	TVITEM item;
	item.mask			= TVIF_IMAGE | TVIF_SELECTEDIMAGE;
	item.iImage			= imageIndex;
	item.iSelectedImage	= imageIndex;
	item.hItem			= treeItem;
	TreeView_SetItem( tree, &item );
	DECLARE_END_PROTECTED
	return 0;
}

static int treeView_setItemText( lua_State* lua )
{
	DECLARE_BEGIN_PROTECTED
	HWND		tree		= lua_tohwnd( lua, 1, WC_TREEVIEW );
	HTREEITEM	treeItem	= (HTREEITEM)(ptrdiff_t) luaL_checkunsigned( lua, 3 );
	const char* text		= luaL_checkstring( lua, 4 );

	CString		unicode		= CNSString::ToTChar( text );
	TVITEM item;
	item.mask			= TVIF_TEXT;
	item.pszText		= unicode.GetBuffer( );
	item.cchTextMax		= unicode.GetLength( );
	item.hItem			= treeItem;
	TreeView_SetItem( tree, &item );
	unicode.ReleaseBuffer( );
	DECLARE_END_PROTECTED
	return 0;
}
