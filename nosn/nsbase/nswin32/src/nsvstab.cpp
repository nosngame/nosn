#include <nsbase.h>
namespace NSWin32
{
	// ±³¾°±ÊË¢
	HBRUSH backBrush = NULL;

	HBRUSH workBackBrush = NULL;

	// Ñ¡ÖÐµÄtabÏî±ÊË¢
	HBRUSH selectItemBrush = NULL;

	// ÐüÍ£µÄtabÏî±ÊË¢
	HBRUSH hoverItemBrush = NULL;

	// ÐüÍ£µÄtabClose°´Å¥±ÊË¢
	HBRUSH hoverCloseBrush = NULL;

	// µã»÷tabClose°´Å¥±ÊË¢
	HBRUSH clickCloseBrush = NULL;

	// ¹Ø±Õ°´Å¥µÄpen
	HPEN closePen = NULL;

	// ±³¾°±ÊË¢
	HBRUSH backViewBrush = NULL;

	// ViewTabÀàÐÍµÄ±ß¿ò
	HPEN viewBorderPen = NULL;

	HPEN selectViewPen = NULL;

	HBRUSH hoverViewBrush = NULL;

	HBRUSH selectViewBrush = NULL;
	void CNSVsTab::init( )
	{
		backBrush = CreateSolidBrush( RGB( 45, 45, 48 ) );
		selectViewBrush = CreateSolidBrush( RGB( 37, 37, 38 ) );
		workBackBrush = CreateSolidBrush( RGB( 63, 63, 70 ) );
		selectItemBrush = CreateSolidBrush( RGB( 0, 120, 215 ) );
		hoverCloseBrush = CreateSolidBrush( RGB( 82, 176, 239 ) );
		clickCloseBrush = CreateSolidBrush( RGB( 14, 97, 152 ) );
		hoverItemBrush = CreateSolidBrush( RGB( 28, 151, 234 ) );
		hoverViewBrush = CreateSolidBrush( RGB( 63, 63, 70 ) );
		closePen = CreatePen( PS_SOLID, 1, RGB( 244, 243, 230 ) );
		viewBorderPen = CreatePen( PS_SOLID, 1, RGB( 63, 63, 70 ) );

		WNDCLASSEX wcex;
		wcex.cbSize = sizeof( WNDCLASSEX );
		wcex.style = CS_HREDRAW | CS_VREDRAW;
		wcex.lpfnWndProc = CNSVsTab::windowProc;
		wcex.cbClsExtra = 0;
		wcex.cbWndExtra = 0;
		wcex.hInstance = CNSWindow::instance;
		wcex.hIcon = NULL;
		wcex.hCursor = LoadCursor( NULL, IDC_ARROW );
		wcex.hbrBackground = backBrush;
		wcex.lpszMenuName = NULL;
		wcex.lpszClassName = _T( "VSTabInner" );
		wcex.hIconSm = NULL;
		ATOM classAtom = RegisterClassEx( &wcex );
		if ( classAtom == NULL )
		{
			DWORD errorCode = GetLastError( );
			static CNSString errorDesc;
			errorDesc.format( _UTF8( "win32º¯Êý[RegisterClassEx]µ÷ÓÃÊ§°Ü£¬´íÎóÂë: %d" ), errorCode );
			NSException( errorDesc );
		}

		CNSWindow::superClass( _T( "VsTabInner" ), WC_NS_VSTAB );
	}

	void CNSVsTab::exit( )
	{
		UnregisterClass( _T( "VSTabInner" ), CNSWindow::instance );
		UnregisterClass( WC_NS_VSTAB, CNSWindow::instance );

		if ( backBrush != NULL )
			DeleteObject( backBrush );

		if ( selectViewBrush != NULL )
			DeleteObject( selectViewBrush );

		if ( workBackBrush != NULL )
			DeleteObject( workBackBrush );

		if ( selectItemBrush != NULL )
			DeleteObject( selectItemBrush );

		if ( hoverCloseBrush != NULL )
			DeleteObject( hoverCloseBrush );

		if ( clickCloseBrush != NULL )
			DeleteObject( clickCloseBrush );

		if ( hoverItemBrush != NULL )
			DeleteObject( hoverItemBrush );

		if ( closePen != NULL )
			DeleteObject( closePen );

		if ( viewBorderPen != NULL )
			DeleteObject( viewBorderPen );

		if ( hoverViewBrush != NULL )
			DeleteObject( hoverViewBrush );
	}

