#include <nsbase.h>
namespace NSWin32
{
	HBRUSH CNSVsTree::brushFocusSelect = NULL;
	HBRUSH CNSVsTree::brushUnfocusSelect = NULL;
	HPEN CNSVsTree::penNode = NULL;
	HBRUSH CNSVsTree::brushExpand = NULL;
	void CNSVsTree::init( )
	{
		brushFocusSelect = CreateSolidBrush( RGB( 0, 120, 215 ) );
		brushUnfocusSelect = CreateSolidBrush( RGB( 63, 63, 70 ) );
		penNode = CreatePen( PS_SOLID, 1, RGB( 255, 255, 255 ) );
		brushExpand = CreateSolidBrush( RGB( 255, 255, 255 ) );
		CNSWindow::superClass( WC_TREEVIEW, WC_NS_VSTREE );
	}

	void CNSVsTree::exit( )
	{
		UnregisterClass( WC_NS_VSTREE, CNSWindow::instance );
		if ( brushFocusSelect != NULL )
			DeleteObject( brushFocusSelect );

		if ( brushUnfocusSelect != NULL )
			DeleteObject( brushUnfocusSelect );

		if ( penNode != NULL )
			DeleteObject( penNode );

		if ( brushExpand != NULL )
			DeleteObject( brushExpand );
	}

	CNSVsTree::CNSVsTree( const CNSString& windowID ) : CNSWindow( windowID, NS_VSTREE )
	{
	}

	CNSVsTree::~CNSVsTree( )
	{
		if ( mBrushBkItem != NULL )
			DeleteObject( mBrushBkItem );
	}

	bool CNSVsTree::onTreeCustomDraw( CNSWindow* child, WPARAM wParam, LPARAM lParam, LRESULT* result )
	{
		CNSVsTree*		tree = (CNSVsTree*) child;
		LPNMTVCUSTOMDRAW	customDraw = (LPNMTVCUSTOMDRAW) lParam;
		DWORD				dwDrawStage = customDraw->nmcd.dwDrawStage;
		UINT				uItemState = customDraw->nmcd.uItemState;
		HTREEITEM			item = (HTREEITEM) customDraw->nmcd.dwItemSpec;
		HDC					dc = customDraw->nmcd.hdc;

		if ( dwDrawStage == CDDS_PREPAINT )
		{
			*result = CDRF_NOTIFYITEMDRAW;
			return true;
		}
		else if ( dwDrawStage == CDDS_ITEMPREPAINT )
		{
			if ( customDraw->nmcd.rc.bottom == 0 && customDraw->nmcd.rc.right == 0 )
			{
				*result = CDRF_SKIPDEFAULT;
				return true;
			}

			RECT rcItem;
			tree->getItemRect( item, rcItem );
			CNSString itemText = tree->getItemText( item );

			SCROLLINFO si;
			si.cbSize = sizeof( si );
			si.fMask = SIF_ALL;
			GetScrollInfo( tree->getHWnd( ), SB_HORZ, &si );
			OffsetRect( &rcItem, -si.nPos, 0 );
			rcItem.right += si.nPos;

			POINT ptStart;
			ptStart.x = rcItem.left + 10 + customDraw->iLevel * TreeView_GetIndent( tree->getHWnd( ) );
			ptStart.y = rcItem.top;
			customDraw->clrText = tree->getTextColor( );

			if ( uItemState & CDIS_FOCUS )
			{
				FillRect( dc, &rcItem, CNSVsTree::brushFocusSelect );
			}
			else if ( uItemState & CDIS_SELECTED )
			{
				FillRect( dc, &rcItem, CNSVsTree::brushUnfocusSelect );
			}
			else
			{
				if ( tree->mBrushBkItem == NULL )
					tree->mBrushBkItem = CreateSolidBrush( customDraw->clrTextBk );

				FillRect( dc, &rcItem, tree->mBrushBkItem );
			}

			TVITEMEX tvItem;
			tvItem.mask = TVIF_HANDLE | TVIF_IMAGE | TVIF_EXPANDEDIMAGE | TVIF_STATE | TVIF_CHILDREN;
			tvItem.hItem = item;
			tvItem.stateMask = TVIS_EXPANDED;
			TreeView_GetItem( tree->getHWnd( ), &tvItem );

			UINT itemState = tvItem.state;
			if ( tvItem.cChildren == 1 )
			{
				int x = ptStart.x;
				int y = ptStart.y;
				HPEN oldPen = (HPEN) SelectObject( dc, penNode );
				HBRUSH oldBrush = NULL;
				POINT pt[ 3 ];
				if ( itemState & TVIS_EXPANDED )
				{
					oldBrush = (HBRUSH) SelectObject( dc, brushExpand );
					pt[ 0 ] = POINT { x + 7, y + 9 };
					pt[ 1 ] = POINT { x + 7, y + 13 };
					pt[ 2 ] = POINT { x + 3, y + 13 };
					::Polygon( dc, pt, 3 );
					SelectObject( dc, oldBrush );
				}
				else
				{
					pt[ 0 ] = POINT { 3, 5 };
					pt[ 1 ] = POINT { 7, 9 };
					pt[ 2 ] = POINT { 3, 13 };
					::MoveToEx( dc, x + pt[ 0 ].x, y + pt[ 0 ].y, NULL );
					::LineTo( dc, x + pt[ 1 ].x, y + pt[ 1 ].y );
					::LineTo( dc, x + pt[ 2 ].x, y + pt[ 2 ].y );
					::LineTo( dc, x + pt[ 0 ].x, y + pt[ 0 ].y );
				}

				SelectObject( dc, oldPen );
			}

			ptStart.x += 20;
			HIMAGELIST imageList = TreeView_GetImageList( tree->getHWnd( ), TVSIL_NORMAL );
			if ( imageList != NULL )
			{
				if ( itemState & TVIS_EXPANDED )
				{
					if ( tvItem.iExpandedImage != 65534 )
					{
						ImageList_Draw( imageList, tvItem.iExpandedImage, dc, ptStart.x, ptStart.y, ILD_TRANSPARENT );
						ptStart.x += 25;
					}
				}
				else
				{
					if ( tvItem.iImage != 65534 )
					{
						ImageList_Draw( imageList, tvItem.iImage, dc, ptStart.x, ptStart.y, ILD_TRANSPARENT );
						ptStart.x += 25;
					}
				}
			}

			HFONT oldFont = NULL;
			if ( itemState & TVIS_BOLD )
				oldFont = (HFONT) SelectObject( dc, CNSPreDefine::FB_FONT_BASEBOLD );

			COLORREF oldTextColor = SetTextColor( dc, customDraw->clrText );
			int oldTextMode = SetBkMode( dc, TRANSPARENT );

			RECT rcText = rcItem;
			rcText.left = ptStart.x;
			DrawText( dc, (TCHAR*) CNSString::toTChar( itemText ), -1, &rcText, DT_LEFT | DT_VCENTER | DT_SINGLELINE );
			SetTextColor( dc, oldTextColor );
			SetBkMode( dc, oldTextMode );

			if ( itemState & TVIS_BOLD )
				SelectObject( dc, oldFont );
			*result = CDRF_SKIPDEFAULT;
			return true;
		}

		*result = CDRF_DODEFAULT;
		return true;
	}

