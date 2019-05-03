#include <nsbase.h>
namespace NSWin32
{
	HBRUSH CNSVsListBox::backBrush = NULL;
	HBRUSH CNSVsListBox::selectBrush = NULL;
	CNSVsListBox::CNSVsListBox( const CNSString& windowID ) : CNSWindow( windowID, NS_VSLISTBOX )
	{
	}

	void CNSVsListBox::init( )
	{
		backBrush = CreateSolidBrush( RGB( 27, 27, 28 ) );
		selectBrush = CreateSolidBrush( RGB( 0, 120, 215 ) );
		WNDCLASSEX wcex;
		wcex.cbSize = sizeof( WNDCLASSEX );
		wcex.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
		wcex.lpfnWndProc = CNSVsListBox::windowProc;
		wcex.cbClsExtra = 0;
		wcex.cbWndExtra = 0;
		wcex.hInstance = CNSWindow::instance;
		wcex.hIcon = NULL;
		wcex.hCursor = LoadCursor( NULL, IDC_ARROW );
		wcex.hbrBackground = backBrush;
		wcex.lpszMenuName = NULL;
		wcex.lpszClassName = _T( "VSListBoxInner" );
		wcex.hIconSm = NULL;
		ATOM classAtom = RegisterClassEx( &wcex );
		if ( classAtom == NULL )
		{
			DWORD errorCode = GetLastError( );
			static CNSString errorDesc;
			errorDesc.format( _UTF8( "win32函数[RegisterClassEx]调用失败，错误码: %d" ), errorCode );
			NSException( errorDesc );
		}

		CNSWindow::superClass( _T( "VSListBoxInner" ), WC_NS_VSLISTBOX );
	}

	void CNSVsListBox::exit( )
	{
		UnregisterClass( _T( "VSListBoxInner" ), CNSWindow::instance );
		UnregisterClass( WC_NS_VSLISTBOX, CNSWindow::instance );

		if ( backBrush != NULL )
			DeleteObject( backBrush );

		if ( selectBrush != NULL )
			DeleteObject( selectBrush );
	}

