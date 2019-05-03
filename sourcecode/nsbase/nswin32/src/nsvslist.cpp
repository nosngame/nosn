#include <nsbase.h>
namespace NSWin32
{
	HBRUSH	CNSVsList::brItemSelect = NULL;
	HBRUSH	CNSVsList::backTextA = NULL;
	HBRUSH	CNSVsList::backTextB = NULL;
	HBRUSH	CNSVsList::backHeader = NULL;
	HBRUSH	CNSVsList::backHeaderSplit = NULL;
	HPEN	CNSVsList::borderHeaderPen = NULL;

	void CNSVsList::init( )
	{
		CNSVsList::brItemSelect = CreateSolidBrush( RGB( 0, 120, 215 ) );
		CNSVsList::backTextA = CreateSolidBrush( RGB( 37, 37, 38 ) );
		CNSVsList::backTextB = CreateSolidBrush( RGB( 37, 37, 38 ) );
		CNSVsList::backHeader = CreateSolidBrush( RGB( 62, 62, 68 ) );
		CNSVsList::backHeaderSplit = CreateSolidBrush( RGB( 82, 176, 239 ) );
		CNSVsList::borderHeaderPen = CreatePen( PS_SOLID, 1, RGB( 82, 176, 239 ) );
		CNSWindow::superClass( WC_LISTVIEW, WC_NS_VSLIST );
	}

	void CNSVsList::exit( )
	{
		UnregisterClass( WC_NS_VSLIST, CNSWindow::instance );

		if ( CNSVsList::brItemSelect != NULL )
			DeleteObject( CNSVsList::brItemSelect );

		if ( CNSVsList::backTextA != NULL )
			DeleteObject( CNSVsList::backTextA );

		if ( CNSVsList::backTextB != NULL )
			DeleteObject( CNSVsList::backTextB );

		if ( CNSVsList::backHeader != NULL )
			DeleteObject( CNSVsList::backHeader );

		if ( CNSVsList::backHeaderSplit != NULL )
			DeleteObject( CNSVsList::backHeaderSplit );

		if ( CNSVsList::borderHeaderPen != NULL )
			DeleteObject( CNSVsList::borderHeaderPen );;
	}

	CHeaderCtrl::CHeaderCtrl( const CNSString& windowID, HWND wnd ) : CNSWindow( windowID, NS_VSHEADER )
	{
		mHWnd = wnd;
		mProcessAnchor = false;
	}

	CNSVsList::CNSVsList( const CNSString& windowID ) : CNSWindow( windowID, NS_VSLIST ), mColumnCounter( 0 ), mItemCounter( 0 ), mHeader( NULL )
	{
	}

	void CNSVsList::scrollVert( int pos )
	{
		RECT rcItem;
		ListView_GetItemRect( getHWnd( ), 0, &rcItem, LVIR_BOUNDS );
		int itemHeight = rcItem.bottom - rcItem.top;

		SCROLLINFO si;
		si.cbSize = sizeof( si );
		si.fMask = SIF_POS;
		GetScrollInfo( getHWnd( ), SB_VERT, &si );
		ListView_Scroll( getHWnd( ), 0, ( pos - si.nPos ) * itemHeight );
	}

	void CNSVsList::scrollHorz( int pos )
	{
		SCROLLINFO si;
		si.cbSize = sizeof( si );
		si.fMask = SIF_POS;
		GetScrollInfo( getHWnd( ), SB_HORZ, &si );

		int delta = pos - si.nPos;
		ListView_Scroll( getHWnd( ), delta, 0 );
	}

