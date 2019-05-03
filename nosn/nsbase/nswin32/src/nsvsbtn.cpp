#include <nsbase.h>
namespace NSWin32
{
	HBRUSH CNSVsBtn::brushDisableBack = NULL;
	HBRUSH CNSVsBtn::brushBack = NULL;
	HBRUSH CNSVsBtn::brushSelect = NULL;
	HBRUSH CNSVsBtn::brushCheck = NULL;
	HBRUSH CNSVsBtn::brushCheckHover = NULL;
	HPEN CNSVsBtn::penDisableBorder = NULL;
	HPEN CNSVsBtn::penNormalBorder = NULL;
	HPEN CNSVsBtn::penHoverBorder = NULL;
	HPEN CNSVsBtn::penCheckBorder = NULL;
	CNSVsBtn::CNSVsBtn( const CNSString& windowID, EVsButtonStyle style ) : CNSWindow( windowID, NS_VSBTN ), mState( EVsButtonState::STATE_NORMAL ),
		mInTracking( false ), mStyle( style ), mImageDC( NULL )
	{
	}

	void CNSVsBtn::init( )
	{
		CNSVsBtn::brushDisableBack = CreateSolidBrush( RGB( 93, 93, 93 ) );
		CNSVsBtn::brushBack = CreateSolidBrush( RGB( 63, 63, 70 ) );
		CNSVsBtn::brushSelect = CreateSolidBrush( RGB( 0, 151, 251 ) );
		CNSVsBtn::brushCheck = CreateSolidBrush( RGB( 0, 151, 251 ) );
		CNSVsBtn::brushCheckHover = CreateSolidBrush( RGB( 82, 176, 239 ) );
		CNSVsBtn::penDisableBorder = CreatePen( PS_SOLID, 1, RGB( 185, 185, 185 ) );
		CNSVsBtn::penNormalBorder = CreatePen( PS_SOLID, 1, RGB( 85, 85, 85 ) );
		CNSVsBtn::penHoverBorder = CreatePen( PS_SOLID, 1, RGB( 0, 151, 251 ) );
		CNSVsBtn::penCheckBorder = CreatePen( PS_SOLID, 1, RGB( 0, 151, 251 ) );

		WNDCLASSEX wcex;
		wcex.cbSize = sizeof( WNDCLASSEX );
		wcex.style = CS_HREDRAW | CS_VREDRAW;
		wcex.lpfnWndProc = CNSCustom::windowProc;
		wcex.cbClsExtra = 0;
		wcex.cbWndExtra = 0;
		wcex.hInstance = CNSWindow::instance;
		wcex.hIcon = NULL;
		wcex.hCursor = ::LoadCursor( NULL, IDC_ARROW );
		wcex.hbrBackground = brushBack;
		wcex.lpszMenuName = NULL;
		wcex.lpszClassName = _T( "NSVsBtnInner" );
		wcex.hIconSm = NULL;
		ATOM classAtom = RegisterClassEx( &wcex );
		if ( classAtom == NULL )
		{
			DWORD errorCode = GetLastError( );
			static CNSString errorDesc;
			errorDesc.format( _UTF8( "win32º¯Êý[RegisterClassEx]µ÷ÓÃÊ§°Ü£¬´íÎóÂë: %d" ), errorCode );
			NSException( errorDesc );
		}

		CNSWindow::superClass( _T( "NSVsBtnInner" ), WC_NS_VSBTN );
	}

	void CNSVsBtn::exit( )
	{
		UnregisterClass( _T( "NSVsBtnInner" ), CNSWindow::instance );
		UnregisterClass( WC_NS_VSBTN, CNSWindow::instance );

		if ( CNSVsBtn::brushDisableBack != NULL )
			DeleteObject( CNSVsBtn::brushDisableBack );

		if ( CNSVsBtn::brushBack != NULL )
			DeleteObject( CNSVsBtn::brushBack );

		if ( CNSVsBtn::brushSelect != NULL )
			DeleteObject( CNSVsBtn::brushSelect );

		if ( CNSVsBtn::brushCheck != NULL )
			DeleteObject( CNSVsBtn::brushCheck );

		if ( CNSVsBtn::brushCheckHover != NULL )
			DeleteObject( CNSVsBtn::brushCheckHover );

		if ( CNSVsBtn::penDisableBorder != NULL )
			DeleteObject( CNSVsBtn::penDisableBorder );

		if ( CNSVsBtn::penNormalBorder != NULL )
			DeleteObject( CNSVsBtn::penNormalBorder );

		if ( CNSVsBtn::penHoverBorder != NULL )
			DeleteObject( CNSVsBtn::penHoverBorder );

		if ( CNSVsBtn::penCheckBorder != NULL )
			DeleteObject( CNSVsBtn::penCheckBorder );
	}

