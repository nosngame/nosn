#include <nsbase.h>

namespace NSWin32
{
	class CMoveCache
	{
	public:
		HWND mHWnd;
		int posX;
		int posY;
		int width;
		int height;
	public:
		CMoveCache( HWND wnd, int x, int y, int w, int h ) : mHWnd( wnd ), posX( x ), posY( y ), width( w ), height( h )
		{
		}
	};

	int CNSFrame::grabWidth = 0;
	HBRUSH	CNSFrame::frameBack = NULL;
	HBRUSH	brushGrab = NULL;
	HPEN	penGrab = NULL;
	HPEN	penBorderFocus = NULL;
	HPEN	penBorderUnFocus = NULL;
	CNSFrame::CNSFrame( const CNSString& windowID, unsigned int type ) : CNSWindow( windowID, NS_FRAME ), mFrameType( (EFrameType) type )
	{
	}

	void CNSFrame::init( )
	{
		penGrab = CreatePen( PS_SOLID, 1, RGB( 63, 63, 70 ) );
		penBorderFocus = CreatePen( PS_SOLID, 1, RGB( 0, 122, 204 ) );
		penBorderUnFocus = CreatePen( PS_SOLID, 1, RGB( 45, 45, 48 ) );
		brushGrab = CreateSolidBrush( RGB( 45, 45, 48 ) );
		frameBack = CreateSolidBrush( RGB( 45, 45, 48 ) );
		grabWidth = 8;

		WNDCLASSEX wcex;
		wcex.cbSize = sizeof( WNDCLASSEX );
		wcex.style = CS_HREDRAW | CS_VREDRAW;
		wcex.lpfnWndProc = CNSFrame::windowProc;
		wcex.cbClsExtra = 0;
		wcex.cbWndExtra = 0;
		wcex.hInstance = CNSWindow::instance;
		wcex.hIcon = NULL;
		wcex.hCursor = LoadCursor( NULL, IDC_ARROW );
		wcex.hbrBackground = frameBack;
		wcex.lpszMenuName = NULL;
		wcex.lpszClassName = _T( "FrameInner" );
		wcex.hIconSm = NULL;
		ATOM classAtom = RegisterClassEx( &wcex );
		if ( classAtom == NULL )
		{
			DWORD errorCode = GetLastError( );
			static CNSString errorDesc;
			errorDesc.format( _UTF8( "win32函数[RegisterClassEx]调用失败，错误码: %d" ), errorCode );
			NSException( errorDesc );
		}

		CNSWindow::superClass( _T( "FrameInner" ), WC_NS_FRAME );
	}

	void CNSFrame::exit( )
	{
		if ( penGrab != NULL )
			DeleteObject( penGrab );

		if ( brushGrab != NULL )
			DeleteObject( brushGrab );

		if ( frameBack != NULL )
			DeleteObject( frameBack );

		if ( penBorderFocus != NULL )
			DeleteObject( penBorderFocus );

		if ( penBorderUnFocus != NULL )
			DeleteObject( penBorderUnFocus );
		UnregisterClass( _T( "FrameInner" ), CNSWindow::instance );
		UnregisterClass( WC_NS_FRAME, CNSWindow::instance );
	}

	void CNSFrame::regLuaLib( )
	{
		NSBase::CNSLuaStack& luaStack = NSBase::CNSLuaStack::getLuaStack( );
		luaStack.newTable( );
		luaStack.pushField( "STYLE_FULL", CNSFrame::STYLE_FULL );
		luaStack.pushField( "STYLE_POPUP", CNSFrame::STYLE_POPUP );
		luaStack.pushField( "STYLE_MINIPOPUP", CNSFrame::STYLE_MINIPOPUP );
		luaStack.pushField( "STYLE_VSPOPUP", CNSFrame::STYLE_VSPOPUP );
		luaStack.pushField( "STYLE_CHILD", CNSFrame::STYLE_CHILD );
		luaStack.setGlobalTable( "frameStyle" );

		luaStack.newTable( );
		luaStack.pushField( "RESULT_OK", CNSFrame::RESULT_OK );
		luaStack.pushField( "RESULT_CANCEL", CNSFrame::RESULT_CANCEL );
		luaStack.setGlobalTable( "resultCode" );
	}