	// Visual studio Style File TabControl ´°¿Ú»Øµ÷º¯Êý
	LRESULT CNSVsTab::windowProc( HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam )
	{
		try
		{
			CNSVsTab* tab = (CNSVsTab*) CNSWindow::fromHWnd( wnd );
			if ( tab == NULL )
				return DefWindowProc( wnd, msg, wParam, lParam );

			switch ( msg )
			{
				case WM_WINDOWPOSCHANGING:
				{
					break;
				}
				case WM_SIZE:
				{
					if ( tab->mMemDC != NULL )
					{
						SelectObject( tab->mMemDC, tab->mOldFont );
						SelectObject( tab->mMemDC, tab->mOldMemBmp );
						DeleteDC( tab->mMemDC );
					}

					if ( tab->mMemBitmap != NULL )
						DeleteObject( tab->mMemBitmap );

					int width = LOWORD( lParam );
					int height = HIWORD( lParam );
					HDC dc = GetDC( wnd );
					tab->mMemDC = CreateCompatibleDC( dc );
					tab->mMemBitmap = CreateCompatibleBitmap( dc, width, height );
					tab->mOldMemBmp = (HBITMAP) SelectObject( tab->mMemDC, tab->mMemBitmap );
					tab->mOldFont = (HFONT) SelectObject( tab->mMemDC, CNSPreDefine::FB_FONT_BASE );

					tab->mNeedRedraw = true;
					InvalidateRect( tab->mHWnd, NULL, FALSE );
					break;
				}
				case WM_PAINT:
				{
					if ( tab->mNeedRedraw == true )
					{
						tab->redraw( );
						tab->mNeedRedraw = false;
					}

					PAINTSTRUCT ps;
					BeginPaint( wnd, &ps );
					int width = ps.rcPaint.right - ps.rcPaint.left;
					int height = ps.rcPaint.bottom - ps.rcPaint.top;
					BitBlt( ps.hdc, ps.rcPaint.left, ps.rcPaint.top, width, height, tab->mMemDC, ps.rcPaint.left, ps.rcPaint.top, SRCCOPY );
					EndPaint( wnd, &ps );
					break;
				}
				case WM_LBUTTONDOWN:
				{
					POINT pt = { LOWORD( lParam ), HIWORD( lParam ) };

					HLISTINDEX beginIndex = tab->mItems.getHead( );
					for ( ; beginIndex != NULL; tab->mItems.getNext( beginIndex ) )
					{
						CNSVsTab::CTabItem* item = &tab->mItems.getValue( beginIndex );

						RECT& rcItem = item->mRect;
						RECT& rcClose = item->mCloseRect;
						if ( PtInRect( &rcItem, pt ) == TRUE )
						{
							if ( item == tab->curSel )
							{
								if ( PtInRect( &rcClose, pt ) == TRUE )
								{
									tab->clickClose = item;
									tab->drawItem( item, true );
									SetCapture( wnd );
								}
							}
							else
							{
								if ( PtInRect( &rcClose, pt ) == TRUE )
								{
									tab->clickClose = item;
									tab->drawItem( item, true );
									SetCapture( wnd );
								}
								else
								{
									CTabItem* oldSel = tab->curSel;
									tab->curSel = item;
									if ( oldSel != NULL )
										tab->drawItem( oldSel, true );

									tab->drawItem( item, true );
									int oldSelID = oldSel == NULL ? -1 : oldSel->mItemID;
									tab->notifySelect( oldSelID, item->mItemID );
								}
							}
							break;
						}
					}
					break;
				}
				case WM_MOUSEMOVE:
				{
					POINT pt = { LOWORD( lParam ), HIWORD( lParam ) };
					CTabItem* hover = NULL;
					HLISTINDEX beginIndex = tab->mItems.getHead( );
					for ( ; beginIndex != NULL; tab->mItems.getNext( beginIndex ) )
					{
						CTabItem* item = (CTabItem*) &tab->mItems.getValue( beginIndex );
						if ( PtInRect( &item->mRect, pt ) == TRUE )
						{
							TRACKMOUSEEVENT tme;
							tme.cbSize = sizeof( TRACKMOUSEEVENT );
							tme.dwFlags = TME_LEAVE;
							tme.hwndTrack = tab->getHWnd( );
							tme.dwHoverTime = HOVER_DEFAULT;
							TrackMouseEvent( &tme );
							hover = item;
							break;
						}
					}

					// Èç¹ûÓÐÐüÍ£
					if ( hover != NULL )
					{
						bool needRedraw = false;
						RECT& rcClose = hover->mCloseRect;
						if ( PtInRect( &rcClose, pt ) == TRUE )
						{
							if ( tab->hoverClose != hover )
							{
								tab->hoverClose = hover;
								tab->drawItem( hover, true );
							}
						}
						else
						{
							if ( tab->hoverClose == hover )
							{
								tab->hoverClose = NULL;
								tab->drawItem( hover, true );
							}
						}
					}
					else if ( tab->hoverClose != NULL )
					{
						CTabItem* oldHoverClose = tab->hoverClose;
						tab->hoverClose = NULL;
						tab->drawItem( oldHoverClose, true );
					}

					if ( tab->hoverItem != hover )
					{
						CTabItem* oldHover = tab->hoverItem;
						tab->hoverItem = hover;
						tab->drawItem( oldHover, true );
						tab->drawItem( hover, true );
					}
					break;
				}
				case WM_MOUSELEAVE:
				{
					CTabItem* tempItem = tab->hoverItem;
					tab->hoverItem = NULL;
					tab->drawItem( tempItem, true );
					break;
				}
				case WM_LBUTTONUP:
					if ( tab->clickClose != NULL )
					{
						ReleaseCapture( );

						POINT pt = { LOWORD( lParam ), HIWORD( lParam ) };
						RECT& rcClose = tab->clickClose->mCloseRect;
						if ( PtInRect( &rcClose, pt ) == TRUE )
							tab->notifyClose( tab->clickClose->mItemID );

						CTabItem* tempItem = tab->clickClose;
						tab->hoverClose = NULL;
						tab->clickClose = NULL;
						tab->drawItem( tempItem, true );
					}
					break;
				case WM_DESTROY:
				{
					SelectObject( tab->mMemDC, tab->mOldMemBmp );
					DeleteDC( tab->mMemDC );
					DeleteObject( tab->mMemBitmap );
					break;
				}
				default:
					return DefWindowProc( wnd, msg, wParam, lParam );
			}
		}
		catch ( CNSException& e )
		{
			NSLog::exception( _UTF8( "º¯ÊýCNSVsTab::windowProc·¢ÉúÒì³£\n´íÎóÃèÊö: \n\t%s\nC++µ÷ÓÃ¶ÑÕ»: \n%s" ), e.mErrorDesc, NSBase::NSFunction::getStackInfo( ).getBuffer( ) );
		}

		return 0;
	}