	bool CNSVsList::onListCustomDraw( CNSWindow* child, WPARAM wParam, LPARAM lParam, LRESULT* result )
	{
		CNSVsList*		fileList = (CNSVsList*) child;
		LPNMLVCUSTOMDRAW	customDraw = (LPNMLVCUSTOMDRAW) lParam;
		// Take the default processing unless we set this to something else below.
		*result = 0;

		// First thing - check the draw stage. If it's the control's prepaint
		// stage, then tell Windows we want messages for every item.
		if ( CDDS_PREPAINT == customDraw->nmcd.dwDrawStage )
		{
			*result = CDRF_NOTIFYITEMDRAW | CDRF_NOTIFYPOSTPAINT;
		}
		else if ( CDDS_ITEMPREPAINT == customDraw->nmcd.dwDrawStage )
		{
			int			itemIndex = (int) customDraw->nmcd.dwItemSpec;
			RECT rcList;
			GetClientRect( fileList->getHWnd( ), &rcList );
			if ( fileList->isItemSelect( itemIndex ) == true )
			{
				FillRect( customDraw->nmcd.hdc, &customDraw->nmcd.rc, brItemSelect );
				for ( int i = 0; i < fileList->getColumnCount( ); i ++ )
				{
					int x = i;
					int y = itemIndex;
					RECT rc;
					ListView_GetSubItemRect( fileList->getHWnd( ), y, x, LVIR_LABEL, &rc );
					if ( i != 0 )
						rc.left += 6;
					else
						rc.left += 2;

					CNSString	itemText = fileList->getItemText( x, y );
					TCHAR*		text = CNSString::toTChar( itemText );
					int			oldMode = SetBkMode( customDraw->nmcd.hdc, TRANSPARENT );
					COLORREF	oldTextClr = SetTextColor( customDraw->nmcd.hdc, RGB( 241, 241, 241 ) );
					DrawText( customDraw->nmcd.hdc, text, lstrlen( text ), &rc, DT_LEFT | DT_VCENTER | DT_SINGLELINE );
					SetBkMode( customDraw->nmcd.hdc, oldMode );
					SetTextColor( customDraw->nmcd.hdc, oldTextClr );
				}

				LV_ITEM item;
				item.mask = LVIF_IMAGE;
				item.iItem = itemIndex;
				item.iSubItem = 0;
				ListView_GetItem( fileList->getHWnd( ), &item );

				RECT rc;
				ListView_GetSubItemRect( fileList->getHWnd( ), itemIndex, 0, LVIR_ICON, &rc );

				HIMAGELIST imageList = (HIMAGELIST) ListView_GetImageList( fileList->mHWnd, LVSIL_SMALL );
				int cx, cy;
				ImageList_GetIconSize( imageList, &cx, &cy );
				int iconY = rc.top + ( ( ( rc.bottom - rc.top ) - cy ) >> 1 );
				int iconX = rc.left;
				ImageList_Draw( imageList, item.iImage, customDraw->nmcd.hdc, iconX, iconY, ILD_TRANSPARENT );
				*result = CDRF_SKIPDEFAULT;
				return true;
			}

			if ( itemIndex % 2 == 0 )
				FillRect( customDraw->nmcd.hdc, &customDraw->nmcd.rc, backTextA );
			else
				FillRect( customDraw->nmcd.hdc, &customDraw->nmcd.rc, backTextB );

			for ( int i = 0; i < fileList->getColumnCount( ); i ++ )
			{
				int colWidth = fileList->getColumnWidth( i );
				int x = i;
				int y = itemIndex;
				CNSString itemText = fileList->getItemText( x, y );

				RECT rc;
				ListView_GetSubItemRect( fileList->getHWnd( ), y, x, LVIR_LABEL, &rc );
				if ( i != 0 )
					rc.left += 6;
				else
					rc.left += 2;

				// 绘制文本底色
				TCHAR* text = CNSString::toTChar( itemText );
				int oldMode = SetBkMode( customDraw->nmcd.hdc, TRANSPARENT );
				COLORREF oldTextClr = SetTextColor( customDraw->nmcd.hdc, RGB( 241, 241, 241 ) );
				DrawText( customDraw->nmcd.hdc, text, lstrlen( text ), &rc, DT_LEFT | DT_VCENTER | DT_SINGLELINE );
				SetTextColor( customDraw->nmcd.hdc, oldTextClr );
				SetBkMode( customDraw->nmcd.hdc, oldMode );
			}

			LV_ITEM item;
			item.mask = LVIF_IMAGE;
			item.iItem = itemIndex;
			item.iSubItem = 0;
			ListView_GetItem( fileList->getHWnd( ), &item );

			RECT rc;
			ListView_GetSubItemRect( fileList->getHWnd( ), itemIndex, 0, LVIR_ICON, &rc );

			HIMAGELIST imageList = (HIMAGELIST) ListView_GetImageList( fileList->mHWnd, LVSIL_SMALL );
			int cx, cy;
			ImageList_GetIconSize( imageList, &cx, &cy );
			int iconY = rc.top + ( ( ( rc.bottom - rc.top ) - cy ) >> 1 );
			int iconX = rc.left;
			ImageList_Draw( imageList, item.iImage, customDraw->nmcd.hdc, iconX, iconY, ILD_TRANSPARENT );
			// Tell Windows to paint the control itself.
			*result = CDRF_SKIPDEFAULT;
		}

		return true;
	}