	bool CNSFrame::onGrabRender( CNSWindow* child, WPARAM wParam, LPARAM lParam, LRESULT* result )
	{
		HBRUSH brBack = CreateSolidBrush( RGB( 0, 0, 0 ) );
		PAINTSTRUCT ps;
		HDC dc = BeginPaint( child->getHWnd( ), &ps );
		RECT rc;
		GetClientRect( child->getHWnd( ), &rc );

		FillRect( dc, &rc, brBack );

		DeleteObject( brBack );
		EndPaint( child->getHWnd( ), &ps );
		return true;
	}

	LRESULT CALLBACK CNSFrame::windowProc( HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam )
	{
		try
		{
			NSWin32::CNSFrame* frame = ( NSWin32::CNSFrame* ) CNSWindow::fromHWnd( wnd );
			switch ( msg )
			{
				case WM_NCDESTROY:
				{
					if ( frame->mBackBrush != NULL )
						DeleteObject( frame->mBackBrush );

					break;
				}
				case WM_DESTROY:
				{
					if ( frame->mExitWhenDestroy == true )
						PostQuitMessage( 0 );

					break;
				}
				case WM_GETMINMAXINFO:
				{
					MINMAXINFO* minmaxInfo = (MINMAXINFO*) lParam;
					RECT rc;
					rc.left = 0;
					rc.top = 0;
					rc.right = 204;
					rc.bottom = 204;
					AdjustWindowRect( &rc, WS_OVERLAPPEDWINDOW, FALSE );

					minmaxInfo->ptMinTrackSize.x = rc.right - rc.left;
					minmaxInfo->ptMinTrackSize.y = rc.bottom - rc.top;
					break;
				}
				case WM_SETCURSOR:
				{
					if ( frame->mIsHoverGrab == true || frame->mCurGrab != NULL )
					{
						HCURSOR cursor = NULL;
						if ( frame->mSplitType == ESplitType::SPLIT_HORZ )
							cursor = LoadCursor( NULL, IDC_SIZEWE );
						else
							cursor = LoadCursor( NULL, IDC_SIZENS );

						SetCursor( cursor );
					}
					else
						return DefWindowProc( wnd, msg, wParam, lParam );

					break;
				}
				case WM_LBUTTONUP:
				{
					int offsetX = frame->mLastCursor.x - frame->mDownCursor.x;
					int offsetY = frame->mLastCursor.y - frame->mDownCursor.y;
					RECT rcNew;
					rcNew.left = 0;
					rcNew.top = 0;
					if ( frame->mCurGrab != NULL )
					{
						CNSMap< HWND, CNSVector< CMoveCache > > moves;
						if ( frame->mCurGrab->mpLeft != NULL )
						{
							RECT rcLeft;
							frame->mCurGrab->mpLeft->getWindowRect( rcLeft );
							POINT ptLeft = { rcLeft.left, rcLeft.top };
							ScreenToClient( frame->getHWnd( ), &ptLeft );

							if ( frame->mSplitType == ESplitType::SPLIT_HORZ )
							{
								int x = ptLeft.x;
								int y = ptLeft.y;
								int width = x + ( rcLeft.right - rcLeft.left ) + offsetX;
								int height = y + ( rcLeft.bottom - rcLeft.top );
								rcNew.right = width;
								rcNew.bottom = height;
								MoveWindow( frame->mCurGrab->mpLeft->getHWnd( ), x, y, width, height, TRUE );
							}
							else if ( frame->mSplitType == ESplitType::SPLIT_VERT )
							{
								int x = ptLeft.x;
								int y = ptLeft.y;
								int width = x + ( rcLeft.right - rcLeft.left );
								int height = y + ( rcLeft.bottom - rcLeft.top ) + offsetY;
								rcNew.right = width;
								rcNew.bottom = height;
								MoveWindow( frame->mCurGrab->mpLeft->getHWnd( ), x, y, width, height, TRUE );
							}
						}

						if ( frame->mCurGrab->mpRight != NULL )
						{
							RECT rcRight;
							frame->mCurGrab->mpRight->getWindowRect( rcRight );
							POINT ptRight = { rcRight.left, rcRight.top };
							ScreenToClient( frame->getHWnd( ), &ptRight );

							if ( frame->mSplitType == ESplitType::SPLIT_HORZ )
							{
								int x = ptRight.x + offsetX;
								int y = ptRight.y;
								int width = ( rcRight.right - rcRight.left ) - offsetX;
								int height = y + ( rcRight.bottom - rcRight.top );
								rcNew.right = width;
								rcNew.bottom = height;
								MoveWindow( frame->mCurGrab->mpRight->getHWnd( ), x, y, width, height, TRUE );

							}
							else if ( frame->mSplitType == ESplitType::SPLIT_VERT )
							{
								int x = ptRight.x;
								int y = ptRight.y + offsetY;
								int width = x + ( rcRight.right - rcRight.left );
								int height = ( rcRight.bottom - rcRight.top ) - offsetY;
								rcNew.right = width;
								rcNew.bottom = height;
								MoveWindow( frame->mCurGrab->mpRight->getHWnd( ), x, y, width, height, TRUE );
							}
						}

						ReleaseCapture( );
						CNSCustom* grabber = (CNSCustom*) CNSWindow::getWindow( "frameSplitGrabber" );
						if ( grabber != NULL )
							grabber->destroy( );

						frame->mCurGrab = NULL;

						RECT rcInva;
						frame->getClientRect( rcInva );
						if ( frame->mSplitType == ESplitType::SPLIT_HORZ )
						{
							rcInva.left = min( frame->mLastCursor.x, frame->mDownCursor.x ) - CNSWindow::scrollWidth - CNSFrame::grabWidth - 1;
							rcInva.right = max( frame->mLastCursor.x, frame->mDownCursor.x ) + CNSFrame::grabWidth + 1;
						}
						else if ( frame->mSplitType == ESplitType::SPLIT_VERT )
						{
							rcInva.top = min( frame->mLastCursor.y, frame->mDownCursor.y ) - CNSWindow::scrollWidth - CNSFrame::grabWidth - 1;
							rcInva.bottom = max( frame->mLastCursor.y, frame->mDownCursor.y ) + CNSFrame::grabWidth + 1;
						}
						InvalidateRect( frame->getHWnd( ), &rcInva, FALSE );
					}
					break;
				}
				case WM_LBUTTONDOWN:
				{
					int x = LOWORD( lParam );
					int y = HIWORD( lParam );
					CFrameGrabber* grabber = &frame->mGrabbers;
					if ( PtInRect( &grabber->mGrabbRect, POINT { x, y } ) == TRUE )
						frame->mCurGrab = grabber;

					if ( frame->mCurGrab != NULL )
					{
						SetCapture( frame->getHWnd( ) );
						frame->mLastCursor.x = x;
						frame->mLastCursor.y = y;
						frame->mDownCursor.x = x;
						frame->mDownCursor.y = y;
					}
					break;
				}
				case WM_SIZE:
				{
					int width = LOWORD( lParam );
					int height = HIWORD( lParam );
					RECT rc;
					GetClientRect( frame->getHWnd( ), &rc );

					CFrameGrabber* grabber = &frame->mGrabbers;
					if ( frame->mSplitType == CNSFrame::ESplitType::SPLIT_HORZ )
					{
						grabber->mGrabbRect.bottom = grabber->mGrabbRect.top + height;
						if ( grabber->mpLeft != NULL )
						{
							int left = rc.left;
							int right = grabber->mGrabbRect.left;
							int top = grabber->mGrabbRect.top;
							int bottom = grabber->mGrabbRect.bottom;
							MoveWindow( grabber->mpLeft->getHWnd( ), left, top, right - left, bottom - top, TRUE );
						}

						if ( grabber->mpRight != NULL )
						{
							int left = grabber->mGrabbRect.right;
							int right = rc.right;
							int top = grabber->mGrabbRect.top;
							int bottom = grabber->mGrabbRect.bottom;
							MoveWindow( grabber->mpRight->getHWnd( ), left, top, right - left, bottom - top, TRUE );
						}
					}
					else if ( frame->mSplitType == CNSFrame::ESplitType::SPLIT_VERT )
					{
						grabber->mGrabbRect.right = grabber->mGrabbRect.left + width;
						if ( grabber->mpLeft != NULL )
						{
							int left = grabber->mGrabbRect.left;
							int right = grabber->mGrabbRect.right;
							int top = rc.top;
							int bottom = grabber->mGrabbRect.top;
							MoveWindow( grabber->mpLeft->getHWnd( ), left, top, right - left, bottom - top, TRUE );
						}

						if ( grabber->mpRight != NULL )
						{
							int left = grabber->mGrabbRect.left;
							int right = grabber->mGrabbRect.right;
							int top = grabber->mGrabbRect.bottom;
							int bottom = rc.bottom;
							MoveWindow( grabber->mpRight->getHWnd( ), left, top, right - left, bottom - top, TRUE );
						}
					}

					InvalidateRect( frame->getHWnd( ), &grabber->mGrabbRect, FALSE );
					break;
				}
				case WM_MOUSELEAVE:
				{
					frame->mIsHoverGrab = false;
					break;
				}
				case WM_MOUSEMOVE:
				{
					short x = LOWORD( lParam );
					short y = HIWORD( lParam );
					if ( wParam & MK_LBUTTON )
					{
						if ( frame->mCurGrab != NULL )
						{
							int offsetX = x - frame->mLastCursor.x;
							int offsetY = y - frame->mLastCursor.y;
							if ( frame->mCurGrab->mpLeft != NULL )
							{
								if ( frame->mSplitType == ESplitType::SPLIT_HORZ )
								{
									if ( offsetX < 0 )
										offsetX = max( 100 - frame->mLastCursor.x, offsetX );
								}
								else
								{
									if ( offsetY < 0 )
										offsetY = max( 100 - frame->mLastCursor.y, offsetY );
								}
							}

							if ( frame->mCurGrab->mpRight != NULL )
							{
								RECT rcFrame;
								frame->getWindowRect( rcFrame );
								if ( frame->mSplitType == ESplitType::SPLIT_HORZ )
								{
									if ( offsetX > 0 )
										offsetX = min( ( rcFrame.right - rcFrame.left ) - 100 - frame->mLastCursor.x, offsetX );
								}
								else
								{
									if ( offsetY > 0 )
										offsetY = min( ( rcFrame.bottom - rcFrame.top ) - 100 - frame->mLastCursor.y, offsetY );
								}
							}

							if ( frame->mSplitType == ESplitType::SPLIT_HORZ )
							{
								frame->mCurGrab->mGrabbRect.left += offsetX;
								frame->mCurGrab->mGrabbRect.right += offsetX;
							}
							else
							{
								frame->mCurGrab->mGrabbRect.top += offsetY;
								frame->mCurGrab->mGrabbRect.bottom += offsetY;
							}

							frame->mLastCursor.x += offsetX;
							frame->mLastCursor.y += offsetY;

							POINT ptTL = { frame->mCurGrab->mGrabbRect.left, frame->mCurGrab->mGrabbRect.top };
							int width = frame->mCurGrab->mGrabbRect.right - frame->mCurGrab->mGrabbRect.left;
							int height = frame->mCurGrab->mGrabbRect.bottom - frame->mCurGrab->mGrabbRect.top;
							ClientToScreen( frame->getHWnd( ), &ptTL );
							RECT rc = { ptTL.x, ptTL.y, ptTL.x + width, ptTL.y + height };
							CNSCustom* grabber = (CNSCustom*) CNSWindow::getWindow( "frameSplitGrabber" );
							if ( grabber == NULL )
							{
								grabber = CNSWindow::newCustom( "frameSplitGrabber", WS_VISIBLE | WS_POPUP, WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TOOLWINDOW, rc, NULL );
								grabber->registerMessage( WM_PAINT, CNSFrame::onGrabRender );
								SetLayeredWindowAttributes( grabber->getHWnd( ), RGB( 240, 240, 240 ), (int) 128, LWA_COLORKEY | LWA_ALPHA );
							}
							else
								grabber->move( ptTL.x, ptTL.y, 0, false );
						}
					}

					frame->mIsHoverGrab = false;
					CFrameGrabber* grabber = &frame->mGrabbers;
					if ( x >= grabber->mGrabbRect.left && x <= grabber->mGrabbRect.right && y >= grabber->mGrabbRect.top && y <= grabber->mGrabbRect.bottom )
						frame->mIsHoverGrab = true;

					TRACKMOUSEEVENT tme;
					tme.cbSize = sizeof( TRACKMOUSEEVENT );
					tme.dwFlags = TME_LEAVE;
					tme.hwndTrack = frame->getHWnd( );
					tme.dwHoverTime = HOVER_DEFAULT;
					TrackMouseEvent( &tme );
				}
				break;
				case WM_CLOSE:
				{
					if ( frame->mResult != NULL )
						frame->mResult->mResult = CNSFrame::EResultCode::RESULT_CANCEL;

					if ( frame->mIsModalDialog == false )
						frame->notifyParent( CNSFrame::EResultCode::RESULT_CANCEL );

					return DefWindowProc( wnd, msg, wParam, lParam );
				}
				case WM_PAINT:
				{
					PAINTSTRUCT ps;
					HDC		hdc = BeginPaint( wnd, &ps );

					if ( frame->mBackBrush == NULL )
						frame->mBackBrush = CreateSolidBrush( frame->mBackColor );

					FillRect( hdc, &ps.rcPaint, frame->mBackBrush );
					if ( frame->mSplitType == ESplitType::SPLIT_HORZ )
					{
						RECT* rc = &frame->mGrabbers.mGrabbRect;
						FillRect( hdc, rc, brushGrab );
						HPEN oldPen = (HPEN) SelectObject( hdc, penGrab );
						MoveToEx( hdc, rc->left, rc->top + CNSWindow::titleHeight - 1, NULL );
						LineTo( hdc, rc->left, rc->bottom );
						SelectObject( hdc, oldPen );
					}
					else if ( frame->mSplitType == ESplitType::SPLIT_VERT )
					{
						RECT* rc = &frame->mGrabbers.mGrabbRect;
						FillRect( hdc, rc, brushGrab );
						HPEN oldPen = (HPEN) SelectObject( hdc, penGrab );
						MoveToEx( hdc, rc->left, rc->bottom - 1, NULL );
						LineTo( hdc, rc->right, rc->bottom - 1 );
						SelectObject( hdc, oldPen );
					}

					EndPaint( wnd, &ps );
					break;
				}
				break;
				default:
					return DefWindowProc( wnd, msg, wParam, lParam );
			}
		}
		catch ( CNSException& e )
		{
			NSLog::exception( _UTF8( "函数[CNSFrame::windowProc]发生异常\n错误描述: \n\t%s\nC++调用堆栈: \n%s" ), e.mErrorDesc, NSBase::NSFunction::getStackInfo( ).getBuffer( ) );
		}

		return 0;
	}