	void CNSVsTree::onPostCreateWindow( CNSWindow* parent )
	{
		CNSWindow::onPostCreateWindow( parent );
		setItemHeight( 20 );

		setBkColor( RGB( 37, 37, 38 ) );
		setTextColor( RGB( 241, 241, 241 ) );
		registerEvent( CTreeCtrlEvent::EventCustomDraw, onTreeCustomDraw );
	}

	COLORREF CNSVsTree::getTextColor( ) const
	{
		return TreeView_GetTextColor( mHWnd );
	}

	void CNSVsTree::setTextColor( COLORREF color )
	{
		TreeView_SetTextColor( mHWnd, color );
	}

	void CNSVsTree::setBkColor( COLORREF color )
	{
		if ( mBrushBkItem != NULL )
			DeleteObject( mBrushBkItem );

		mBrushBkItem = NULL;
		TreeView_SetBkColor( mHWnd, color );
	}

	COLORREF CNSVsTree::getBkColor( ) const
	{
		return TreeView_GetBkColor( mHWnd );
	}

	void CNSVsTree::clear( )
	{
		TreeView_DeleteAllItems( mHWnd );
	}

	void CNSVsTree::setItemText( HTREEITEM item, const CNSString& text )
	{
		TVITEMEX tvItem = { 0 };
		tvItem.mask = TVIF_TEXT;
		tvItem.pszText = (TCHAR*) CNSString::toTChar( text );
		tvItem.cchTextMax = lstrlen( tvItem.pszText );
		tvItem.hItem = item;
		TreeView_SetItem( mHWnd, &tvItem );
	}