	LRESULT CNSVsListBox::windowProc( HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam )
	{
		try
		{
			CNSVsListBox* listBox = (CNSVsListBox*) CNSWindow::fromHWnd( wnd );
			if ( listBox == NULL )
				return DefWindowProc( wnd, msg, wParam, lParam );

			switch ( msg )
			{
				case WM_VSCROLL:
				{
					if ( LOWORD( wParam ) == SB_LINEDOWN )
					{
						SCROLLINFO si;
						si.cbSize = sizeof( si );
						si.fMask = SIF_POS;
						GetScrollInfo( listBox->getHWnd( ), SB_VERT, &si );

						si.fMask = SIF_POS;
						si.nPos = si.nPos + listBox->getItemHeight( );
						SetScrollInfo( listBox->getHWnd( ), SB_VERT, &si, FALSE );
					}

					if ( LOWORD( wParam ) == SB_LINEUP )
					{
						SCROLLINFO si;
						si.cbSize = sizeof( si );
						si.fMask = SIF_POS;
						GetScrollInfo( listBox->getHWnd( ), SB_VERT, &si );

						si.fMask = SIF_POS;
						si.nPos = si.nPos - listBox->getItemHeight( );
						SetScrollInfo( listBox->getHWnd( ), SB_VERT, &si, FALSE );
					}

					listBox->mNeedRedraw = true;
					InvalidateRect( listBox->getHWnd( ), NULL, FALSE );
					break;
				}
				case WM_HSCROLL:
				{
					if ( LOWORD( wParam ) == SB_LINEDOWN )
					{
						SCROLLINFO si;
						si.cbSize = sizeof( si );
						si.fMask = SIF_POS;
						GetScrollInfo( listBox->getHWnd( ), SB_HORZ, &si );

						si.fMask = SIF_POS;
						si.nPos = si.nPos + 1;
						SetScrollInfo( listBox->getHWnd( ), SB_HORZ, &si, FALSE );
					}

					if ( LOWORD( wParam ) == SB_LINEUP )
					{
						SCROLLINFO si;
						si.cbSize = sizeof( si );
						si.fMask = SIF_POS;
						GetScrollInfo( listBox->getHWnd( ), SB_HORZ, &si );

						si.fMask = SIF_POS;
						si.nPos = si.nPos - 1;
						SetScrollInfo( listBox->getHWnd( ), SB_HORZ, &si, FALSE );
					}

					listBox->mNeedRedraw = true;
					InvalidateRect( listBox->getHWnd( ), NULL, FALSE );
					break;
				}
				case WM_MOUSEWHEEL:
				{
					short delta = (short) HIWORD( wParam );
					int times = delta / WHEEL_DELTA;

					SCROLLINFO siv = { 0 };
					siv.cbSize = sizeof( siv );
					siv.fMask = SIF_POS;
					GetScrollInfo( listBox->mHWnd, SB_VERT, &siv );
					listBox->scrollVert( siv.nPos - times * 4 );
					break;
				}
				case WM_SIZE:
				{
					if ( listBox->mMemDC != NULL )
					{
						SelectObject( listBox->mMemDC, listBox->mOldFont );
						SelectObject( listBox->mMemDC, listBox->mOldMemBmp );
						DeleteDC( listBox->mMemDC );
					}

					if ( listBox->mMemBitmap != NULL )
						DeleteObject( listBox->mMemBitmap );

					int width = LOWORD( lParam );
					int height = HIWORD( lParam );
					HDC dc = GetDC( wnd );
					listBox->mMemDC = CreateCompatibleDC( dc );
					listBox->mMemBitmap = CreateCompatibleBitmap( dc, width, height );
					listBox->mOldMemBmp = (HBITMAP) SelectObject( listBox->mMemDC, listBox->mMemBitmap );
					listBox->mOldFont = (HFONT) SelectObject( listBox->mMemDC, CNSPreDefine::FB_FONT_BASE );

					listBox->mNeedRedraw = true;
					break;
				}
				case WM_KEYDOWN:
				{
					if ( wParam == VK_UP )
					{
						if ( listBox->mCurSel == NULL )
						{
							HLISTINDEX beginIndex = listBox->mItems.getHead( );
							CListBoxItem* item = &listBox->mItems.getValue( beginIndex );
							if ( item != NULL )
								listBox->setCurSel( item->mItemID );
							break;
						}

						HLISTINDEX index = listBox->mItems.findNodeIndex( listBox->mCurSel->mItemID );
						if ( index == NULL )
							break;

						listBox->mItems.getPrev( index );
						if ( index == NULL )
							break;

						CListBoxItem* item = &listBox->mItems.getValue( index );
						if ( item != NULL )
							listBox->setCurSel( item->mItemID );
					}

					if ( wParam == VK_DOWN )
					{
						if ( listBox->mCurSel == NULL )
						{
							HLISTINDEX beginIndex = listBox->mItems.getHead( );
							CListBoxItem* item = &listBox->mItems.getValue( beginIndex );
							if ( item != NULL )
								listBox->setCurSel( item->mItemID );
							break;
						}

						HLISTINDEX index = listBox->mItems.findNodeIndex( listBox->mCurSel->mItemID );
						if ( index == NULL )
							break;

						listBox->mItems.getNext( index );
						if ( index == NULL )
							break;

						CListBoxItem* item = &listBox->mItems.getValue( index );
						if ( item != NULL )
							listBox->setCurSel( item->mItemID );
					}
					break;
				}
				case WM_LBUTTONDOWN:
				{
					SCROLLINFO siv = { 0 };
					siv.cbSize = sizeof( siv );
					siv.fMask = SIF_POS;
					GetScrollInfo( listBox->mHWnd, SB_VERT, &siv );

					SCROLLINFO sih = { 0 };
					sih.cbSize = sizeof( sih );
					sih.fMask = SIF_POS;
					GetScrollInfo( listBox->mHWnd, SB_HORZ, &sih );

					RECT rc;
					listBox->getClientRect( rc );
					POINT pt = { LOWORD( lParam ) + sih.nPos, HIWORD( lParam ) + siv.nPos };
					HLISTINDEX beginIndex = listBox->mItems.getHead( );
					for ( ; beginIndex != NULL; listBox->mItems.getNext( beginIndex ) )
					{
						CListBoxItem* item = &listBox->mItems.getValue( beginIndex );
						if ( pt.y >= item->mRect.top && pt.y <= item->mRect.bottom )
						{
							listBox->setCurSel( item->mItemID );
							break;
						}
					}

					listBox->focus( );
					break;
				}
				case WM_PAINT:
				{
					if ( listBox->mNeedRedraw == true )
					{
						listBox->redraw( );
						listBox->mNeedRedraw = false;
					}

					PAINTSTRUCT ps;
					BeginPaint( wnd, &ps );
					int width = ps.rcPaint.right - ps.rcPaint.left;
					int height = ps.rcPaint.bottom - ps.rcPaint.top;
					BitBlt( ps.hdc, ps.rcPaint.left, ps.rcPaint.top, width, height, listBox->mMemDC, ps.rcPaint.left, ps.rcPaint.top, SRCCOPY );
					EndPaint( wnd, &ps );
					break;
				}
				case WM_DESTROY:
					DeleteDC( listBox->mMemDC );
					DeleteObject( listBox->mMemBitmap );
					DeleteObject( listBox->mBackBrush );
					break;
				default:
					return DefWindowProc( wnd, msg, wParam, lParam );
			}
		}
		catch ( CNSException& e )
		{
			NSLog::exception( _UTF8( "函数CNSVsListBox::windowProc发生异常\n错误描述: \n\t%s\nC++调用堆栈: \n%s" ), e.mErrorDesc, NSBase::NSFunction::getStackInfo( ).getBuffer( ) );
		}

		return 0;
	}