	void CNSFrame::bindLeft( CNSWindow* left )
	{
		if ( mSplitType == SPLIT_NONE )
			return;

		// 如果指定窗口是左分割窗口，那么不能在指定
		if ( mGrabbers.mpLeft != NULL && mGrabbers.mpLeft->mWindowID == left->mWindowID )
			return;

		RECT rc;
		GetClientRect( mHWnd, &rc );
		left->processAnchor( false );

		RECT rcLeft = { 0, 0, 0, 0 };
		if ( mSplitType == ESplitType::SPLIT_HORZ )
		{
			rcLeft.left = rc.left;
			rcLeft.right = mGrabbers.mGrabbRect.left;
			rcLeft.top = rc.top;
			rcLeft.bottom = rc.bottom;
		}
		else if ( mSplitType == ESplitType::SPLIT_VERT )
		{
			rcLeft.left = rc.left;
			rcLeft.right = rc.right;
			rcLeft.top = rc.top;
			rcLeft.bottom = mGrabbers.mGrabbRect.top;
		}

		mGrabbers.mpLeft = left;
		MoveWindow( left->getHWnd( ), rcLeft.left, rcLeft.top, rcLeft.right - rcLeft.left, rcLeft.bottom - rcLeft.top, FALSE );
	}

	void CNSFrame::bindRight( CNSWindow* right )
	{
		if ( mSplitType == SPLIT_NONE )
			return;

		RECT rc;
		GetClientRect( mHWnd, &rc );
		right->processAnchor( false );

		RECT rcRight = { 0, 0, 0, 0 };
		if ( mSplitType == ESplitType::SPLIT_HORZ )
		{
			rcRight.left = mGrabbers.mGrabbRect.right;
			rcRight.right = rc.right;
			rcRight.top = rc.top;
			rcRight.bottom = rc.bottom;
		}
		else if ( mSplitType == ESplitType::SPLIT_VERT )
		{
			rcRight.left = rc.left;
			rcRight.right = rc.right;
			rcRight.top = mGrabbers.mGrabbRect.bottom;
			rcRight.bottom = rc.bottom;
		}

		mGrabbers.mpRight = right;
		MoveWindow( right->getHWnd( ), rcRight.left, rcRight.top, rcRight.right - rcRight.left, rcRight.bottom - rcRight.top, FALSE );
	}