	CNSVsTab::CNSVsTab( const CNSString& windowID, EVsTabStyle tabType ) : CNSWindow( windowID, NS_VSTAB ), mTabType( tabType )
	{
	}

	void CNSVsTab::onPostCreateWindow( CNSWindow* parent )
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

	int CNSVsTab::addItem( const CNSString& text, int imageIndex, intptr_t userdata )
	{
		static int sTabItemID = 1;
		CNSVsTab::CTabItem newItem( sTabItemID ++, text, imageIndex, userdata );
		CNSVsTab::CTabItem* item = &mItems.insert( newItem.mItemID, newItem );

		mNeedRedraw = true;
		InvalidateRect( mHWnd, NULL, FALSE );
		return item->mItemID;
	}

	void CNSVsTab::deleteItem( int itemID )
	{
		if ( clickClose != NULL && clickClose->mItemID == itemID )
			clickClose = NULL;

		if ( hoverClose != NULL && hoverClose->mItemID == itemID )
			hoverClose = NULL;

		if ( hoverItem != NULL && hoverItem->mItemID == itemID )
			hoverItem = NULL;

		int curItemID = getCurSel( );
		mItems.erase( itemID );
		if ( itemID == curItemID )
		{
			if ( mItems.getCount( ) > 0 )
			{
				HLISTINDEX beginIndex = mItems.getHead( );
				curSel = &mItems.getValue( beginIndex );
				notifySelect( curItemID, curSel->mItemID );
			}
			else
			{
				curSel = NULL;
				notifySelect( curItemID, -1 );
			}
		}

		mNeedRedraw = true;
		InvalidateRect( mHWnd, NULL, FALSE );
	}