	int CNSVsListBox::addItem( const CNSString& text, int image, intptr_t userdata )
	{
		CNSVsListBox::CListBoxItem newItem( text, image, userdata );
		CNSVsListBox::CListBoxItem* item = &mItems.insert( newItem.mItemID, newItem );

		TCHAR* charText = CNSString::toTChar( item->mText );
		SIZE size;
		GetTextExtentPoint( mMemDC, charText, lstrlen( charText ), &size );
		item->mTextWidth = size.cx;

		mNeedRedraw = true;
		InvalidateRect( mHWnd, NULL, FALSE );
		return item->mItemID;
	}

	void CNSVsListBox::setTextColor( COLORREF color )
	{
		mTextColor = color;
		mNeedRedraw = true;
		InvalidateRect( mHWnd, NULL, FALSE );
	}

	void CNSVsListBox::setBkColor( COLORREF color )
	{
		if ( mBackBrush != NULL )
			DeleteObject( mBackBrush );

		mBackBrush = NULL;
		mBackColor = color;
		mNeedRedraw = true;
		InvalidateRect( mHWnd, NULL, FALSE );
	}

	void CNSVsListBox::setBorderColor( COLORREF color )
	{
		if ( mBorderPen != NULL )
			DeleteObject( mBorderPen );

		mBorderPen = NULL;
		mBorderColor = color;
		mNeedRedraw = true;
		InvalidateRect( mHWnd, NULL, FALSE );
	}

	void CNSVsListBox::deleteItem( int itemID )
	{
		HLISTINDEX beginIndex = mItems.findNodeIndex( itemID );
		if ( beginIndex == NULL )
			return;

		for ( ; beginIndex != NULL; mItems.getNext( beginIndex ) )
		{
			CNSVsListBox::CListBoxItem* item = &mItems.getValue( beginIndex );
			item->mNeedRecalc = true;
		}

		mItems.erase( itemID );
		mNeedRedraw = true;
		InvalidateRect( mHWnd, NULL, FALSE );
	}

	int CNSVsListBox::getCurSel( ) const
	{
		if ( mCurSel == NULL )
			return -1;

		return mCurSel->mItemID;
	}

