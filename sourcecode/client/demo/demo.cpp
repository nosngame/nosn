#include "precomplie.h"
void demoErrorProc1( const char* text, int level )
{
	// 如果是异常才处理
	if ( level == 1 )
		MessageBox( NULL, CNSString::toTChar( text ), _T( "异常" ), MB_OK | MB_ICONERROR );
}

int APIENTRY _tWinMain( _In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPTSTR    lpCmdLine,
	_In_ int       nCmdShow )
{
	HMODULE module = ::LoadLibrary( _T( "nsclient.dll" ) );
	if ( module == NULL )
	{
		CNSString errorDesc;
		errorDesc.format( _UTF8( "模块nsclient.dll加载失败, 错误码 - %d" ), GetLastError( ) );
		MessageBox( NULL, CNSString::toTChar( errorDesc ), _T( "异常" ), MB_OK | MB_ICONERROR );
		return 2;
	}
	
	NSDemo::initProc = ( NSDemo::nsClientInit )GetProcAddress( module, "nsClientInit" );
	NSDemo::exitProc = ( NSDemo::nsClientExit )GetProcAddress( module, "nsClientExit" );
	NSDemo::updateProc = ( NSDemo::nsClientUpdate )GetProcAddress( module, "nsClientUpdate" );
	if ( NSDemo::initProc( "evo", demoErrorProc, true ) == false )
		return 1;

	while ( 1 )
	{
		MSG msg;
		if ( ::PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) == TRUE )
		{
			if ( msg.message == WM_QUIT )
				break;

			::TranslateMessage( &msg );
			::DispatchMessage( &msg );
		}
		else
		{
			NSDemo::updateProc( );
		}
	}

	NSDemo::exitProc( );
	return 0;
}

