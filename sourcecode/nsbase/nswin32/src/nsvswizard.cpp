#include <nsbase.h>
namespace NSWin32
{
	HBRUSH CVsWizard::backBrush = NULL;
	HBRUSH CVsWizard::wizardMainBrush = NULL;
	HBRUSH CVsWizard::selectBrush = NULL;
	HFONT CVsWizard::mTitleFont = NULL;
	HFONT CVsWizard::mSubTitleFont = NULL;

	CVsWizard::CVsWizard( const CNSString& windowID ) : CNSWindow( windowID, NS_VSWIZARD )
	{
	}

	void CVsWizard::init( )
	{
		backBrush = CreateSolidBrush( RGB( 45, 45, 48 ) );
		wizardMainBrush = CreateSolidBrush( RGB( 37, 37, 38 ) );
		selectBrush = CreateSolidBrush( RGB( 0, 122, 204 ) );
		mTitleFont = CreateFont( 24,	// nHeight 
								 0,							// nWidth 
								 0,							// nEscapement 
								 0,							// nOrientation 
								 FW_SEMIBOLD,				// nWeight 
								 FALSE,						// bItalic 
								 FALSE,						// bUnderline 
								 0,							// cStrikeOut 
								 ANSI_CHARSET,				// nCharSet 
								 OUT_DEFAULT_PRECIS,			// nOutPrecision 
								 CLIP_DEFAULT_PRECIS,		// nClipPrecision 
								 PROOF_QUALITY,				// nQuality 
								 FIXED_PITCH | FF_MODERN,	// nPitchAndFamily 
								 _T( "Microsoft YaHei UI" ) );	// lpszFac

		mSubTitleFont = CreateFont( 22,	// nHeight 
									0,							// nWidth 
									0,							// nEscapement 
									0,							// nOrientation 
									FW_NORMAL,					// nWeight 
									FALSE,						// bItalic 
									FALSE,						// bUnderline 
									0,							// cStrikeOut 
									ANSI_CHARSET,				// nCharSet 
									OUT_DEFAULT_PRECIS,			// nOutPrecision 
									CLIP_DEFAULT_PRECIS,		// nClipPrecision 
									PROOF_QUALITY,				// nQuality 
									FIXED_PITCH | FF_MODERN,	// nPitchAndFamily 
									_T( "Microsoft YaHei UI" ) );	// lpszFac

		WNDCLASSEX wcex;
		wcex.cbSize = sizeof( WNDCLASSEX );
		wcex.style = CS_HREDRAW | CS_VREDRAW;
		wcex.lpfnWndProc = CVsWizard::windowProc;
		wcex.cbClsExtra = 0;
		wcex.cbWndExtra = 0;
		wcex.hInstance = CNSWindow::instance;
		wcex.hIcon = NULL;
		wcex.hCursor = LoadCursor( NULL, IDC_ARROW );
		wcex.hbrBackground = backBrush;
		wcex.lpszMenuName = NULL;
		wcex.lpszClassName = _T( "VSWizardInner" );
		wcex.hIconSm = NULL;
		ATOM classAtom = RegisterClassEx( &wcex );
		if ( classAtom == NULL )
		{
			DWORD errorCode = GetLastError( );
			static CNSString errorDesc;
			errorDesc.format( _UTF8( "win32函数[RegisterClassEx]调用失败，错误码: %d" ), errorCode );
			NSException( errorDesc );
		}

		CNSWindow::superClass( _T( "VSWizardInner" ), WC_NS_VSWIZARD );
	}

	void CVsWizard::exit( )
	{
		UnregisterClass( _T( "VSWizardInner" ), CNSWindow::instance );
		UnregisterClass( WC_NS_VSWIZARD, CNSWindow::instance );

		if ( backBrush != NULL )
			DeleteObject( backBrush );

		if ( wizardMainBrush != NULL )
			DeleteObject( wizardMainBrush );

		if ( selectBrush != NULL )
			DeleteObject( selectBrush );

		if ( mTitleFont != NULL )
			DeleteObject( mTitleFont );

		if ( mSubTitleFont != NULL )
			DeleteObject( mSubTitleFont );
	}