	int CNSVsTab::calcItemRect( CNSVsTab::CTabItem* item, int start )
	{
		TEXTMETRIC tm;
		GetTextMetrics( mMemDC, &tm );
		TCHAR* text = CNSString::toTChar( item->mName );

		SIZE size;
		GetTextExtentPoint( mMemDC, text, lstrlen( text ), &size );

		if ( mTabType == TAB_FILETYPE )
		{
			int left = start;
			int right = start + size.cx + 30;
			int top = 0;
			int bottom = tm.tmHeight + 5;
			item->mRect.left = left;
			item->mRect.right = right;
			item->mRect.top = top;
			item->mRect.bottom = bottom;

			item->mCloseRect.top = top + 5;
			item->mCloseRect.bottom = bottom - 5;
			item->mCloseRect.left = right - ( item->mCloseRect.bottom - item->mCloseRect.top ) - 3;
			item->mCloseRect.right = right - 3;
			return right;
		}
		else if ( mTabType == TAB_VIEWTYPE_UP )
		{
			int cx, cy = 0;
			ImageList_GetIconSize( mImageList, &cx, &cy );

			int left = start;
			int right = start + size.cx + cx + 10;
			int top = 0;
			int bottom = tm.tmHeight + 5;
			item->mRect.left = left;
			item->mRect.right = right;
			item->mRect.top = top;
			item->mRect.bottom = bottom;
			return right;
		}
		else if ( mTabType == TAB_VIEWTYPE_DOWN )
		{
			RECT rc;
			getClientRect( rc );

			int cx, cy = 0;
			ImageList_GetIconSize( mImageList, &cx, &cy );

			int left = start;
			int right = start + size.cx + cx + 10;
			int top = rc.bottom - tm.tmHeight - 5;
			int bottom = rc.bottom;
			item->mRect.left = left;
			item->mRect.right = right;
			item->mRect.top = top;
			item->mRect.bottom = bottom;
			return right;
		}

		return 0;
	}

	void CNSVsTab::redraw( )
	{
		HLISTINDEX beginIndex = mItems.getHead( );
		for ( int start = 0; beginIndex != NULL; mItems.getNext( beginIndex ) )
		{
			CNSVsTab::CTabItem* item = ( CNSVsTab::CTabItem* ) &mItems.getValue( beginIndex );
			start = calcItemRect( item, start );
		}

		// Ìî³ä±³¾°
		RECT rc;
		getClientRect( rc );
		FillRect( mMemDC, &rc, backBrush );

		RECT workRect;
		getWorkRect( workRect );
		InflateRect( &workRect, 1, 1 );
		if ( mItems.getCount( ) > 0 )
			FillRect( mMemDC, &workRect, workBackBrush );

		beginIndex = mItems.getHead( );
		for ( ; beginIndex != NULL; mItems.getNext( beginIndex ) )
		{
			CNSVsTab::CTabItem* item = ( CNSVsTab::CTabItem* ) &mItems.getValue( beginIndex );
			drawItem( item );
		}

		if ( mTabType == TAB_FILETYPE && mItems.getCount( ) > 0 )
		{
			RECT rcHeader;
			getHeaderRect( rcHeader );
			rcHeader.top = rcHeader.bottom - 2;
			FillRect( mMemDC, &rcHeader, selectItemBrush );
		}
	}

	void CNSVsTab::getWorkRect( RECT& workRect ) const
	{
		TEXTMETRIC tm;
		GetTextMetrics( mMemDC, &tm );
		if ( mTabType == TAB_FILETYPE )
		{
			int itemHeight = tm.tmHeight + 5;
			int headerHeight = itemHeight + 2;

			GetClientRect( mHWnd, &workRect );
			workRect.top = headerHeight + 1;
			workRect.right = workRect.right - 1;
			workRect.left = workRect.left + 1;
		}
		else if ( mTabType == TAB_VIEWTYPE_UP )
		{
			GetClientRect( mHWnd, &workRect );
			workRect.top = tm.tmHeight + 5 + 1;
			workRect.right = workRect.right - 1;
			workRect.left = workRect.left + 1;
			workRect.bottom = workRect.bottom - 1;
		}
		else if ( mTabType == TAB_VIEWTYPE_DOWN )
		{
			GetClientRect( mHWnd, &workRect );
			workRect.top = workRect.top + 1;
			workRect.bottom = workRect.bottom - tm.tmHeight - 5 - 1;
			workRect.right = workRect.right - 1;
			workRect.left = workRect.left + 1;
		}
	}