	void CNSVsBtn::regLuaLib( )
	{
		NSBase::CNSLuaStack& luaStack = NSBase::CNSLuaStack::getLuaStack( );
		luaStack.newTable( );
		luaStack.pushField( "STYLE_TEXT", CNSVsBtn::STYLE_TEXT );
		luaStack.pushField( "STYLE_IMAGE", CNSVsBtn::STYLE_IMAGE );
		luaStack.pushField( "STYLE_PUSHBUTTON", CNSVsBtn::STYLE_PUSHBUTTON );
		luaStack.pushField( "STYLE_CHECK", CNSVsBtn::STYLE_CHECK );
		luaStack.setGlobalTable( "btnStyle" );
	}

	bool CNSVsBtn::onButtonMouseMove( CNSWindow* child, WPARAM wParam, LPARAM lParam, LRESULT* result )
	{
		CNSVsBtn* btn = (CNSVsBtn*) child;
		if ( btn->mInTracking == false )
		{
			btn->mInTracking = true;
			TRACKMOUSEEVENT tm;
			tm.cbSize = sizeof( TRACKMOUSEEVENT );
			tm.hwndTrack = child->getHWnd( );
			tm.dwFlags = TME_HOVER | TME_LEAVE;
			tm.dwHoverTime = 1;
			TrackMouseEvent( &tm );
		}
		return true;
	}

	bool CNSVsBtn::onButtonMouseHover( CNSWindow* child, WPARAM wParam, LPARAM lParam, LRESULT* result )
	{
		CNSVsBtn* btn = (CNSVsBtn*) child;
		if ( btn->mStyle & EVsButtonStyle::STYLE_PUSHBUTTON )
		{
			btn->mState = EVsButtonState::STATE_HOVER;
		}
		else if ( btn->mStyle & EVsButtonStyle::STYLE_CHECK )
		{
			btn->mState = EVsButtonState::STATE_HOVER;
		}

		InvalidateRect( child->getHWnd( ), NULL, FALSE );
		return true;
	}

	bool CNSVsBtn::onButtonMouseLeave( CNSWindow* child, WPARAM wParam, LPARAM lParam, LRESULT* result )
	{
		CNSVsBtn* btn = (CNSVsBtn*) child;
		btn->mInTracking = false;
		if ( btn->mStyle & EVsButtonStyle::STYLE_PUSHBUTTON )
		{
			if ( btn->mState == EVsButtonState::STATE_HOVER )
				btn->mState = EVsButtonState::STATE_NORMAL;
		}
		else if ( btn->mStyle & EVsButtonStyle::STYLE_CHECK )
		{
			if ( btn->mState == EVsButtonState::STATE_HOVER )
				btn->mState = EVsButtonState::STATE_NORMAL;
		}

		InvalidateRect( child->getHWnd( ), NULL, FALSE );
		return true;
	}

	bool CNSVsBtn::onButtonLButtonDown( CNSWindow* child, WPARAM wParam, LPARAM lParam, LRESULT* result )
	{
		CNSVsBtn* btn = (CNSVsBtn*) child;
		if ( btn->mStyle & EVsButtonStyle::STYLE_PUSHBUTTON )
			btn->mState = EVsButtonState::STATE_DOWN;
		else if ( btn->mStyle & EVsButtonStyle::STYLE_CHECK )
			btn->mState = EVsButtonState::STATE_DOWN;

		SetCapture( btn->mHWnd );
		InvalidateRect( child->getHWnd( ), NULL, FALSE );
		return true;
	}