	// Visual studio Style Wizard 窗口回调函数
	LRESULT CVsWizard::windowProc( HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam )
	{
		try
		{
			CVsWizard* wizard = (CVsWizard*) CNSWindow::fromHWnd( wnd );
			if ( wizard == NULL )
				return DefWindowProc( wnd, msg, wParam, lParam );

			switch ( msg )
			{
				case WM_SIZE:
				{
					if ( wizard->mMemDC != NULL )
					{
						SelectObject( wizard->mMemDC, wizard->mOldFont );
						SelectObject( wizard->mMemDC, wizard->mOldMemBmp );
						DeleteDC( wizard->mMemDC );
					}

					if ( wizard->mMemBitmap != NULL )
						DeleteObject( wizard->mMemBitmap );

					int width = LOWORD( lParam );
					int height = HIWORD( lParam );
					HDC dc = GetDC( wnd );
					wizard->mMemDC = CreateCompatibleDC( dc );
					wizard->mMemBitmap = CreateCompatibleBitmap( dc, width, height );
					wizard->mOldMemBmp = (HBITMAP) SelectObject( wizard->mMemDC, wizard->mMemBitmap );
					wizard->mOldFont = (HFONT) SelectObject( wizard->mMemDC, CNSPreDefine::FB_FONT_BASE );
					ReleaseDC( wnd, dc );
					wizard->redraw( );
					break;
				}
				case WM_PAINT:
				{
					PAINTSTRUCT ps;
					BeginPaint( wnd, &ps );
					int width = ps.rcPaint.right - ps.rcPaint.left;
					int height = ps.rcPaint.bottom - ps.rcPaint.top;
					BitBlt( ps.hdc, ps.rcPaint.left, ps.rcPaint.top, width, height, wizard->mMemDC, ps.rcPaint.left, ps.rcPaint.top, SRCCOPY );
					EndPaint( wnd, &ps );
					break;
				}
				case WM_LBUTTONDOWN:
				{
					POINT pt = { LOWORD( lParam ), HIWORD( lParam ) };
					for ( unsigned int i = 0; i < wizard->mItems.getCount( ); i ++ )
					{
						RECT rcItem;
						wizard->getItemRect( i, rcItem );
						if ( PtInRect( &rcItem, pt ) == TRUE )
						{
							wizard->select( i );
							break;
						}
					}
					break;
				}
				case WM_DESTROY:
				{
					SelectObject( wizard->mMemDC, wizard->mOldFont );
					SelectObject( wizard->mMemDC, wizard->mOldMemBmp );
					DeleteDC( wizard->mMemDC );
					DeleteObject( wizard->mMemBitmap );
					break;
				}
				default:
					return DefWindowProc( wnd, msg, wParam, lParam );
			}
		}
		catch ( CNSException& e )
		{
			NSLog::exception( _UTF8( "函数CVsWizard::windowProc发生异常\n错误描述: \n\t%s\nC++调用堆栈: \n%s" ), e.mErrorDesc, NSBase::NSFunction::getStackInfo( ).getBuffer( ) );
		}

		return 0;
	}