	void CNSVsTab::getHeaderRect( RECT& rcHeader ) const
	{
		TEXTMETRIC tm;
		GetTextMetrics( mMemDC, &tm );

		if ( mTabType == TAB_FILETYPE )
		{
			int itemHeight = tm.tmHeight + 5;
			int headerHeight = itemHeight + 2;

			GetClientRect( mHWnd, &rcHeader );
			rcHeader.bottom = headerHeight;
		}
		else if ( mTabType == TAB_VIEWTYPE_UP )
		{
			GetClientRect( mHWnd, &rcHeader );
			rcHeader.bottom = tm.tmHeight + 5;
		}
		else if ( mTabType == TAB_VIEWTYPE_DOWN )
		{
			GetClientRect( mHWnd, &rcHeader );
			rcHeader.top = rcHeader.bottom - tm.tmHeight - 5;
		}
	}

	void CNSVsTab::notifySelect( int oldIndex, int newIndex )
	{
		CNSWindow* parent = getParent( );
		if ( parent != NULL )
		{
			VSTabSelectChanged closeItem;
			closeItem.hdr.code = EVsTabCtrlEvent::EventSelectChanged;
			closeItem.hdr.idFrom = GetDlgCtrlID( mHWnd );
			closeItem.hdr.hwndFrom = mHWnd;
			closeItem.newItemID = newIndex;
			closeItem.oldItemID = oldIndex;
			SendMessage( parent->getHWnd( ), WM_NOTIFY, closeItem.hdr.idFrom, (LPARAM) &closeItem );
		}
	}

	void CNSVsTab::notifyClose( int itemID )
	{
		CNSWindow* parent = getParent( );
		if ( parent != NULL )
		{
			VSTabClose closeItem;
			closeItem.hdr.code = EVsTabCtrlEvent::EventClose;
			closeItem.hdr.idFrom = GetDlgCtrlID( mHWnd );
			closeItem.hdr.hwndFrom = mHWnd;
			closeItem.deleteItemID = itemID;
			SendMessage( parent->getHWnd( ), WM_NOTIFY, closeItem.hdr.idFrom, (LPARAM) &closeItem );
		}
	}