	void CNSVsTree::setImageList( CNSVector< CNSString >& images )
	{
		HIMAGELIST imageList = TreeView_GetImageList( mHWnd, TVSIL_NORMAL );
		if ( imageList != NULL )
			ImageList_Destroy( imageList );

		imageList = ImageList_Create( 20, 20, ILC_MASK | ILC_COLOR24, 4, 2 );
		HDC dc = GetDC( mHWnd );
		for ( unsigned int i = 0; i < images.getCount( ); i ++ )
		{
			CImage* dataImage = CImage::loadPngImageBGRA( images[ i ] );
			if ( dataImage == NULL )
				continue;

			HBITMAP bitmapImage = dataImage->getHBitmap( dc );
			ImageList_Add( imageList, bitmapImage, bitmapImage );
			DeleteObject( bitmapImage );
		}
		ReleaseDC( mHWnd, dc );
		TreeView_SetImageList( mHWnd, imageList, TVSIL_NORMAL );
	}

	void CNSVsTree::setItemImage( HTREEITEM item, int image, int expandImage )
	{
		TVITEMEX tvItem;
		tvItem.mask = TVIF_HANDLE | TVIF_IMAGE | TVIF_EXPANDEDIMAGE | TVIF_SELECTEDIMAGE;
		tvItem.iImage = image;
		tvItem.hItem = item;
		tvItem.iSelectedImage = image;
		tvItem.iExpandedImage = expandImage;
		tvItem.uStateEx = TVIF_EXPANDEDIMAGE;
		TreeView_SetItem( mHWnd, &tvItem );
	}

	HTREEITEM CNSVsTree::addItem( HTREEITEM parent, HTREEITEM after, const CNSString& text, void* data, int image, int expandImage )
	{
		TVINSERTSTRUCT item;
		item.itemex.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_EXPANDEDIMAGE | TVIF_SELECTEDIMAGE;
		item.hInsertAfter = after;
		item.hParent = parent;
		item.itemex.lParam = (LPARAM) data;
		item.itemex.pszText = (TCHAR*) CNSString::toTChar( text );
		item.itemex.cchTextMax = lstrlen( item.itemex.pszText );
		item.itemex.iImage = image;
		item.itemex.iSelectedImage = image;
		item.itemex.uStateEx = TVIF_EXPANDEDIMAGE;
		item.itemex.iExpandedImage = expandImage;
		return (HTREEITEM) TreeView_InsertItem( mHWnd, &item );
	}

	HTREEITEM CNSVsTree::getCurSel( ) const
	{
		return TreeView_GetSelection( mHWnd );
	}

	void CNSVsTree::setItemBold( HTREEITEM item, bool enable )
	{
		int state = 0;
		if ( enable == true )
			state = TVIS_BOLD;
		else
			state = 0;

		TreeView_SetItemState( mHWnd, item, state, TVIS_BOLD );
	}

	void CNSVsTree::setCurSel( HTREEITEM item )
	{
		HTREEITEM parent = TreeView_GetParent( mHWnd, item );
		for ( ; parent != NULL; parent = TreeView_GetParent( mHWnd, parent ) )
			TreeView_Expand( mHWnd, parent, TVE_EXPAND );

		TreeView_SelectItem( mHWnd, item );
	}

	void CNSVsTree::setItemData( HTREEITEM item, void* data )
	{
		TV_ITEM tvItem;
		tvItem.mask = TVIF_PARAM | TVIF_HANDLE;
		tvItem.lParam = (LPARAM) data;
		tvItem.hItem = item;
		TreeView_SetItem( mHWnd, &tvItem );
	}

	void* CNSVsTree::getItemData( HTREEITEM item )
	{
		TV_ITEM tvItem;
		tvItem.mask = TVIF_PARAM | TVIF_HANDLE;
		tvItem.hItem = item;
		TreeView_GetItem( mHWnd, &tvItem );
		return (void*) tvItem.lParam;
	}

	void CNSVsTree::setItemHeight( unsigned int height )
	{
		TreeView_SetItemHeight( mHWnd, height );
	}

	void CNSVsTree::getItemRect( HTREEITEM item, RECT& rc ) const
	{
		TreeView_GetItemRect( mHWnd, item, &rc, FALSE );
	}

	CNSString& CNSVsTree::getItemText( HTREEITEM item )
	{
		static CNSString itemPath;

		TCHAR buffer[ 256 ] = { 0 };
		TV_ITEM tvItem;
		tvItem.mask = TVIF_TEXT | TVIF_HANDLE;
		tvItem.hItem = item;
		tvItem.pszText = buffer;
		tvItem.cchTextMax = 256;
		TreeView_GetItem( mHWnd, &tvItem );
		itemPath = CNSString::fromTChar( (char*) buffer );
		return itemPath;
	}