	void CVsWizard::onPostCreateWindow( CNSWindow* parent )
	{
		CNSWindow::onPostCreateWindow( parent );
		RECT rcPrev;
		rcPrev.left = 465;
		rcPrev.top = 557;
		rcPrev.right = 540;
		rcPrev.bottom = 580;
		NSWin32::CNSVsBtn* wizardPrev = NSWin32::CNSWindow::newVsBtn( "debugWizardPrev", NSWin32::CNSVsBtn::EVsButtonStyle::STYLE_PUSHBUTTON | NSWin32::CNSVsBtn::EVsButtonStyle::STYLE_TEXT, rcPrev, this );
		wizardPrev->setText( _UTF8( "上一步" ) );
		wizardPrev->mLeftAnchor = CNSWindow::EAnchor::ANCHOR_RIGHT;
		wizardPrev->mRightAnchor = CNSWindow::EAnchor::ANCHOR_RIGHT;
		wizardPrev->mTopAnchor = CNSWindow::EAnchor::ANCHOR_BOTTOM;
		wizardPrev->mBottomAnchor = CNSWindow::EAnchor::ANCHOR_BOTTOM;
		wizardPrev->registerEvent( NSWin32::CNSVsBtn::EVsButtonEvent::EventClicked, onWizardPrevClicked );

		RECT rcNext;
		rcNext.left = 545;
		rcNext.top = 557;
		rcNext.right = 620;
		rcNext.bottom = 580;
		NSWin32::CNSVsBtn* wizardNext = NSWin32::CNSWindow::newVsBtn( "debugWizardNext", NSWin32::CNSVsBtn::EVsButtonStyle::STYLE_PUSHBUTTON | NSWin32::CNSVsBtn::EVsButtonStyle::STYLE_TEXT, rcNext, this );
		wizardNext->setText( _UTF8( "下一步" ) );
		wizardNext->mLeftAnchor = CNSWindow::EAnchor::ANCHOR_RIGHT;
		wizardNext->mRightAnchor = CNSWindow::EAnchor::ANCHOR_RIGHT;
		wizardNext->mTopAnchor = CNSWindow::EAnchor::ANCHOR_BOTTOM;
		wizardNext->mBottomAnchor = CNSWindow::EAnchor::ANCHOR_BOTTOM;
		wizardNext->registerEvent( NSWin32::CNSVsBtn::EVsButtonEvent::EventClicked, onWizardNextClicked );

		RECT rcConfirm;
		rcConfirm.left = 625;
		rcConfirm.top = 557;
		rcConfirm.right = 700;
		rcConfirm.bottom = 580;
		NSWin32::CNSVsBtn* wizardConfirm = NSWin32::CNSWindow::newVsBtn( "debugWizardConfirm", NSWin32::CNSVsBtn::EVsButtonStyle::STYLE_PUSHBUTTON | NSWin32::CNSVsBtn::EVsButtonStyle::STYLE_TEXT, rcConfirm, this );
		wizardConfirm->setText( _UTF8( "确定" ) );
		wizardConfirm->mLeftAnchor = CNSWindow::EAnchor::ANCHOR_RIGHT;
		wizardConfirm->mRightAnchor = CNSWindow::EAnchor::ANCHOR_RIGHT;
		wizardConfirm->mTopAnchor = CNSWindow::EAnchor::ANCHOR_BOTTOM;
		wizardConfirm->mBottomAnchor = CNSWindow::EAnchor::ANCHOR_BOTTOM;
		wizardConfirm->registerEvent( NSWin32::CNSVsBtn::EVsButtonEvent::EventClicked, onWizardConfirmClicked );

		RECT rcCancel;
		rcCancel.left = 705;
		rcCancel.top = 557;
		rcCancel.right = 780;
		rcCancel.bottom = 580;
		NSWin32::CNSVsBtn* wizardCancel = NSWin32::CNSWindow::newVsBtn( "debugWizardCancel", NSWin32::CNSVsBtn::EVsButtonStyle::STYLE_PUSHBUTTON | NSWin32::CNSVsBtn::EVsButtonStyle::STYLE_TEXT, rcCancel, this );
		wizardCancel->setText( _UTF8( "取消" ) );
		wizardCancel->mLeftAnchor = CNSWindow::EAnchor::ANCHOR_RIGHT;
		wizardCancel->mRightAnchor = CNSWindow::EAnchor::ANCHOR_RIGHT;
		wizardCancel->mTopAnchor = CNSWindow::EAnchor::ANCHOR_BOTTOM;
		wizardCancel->mBottomAnchor = CNSWindow::EAnchor::ANCHOR_BOTTOM;
		wizardCancel->registerEvent( NSWin32::CNSVsBtn::EVsButtonEvent::EventClicked, onWizardCancelClicked );

		RECT rc;
		GetClientRect( mHWnd, &rc );

		int width = rc.right - rc.left;
		int height = rc.bottom - rc.top;
		HDC dc = GetDC( mHWnd );
		mMemDC = CreateCompatibleDC( dc );
		mMemBitmap = CreateCompatibleBitmap( dc, width, height );
		mOldMemBmp = (HBITMAP) SelectObject( mMemDC, mMemBitmap );
		mOldFont = (HFONT) SelectObject( mMemDC, CNSPreDefine::FB_FONT_BASE );
		ReleaseDC( mHWnd, dc );
	}

