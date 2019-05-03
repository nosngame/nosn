#include <nsbase.h>

namespace NSWin32
{
	HBRUSH	FB_BRUSH_BACK = NULL;
	CNSCustom::CNSCustom( const CNSString& windowID ) : CNSWindow( windowID, NS_CUSTOM )
	{
	}

	void CNSCustom::pushUserData( CNSLuaStack& luaStack, CNSCustom* ref )
	{
		static CNSVector< const luaL_Reg* > reg;
		reg.clear( );
		reg.pushback( NSWin32::NSWindow );
		luaStack.pushNSWeakRef( ref, NS_CUSTOM, reg );
	}

	void CNSCustom::popUserData( const CNSLuaStack& luaStack, CNSCustom*& ref )
	{
		ref = (CNSCustom*) luaStack.popNSWeakRef( NS_CUSTOM );
	}

	LRESULT CALLBACK CNSCustom::windowProc( HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam )
	{
		// 自定义控件的处理函数什么也不用做
		NSWin32::CNSCustom* custom = ( NSWin32::CNSCustom* ) CNSWindow::fromHWnd( wnd );
		if ( custom == NULL )
			return DefWindowProc( wnd, msg, wParam, lParam );

		return DefWindowProc( wnd, msg, wParam, lParam );
	}

	void CNSCustom::init( )
	{
		FB_BRUSH_BACK = CreateSolidBrush( RGB( 45, 45, 48 ) );

		WNDCLASSEX wcex;
		wcex.cbSize = sizeof( WNDCLASSEX );
		wcex.style = CS_HREDRAW | CS_VREDRAW;
		wcex.lpfnWndProc = CNSCustom::windowProc;
		wcex.cbClsExtra = 0;
		wcex.cbWndExtra = 0;
		wcex.hInstance = CNSWindow::instance;
		wcex.hIcon = NULL;
		wcex.hCursor = LoadCursor( NULL, IDC_ARROW );
		wcex.hbrBackground = FB_BRUSH_BACK;
		wcex.lpszMenuName = NULL;
		wcex.lpszClassName = _T( "CustomInner" );
		wcex.hIconSm = NULL;
		ATOM classAtom = RegisterClassEx( &wcex );
		if ( classAtom == NULL )
		{
			DWORD errorCode = GetLastError( );
			static CNSString errorDesc;
			errorDesc.format( _UTF8( "win32函数[RegisterClassEx]调用失败，错误码: %d" ), errorCode );
			NSException( errorDesc );
		}

		CNSWindow::superClass( _T( "CustomInner" ), WC_NS_CUSTOM );
	}

	void CNSCustom::exit( )
	{
		if ( FB_BRUSH_BACK != NULL )
			DeleteObject( FB_BRUSH_BACK );

		UnregisterClass( _T( "CustomInner" ), CNSWindow::instance );
		UnregisterClass( WC_NS_CUSTOM, CNSWindow::instance );
	}
}