	void CNSVsTab::drawItem( CTabItem* item, bool redraw )
	{
		if ( item == NULL )
			return;

		if ( mTabType == TAB_FILETYPE )
		{
			HPEN		oldPen = (HPEN) SelectObject( mMemDC, closePen );
			COLORREF	oldTextClr = SetTextColor( mMemDC, RGB( 255, 255, 255 ) );
			int			oldMode = SetBkMode( mMemDC, TRANSPARENT );

			RECT rcItem = item->mRect;
			RECT rcClose = item->mCloseRect;

			RECT rcText = rcItem;
			InflateRect( &rcText, -3, -3 );

			TCHAR* text = CNSString::toTChar( item->mName );
			if ( item == curSel )
			{
				FillRect( mMemDC, &rcItem, selectItemBrush );

				if ( hoverClose == item )
					FillRect( mMemDC, &rcClose, hoverItemBrush );

				if ( clickClose == item )
					FillRect( mMemDC, &rcClose, clickCloseBrush );

				MoveToEx( mMemDC, rcClose.left + 3, rcClose.top + 3, NULL );
				LineTo( mMemDC, rcClose.right - 2, rcClose.bottom - 2 );
				MoveToEx( mMemDC, rcClose.left + 3, rcClose.bottom - 3, NULL );
				LineTo( mMemDC, rcClose.right - 2, rcClose.top + 2 );
			}
			else if ( item == hoverItem )
			{
				FillRect( mMemDC, &rcItem, hoverItemBrush );

				if ( hoverClose == item )
					FillRect( mMemDC, &rcClose, hoverCloseBrush );

				if ( clickClose == item )
					FillRect( mMemDC, &rcClose, clickCloseBrush );

				MoveToEx( mMemDC, rcClose.left + 3, rcClose.top + 3, NULL );
				LineTo( mMemDC, rcClose.right - 2, rcClose.bottom - 2 );
				MoveToEx( mMemDC, rcClose.left + 3, rcClose.bottom - 3, NULL );
				LineTo( mMemDC, rcClose.right - 2, rcClose.top + 2 );
			}
			else
				FillRect( mMemDC, &rcItem, backBrush );

			DrawText( mMemDC, text, -1, &rcText, DT_LEFT | DT_VCENTER | DT_SINGLELINE );
			SelectObject( mMemDC, oldPen );
			SetBkMode( mMemDC, oldMode );
			SetTextColor( mMemDC, oldTextClr );

			if ( redraw == true )
				InvalidateRect( mHWnd, &rcItem, FALSE );
		}
		else
		{
			COLORREF	oldTextClr;
			int			oldMode = SetBkMode( mMemDC, TRANSPARENT );
			int			cx, cy = 0;
			ImageList_GetIconSize( mImageList, &cx, &cy );

			RECT rcItem = item->mRect;
			if ( item == curSel )
			{
				RECT rc = rcItem;
				POINT pt[ 4 ] = { POINT { 0, 0 } };
				if ( mTabType == TAB_VIEWTYPE_UP )
				{
					rc.bottom ++;
					pt[ 0 ] = { rcItem.left, rcItem.bottom + 1 };
					pt[ 1 ] = { rcItem.left, rcItem.top };
					pt[ 2 ] = { rcItem.right - 1, rcItem.top };
					pt[ 3 ] = { rcItem.right - 1, rcItem.bottom + 1 };
				}
				else if ( mTabType == TAB_VIEWTYPE_DOWN )
				{
					rc.top --;
					pt[ 0 ] = { rcItem.left, rcItem.top - 1 };
					pt[ 1 ] = { rcItem.left, rcItem.bottom - 1 };
					pt[ 2 ] = { rcItem.right - 1, rcItem.bottom - 1 };
					pt[ 3 ] = { rcItem.right - 1, rcItem.top - 2 };
				}

				FillRect( mMemDC, &rc, selectViewBrush );
				HPEN oldPen = (HPEN) SelectObject( mMemDC, viewBorderPen );
				MoveToEx( mMemDC, pt[ 0 ].x, pt[ 0 ].y, NULL );
				LineTo( mMemDC, pt[ 1 ].x, pt[ 1 ].y );
				LineTo( mMemDC, pt[ 2 ].x, pt[ 2 ].y );
				LineTo( mMemDC, pt[ 3 ].x, pt[ 3 ].y );

				SelectObject( mMemDC, oldPen );
				oldTextClr = SetTextColor( mMemDC, RGB( 0, 151, 251 ) );
			}
			else if ( item == hoverItem )
			{
				HPEN oldPen = (HPEN) SelectObject( mMemDC, viewBorderPen );
				if ( mTabType == TAB_VIEWTYPE_UP )
				{
					MoveToEx( mMemDC, rcItem.left, rcItem.bottom, NULL );
					LineTo( mMemDC, rcItem.right - 1, rcItem.bottom );
				}
				if ( mTabType == TAB_VIEWTYPE_DOWN )
				{
					MoveToEx( mMemDC, rcItem.left, rcItem.top - 1, NULL );
					LineTo( mMemDC, rcItem.right - 1, rcItem.top - 1 );
				}
				SelectObject( mMemDC, oldPen );

				FillRect( mMemDC, &rcItem, hoverViewBrush );
				oldTextClr = SetTextColor( mMemDC, RGB( 85, 170, 255 ) );
			}
			else
			{
				FillRect( mMemDC, &rcItem, backBrush );
				oldTextClr = SetTextColor( mMemDC, RGB( 208, 208, 208 ) );

				HPEN oldPen = (HPEN) SelectObject( mMemDC, viewBorderPen );
				if ( mTabType == TAB_VIEWTYPE_UP )
				{
					MoveToEx( mMemDC, rcItem.left, rcItem.bottom, NULL );
					LineTo( mMemDC, rcItem.right - 1, rcItem.bottom );
				}
				else if ( mTabType == TAB_VIEWTYPE_DOWN )
				{
					MoveToEx( mMemDC, rcItem.left, rcItem.top - 1, NULL );
					LineTo( mMemDC, rcItem.right - 1, rcItem.top - 1 );
				}
				SelectObject( mMemDC, oldPen );
			}

			RECT rcText = rcItem;
			rcText.left = rcText.left + cx + 2;
			if ( item == curSel )
			{
				if ( mTabType == TAB_VIEWTYPE_UP )
					rcText.top += 2;
				else if ( mTabType == TAB_VIEWTYPE_DOWN )
					rcText.top -= 2;
			}

			if ( item->mImageIndex != -1 )
			{
				int top = rcItem.top + ( ( rcItem.bottom - rcItem.top ) - cy ) / 2;
				if ( item == curSel )
				{
					if ( mTabType == TAB_VIEWTYPE_UP )
						top ++;
					else if ( mTabType == TAB_VIEWTYPE_DOWN )
						top --;
				}

				ImageList_Draw( mImageList, item->mImageIndex, mMemDC, rcItem.left + 1, top, ILD_TRANSPARENT );
			}

			TCHAR* text = CNSString::toTChar( item->mName );
			DrawText( mMemDC, text, -1, &rcText, DT_LEFT | DT_VCENTER | DT_SINGLELINE );
			SetBkMode( mMemDC, oldMode );
			SetTextColor( mMemDC, oldTextClr );

			if ( redraw == true )
			{
				RECT rc = rcItem;
				if ( mTabType == TAB_VIEWTYPE_UP )
					rc.bottom ++;
				else if ( mTabType == TAB_VIEWTYPE_DOWN )
					rc.top --;

				InvalidateRect( mHWnd, &rc, FALSE );
			}
		}
	}