	void CVsWizard::select( int index )
	{
		if ( index < 0 || index >= (int) mItems.getCount( ) )
			return;

		CNSFrame* oldFrame = mItems[ mCurSel ].mFrame;
		RECT rcOldItem;
		getItemRect( mCurSel, rcOldItem );
		InvalidateRect( getHWnd( ), &rcOldItem, FALSE );
		if ( oldFrame != NULL )
			oldFrame->hide( );

		mCurSel = index;

		RECT rcNewItem;
		getItemRect( mCurSel, rcNewItem );
		InvalidateRect( getHWnd( ), &rcNewItem, FALSE );
		mSubTitle = mItems[ index ].mDesc;
		InvalidateRect( getHWnd( ), &rcNewItem, FALSE );

		RECT rcSubTitle;
		getSubTitleRect( rcSubTitle );
		InvalidateRect( getHWnd( ), &rcSubTitle, FALSE );

		CNSFrame* newFrame = mItems[ mCurSel ].mFrame;
		if ( newFrame != NULL )
			newFrame->show( );
		redraw( );
	}

	void CVsWizard::getItemRect( int index, RECT& rc )
	{
		rc.left = 25;
		rc.right = rc.left + 200;
		rc.top = 100 + index * 36;
		rc.bottom = rc.top + 36;
	}

	void CVsWizard::getSubTitleRect( RECT& rc )
	{
		rc.left = 25;
		rc.right = rc.left + 200;
		rc.top = 55;
		rc.bottom = rc.top + 20;
	}

	void CVsWizard::getTitleRect( RECT& rc )
	{
		rc.left = 25;
		rc.right = rc.left + 200;
		rc.top = 25;
		rc.bottom = rc.top + 30;
	}

	void CVsWizard::getWorkRect( RECT& rc )
	{
		RECT rcClient;
		GetClientRect( mHWnd, &rcClient );

		rc.left = 225;
		rc.right = rcClient.right;
		rc.top = 100;
		rc.bottom = rcClient.bottom - 60;
	}

	void CVsWizard::redraw( )
	{
		// 填充背景
		RECT rc;
		GetClientRect( mHWnd, &rc );

		RECT rcWizardMain = rc;
		rcWizardMain.bottom -= 60;
		FillRect( mMemDC, &rcWizardMain, wizardMainBrush );

		int oldMode = SetBkMode( mMemDC, TRANSPARENT );
		int oldTextClr = SetTextColor( mMemDC, RGB( 255, 255, 255 ) );

		HFONT oldFont = (HFONT) SelectObject( mMemDC, mTitleFont );
		TCHAR* title = CNSString::toTChar( mTitle );
		RECT rcTitle;
		getTitleRect( rcTitle );
		DrawText( mMemDC, title, -1, &rcTitle, DT_LEFT | DT_VCENTER | DT_SINGLELINE );
		SelectObject( mMemDC, oldFont );

		oldFont = (HFONT) SelectObject( mMemDC, mSubTitleFont );
		TCHAR* subTitle = CNSString::toTChar( mSubTitle );
		RECT rcSubTitle;
		getSubTitleRect( rcSubTitle );
		DrawText( mMemDC, subTitle, -1, &rcSubTitle, DT_LEFT | DT_VCENTER | DT_SINGLELINE );
		SelectObject( mMemDC, oldFont );

		for ( unsigned int i = 0; i < mItems.getCount( ); i ++ )
		{
			RECT rcItem;
			getItemRect( i, rcItem );
			if ( mCurSel == i )
				FillRect( mMemDC, &rcItem, selectBrush );

			RECT rcItemText = rcItem;
			rcItemText.left += 10;
			TCHAR* desc = CNSString::toTChar( mItems[ i ].mDesc );
			DrawText( mMemDC, desc, -1, &rcItemText, DT_LEFT | DT_VCENTER | DT_SINGLELINE );
		}
		SetTextColor( mMemDC, oldTextClr );
		SetBkMode( mMemDC, oldMode );

		RECT rcWizardBottom = rc;
		rcWizardBottom.top = rcWizardBottom.bottom - 60;
		FillRect( mMemDC, &rcWizardBottom, backBrush );
	}