	void CNSFrame::unSplitLeft( )
	{
		mSplitType = ESplitType::SPLIT_NONE;
		mGrabbers.mGrabbRect = { 0, 0, 0, 0 };
		mGrabbers.mpLeft = NULL;

		if ( mGrabbers.mpRight != NULL )
		{
			RECT rc;
			GetClientRect( mHWnd, &rc );
			MoveWindow( mGrabbers.mpRight->getHWnd( ), rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, TRUE );
		}
	}

	void CNSFrame::unSplitRight( )
	{
		mSplitType = ESplitType::SPLIT_NONE;
		mGrabbers.mGrabbRect = { 0, 0, 0, 0 };
		mGrabbers.mpRight = NULL;

		if ( mGrabbers.mpLeft != NULL )
		{
			RECT rc;
			GetClientRect( mHWnd, &rc );
			MoveWindow( mGrabbers.mpLeft->getHWnd( ), rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, TRUE );
		}
	}

	void CNSFrame::split( ESplitType type, float percent )
	{
		// 如果已经分割了，那么不能在分割
		if ( mSplitType != ESplitType::SPLIT_NONE )
			return;

		mSplitType = type;
		RECT rc;
		GetClientRect( mHWnd, &rc );

		RECT graberRect = { 0 };
		if ( type == ESplitType::SPLIT_HORZ )
		{
			int center = (int) ( ( rc.right - rc.left ) * percent );
			graberRect.top = rc.top;
			graberRect.bottom = rc.bottom;
			graberRect.left = center - grabWidth / 2;
			graberRect.right = center + grabWidth / 2;

		}
		else if ( type == ESplitType::SPLIT_VERT )
		{
			int center = (int) ( ( rc.bottom - rc.top ) * percent );
			graberRect.top = center - grabWidth / 2;
			graberRect.bottom = center + grabWidth / 2;
			graberRect.left = rc.left;
			graberRect.right = rc.right;
		}
		mGrabbers.mGrabbRect = graberRect;
		mGrabbers.mpLeft = NULL;
		mGrabbers.mpRight = NULL;

		InvalidateRect( mHWnd, &graberRect, FALSE );
	}