	void CNSVsListBox::setCurSel( int itemID )
	{
		if ( mCurSel != NULL && mCurSel->mItemID == itemID )
			return;

		CListBoxItem* oldSel = mCurSel;
		mCurSel = mItems.get( itemID );
		if ( mCurSel == NULL )
			return;

		SCROLLINFO siv = { 0 };
		siv.cbSize = sizeof( siv );
		siv.fMask = SIF_POS;
		GetScrollInfo( mHWnd, SB_VERT, &siv );

		SCROLLINFO sih = { 0 };
		sih.cbSize = sizeof( sih );
		sih.fMask = SIF_POS;
		GetScrollInfo( mHWnd, SB_HORZ, &sih );

		int oldItemID = -1;
		int newItemID = -1;
		int oldMode = SetBkMode( mMemDC, TRANSPARENT );
		COLORREF oldClr = SetTextColor( mMemDC, mTextColor );

		RECT rc;
		getClientRect( rc );

		int cx, cy = 0;
		ImageList_GetIconSize( mImageList, &cx, &cy );
		if ( oldSel != NULL )
		{
			if ( mBackBrush == NULL )
				mBackBrush = CreateSolidBrush( mBackColor );

			RECT rcItem = oldSel->mRect;
			OffsetRect( &rcItem, -sih.nPos, -siv.nPos );

			rcItem.left = rc.left;
			rcItem.right = rc.right;
			if ( notifyDrawItem( sih.nPos, siv.nPos, cx, cy, oldSel ) == 0 )
			{
				FillRect( mMemDC, &rcItem, mBackBrush );
				drawItemImage( sih.nPos, siv.nPos, oldSel );
				drawItemText( sih.nPos, siv.nPos, oldSel );
			}

			InvalidateRect( mHWnd, &rcItem, FALSE );
			oldItemID = oldSel->mItemID;
		}

		if ( mCurSel != NULL )
		{
			RECT rcItem = mCurSel->mRect;
			OffsetRect( &rcItem, -sih.nPos, -siv.nPos );

			rcItem.left = rc.left;
			rcItem.right = rc.right;
			if ( notifyDrawItem( sih.nPos, siv.nPos, cx, cy, mCurSel ) == 0 )
			{
				FillRect( mMemDC, &rcItem, CNSVsListBox::selectBrush );
				drawItemImage( sih.nPos, siv.nPos, mCurSel );
				drawItemText( sih.nPos, siv.nPos, mCurSel );
			}

			InvalidateRect( mHWnd, &rcItem, FALSE );
			newItemID = mCurSel->mItemID;

			// 确保当前选中的item可见
			int clientHeight = rc.bottom - rc.top;
			if ( rcItem.top < 0 )
				scrollVert( mCurSel->mRect.top );
			if ( rcItem.bottom > clientHeight )
				scrollVert( mCurSel->mRect.bottom - clientHeight );
		}
		SetBkMode( mMemDC, oldClr );
		SetTextColor( mMemDC, oldMode );
		notifySelectChanged( oldItemID, newItemID );
	}

	void CNSVsListBox::setItemData( int itemID, intptr_t userdata )
	{
		CListBoxItem* item = mItems.get( itemID );
		if ( item == NULL )
			return;

		item->mUserData = userdata;
	}

	intptr_t CNSVsListBox::getItemData( int itemID ) const
	{
		const CListBoxItem* item = mItems.get( itemID );
		if ( item == NULL )
			return 0;

		return item->mUserData;
	}

	int CNSVsListBox::getItemCount( ) const
	{
		return mItems.getCount( );
	}

	int CNSVsListBox::getItemHeight( ) const
	{
		return mItemHeight;
	}

	void CNSVsListBox::setItemImage( int itemID, int imageIndex )
	{
		CListBoxItem* item = mItems.get( itemID );
		if ( item == NULL )
			return;

		item->mImage = imageIndex;

		SCROLLINFO siv;
		siv.cbSize = sizeof( siv );
		siv.fMask = SIF_POS;
		GetScrollInfo( mHWnd, SB_VERT, &siv );

		SCROLLINFO sih;
		sih.cbSize = sizeof( sih );
		sih.fMask = SIF_POS;
		GetScrollInfo( mHWnd, SB_HORZ, &sih );

		int cx, cy = 0;
		ImageList_GetIconSize( mImageList, &cx, &cy );
		RECT rcItem = item->mRect;
		OffsetRect( &rcItem, -sih.nPos, -siv.nPos );
		// 重新绘制缓冲区
		if ( notifyDrawItem( sih.nPos, siv.nPos, cx, cy, item ) == 0 )
		{
			RECT rc;
			getClientRect( rc );

			rcItem.left = rc.left;
			rcItem.right = rc.right;
			if ( item == mCurSel )
				FillRect( mMemDC, &rcItem, CNSVsListBox::selectBrush );
			else
				FillRect( mMemDC, &rcItem, mBackBrush );

			drawItemImage( sih.nPos, siv.nPos, item );
			drawItemText( sih.nPos, siv.nPos, item );
		}

		// 通知windows，将应用缓冲区提交到窗口缓冲区，窗口缓冲区不需要用背景笔刷重绘
		InvalidateRect( mHWnd, &rcItem, FALSE );
	}