	int CVsWizard::addItem( const CNSString& desc )
	{
		int index = mItems.getCount( );
		mItems.pushback( CVsWizard::CWizardItem( desc, NULL ) );
		if ( index == mCurSel )
			mSubTitle = desc;

		redraw( );
		return index;
	}

	void CVsWizard::setItemFrame( int index, CNSFrame* frame )
	{
		if ( index < 0 || index >= (int) mItems.getCount( ) )
			return;

		RECT rcWork;
		getWorkRect( rcWork );
		::MoveWindow( frame->getHWnd( ), rcWork.left, rcWork.top, rcWork.right - rcWork.left, rcWork.bottom - rcWork.top, FALSE );

		int style = GetWindowStyle( frame->getHWnd( ) );
		style = style & ~WS_VISIBLE;
		SetWindowLongPtr( frame->getHWnd( ), GWL_STYLE, style );
		mItems[ index ].mFrame = frame;
		if ( mCurSel == index )
			frame->show( );
	}

	void CVsWizard::setWizardTitle( const CNSString& title )
	{
		mTitle = title;
		redraw( );
	}

	void CVsWizard::next( )
	{
		select( mCurSel + 1 );
	}

	void CVsWizard::prev( )
	{
		select( mCurSel - 1 );
	}

	void CVsWizard::setCurPage( int index )
	{
		select( index );
	}

	bool CVsWizard::onWizardPrevClicked( NSWin32::CNSWindow* child, WPARAM wParam, LPARAM lParam, LRESULT* result )
	{
		NSWin32::CVsWizard* wizard = ( NSWin32::CVsWizard* ) child->getParent( );
		if ( wizard == NULL )
			return true;

		wizard->prev( );
		return true;
	}

	bool CVsWizard::onWizardNextClicked( NSWin32::CNSWindow* child, WPARAM wParam, LPARAM lParam, LRESULT* result )
	{
		NSWin32::CVsWizard* wizard = ( NSWin32::CVsWizard* ) child->getParent( );
		if ( wizard == NULL )
			return true;

		wizard->next( );
		return true;
	}

	bool CVsWizard::onWizardConfirmClicked( NSWin32::CNSWindow* child, WPARAM wParam, LPARAM lParam, LRESULT* result )
	{
		CVsWizard* wizard = (CVsWizard*) child->getParent( );
		if ( wizard == NULL )
			return true;

		wizard->notifyParent( EVsWizardEvent::EventConfirm );
		return true;
	}

	bool CVsWizard::onWizardCancelClicked( NSWin32::CNSWindow* child, WPARAM wParam, LPARAM lParam, LRESULT* result )
	{
		CVsWizard* wizard = (CVsWizard*) child->getParent( );
		if ( wizard == NULL )
			return true;

		wizard->notifyParent( EVsWizardEvent::EventCancel );
		return true;
	}

	void CVsWizard::pushUserData( CNSLuaStack& luaStack, CVsWizard* ref )
	{
		static CNSVector< const luaL_Reg* > reg;
		reg.clear( );
		reg.pushback( NSWin32::NSWindow );
		reg.pushback( NSWin32::NSVsWizard );
		luaStack.pushNSWeakRef( ref, NS_VSWIZARD, reg );
	}

	void CVsWizard::popUserData( const CNSLuaStack& luaStack, CVsWizard*& ref )
	{
		ref = (CVsWizard*) luaStack.popNSWeakRef( NS_VSWIZARD );
	}
}