	bool CNSVsList::onHeaderRender( CNSWindow* child, WPARAM wParam, LPARAM lParam, LRESULT* result )
	{
		PAINTSTRUCT ps;
		BeginPaint( child->getHWnd( ), &ps );

		HFONT	font = GetWindowFont( child->getHWnd( ) );
		HPEN	oldPen = (HPEN) SelectObject( ps.hdc, borderHeaderPen );
		HBRUSH	oldBrush = (HBRUSH) SelectObject( ps.hdc, backHeaderSplit );
		HFONT	oldFont = (HFONT) SelectObject( ps.hdc, font );

		RECT rc;
		GetClientRect( child->getHWnd( ), &rc );
		FillRect( ps.hdc, &rc, backHeader );
		Rectangle( ps.hdc, rc.left, rc.bottom - 3, rc.right, rc.bottom );

		int count = Header_GetItemCount( child->getHWnd( ) );
		for ( int i = 0; i < count; i ++ )
		{
			RECT	rc;
			Header_GetItemRect( child->getHWnd( ), i, &rc );

			MoveToEx( ps.hdc, rc.right - 1, rc.bottom - 6, NULL );
			LineTo( ps.hdc, rc.right - 1, rc.top - 3 );

			TCHAR text[ 255 ];
			HDITEM item;
			item.mask = HDI_TEXT;
			item.pszText = text;
			item.cchTextMax = 255;
			Header_GetItem( child->getHWnd( ), i, &item );

			// 绘制文本底色
			int oldMode = SetBkMode( ps.hdc, TRANSPARENT );
			COLORREF oldTextClr = SetTextColor( ps.hdc, RGB( 241, 241, 241 ) );
			rc.left += 2;
			DrawText( ps.hdc, text, lstrlen( text ), &rc, DT_LEFT | DT_VCENTER | DT_SINGLELINE );
			SetTextColor( ps.hdc, oldTextClr );
		}
		EndPaint( child->getHWnd( ), &ps );

		SelectObject( ps.hdc, oldFont );
		SelectObject( ps.hdc, oldPen );
		SelectObject( ps.hdc, oldBrush );
		return true;
	}

	void CNSVsList::addColumn( const CNSString& text, unsigned int width )
	{
		LV_COLUMN column;
		column.mask = LVCF_TEXT | LVCF_WIDTH | LVCFMT_SPLITBUTTON;
		column.cx = width;
		column.pszText = CNSString::toTChar( text );
		column.cchTextMax = lstrlen( column.pszText );

		ListView_InsertColumn( mHWnd, mColumnCounter ++, &column );
	}

	int CNSVsList::addItem( const CNSString& text )
	{
		LV_ITEM item;
		item.mask = LVIF_TEXT;
		item.iItem = mItemCounter ++;
		item.iSubItem = 0;
		item.pszText = CNSString::toTChar( text );
		item.cchTextMax = lstrlen( item.pszText );

		return ListView_InsertItem( mHWnd, &item );
	}

	void CNSVsList::setItem( int x, int y, const CNSString& text )
	{
		LV_ITEM item;
		item.mask = LVIF_TEXT;
		item.iItem = y;
		item.iSubItem = x;
		item.pszText = CNSString::toTChar( text );
		item.cchTextMax = lstrlen( item.pszText );

		ListView_SetItem( mHWnd, &item );
	}