	void CNSFrame::drawTitle( HDC dc )
	{
		HPEN oldPen = NULL;
		if ( mTitleHighlight == true )
			oldPen = (HPEN) SelectObject( dc, penBorderFocus );
		else
			oldPen = (HPEN) SelectObject( dc, penBorderUnFocus );

		RECT rc;
		getWindowRect( rc );
		MoveToEx( dc, 0, 0, NULL );
		LineTo( dc, 0, rc.bottom - rc.top - 1 );
		LineTo( dc, rc.right - rc.left - 1, rc.bottom - rc.top - 1 );
		LineTo( dc, rc.right - rc.left - 1, 0 );
		LineTo( dc, 0, 0 );

		SelectObject( dc, oldPen );

		if ( mEnableTitle == true )
			CNSWindow::drawTitle( dc, false );
	}

	void CNSFrame::getTitleRect( RECT& rc )
	{
		if ( mEnableTitle == true )
		{
			GetWindowRect( mHWnd, &rc );
			OffsetRect( &rc, -rc.left, -rc.top );

			rc.bottom = rc.top + titleHeight;

			if ( mFrameType == CNSFrame::EFrameType::STYLE_VSPOPUP )
			{
				rc.left ++;
				rc.right --;
				rc.top ++;
				rc.bottom ++;
			}
		}
	}