	void CNSVsListBox::clear( )
	{
		mCurSel = NULL;
		mItems.clear( );
		mNeedRedraw = true;
		InvalidateRect( mHWnd, NULL, FALSE );
	}

	HIMAGELIST CNSVsListBox::getImageList( ) const
	{
		return mImageList;
	}

	HBRUSH CNSVsListBox::getBackBrush( ) const
	{
		if ( mBackBrush == NULL )
			mBackBrush = CreateSolidBrush( mBackColor );

		return mBackBrush;
	}

	void CNSVsListBox::setImageList( int cx, int cy, CNSVector< CNSString >& images )
	{
		if ( mImageList != NULL )
			ImageList_Destroy( mImageList );

		mImageList = ImageList_Create( cx, cy, ILC_MASK | ILC_COLOR24, 4, 2 );
		HDC dc = GetDC( mHWnd );
		for ( unsigned int i = 0; i < images.getCount( ); i ++ )
		{
			CImage* dataImage = CImage::loadPngImageBGRA( images[ i ] );
			if ( dataImage == NULL )
				continue;

			HBITMAP bitmapImage = dataImage->getHBitmap( dc );
			ImageList_Add( mImageList, bitmapImage, bitmapImage );
			DeleteObject( bitmapImage );
		}
		ReleaseDC( mHWnd, dc );
	}

	void CNSVsListBox::drawItemImage( int x, int y, CListBoxItem* item )
	{
		if ( item->mImage != -1 )
		{
			RECT rc = item->mImageRect;
			OffsetRect( &rc, -x, -y );
			ImageList_Draw( mImageList, item->mImage, mMemDC, rc.left, rc.top, ILD_TRANSPARENT );
		}
	}

	void CNSVsListBox::drawItemText( int x, int y, CListBoxItem* item )
	{
		int cx, cy = 0;
		ImageList_GetIconSize( mImageList, &cx, &cy );

		TCHAR* text = CNSString::toTChar( item->mText );
		RECT rcText = item->mRect;
		rcText.left += cx;
		rcText.right += x;
		OffsetRect( &rcText, -x, -y );
		DrawText( mMemDC, text, -1, &rcText, DT_LEFT | DT_VCENTER | DT_SINGLELINE );
	}

	void CNSVsListBox::notifyDeleteItem( int itemID )
	{
		CNSWindow* parent = getParent( );
		if ( parent != NULL )
		{
			VSListBoxDeleteItem deleteItem;
			deleteItem.hdr.code = EVsListBoxEvent::EventDeleteItem;
			deleteItem.hdr.idFrom = GetDlgCtrlID( mHWnd );
			deleteItem.hdr.hwndFrom = mHWnd;
			deleteItem.deleteItemID = itemID;
			SendMessage( parent->getHWnd( ), WM_NOTIFY, deleteItem.hdr.idFrom, (LPARAM) &deleteItem );
		}
	}

	void CNSVsListBox::notifySelectChanged( int oldItemID, int newItemID )
	{
		CNSWindow* parent = getParent( );
		if ( parent != NULL )
		{
			VSListBoxSelectChanged selectChanged;
			selectChanged.hdr.code = EVsListBoxEvent::EventSelectChanged;
			selectChanged.hdr.idFrom = GetDlgCtrlID( mHWnd );
			selectChanged.hdr.hwndFrom = mHWnd;
			selectChanged.oldItemID = oldItemID;
			selectChanged.newItemID = newItemID;
			SendMessage( parent->getHWnd( ), WM_NOTIFY, selectChanged.hdr.idFrom, (LPARAM) &selectChanged );
		}
	}

	int CNSVsListBox::notifyCalcItem( int imageCX, int imageCY, CListBoxItem* item )
	{
		CNSWindow* parent = getParent( );
		if ( parent != NULL )
		{
			VSListBoxCalcItem calcItem;
			calcItem.hdr.code = EVsListBoxEvent::EventCalcItem;
			calcItem.hdr.idFrom = GetDlgCtrlID( mHWnd );
			calcItem.hdr.hwndFrom = mHWnd;
			calcItem.item = item;
			calcItem.dc = mMemDC;
			calcItem.mImageCY = imageCY;
			calcItem.mImageCX = imageCX;
			return (int) SendMessage( parent->getHWnd( ), WM_NOTIFY, calcItem.hdr.idFrom, (LPARAM) &calcItem );
		}

		return 0;
	}