	void CNSVsList::setImageList( HIMAGELIST imageList )
	{
		ListView_SetImageList( mHWnd, imageList, LVSIL_SMALL );
	}

	void CNSVsList::setItemImage( int x, int y, int iImage )
	{
		LV_ITEM item;
		item.mask = LVIF_IMAGE;
		item.iItem = y;
		item.iSubItem = x;
		item.iImage = iImage;
		ListView_SetItem( mHWnd, &item );
	}

	void CNSVsList::clear( )
	{
		mItemCounter = 0;
		ListView_DeleteAllItems( mHWnd );
	}

	bool CNSVsList::isItemSelect( int index ) const
	{
		return ::SendMessage( mHWnd, LVM_GETITEMSTATE, index, LVIS_SELECTED );
	}

	void CNSVsList::setCurSel( int index )
	{
		ListView_SetItemState( mHWnd, index, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED );
	}

	int CNSVsList::getColumnWidth( int index ) const
	{
		LVCOLUMN col;
		col.mask = LVCF_WIDTH;
		::SendMessage( mHWnd, LVM_GETCOLUMN, index, (LPARAM) (LVCOLUMN*) &col );
		return col.cx;
	}

	CNSString CNSVsList::getItemText( int x, int y )
	{
		TCHAR buffer[ 256 ];
		LV_ITEM item;
		item.iSubItem = x;
		item.cchTextMax = 256;
		item.pszText = (TCHAR*) buffer;
		::SendMessage( mHWnd, LVM_GETITEMTEXT, y, (LPARAM) (LV_ITEM*) &item );
		return CNSString::fromTChar( buffer );
	}

	CNSVector< int >& CNSVsList::getCurSel( )
	{
		static CNSVector< int > selection;
		selection.clear( );
		int rowIndex = ListView_GetNextItem( mHWnd, -1, LVNI_SELECTED );
		while ( rowIndex != -1 )
		{
			selection.pushback( rowIndex );
			rowIndex = ListView_GetNextItem( mHWnd, rowIndex, LVNI_SELECTED );
		}

		return selection;
	}

	void CNSVsList::onPostCreateWindow( CNSWindow* parent )
	{
		CNSWindow::onPostCreateWindow( parent );
		HWND header = ListView_GetHeader( mHWnd );
		mHeader = new CHeaderCtrl( mWindowID + "_header", header );

		WNDPROC oldProc = (WNDPROC) GetClassLongPtr( header, GCLP_WNDPROC );
		if ( oldProc == NULL )
			return;

		ATOM classAtom = (ATOM) GetClassLongPtr( header, GCW_ATOM );
		if ( classAtom == NULL )
			return;

		CNSWindow::mWndProc.insert( classAtom, oldProc );
		SetWindowLongPtr( header, GWLP_WNDPROC, (LONG_PTR) CNSWindow::windowProc );
		SetWindowLongPtr( header, GWLP_USERDATA, (LONG_PTR) mHeader );

		sWindows.insert( mHeader->mWindowID, mHeader );
		mHeader->registerMessage( WM_PAINT, CNSVsList::onHeaderRender );

		setBkColor( RGB( 37, 37, 38 ) );
		setTextColor( RGB( 241, 241, 241 ) );
		ListView_SetExtendedListViewStyle( mHWnd, LVS_EX_FULLROWSELECT );
		registerEvent( CNSVsList::EListCtrlEvent::EventCustomDraw, onListCustomDraw );
	}

	void CNSVsList::pushUserData( CNSLuaStack& luaStack, CNSVsList* ref )
	{
		static CNSVector< const luaL_Reg* > reg;
		reg.clear( );
		reg.pushback( NSWin32::NSWindow );
		reg.pushback( NSWin32::NSVsList );
		luaStack.pushNSWeakRef( ref, NS_VSLIST, reg );
	}

	void CNSVsList::popUserData( const CNSLuaStack& luaStack, CNSVsList*& ref )
	{
		ref = (CNSVsList*) luaStack.popNSWeakRef( NS_VSLIST );
	}
}