	bool CNSFrame::onNcCalcSize( bool clientArea, NCCALCSIZE_PARAMS* calcSize )
	{
		if ( mFrameType == CNSFrame::EFrameType::STYLE_VSPOPUP )
		{
			if ( clientArea == true )
			{
				// 改变窗口大小的时候触发
				RECT rcNewWindow = calcSize->rgrc[ 0 ];
				RECT rcOldWindow = calcSize->rgrc[ 1 ];
				RECT rcOldClient = calcSize->rgrc[ 2 ];

				RECT rcNewClient = calcSize->rgrc[ 0 ];
				InflateRect( &rcNewClient, -1, -1 );

				LONG style = GetWindowStyle( mHWnd );
				int curHeight = rcNewClient.bottom - rcNewClient.top;
				int curWidth = rcNewClient.right - rcNewClient.left;
				if ( style & WS_HSCROLL )
				{
					int newHeight = curHeight - scrollWidth;
					rcNewClient.bottom = rcNewClient.top + newHeight;
				}

				if ( style & WS_VSCROLL )
				{
					int newWidth = curWidth - scrollWidth;
					rcNewClient.right = rcNewClient.left + newWidth;
				}

				if ( mEnableTitle == true )
				{
					rcNewClient.top += titleHeight;
					HDC dc = GetWindowDC( mHWnd );
					drawTitle( dc );
					ReleaseDC( getHWnd( ), dc );
				}

				if ( style & WS_HSCROLL || style & WS_VSCROLL )
				{
					HDC dc = GetWindowDC( getHWnd( ) );
					redrawScrollbar( dc );
					ReleaseDC( getHWnd( ), dc );
				}

				calcSize->rgrc[ 0 ] = rcNewClient;
				calcSize->rgrc[ 1 ] = rcNewWindow;
				calcSize->rgrc[ 2 ] = rcOldWindow;
			}
			else
			{
				// 在CreateWindow函数里面调用的，CreateWindow函数返回之后，这个函数的这个流程就被执行
				RECT* rcClient = (RECT*) calcSize;
			}

			return true;
		}

		return CNSWindow::onNcCalcSize( clientArea, calcSize );
	}