	int CNSVsListBox::notifyDrawItem( int x, int y, int imageCX, int imageCY, CListBoxItem* item )
	{
		CNSWindow* parent = getParent( );
		if ( parent != NULL )
		{
			VSListBoxDrawItem drawItem;
			drawItem.hdr.code = EVsListBoxEvent::EventDrawItem;
			drawItem.hdr.idFrom = GetDlgCtrlID( mHWnd );
			drawItem.hdr.hwndFrom = mHWnd;
			drawItem.item = item;
			drawItem.x = x;
			drawItem.y = y;
			drawItem.mImageCX = imageCX;
			drawItem.mImageCY = imageCY;
			drawItem.dc = mMemDC;
			return ( int ) ::SendMessage( parent->getHWnd( ), WM_NOTIFY, drawItem.hdr.idFrom, (LPARAM) &drawItem );
		}

		return 0;
	}

	void CNSVsListBox::redraw( )
	{
		TEXTMETRIC tm;
		GetTextMetrics( mMemDC, &tm );

		int cx, cy = 0;
		ImageList_GetIconSize( mImageList, &cx, &cy );

		int width = 0;
		int height = 0;
		HLISTINDEX beginIndex = mItems.getHead( );
		for ( ; beginIndex != NULL; mItems.getNext( beginIndex ) )
		{
			CListBoxItem* item = &mItems.getValue( beginIndex );
			if ( item->mNeedRecalc == true )
			{
				item->mNeedRecalc = false;
				if ( notifyCalcItem( cx, cy, item ) == 0 )
				{
					int left = 0;
					int right = item->mTextWidth + cx;
					int top = 0;
					int bottom = mItemHeight;
					item->mRect.left = left;
					item->mRect.right = right;
					item->mRect.top = top;
					item->mRect.bottom = bottom;

					int imageTop = top + ( ( mItemHeight - cy ) >> 1 );
					item->mImageRect.left = 0;
					item->mImageRect.right = cx;
					item->mImageRect.top = imageTop;
					item->mImageRect.bottom = imageTop + cy;
				}

				OffsetRect( &item->mRect, 0, height );
				OffsetRect( &item->mImageRect, 0, height );
			}

			height = item->mRect.bottom;
			width = max( width, item->mRect.right );
		}

		RECT rc;
		getClientRect( rc );
		int clientHeight = rc.bottom - rc.top;
		int clientWidth = rc.right - rc.left;
		bool showScrollV = false;
		bool showScrollH = false;
		if ( height > clientHeight && width > clientWidth )
		{
			showScrollV = true;
			showScrollH = true;
		}
		else if ( height > clientHeight )
			showScrollV = true;
		else if ( width > clientWidth )
			showScrollH = true;

		if ( showScrollV == true )
		{
			SCROLLINFO si;
			si.cbSize = sizeof( si );
			si.fMask = SIF_RANGE | SIF_PAGE;
			si.nMin = 0;
			si.nMax = height;
			si.nPage = clientHeight;
			SetScrollInfo( mHWnd, SB_VERT, &si, FALSE );
		}

		if ( showScrollH == true )
		{
			SCROLLINFO si;
			si.cbSize = sizeof( si );
			si.fMask = SIF_RANGE | SIF_PAGE;
			si.nMin = 0;
			si.nMax = width;
			si.nPage = clientWidth;
			SetScrollInfo( mHWnd, SB_HORZ, &si, FALSE );
		}

		if ( showScrollV == true || showScrollH == true )
		{
			HDC dc = GetWindowDC( mHWnd );
			redrawScrollbar( dc );
			ReleaseDC( mHWnd, dc );
		}

		bool needRedraw = false;
		SCROLLBARINFO sbi;
		sbi.cbSize = sizeof( SCROLLBARINFO );
		GetScrollBarInfo( mHWnd, OBJID_VSCROLL, &sbi );
		if ( showScrollV == true && sbi.rgstate[ 0 ] & STATE_SYSTEM_INVISIBLE )
		{
			ShowScrollBar( mHWnd, SB_VERT, TRUE );
			needRedraw = true;
		}
		else if ( showScrollV == false && !( sbi.rgstate[ 0 ] & STATE_SYSTEM_INVISIBLE ) )
		{
			ShowScrollBar( mHWnd, SB_VERT, FALSE );
			needRedraw = true;
		}

		sbi.cbSize = sizeof( SCROLLBARINFO );
		GetScrollBarInfo( mHWnd, OBJID_HSCROLL, &sbi );
		if ( showScrollH == true && sbi.rgstate[ 0 ] & STATE_SYSTEM_INVISIBLE )
		{
			ShowScrollBar( mHWnd, SB_HORZ, TRUE );
			needRedraw = true;
		}
		else if ( showScrollH == false && !( sbi.rgstate[ 0 ] & STATE_SYSTEM_INVISIBLE ) )
		{
			ShowScrollBar( mHWnd, SB_HORZ, FALSE );
			needRedraw = true;
		}

		if ( needRedraw == true )
		{
			redraw( );
			return;
		}

		if ( mBackBrush == NULL )
			mBackBrush = CreateSolidBrush( mBackColor );

		// 填充背景
		FillRect( mMemDC, &rc, mBackBrush );

		SCROLLINFO siv = { 0 };
		siv.cbSize = sizeof( siv );
		siv.fMask = SIF_POS;
		GetScrollInfo( mHWnd, SB_VERT, &siv );

		SCROLLINFO sih = { 0 };
		sih.cbSize = sizeof( sih );
		sih.fMask = SIF_POS;
		GetScrollInfo( mHWnd, SB_HORZ, &sih );

		int oldMode = SetBkMode( mMemDC, TRANSPARENT );
		COLORREF oldClr = SetTextColor( mMemDC, mTextColor );
		beginIndex = mItems.getHead( );
		for ( int start = 0; beginIndex != NULL; mItems.getNext( beginIndex ) )
		{
			CListBoxItem* item = &mItems.getValue( beginIndex );
			RECT rcItem = item->mRect;
			OffsetRect( &rcItem, -sih.nPos, -siv.nPos );
			if ( rcItem.bottom < 0 )
				continue;

			if ( rcItem.top > clientHeight )
				break;

			if ( notifyDrawItem( sih.nPos, siv.nPos, cx, cy, item ) == 0 )
			{
				if ( item == mCurSel )
				{
					RECT rc;
					getClientRect( rc );
					rcItem.left = rc.left;
					rcItem.right = rc.right;
					FillRect( mMemDC, &rcItem, CNSVsListBox::selectBrush );
				}

				drawItemImage( sih.nPos, siv.nPos, item );
				drawItemText( sih.nPos, siv.nPos, item );
			}
		}
		SetTextColor( mMemDC, oldClr );
		SetBkMode( mMemDC, oldMode );
	}

