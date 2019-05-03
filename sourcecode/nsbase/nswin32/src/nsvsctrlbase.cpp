#include <FBBase.h>
namespace FBGui
{
	HBRUSH CVsWizard::backBrush = NULL;
	CVsWizard::CVsWizard( const CFBString& windowID ) : CWindow( windowID )
	{
	}

	void CVsWizard::init( const CFBString& pluginsPath )
	{
		backBrush			= CreateSolidBrush( RGB( 45, 45, 48 ) );
		WNDCLASSEX wcex;
		wcex.cbSize			= sizeof(WNDCLASSEX);
		wcex.style			= CS_HREDRAW | CS_VREDRAW;
		wcex.lpfnWndProc	= CVsWizard::windowProc;
		wcex.cbClsExtra		= 0;
		wcex.cbWndExtra		= 0;
		wcex.hInstance		= CWindow::instance;
		wcex.hIcon			= NULL;
		wcex.hCursor		= LoadCursor( NULL, IDC_ARROW );
		wcex.hbrBackground	= backBrush;
		wcex.lpszMenuName	= NULL;
		wcex.lpszClassName	= _T("VSWizardInner");
		wcex.hIconSm		= NULL;
		RegisterClassEx( &wcex );

		CWindow::superClass( _T("VSWizardInner"), WC_FB_VSWIZARD );
	}

	void CVsWizard::exit( )
	{
		UnregisterClass( _T("VSWizardInner"), CWindow::instance );
		UnregisterClass( WC_FB_VSWIZARD, CWindow::instance );
		DeleteObject( backBrush );
	}

	// Visual studio Style Wizard 窗口回调函数
	LRESULT CVsWizard::windowProc( HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam )
	{
		try
		{
			CVsWizard* vsTabCtrl = (CVsWizard*) CWindow::fromHWnd( wnd );
			if ( vsTabCtrl == NULL )
				return DefWindowProc( wnd, msg, wParam, lParam );

			switch( msg )
			{
			case WM_SIZE:
				{
					//if ( vsTabCtrl->mMemDC != NULL )
					//{
					//	SelectObject( vsTabCtrl->mMemDC, vsTabCtrl->mOldFont );
					//	SelectObject( vsTabCtrl->mMemDC, vsTabCtrl->mOldMemBmp );
					//	DeleteDC( vsTabCtrl->mMemDC );
					//}

					//if ( vsTabCtrl->mMemBitmap != NULL )
					//	DeleteObject( vsTabCtrl->mMemBitmap );

					//int width				= LOWORD( lParam );
					//int height				= HIWORD( lParam );
					//HDC dc					= GetDC( wnd );
					//vsTabCtrl->mMemDC		= CreateCompatibleDC( dc );
					//vsTabCtrl->mMemBitmap	= CreateCompatibleBitmap( dc, width, height );
					//vsTabCtrl->mOldMemBmp	= (HBITMAP) SelectObject( vsTabCtrl->mMemDC, vsTabCtrl->mMemBitmap );
					//vsTabCtrl->mOldFont		= (HFONT) SelectObject( vsTabCtrl->mMemDC, CFBPreDefine::FB_FONT_BASE );

					//vsTabCtrl->redraw( );
					break;
				}
			case WM_PAINT:
				{
					//PAINTSTRUCT ps;
					//BeginPaint( wnd, &ps );
					//int width	= ps.rcPaint.right - ps.rcPaint.left;
					//int height	= ps.rcPaint.bottom - ps.rcPaint.top;
					//BitBlt( ps.hdc, ps.rcPaint.left, ps.rcPaint.top, width, height, vsTabCtrl->mMemDC, ps.rcPaint.left, ps.rcPaint.top, SRCCOPY );
					//EndPaint( wnd, &ps );
					break;
				}
			case WM_DESTROY:
				//DeleteDC( vsTabCtrl->mMemDC );
				//DeleteObject( vsTabCtrl->mMemBitmap );
				break;
			default:
				return DefWindowProc( wnd, msg, wParam, lParam );
			}
		}
		catch( CFBException& e )
		{
			FBLog::exception( _UTF8( "error: %s\r\n行号: %d\r\n文件: %s\r\n函数: %s" ), e.mErrorDesc, e.mLineNumber, e.mpFileName, e.mpFuncName );
		}

		return 0;
	}

	void CVsWizard::onPostCreateWindow( )
	{
		CWindow::onPostCreateWindow( );
		//RECT rc;
		//GetClientRect( mHWnd, &rc );

		//int width	= rc.right - rc.left;
		//int height	= rc.bottom - rc.top;
		//HDC dc		= GetDC( mHWnd );
		//mMemDC		= CreateCompatibleDC( dc );
		//mMemBitmap	= CreateCompatibleBitmap( dc, width, height );
		//mOldMemBmp	= (HBITMAP) SelectObject( mMemDC, mMemBitmap );
		//mOldFont	= (HFONT) SelectObject( mMemDC, CFBPreDefine::FB_FONT_BASE );
	}
}