	bool CNSVsBtn::onButtonLButtonUp( CNSWindow* child, WPARAM wParam, LPARAM lParam, LRESULT* result )
	{
		CNSVsBtn* btn = (CNSVsBtn*) child;
		if ( btn->mStyle & EVsButtonStyle::STYLE_PUSHBUTTON )
			btn->mState = EVsButtonState::STATE_HOVER;
		else if ( btn->mStyle & EVsButtonStyle::STYLE_CHECK )
		{
			if ( btn->mIsChecked == true )
				btn->mIsChecked = false;
			else
				btn->mIsChecked = true;
			btn->mState = EVsButtonState::STATE_HOVER;
		}

		ReleaseCapture( );
		InvalidateRect( child->getHWnd( ), NULL, FALSE );
		btn->notifyParent( EVsButtonEvent::EventClicked );
		return true;
	}

	bool CNSVsBtn::onButtonNcDestroy( CNSWindow* child, WPARAM wParam, LPARAM lParam, LRESULT* result )
	{
		CNSVsBtn* btn = (CNSVsBtn*) child;
		SelectObject( btn->mImageDC, btn->mOldImage );
		DeleteDC( btn->mImageDC );
		DeleteObject( btn->mImage );
		return true;
	}

	bool CNSVsBtn::onButtonRender( CNSWindow* child, WPARAM wParam, LPARAM lParam, LRESULT* result )
	{
		CNSVsBtn* btn = (CNSVsBtn*) child;
		RECT rc;
		child->getClientRect( rc );
		PAINTSTRUCT ps;
		BeginPaint( child->getHWnd( ), &ps );

		HBRUSH	oldBr = NULL;
		HPEN	oldPen = NULL;
		if ( btn->mStyle & EVsButtonStyle::STYLE_PUSHBUTTON )
		{
			if ( btn->isEnable( ) == false )
			{
				oldPen = (HPEN) SelectObject( ps.hdc, penDisableBorder );
				oldBr = (HBRUSH) SelectObject( ps.hdc, brushDisableBack );
			}
			else if ( btn->mState == CNSVsBtn::EVsButtonState::STATE_HOVER )
			{
				oldPen = (HPEN) SelectObject( ps.hdc, penHoverBorder );
				oldBr = (HBRUSH) SelectObject( ps.hdc, brushBack );
			}
			else if ( btn->mState == CNSVsBtn::EVsButtonState::STATE_DOWN )
			{
				oldPen = (HPEN) SelectObject( ps.hdc, penHoverBorder );
				oldBr = (HBRUSH) SelectObject( ps.hdc, brushSelect );
			}
			else
			{
				oldPen = (HPEN) SelectObject( ps.hdc, penNormalBorder );
				oldBr = (HBRUSH) SelectObject( ps.hdc, brushBack );
			}
		}
		else if ( btn->mStyle & EVsButtonStyle::STYLE_CHECK )
		{
			// Êó±êÐüÍ£×´Ì¬
			if ( btn->mState == CNSVsBtn::EVsButtonState::STATE_HOVER )
			{
				// check×´Ì¬
				if ( btn->mIsChecked == true )
				{
					oldPen = (HPEN) SelectObject( ps.hdc, penHoverBorder );
					oldBr = (HBRUSH) SelectObject( ps.hdc, brushCheckHover );
				}
				else
				{
					oldPen = (HPEN) SelectObject( ps.hdc, penHoverBorder );
					oldBr = (HBRUSH) SelectObject( ps.hdc, brushBack );
				}
			}
			// Êó±ê°´ÏÂ×´Ì¬
			else if ( btn->mState == CNSVsBtn::EVsButtonState::STATE_DOWN )
			{
				oldPen = (HPEN) SelectObject( ps.hdc, penHoverBorder );
				oldBr = (HBRUSH) SelectObject( ps.hdc, brushSelect );
			}
			// Êó±êÔÚ¿Ø¼þÍâÃæ×´Ì¬
			else
			{
				// check×´Ì¬
				if ( btn->mIsChecked == true )
				{
					oldPen = (HPEN) SelectObject( ps.hdc, penCheckBorder );
					oldBr = (HBRUSH) SelectObject( ps.hdc, brushCheck );
				}
				else
				{
					oldPen = (HPEN) SelectObject( ps.hdc, penNormalBorder );
					oldBr = (HBRUSH) SelectObject( ps.hdc, brushBack );
				}
			}
		}
		Rectangle( ps.hdc, rc.left, rc.top, rc.right, rc.bottom );
		SelectObject( ps.hdc, oldPen );
		SelectObject( ps.hdc, oldBr );

		if ( btn->mStyle & EVsButtonStyle::STYLE_TEXT )
		{
			int	oldMode = SetBkMode( ps.hdc, TRANSPARENT );
			COLORREF oldTextClr;
			if ( btn->isEnable( ) == false )
				oldTextClr = SetTextColor( ps.hdc, RGB( 192, 192, 192 ) );
			else
				oldTextClr = SetTextColor( ps.hdc, RGB( 255, 255, 255 ) );

			HFONT oldFont = (HFONT) SelectObject( ps.hdc, CNSPreDefine::FB_FONT_BASE );
			TCHAR* text = CNSString::toTChar( child->getText( ) );
			DrawText( ps.hdc, text, lstrlen( text ), &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE );
			SelectObject( ps.hdc, oldFont );
			SetTextColor( ps.hdc, oldTextClr );
			SetBkMode( ps.hdc, oldMode );
		}
		else if ( btn->mStyle & EVsButtonStyle::STYLE_IMAGE )
		{
			int width = rc.right - rc.left;
			int height = rc.bottom - rc.top;

			int srcWidth = 0;
			int srcHeight = 0;
			if ( btn->mImageData != NULL )
			{
				srcWidth = btn->mImageData->mWidth;
				srcHeight = btn->mImageData->mHeight;
			}

			int x = ( width - srcWidth ) >> 1;
			int y = ( height - srcHeight ) >> 1;

			if ( btn->mImageDC == NULL && btn->mImageData != NULL )
			{
				btn->mImageDC = CreateCompatibleDC( ps.hdc );
				btn->mImage = btn->mImageData->getHBitmap( ps.hdc );
				btn->mOldImage = (HBITMAP) SelectObject( btn->mImageDC, btn->mImage );
			}

			if ( btn->mImageData != NULL )
			{
				if ( btn->mImageData->mBitPerPixel == 32 )
				{
					BLENDFUNCTION blend;
					blend.SourceConstantAlpha = 255;
					blend.BlendOp = AC_SRC_OVER;
					blend.AlphaFormat = AC_SRC_ALPHA;
					blend.BlendFlags = 0;
					::AlphaBlend( ps.hdc, x, y, srcWidth, srcHeight, btn->mImageDC, 0, 0, srcWidth, srcHeight, blend );
				}
				else if ( btn->mImageData->mBitPerPixel == 24 )
					::BitBlt( ps.hdc, x, y, srcWidth, srcHeight, btn->mImageDC, 0, 0, SRCCOPY );
			}
		}

		EndPaint( child->getHWnd( ), &ps );
		return true;
	}