	int CNSVsTab::getCurSel( ) const
	{
		if ( curSel == NULL )
			return -1;

		return curSel->mItemID;
	}

	int CNSVsTab::getItemCount( ) const
	{
		return mItems.getCount( );
	}

	void CNSVsTab::setItemText( int index, const CNSString& text )
	{
		CNSVsTab::CTabItem* item = mItems.get( index );
		if ( item == NULL )
			return;

		item->mName = text;
		mNeedRedraw = true;

		RECT rcItem;
		getClientRect( rcItem );
		InvalidateRect( mHWnd, &rcItem, FALSE );
	}

	void CNSVsTab::setCurSel( int itemID )
	{
		int oldSel = curSel == NULL ? -1 : curSel->mItemID;
		int newSel = itemID;

		if ( oldSel == newSel )
			return;

		CTabItem* oldItem = mItems.get( oldSel );
		if ( oldItem != NULL )
			InvalidateRect( mHWnd, &oldItem->mRect, FALSE );

		CTabItem* newItem = mItems.get( newSel );
		if ( newItem != NULL )
			InvalidateRect( mHWnd, &newItem->mRect, FALSE );

		curSel = newItem;
		notifySelect( oldSel, newSel );
		mNeedRedraw = true;
	}

	void CNSVsTab::setItemData( int itemID, intptr_t userdata )
	{
		CNSVsTab::CTabItem* item = mItems.get( itemID );
		if ( item == NULL )
			return;

		item->mUserData = userdata;
	}

	void CNSVsTab::getItems( CNSVector< int >& itemIDs )
	{
		HLISTINDEX beginIndex = mItems.getHead( );
		for ( ; beginIndex != NULL; mItems.getNext( beginIndex ) )
		{
			CNSVsTab::CTabItem* item = &mItems.getValue( beginIndex );
			itemIDs.pushback( item->mItemID );
		}
	}

	intptr_t CNSVsTab::getItemData( int itemID ) const
	{
		const CNSVsTab::CTabItem* item = mItems.get( itemID );
		if ( item == NULL )
			return 0;

		return item->mUserData;
	}

	const CNSString& CNSVsTab::getItemText( int itemID ) const
	{
		const CNSVsTab::CTabItem* item = mItems.get( itemID );
		if ( item == NULL )
		{
			static CNSString null;
			return null;
		}

		return item->mName;
	}

	void CNSVsTab::setItemImage( int itemID, int imageIndex )
	{
		CNSVsTab::CTabItem* item = mItems.get( itemID );
		if ( item == NULL )
			return;

		item->mImageIndex = imageIndex;
		drawItem( item, true );
	}

	void CNSVsTab::setImageList( int cx, int cy, CNSVector< CNSString >& images )
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

	void CNSVsTab::pushUserData( CNSLuaStack& luaStack, CNSVsTab* ref )
	{
		static CNSVector< const luaL_Reg* > reg;
		reg.clear( );
		reg.pushback( NSWin32::NSWindow );
		reg.pushback( NSWin32::NSVsTab );
		luaStack.pushNSWeakRef( ref, NS_VSTAB, reg );
	}

	void CNSVsTab::popUserData( const CNSLuaStack& luaStack, CNSVsTab*& ref )
	{
		ref = (CNSVsTab*) luaStack.popNSWeakRef( NS_VSTAB );
	}
}