	void CNSVsListBox::onPostCreateWindow( CNSWindow* parent )
	{
		CNSWindow::onPostCreateWindow( parent );
		RECT rc;
		GetClientRect( mHWnd, &rc );

		int width = rc.right - rc.left;
		int height = rc.bottom - rc.top;
		HDC dc = GetDC( mHWnd );
		mMemDC = CreateCompatibleDC( dc );
		mMemBitmap = CreateCompatibleBitmap( dc, width, height );
		mOldMemBmp = (HBITMAP) SelectObject( mMemDC, mMemBitmap );
		mOldFont = (HFONT) SelectObject( mMemDC, CNSPreDefine::FB_FONT_BASE );
	}

	void CNSVsListBox::pushUserData( CNSLuaStack& luaStack, CNSVsListBox* ref )
	{
		static CNSVector< const luaL_Reg* > reg;
		reg.clear( );
		reg.pushback( NSWin32::NSWindow );
		reg.pushback( NSWin32::NSVsListBox );
		luaStack.pushNSWeakRef( ref, NS_VSLISTBOX, reg );
	}

	void CNSVsListBox::popUserData( const CNSLuaStack& luaStack, CNSVsListBox*& ref )
	{
		ref = (CNSVsListBox*) luaStack.popNSWeakRef( NS_VSLISTBOX );
	}
}