	CNSString& CNSVsTree::getItemPath( HTREEITEM item )
	{
		static CNSString itemPath;
		itemPath.clear( );
		TCHAR buffer[ 256 ] = { 0 };
		HTREEITEM parent = item;
		for ( ; parent != NULL; parent = TreeView_GetParent( mHWnd, parent ) )
		{
			TV_ITEM tvItem;
			tvItem.mask = TVIF_TEXT | TVIF_HANDLE;
			tvItem.hItem = parent;
			tvItem.pszText = buffer;
			tvItem.cchTextMax = 256;
			TreeView_GetItem( mHWnd, &tvItem );
			if ( parent == item )
				itemPath.insert( 0, CNSString::fromTChar( (char*) buffer ) );
			else
				itemPath.insert( 0, CNSString::fromTChar( (char*) buffer ) + "/" );
		}

		return itemPath;
	}

	HTREEITEM CNSVsTree::findItemHelper( HTREEITEM firstItem, CNSVector< CNSString >& path, int index )
	{
		TCHAR buffer[ 256 ];
		CNSString itemText;
		for ( ; firstItem != NULL; firstItem = TreeView_GetNextSibling( mHWnd, firstItem ) )
		{
			TV_ITEM item;
			item.mask = TVIF_TEXT | TVIF_HANDLE;
			item.hItem = firstItem;
			item.pszText = buffer;
			item.cchTextMax = 256;
			TreeView_GetItem( mHWnd, &item );

			itemText = CNSString::fromTChar( (char*) buffer );
			if ( path[ index ] == itemText )
			{
				if ( index == path.getCount( ) - 1 )
					return firstItem;

				HTREEITEM child = TreeView_GetChild( mHWnd, firstItem );
				if ( child == NULL )
					return NULL;

				return findItemHelper( child, path, index + 1 );
			}
		}

		return NULL;
	}

	HTREEITEM CNSVsTree::findItem( const CNSString& treePath )
	{
		static CNSVector< CNSString > result;
		result.clear( );

		treePath.split( "/", result );
		int index = 0;
		HTREEITEM root = TreeView_GetRoot( mHWnd );
		return findItemHelper( root, result, index );
	}

	void CNSVsTree::select( HTREEITEM item )
	{
		TreeView_SelectItem( mHWnd, item );
	}

	void CNSVsTree::deleteItem( HTREEITEM item )
	{
		TreeView_DeleteItem( mHWnd, item );
	}

	HTREEITEM CNSVsTree::getParentItem( HTREEITEM item )
	{
		return TreeView_GetParent( mHWnd, item );
	}

	void CNSVsTree::getSubItems( HTREEITEM item, CNSVector< HTREEITEM >& subItems, bool recursion )
	{
		HTREEITEM firstItem = TreeView_GetChild( mHWnd, item );
		if ( firstItem == NULL )
			return;

		for ( ; firstItem != NULL; firstItem = TreeView_GetNextSibling( mHWnd, firstItem ) )
		{
			subItems.pushback( firstItem );
			if ( recursion == true )
			{
				CNSVector< HTREEITEM > subsubItems;
				getSubItems( firstItem, subsubItems, recursion );
				for ( unsigned int i = 0; i < subsubItems.getCount( ); i ++ )
					subItems.pushback( subsubItems[ i ] );
			}
		}
	}

	void CNSVsTree::deleteAllSubItems( HTREEITEM item )
	{
		CNSVector< HTREEITEM > children;
		getSubItems( item, children );
		for ( unsigned int i = 0; i < children.getCount( ); i ++ )
			TreeView_DeleteItem( mHWnd, children[ i ] );
	}

	void CNSVsTree::pushUserData( CNSLuaStack& luaStack, CNSVsTree* ref )
	{
		static CNSVector< const luaL_Reg* > reg;
		reg.clear( );
		reg.pushback( NSWin32::NSWindow );
		reg.pushback( NSWin32::NSVsTree );
		luaStack.pushNSWeakRef( ref, NS_VSTREE, reg );
	}

	void CNSVsTree::popUserData( const CNSLuaStack& luaStack, CNSVsTree*& ref )
	{
		ref = (CNSVsTree*) luaStack.popNSWeakRef( NS_VSTREE );
	}
}