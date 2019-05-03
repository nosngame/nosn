#include <nsbase.h>
namespace NSWin32
{
	HBRUSH CNSVsStatic::backBrush = NULL;
	void CNSVsStatic::init( )
	{
		backBrush = CreateSolidBrush( RGB( 45, 45, 48 ) );

		WNDCLASSEX wcex;
		wcex.cbSize = sizeof( WNDCLASSEX );
		wcex.style = CS_HREDRAW | CS_VREDRAW;
		wcex.lpfnWndProc = CNSVsStatic::windowProc;
		wcex.cbClsExtra = 0;
		wcex.cbWndExtra = 0;
		wcex.hInstance = CNSWindow::instance;
		wcex.hIcon = NULL;
		wcex.hCursor = LoadCursor( NULL, IDC_ARROW );
		wcex.hbrBackground = backBrush;
		wcex.lpszMenuName = NULL;
		wcex.lpszClassName = _T( "VSStaticInner" );
		wcex.hIconSm = NULL;
		ATOM classAtom = RegisterClassEx( &wcex );
		if ( classAtom == NULL )
		{
			DWORD errorCode = GetLastError( );
			static CNSString errorDesc;
			errorDesc.format( _UTF8( "win32函数[RegisterClassEx]调用失败，错误码: %d" ), errorCode );
			NSException( errorDesc );
		}

		CNSWindow::superClass( _T( "VSStaticInner" ), WC_NS_VSSTATIC );
	}

	void CNSVsStatic::exit( )
	{
		UnregisterClass( _T( "VSStaticInner" ), CNSWindow::instance );
		UnregisterClass( WC_NS_VSSTATIC, CNSWindow::instance );

		if ( backBrush != NULL )
			DeleteObject( backBrush );
	}

	CNSVsStatic::CNSVsStatic( const CNSString& windowID ) : CNSWindow( windowID, NS_VSSTATIC )
	{
	}

	void CNSVsStatic::onPostCreateWindow( CNSWindow* parent )
	{
		CNSWindow::onPostCreateWindow( parent );
	}

	void CNSVsStatic::pushUserData( CNSLuaStack& luaStack, CNSVsStatic* ref )
	{
		static CNSVector< const luaL_Reg* > reg;
		reg.clear( );
		reg.pushback( NSWin32::NSWindow );
		reg.pushback( NSWin32::NSVsStatic );
		luaStack.pushNSWeakRef( ref, NS_VSSTATIC, reg );
	}

	void CNSVsStatic::popUserData( const CNSLuaStack& luaStack, CNSVsStatic*& ref )
	{
		ref = (CNSVsStatic*) luaStack.popNSWeakRef( NS_VSSTATIC );
	}

	void CNSVsStatic::setBkColor( COLORREF color )
	{
		if ( mBackBrush != NULL )
			DeleteObject( mBackBrush );

		mBackBrush = CreateSolidBrush( color );
	}

	void CNSVsStatic::setTextColor( COLORREF color )
	{
		mTextColor = color;
	}

	// Visual studio Style File TabControl 窗口回调函数
	LRESULT CNSVsStatic::windowProc( HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam )
	{
		try
		{
			CNSVsStatic* vsStaticCtrl = (CNSVsStatic*) CNSWindow::fromHWnd( wnd );
			if ( vsStaticCtrl == NULL )
				return DefWindowProc( wnd, msg, wParam, lParam );

			switch ( msg )
			{
				case WM_PAINT:
				{
					CNSString text = vsStaticCtrl->getText( );
					RECT rc;
					vsStaticCtrl->getClientRect( rc );

					PAINTSTRUCT ps;
					BeginPaint( wnd, &ps );
					if ( vsStaticCtrl->mBackBrush != NULL )
						FillRect( ps.hdc, &rc, vsStaticCtrl->mBackBrush );

					int			oldMode = SetBkMode( ps.hdc, TRANSPARENT );
					COLORREF	oldClr = SetTextColor( ps.hdc, vsStaticCtrl->mTextColor );
					HFONT		oldFont = (HFONT) SelectObject( ps.hdc, CNSPreDefine::FB_FONT_BASE );

					TCHAR* textChar = CNSString::toTChar( text );
					DrawText( ps.hdc, textChar, -1, &rc, DT_LEFT | DT_VCENTER | DT_SINGLELINE );

					SelectObject( ps.hdc, oldFont );
					SetTextColor( ps.hdc, oldClr );
					SetBkMode( ps.hdc, oldMode );

					EndPaint( wnd, &ps );
					break;
				}
				case WM_NCDESTROY:
				{
					if ( vsStaticCtrl->mBackBrush != NULL )
						DeleteObject( vsStaticCtrl->mBackBrush );
					break;
				}
				default:
					return DefWindowProc( wnd, msg, wParam, lParam );
			}
		}
		catch ( CNSException& e )
		{
			NSLog::exception( _UTF8( "函数CNSVsStatic::windowProc发生异常\n错误描述: \n\t%s\nC++调用堆栈: \n%s" ), e.mErrorDesc, NSBase::NSFunction::getStackInfo( ).getBuffer( ) );
		}

		return 0;
	}
}