	void CNSFrame::exitWhenDestroy( )
	{
		mExitWhenDestroy = true;
	}

	void CNSFrame::bindResult( CNSFrame::CDialogResult* result )
	{
		mResult = result;
	}

	void CNSFrame::setBkColor( COLORREF color )
	{
		if ( mBackBrush != NULL )
			DeleteObject( mBackBrush );

		mBackBrush = NULL;
		mBackColor = color;
		InvalidateRect( mHWnd, NULL, FALSE );
	}

	void CNSFrame::closeFrame( CNSFrame::EResultCode code )
	{
		if ( mResult != NULL )
			mResult->mResult = code;

		if ( mIsModalDialog == false )
			notifyParent( code );

		destroy( );
	}

	void CNSFrame::onPostCreateWindow( CNSWindow* parent )
	{
		CNSWindow::onPostCreateWindow( parent );
	}

	void CNSFrame::pushUserData( CNSLuaStack& luaStack, CNSFrame* ref )
	{
		static CNSVector< const luaL_Reg* > reg;
		reg.clear( );
		reg.pushback( NSWin32::NSWindow );
		reg.pushback( NSWin32::NSFrame );
		luaStack.pushNSWeakRef( ref, NS_FRAME, reg );
	}

	void CNSFrame::popUserData( const CNSLuaStack& luaStack, CNSFrame*& ref )
	{
		ref = (CNSFrame*) luaStack.popNSWeakRef( NS_FRAME );
	}
}