	void CNSVsBtn::setCheck( bool check )
	{
		mIsChecked = check;
		InvalidateRect( getHWnd( ), NULL, FALSE );
	}

	bool CNSVsBtn::getCheck( ) const
	{
		return mIsChecked;
	}

	void CNSVsBtn::setImage( CImage* image )
	{
		mImageData = image;
	}

	void CNSVsBtn::onPostCreateWindow( CNSWindow* parent )
	{
		CNSWindow::onPostCreateWindow( parent );
		registerMessage( WM_PAINT, CNSVsBtn::onButtonRender );
		registerMessage( WM_MOUSEMOVE, CNSVsBtn::onButtonMouseMove );
		registerMessage( WM_MOUSEHOVER, CNSVsBtn::onButtonMouseHover );
		registerMessage( WM_MOUSELEAVE, CNSVsBtn::onButtonMouseLeave );
		registerMessage( WM_LBUTTONDOWN, CNSVsBtn::onButtonLButtonDown );
		registerMessage( WM_LBUTTONUP, CNSVsBtn::onButtonLButtonUp );
		registerMessage( WM_NCDESTROY, CNSVsBtn::onButtonNcDestroy );
	}

	void CNSVsBtn::pushUserData( CNSLuaStack& luaStack, CNSVsBtn* ref )
	{
		static CNSVector< const luaL_Reg* > reg;
		reg.clear( );
		reg.pushback( NSWin32::NSWindow );
		reg.pushback( NSWin32::NSVsBtn );
		luaStack.pushNSWeakRef( ref, NS_VSBTN, reg );
	}

	void CNSVsBtn::popUserData( const CNSLuaStack& luaStack, CNSVsBtn*& ref )
	{
		ref = (CNSVsBtn*) luaStack.popNSWeakRef( NS_VSBTN );
	}
}