#include <nsbase.h>

namespace NSWin32
{
	CNSMap< CNSString, CNSWindow* > CNSWindow::sWindows;
	HINSTANCE CNSWindow::instance = NULL;

	float CNSWindow::messageAlpha = 180.0f;
	int	CNSWindow::messageState = 0;
	int	CNSWindow::messageKeepTick = 1000;
	CNSString CNSWindow::messageText;

	HFONT CNSPreDefine::FB_FONT_NOTIFY = NULL;
	HFONT CNSPreDefine::FB_FONT_BASE = NULL;
	HFONT CNSPreDefine::FB_FONT_BASEBOLD = NULL;

	CNSVector< CNSWindow::CMoveProxy > CNSWindow::CMoveProxy::sMoveList;
	CHotkey CNSWindow::sGlobalHotkey;
	CNSMap< unsigned int, CNSLuaFunction > CNSWindow::sLuaGHotkeyHandler;
	CNSMap< ATOM, WNDPROC > CNSWindow::mWndProc;
	HBRUSH CNSWindow::brushFocusWindow = NULL;
	HBRUSH CNSWindow::brushScrollBack = NULL;
	HBRUSH CNSWindow::brushNCBack = NULL;
	HPEN CNSWindow::penBorder = NULL;
	HBRUSH CNSWindow::brushScrollBtn = NULL;
	HBRUSH CNSWindow::brushScrollHoverBtn = NULL;
	HPEN CNSWindow::penScrollBtn = NULL;
	HBRUSH CNSWindow::brushScrollClickBtn = NULL;
	HPEN CNSWindow::penScrollClickBtn = NULL;
	HPEN CNSWindow::penScrollHoverBtn = NULL;
	HBRUSH CNSWindow::brushScrollThumb = NULL;
	HBRUSH CNSWindow::brushScrollHoverThumb = NULL;
	HBRUSH CNSWindow::brushScrollClickThumb = NULL;
	int	CNSWindow::scrollWidth = 0;
	int	CNSWindow::titleHeight = 0;
	CNSBaseApp*	CNSBaseApp::app = NULL;
	CNSBaseApp::CNSBaseApp( )
	{
		if ( app != NULL )
			NSException( _UTF8( "app对象只能有一个" ) );

		app = this;
		HMODULE inst = GetModuleHandle( NULL );
		mInstance = inst;
	}

	CNSBaseApp::CNSBaseApp( const CNSString& name, bool enableDebug ) : mName( name ), mEnableDebug( enableDebug )
	{
		if ( app != NULL )
			NSException( _UTF8( "app对象只能有一个" ) );

		app = this;
		HMODULE inst = GetModuleHandle( NULL );
		mInstance = inst;
	}

	CNSBaseApp::~CNSBaseApp( )
	{
		CNSBaseApp::app = NULL;
	}

	void CNSBaseApp::run( )
	{
		onInitApp( );
		pumpMessage( );
		onExitApp( );
	}

	void CNSBaseApp::onInitApp( )
	{
		::SetUnhandledExceptionFilter( OurSetUnhandledExceptionFilter );
		CAPIHook apiHook( "kernel32.dll", "SetUnhandledExceptionFilter", (PROC) RedirectedSetUnhandledExceptionFilter );

		// 建立日志文件, 调用该函数后NSLog::log会写入文件
		NSLog::init( mName + "_output.txt" );

		// 初始化Lua库
		NSBase::CNSLuaStack::init( mEnableDebug );

		// 初始化窗口
		CNSWindow::init( mInstance );

		// 初始化网络库
		NSNet::init( );

		// 初始化图形日志
		NSConsole::init( );
		NSConsole::enableDebug( mEnableDebug );

		// 加载本地化文件
		if ( mUseNSLocal == true )
			CNSLocal::getNSLocal( ).load( );

		// 初始化物理服务器列表
		if ( mUseIPName == true )
			loadIPName( );

		// 初始化数据库客户端
		if ( mUseNSMysql == true )
			NSMysql::init( );

		// 初始化curl
		if ( mUseNSHttp == true )
			NSHttp::init( );

		// 初始化http控制接口，只有两个接口，查询全局变量，远程执行一段代码
		if ( mUseNSHttpDebugger == true )
		{
			CNSString controlAddress = name2IPPort( NSHttpDebugger::getAddress( mName ) );
			NSHttpDebugger::init( mName + "_control", controlAddress );
		}
	}

	void CNSBaseApp::onExitApp( )
	{
		if ( mUseNSHttpDebugger == true )
			NSHttpDebugger::exit( );

		if ( mUseNSHttp == true )
			NSHttp::exit( );

		if ( mUseNSMysql == true )
			NSMysql::exit( );

		NSConsole::exit( );
		NSNet::exit( );
		NSBase::exit( );
		NSWin32::CNSWindow::exit( );
		NSBase::CNSLuaStack::exit( );
		NSLog::exit( );
	}

	void CNSBaseApp::onIdle( )
	{
		NSNet::pollEvent( );
		NSWin32::CNSWindow::update( );
		CNSTimer::updateTimer( );
		::Sleep( 1 );
	}

	void CNSBaseApp::pumpMessage( )
	{
		while ( 1 )
		{
			try
			{
				MSG msg;
				if ( ::PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) == TRUE )
				{
					if ( msg.message == WM_QUIT )
						break;

					if ( onPreTranslateMessage( msg ) == false )
					{
						::TranslateMessage( &msg );
						::DispatchMessage( &msg );
					}
				}
				else
				{
					onIdle( );
				}
			}
			catch ( CNSException& e )
			{
				NSLog::exception( _UTF8( "程序主循环发生异常\n错误描述: \n\t%s\nC++调用堆栈: \n%s" ), e.mErrorDesc, NSBase::NSFunction::getStackInfo( ).getBuffer( ) );
			}
		}
	}

	int CNSBaseApp::GenerateMiniDump( HANDLE hFile, PEXCEPTION_POINTERS pExceptionPointers )
	{
		BOOL bOwnDumpFile = FALSE;
		HANDLE hDumpFile = hFile;
		MINIDUMP_EXCEPTION_INFORMATION ExpParam;

		if ( hDumpFile == NULL || hDumpFile == INVALID_HANDLE_VALUE )
		{
			TCHAR szFileName[ MAX_PATH ] = { 0 };
			SYSTEMTIME stLocalTime;
			GetLocalTime( &stLocalTime );

			wsprintf( szFileName, _T( "%04d%02d%02d-%02d%02d%02d-%ld-%ld.dmp" ),
						stLocalTime.wYear, stLocalTime.wMonth, stLocalTime.wDay,
						stLocalTime.wHour, stLocalTime.wMinute, stLocalTime.wSecond,
						GetCurrentProcessId( ), GetCurrentThreadId( ) );
			hDumpFile = CreateFile( szFileName, GENERIC_READ | GENERIC_WRITE,
									FILE_SHARE_WRITE | FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0 );

			bOwnDumpFile = TRUE;
		}

		if ( hDumpFile != INVALID_HANDLE_VALUE )
		{
			ExpParam.ThreadId = GetCurrentThreadId( );
			ExpParam.ExceptionPointers = pExceptionPointers;
			ExpParam.ClientPointers = FALSE;

			MiniDumpWriteDump( GetCurrentProcess( ), GetCurrentProcessId( ),
									hDumpFile, MiniDumpWithFullMemory, ( pExceptionPointers ? &ExpParam : NULL ), NULL, NULL );

			if ( bOwnDumpFile )
				CloseHandle( hDumpFile );
		}

		MessageBoxA( NULL, "进程崩溃", NULL, NULL );
		return EXCEPTION_EXECUTE_HANDLER;
	}

	LONG WINAPI CNSBaseApp::RedirectedSetUnhandledExceptionFilter( EXCEPTION_POINTERS * /*ExceptionInfo*/ )
	{
		// When the CRT calls SetUnhandledExceptionFilter with NULL parameter
		// our handler will not get removed.
		return 0;
	}

	LONG WINAPI CNSBaseApp::OurSetUnhandledExceptionFilter( EXCEPTION_POINTERS* pExceptionInfo )
	{
		GenerateMiniDump( NULL, pExceptionInfo );
		return EXCEPTION_EXECUTE_HANDLER;
	}

	const CNSString& CNSBaseApp::name2IPAddress( const CNSString& machineIP )
	{
		static CNSVector< CNSString > ipSet;
		ipSet.clear( );
		machineIP.split( ":", ipSet );
		static CNSString machine;
		static CNSString address;
		if ( ipSet.getCount( ) == 2 )
		{
			machine = ipSet[ 0 ];
			address = ipSet[ 1 ];
		}
		else if ( ipSet.getCount( ) == 1 )
		{
			address = ipSet[ 0 ];
		}
		else
		{
			static CNSString errorDesc;
			errorDesc.format( _UTF8( "IP地址配置错误，地址标识格式错误[%s], 正确格式为[machine:ip]" ), machineIP.getBuffer( ) );
			NSException( errorDesc );
		}

		const CIPAddress* tpIPAddr = mMachineTable.get( machine );
		if ( address == "inner" )
		{
			if ( tpIPAddr == NULL )
			{
				static CNSString errorDesc;
				errorDesc.format( _UTF8( "IP地址配置错误，不存在Machine标识[%s]" ), machine.getBuffer( ) );
				NSException( errorDesc );
			}

			address.format( "%s", tpIPAddr->mInner.getBuffer( ) );
		}
		else if ( address == "outer" )
		{
			if ( tpIPAddr == NULL )
			{
				static CNSString errorDesc;
				errorDesc.format( _UTF8( "IP地址配置错误，不存在Machine标识[%s]" ), machine.getBuffer( ) );
				NSException( errorDesc );
			}

			address.format( "%s", tpIPAddr->mOuter.getBuffer( ) );
		}
		else if ( address == "client" )
		{
			if ( tpIPAddr == NULL )
			{
				static CNSString errorDesc;
				errorDesc.format( _UTF8( "IP地址配置错误，不存在Machine标识[%s]" ), machine.getBuffer( ) );
				NSException( errorDesc );
			}

			address.format( "%s", tpIPAddr->mClient.getBuffer( ) );
		}
		else if ( address == "local" )
			address.format( "127.0.0.1" );
		else
		{
			static CNSString errorDesc;
			errorDesc.format( _UTF8( "IP地址配置错误，不存在地址标识[%s]" ), address.getBuffer( ) );
			NSException( errorDesc );
		}

		return address;
	}

	const CNSString& CNSBaseApp::name2IPPort( const CNSString& machineIP )
	{
		static CNSVector< CNSString > ipSet;
		ipSet.clear( );

		machineIP.split( ":", ipSet );
		static CNSString machine;
		static CNSString address;
		static CNSString port;
		if ( ipSet.getCount( ) == 3 )
		{
			machine = ipSet[ 0 ];
			address = ipSet[ 1 ];
			port = ipSet[ 2 ];
		}
		else if ( ipSet.getCount( ) == 2 )
		{
			address = ipSet[ 0 ];
			port = ipSet[ 1 ];
		}
		else
		{
			static CNSString errorDesc;
			errorDesc.format( _UTF8( "IP地址配置错误，地址标识格式错误[%s], 正确格式为[machine:ip:port]" ), machineIP.isEmpty( ) == true ? "NULL" : machineIP.getBuffer( ) );
			NSException( errorDesc );
		}

		const CIPAddress* addr = mMachineTable.get( machine );
		if ( address == "inner" )
		{
			if ( addr == NULL )
			{
				static CNSString errorDesc;
				errorDesc.format( _UTF8( "IP地址配置错误，不存在Machine标识[%s]" ), machine.getBuffer( ) );
				NSException( errorDesc );
			}

			address.format( "%s:%s", addr->mInner.getBuffer( ), port.getBuffer( ) );
		}
		else if ( address == "outer" )
		{
			if ( addr == NULL )
			{
				static CNSString errorDesc;
				errorDesc.format( _UTF8( "IP地址配置错误，不存在Machine标识[%s]" ), machine.getBuffer( ) );
				NSException( errorDesc );
			}

			address.format( "%s:%s", addr->mOuter.getBuffer( ), port.getBuffer( ) );
		}
		else if ( address == "client" )
		{
			if ( addr == NULL )
			{
				static CNSString errorDesc;
				errorDesc.format( _UTF8( "IP地址配置错误，不存在Machine标识[%s]" ), machine.getBuffer( ) );
				NSException( errorDesc );
			}

			address.format( "%s:%s", addr->mClient.getBuffer( ), port.getBuffer( ) );
		}
		else if ( address == "local" )
			address.format( "127.0.0.1:%s", port.getBuffer( ) );
		else
		{
			static CNSString errorDesc;
			errorDesc.format( _UTF8( "IP地址配置错误，不存在地址标识[%s]" ), address.getBuffer( ) );
			NSException( errorDesc );
		}

		return address;
	}

	void CNSBaseApp::loadIPName( )
	{
		mMachineTable.clear( );
		TiXmlDocument doc;
		if ( doc.LoadFile( "machine.xml" ) == false )
		{
			static CNSString errorDesc;
			errorDesc.format( _UTF8( "machine.xml打开错误, 错误码[%s]" ), doc.ErrorDesc( ) );
			NSException( errorDesc );
		}

		TiXmlElement* machineTableElement = doc.FirstChildElement( "MachineTable" );
		if ( machineTableElement == NULL )
		{
			static CNSString errorDesc;
			errorDesc.format( _UTF8( "machine.xml文件格式错误, 节点MachineTable没有找到" ) );
			NSException( errorDesc );
		}

		TiXmlElement* machineItem = machineTableElement->FirstChildElement( "Machine" );
		for ( ; machineItem != NULL; machineItem = machineItem->NextSiblingElement( "Machine" ) )
		{
			CNSString machineName = machineItem->Attribute( "Name" );
			CNSString innerIP = machineItem->Attribute( "inner" );
			CNSString outerIP = machineItem->Attribute( "outer" );
			CNSString clientIP = machineItem->Attribute( "client" );
			if ( mMachineTable.findNode( machineName ) != NULL )
			{
				static CNSString errorDesc;
				errorDesc.format( _UTF8( "machine.xml文件格式错误, 节点Machine有重复节点，重复命名为[%s]" ), machineName.getBuffer( ) );
				NSException( errorDesc );
			}

			mMachineTable.insert( machineName, CIPAddress( innerIP, outerIP, clientIP ) );
		}
	}

	void CNSBaseApp::parseCommandLine( )
	{
		TCHAR* cmdLine = GetCommandLine( );
		const CNSString& cmd = CNSString::fromTChar( (char*) cmdLine );
		enum EParseState
		{
			PARSE_BEGIN,
			PARSE_FINDKEY,
			PRASE_FINDVALUE,
		};

		if ( mCmdList.getCount( ) > 0 )
			return;

		CNSVector< CNSString > argList;
		cmd.split( " ", argList );
		for ( unsigned int i = 1; i < argList.getCount( ); i ++ )
		{
			CNSString param = argList[ i ];
			CNSString key;
			CNSString value;
			int state = PARSE_BEGIN;
			for ( unsigned int t = 0; t < param.getCount( ); t ++ )
			{
				if ( state == PARSE_BEGIN && param[ t ] == '-' )
				{
					state = PARSE_FINDKEY;
					continue;
				}

				if ( state == PARSE_FINDKEY && param[ t ] == ':' )
				{
					state = PRASE_FINDVALUE;
					continue;
				}

				if ( state == PARSE_FINDKEY )
					key.pushback( param[ t ] );
				else if ( state == PRASE_FINDVALUE )
					value.pushback( param[ t ] );
			}

			mCmdList.insert( key, value );
		}
	}

	bool CNSBaseApp::getCmdBool( const CNSString& key, bool defaultValue )
	{
		CNSString* value = mCmdList.get( key );
		if ( value == NULL )
			return defaultValue;

		return value->toBoolean( );
	}

	int CNSBaseApp::getCmdInt32( const CNSString& key, int defaultValue )
	{
		CNSString* value = mCmdList.get( key );
		if ( value == NULL )
			return defaultValue;

		return value->toInteger( );
	}

	double CNSBaseApp::getCmdNumber( const CNSString& key, double defaultValue )
	{
		CNSString* value = mCmdList.get( key );
		if ( value == NULL )
			return defaultValue;

		return value->toDouble( );
	}

	CNSString CNSBaseApp::getCmdString( const CNSString& key, const CNSString& defaultValue )
	{
		CNSString* value = mCmdList.get( key );
		if ( value == NULL )
			return defaultValue;

		return *value;
	}

	void __stdcall CNSWindow::MoveTimer( HWND wnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime )
	{
		unsigned int curTick = CNSTimer::getCurTick( );
		CNSWindow::CMoveProxy::updateMove( curTick );
	}

	void __stdcall CNSWindow::FocusTimer( HWND wnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime )
	{
		::SetFocus( wnd );
		KillTimer( wnd, 100 );
	}

	void CNSWindow::redrawScrollbar( HDC dc )
	{
		LONG style = GetWindowStyle( getHWnd( ) );
		if ( style & WS_VSCROLL && style & WS_HSCROLL )
		{
			// 绘制公共矩形区域
			RECT ncRect;
			getScrollNcRect( ncRect );
			FillRect( dc, &ncRect, brushNCBack );
		}

		if ( style & WS_VSCROLL )
		{
			// 绘制滚动条背景
			RECT rcVScrollRect;
			getVScrollRect( rcVScrollRect );
			FillRect( dc, &rcVScrollRect, brushScrollBack );

			// 绘制纵向滚动条Thumb
			drawVScrollThumb( dc );

			// 绘制纵向滚动条向上按钮
			drawScrollUpButton( dc );

			// 绘制纵向滚动条向下按钮
			drawScrollDownButton( dc );
		}

		if ( style & WS_HSCROLL )
		{
			// 绘制滚动条背景
			RECT rcHScrollRect;
			getHScrollRect( rcHScrollRect );
			FillRect( dc, &rcHScrollRect, brushScrollBack );

			// 绘制纵向滚动条Thumb
			drawHScrollThumb( dc );

			// 绘制纵向滚动条向上按钮
			drawScrollLeftButton( dc );

			// 绘制纵向滚动条向下按钮
			drawScrollRightButton( dc );
		}
	}

	void CNSWindow::onLButtonDblClk( int flag, POINT pt )
	{
		DWORD style = GetWindowStyle( mHWnd );
		callWndProc( WM_LBUTTONDBLCLK, flag, MAKELPARAM( pt.x, pt.y ) );
		DWORD newStyle = GetWindowStyle( mHWnd );

		if ( newStyle & WS_CHILD && !( newStyle & WS_CAPTION ) )
		{
			HDC dc = GetWindowDC( mHWnd );
			redrawScrollbar( dc );
			ReleaseDC( mHWnd, dc );

			if ( newStyle != style )
			{
				RECT rcHScroll;
				GetClientRect( mHWnd, &rcHScroll );
				rcHScroll.top = rcHScroll.bottom - scrollWidth;
				InvalidateRect( mHWnd, &rcHScroll, FALSE );

				RECT rcVScroll;
				GetClientRect( mHWnd, &rcVScroll );
				rcVScroll.left = rcVScroll.right - scrollWidth;
				InvalidateRect( mHWnd, &rcVScroll, FALSE );
			}
		}
	}

	void CNSWindow::onLButtonDown( int flag, POINT pt )
	{
		DWORD style = GetWindowStyle( mHWnd );
		callWndProc( WM_LBUTTONDOWN, flag, MAKELPARAM( pt.x, pt.y ) );
		DWORD newStyle = GetWindowStyle( mHWnd );

		if ( newStyle & WS_CHILD && !( newStyle & WS_CAPTION ) )
		{
			HDC dc = GetWindowDC( mHWnd );
			redrawScrollbar( dc );
			ReleaseDC( mHWnd, dc );

			if ( newStyle != style )
			{
				RECT rcHScroll;
				GetClientRect( mHWnd, &rcHScroll );
				rcHScroll.top = rcHScroll.bottom - scrollWidth;
				InvalidateRect( mHWnd, &rcHScroll, FALSE );

				RECT rcVScroll;
				GetClientRect( mHWnd, &rcVScroll );
				rcVScroll.left = rcVScroll.right - scrollWidth;
				InvalidateRect( mHWnd, &rcVScroll, FALSE );
			}
		}
	}

	void CNSWindow::onMouseWheel( int delta, int flag, POINT pt )
	{
		DWORD style = GetWindowStyle( mHWnd );
		if ( style & WS_CHILD && !( style & WS_CAPTION ) )
		{
			HDC dc = GetWindowDC( mHWnd );
			redrawScrollbar( dc );
			ReleaseDC( mHWnd, dc );
		}
	}

	bool CNSWindow::onNcMouseLeave( )
	{
		DWORD style = GetWindowStyle( mHWnd );
		if ( style & WS_CHILD && !( style & WS_CAPTION ) )
		{
			HDC dc = GetWindowDC( mHWnd );
			mHoverScrollUp = false;
			mHoverScrollDown = false;
			mHoverScrollLeft = false;
			mHoverScrollRight = false;
			mHoverScrollVThumb = false;
			mHoverScrollHThumb = false;
			mNcTracking = false;

			// 绘制向上按钮的hover
			redrawScrollbar( dc );
			ReleaseDC( mHWnd, dc );
			return true;
		}

		return false;
	}

	void CNSWindow::onLButtonUp( int flag, POINT pt )
	{
		DWORD style = GetWindowStyle( mHWnd );
		if ( style & WS_CHILD && !( style & WS_CAPTION ) )
		{
			HDC dc = GetWindowDC( mHWnd );
			if ( style & WS_VSCROLL )
			{
				if ( mDownScrollUp == true )
				{
					ReleaseCapture( );
					mDownScrollUp = false;
					redrawScrollbar( dc );
				}
				else if ( mDownScrollDown == true )
				{
					ReleaseCapture( );
					mDownScrollDown = false;
					redrawScrollbar( dc );
				}
				else if ( mDownScrollVThumb == true )
				{
					ReleaseCapture( );
					mVScrollPos = -1;
					mDownScrollVThumb = false;
					redrawScrollbar( dc );
				}
			}

			if ( style & WS_HSCROLL )
			{
				if ( mDownScrollLeft == true )
				{
					ReleaseCapture( );
					mDownScrollLeft = false;
					redrawScrollbar( dc );
				}
				else if ( mDownScrollRight == true )
				{
					ReleaseCapture( );
					mDownScrollRight = false;
					redrawScrollbar( dc );
				}
				if ( mDownScrollHThumb == true )
				{
					ReleaseCapture( );
					mHScrollPos = -1;
					mDownScrollHThumb = false;
					redrawScrollbar( dc );
				}
			}

			ReleaseDC( mHWnd, dc );
		}
	}

	bool CNSWindow::onNcLButtonDblClk( int hitTest, POINT pt )
	{
		DWORD style = GetWindowStyle( mHWnd );
		if ( style & WS_CHILD && !( style & WS_CAPTION ) )
		{
			onScrollBarClicked( pt );
			return true;
		}

		return true;
	}

	bool CNSWindow::onNcLButtonDown( int hitTest, POINT pt )
	{
		DWORD style = GetWindowStyle( mHWnd );
		bool retValue = false;
		if ( mEnableTitle == true )
		{
			retValue = false;
			focus( );
		}

		if ( style & WS_CHILD && !( style & WS_CAPTION ) )
		{
			retValue = true;
			onScrollBarClicked( pt );
		}

		return retValue;
	}

	void CNSWindow::scrollHorz( int pos )
	{
		SCROLLINFO si;
		si.cbSize = sizeof( si );
		si.fMask = SIF_ALL;
		GetScrollInfo( mHWnd, SB_HORZ, &si );

		si.fMask = SIF_POS;
		si.nPos = pos;
		SetScrollInfo( mHWnd, SB_HORZ, &si, FALSE );
		SendMessage( mHWnd, WM_HSCROLL, ( pos << 16 ) | SB_THUMBTRACK, NULL );
	}

	void CNSWindow::scrollVert( int pos )
	{
		SCROLLINFO si;
		si.cbSize = sizeof( si );
		si.fMask = SIF_ALL;
		GetScrollInfo( mHWnd, SB_VERT, &si );

		si.fMask = SIF_POS;
		si.nPos = pos;
		SetScrollInfo( mHWnd, SB_VERT, &si, FALSE );
		SendMessage( mHWnd, WM_VSCROLL, ( pos << 16 ) | SB_THUMBTRACK, NULL );
	}

	void CNSWindow::onMouseMove( int flag, POINT pt )
	{
		DWORD style = GetWindowStyle( mHWnd );
		if ( style & WS_CHILD && !( style & WS_CAPTION ) )
		{
			if ( mDownScrollVThumb == true )
			{
				RECT rcClient;
				getClientRect( rcClient );
				if ( pt.x < ( rcClient.right - rcClient.left - 50 ) )
					return;
				else if ( pt.x > ( rcClient.right - rcClient.left + 50 ) )
					return;

				RECT rcVScroll;
				getVScrollRect( rcVScroll );

				RECT rcVScrollThumb;
				getVScrollThumbRect( rcVScrollThumb );
				int thumbHeight = rcVScrollThumb.bottom - rcVScrollThumb.top;

				HDC dc = GetWindowDC( mHWnd );
				int deltaY = pt.y - mLastMouse.y;
				int rangeMin = rcVScroll.top + scrollWidth;
				int rangeMax = rcVScroll.bottom - scrollWidth - thumbHeight;

				mVScrollPos = mVScrollPos + deltaY;
				mVScrollPos = max( rangeMin, mVScrollPos );
				mVScrollPos = min( rangeMax, mVScrollPos );
				redrawScrollbar( dc );

				SCROLLINFO si;
				si.cbSize = sizeof( si );
				si.fMask = SIF_ALL;
				GetScrollInfo( mHWnd, SB_VERT, &si );

				double percent = ( mVScrollPos - rangeMin ) / (double) ( rangeMax - rangeMin );
				int pos = NSFunction::round( ( si.nMax - si.nMin + 1 - si.nPage ) * percent );
				scrollVert( pos );
				mLastMouse = pt;
				ReleaseDC( mHWnd, dc );
			}

			if ( mDownScrollHThumb == true )
			{
				RECT rcClient;
				getClientRect( rcClient );
				if ( pt.y < ( rcClient.bottom - rcClient.top - 50 ) )
					return;
				else if ( pt.y > ( rcClient.bottom - rcClient.top + 50 ) )
					return;

				RECT rcHScroll;
				getHScrollRect( rcHScroll );

				RECT rcHScrollThumb;
				getHScrollThumbRect( rcHScrollThumb );
				int thumbHeight = rcHScrollThumb.right - rcHScrollThumb.left;

				HDC dc = GetWindowDC( getHWnd( ) );
				int deltaX = pt.x - mLastMouse.x;
				int rangeMin = rcHScroll.left + scrollWidth;
				int rangeMax = rcHScroll.right - scrollWidth - thumbHeight;

				mHScrollPos = mHScrollPos + deltaX;
				mHScrollPos = max( rangeMin, mHScrollPos );
				mHScrollPos = min( rangeMax, mHScrollPos );
				redrawScrollbar( dc );

				SCROLLINFO si;
				si.cbSize = sizeof( si );
				si.fMask = SIF_ALL;
				GetScrollInfo( getHWnd( ), SB_HORZ, &si );

				double percent = ( mHScrollPos - rangeMin ) / (double) ( rangeMax - rangeMin );
				int pos = NSFunction::round( ( si.nMax - si.nMin + 1 - si.nPage ) * percent );
				scrollHorz( pos );
				mLastMouse = pt;
				ReleaseDC( getHWnd( ), dc );
			}
		}
	}

	bool CNSWindow::onNcMouseMove( int hitTest, POINT pt )
	{
		DWORD style = GetWindowStyle( mHWnd );
		if ( style & WS_CHILD && !( style & WS_CAPTION ) )
		{
			HDC dc = GetWindowDC( mHWnd );
			bool needRedraw = false;
			if ( style & WS_VSCROLL )
			{
				// 处理向上按钮
				RECT rcUpRect;
				getScrollUpRect( rcUpRect );

				// 绘制向上按钮的hover
				if ( PtInRect( &rcUpRect, pt ) == TRUE )
				{
					// 如果命中滚动条向上按钮
					if ( mHoverScrollUp == false )
					{
						mHoverScrollUp = true;
						needRedraw = true;
					}
				}
				else
				{
					// 如果命中滚动条向下按钮
					if ( mHoverScrollUp == true )
					{
						mHoverScrollUp = false;
						needRedraw = true;
					}
				}

				RECT rcThumb;
				getVScrollThumbRect( rcThumb );
				if ( PtInRect( &rcThumb, pt ) == TRUE )
				{
					if ( mHoverScrollVThumb == false )
					{
						mHoverScrollVThumb = true;
						needRedraw = true;
					}
				}
				else
				{
					if ( mHoverScrollVThumb == true )
					{
						mHoverScrollVThumb = false;
						needRedraw = true;
					}
				}

				// 处理向下按钮
				RECT rcDownRect;
				getScrollDownRect( rcDownRect );

				// 绘制向下按钮的hover
				if ( PtInRect( &rcDownRect, pt ) == TRUE )
				{
					if ( mHoverScrollDown == false )
					{
						mHoverScrollDown = true;
						needRedraw = true;
					}
				}
				else
				{
					if ( mHoverScrollDown == true )
					{
						mHoverScrollDown = false;
						needRedraw = true;
					}
				}
			}

			if ( style & WS_HSCROLL )
			{
				// 处理向上按钮
				RECT rcLeftRect;
				getScrollLeftRect( rcLeftRect );

				// 绘制向上按钮的hover
				if ( PtInRect( &rcLeftRect, pt ) == TRUE )
				{
					// 如果命中滚动条向上按钮
					if ( mHoverScrollLeft == false )
					{
						mHoverScrollLeft = true;
						needRedraw = true;
					}
				}
				else
				{
					// 如果命中滚动条向下按钮
					if ( mHoverScrollLeft == true )
					{
						mHoverScrollLeft = false;
						needRedraw = true;
					}
				}

				RECT rcThumb;
				getHScrollThumbRect( rcThumb );
				if ( PtInRect( &rcThumb, pt ) == TRUE )
				{
					if ( mHoverScrollHThumb == false )
					{
						mHoverScrollHThumb = true;
						needRedraw = true;
					}
				}
				else
				{
					if ( mHoverScrollHThumb == true )
					{
						mHoverScrollHThumb = false;
						needRedraw = true;
					}
				}

				// 处理向下按钮
				RECT rcRightRect;
				getScrollRightRect( rcRightRect );

				// 绘制向下按钮的hover
				if ( PtInRect( &rcRightRect, pt ) == TRUE )
				{
					if ( mHoverScrollRight == false )
					{
						mHoverScrollRight = true;
						needRedraw = true;
					}
				}
				else
				{
					if ( mHoverScrollRight == true )
					{
						mHoverScrollRight = false;
						needRedraw = true;
					}
				}
			}

			if ( needRedraw == true )
				redrawScrollbar( dc );

			if ( mNcTracking == false )
			{
				TRACKMOUSEEVENT tme;
				tme.cbSize = sizeof( TRACKMOUSEEVENT );
				tme.dwFlags = TME_LEAVE | TME_NONCLIENT;
				tme.hwndTrack = mHWnd;
				tme.dwHoverTime = HOVER_DEFAULT;
				TrackMouseEvent( &tme );
				mNcTracking = true;
			}

			ReleaseDC( mHWnd, dc );
			return true;
		}

		return false;
	}

	void CNSWindow::onKillFocus( HWND newFocus )
	{
	}

	void CNSWindow::onSetFocus( HWND lastFocus )
	{
		CNSSet< CNSWindow* > highList;
		CNSWindow* parent = this;
		for ( ; parent != NULL; parent = parent->getParent( ) )
		{
			if ( parent->mTitleHighlight == false && parent->mEnableTitle == true )
			{
				parent->mTitleHighlight = true;
				HDC dc = GetWindowDC( parent->mHWnd );
				parent->drawTitle( dc );
				ReleaseDC( parent->mHWnd, dc );
			}

			if ( parent->mEnableTitle == true )
				highList.insert( parent );
		}

		CNSWindow* lastParent = CNSWindow::fromHWnd( lastFocus );
		for ( ; lastParent != NULL; lastParent = lastParent->getParent( ) )
		{
			if ( lastParent->mEnableTitle == true && highList.find( lastParent ) == false )
			{
				lastParent->mTitleHighlight = false;
				HDC dc = GetWindowDC( lastParent->mHWnd );
				lastParent->drawTitle( dc );
				ReleaseDC( lastParent->mHWnd, dc );
			}
		}
	}

	void CNSWindow::onHScroll( int flag, int pos )
	{
		DWORD style = GetWindowStyle( getHWnd( ) );
		if ( style & WS_CHILD && !( style & WS_CAPTION ) )
		{
			HDC dc = GetWindowDC( getHWnd( ) );
			redrawScrollbar( dc );
			ReleaseDC( getHWnd( ), dc );
		}
	}

	void CNSWindow::onVScroll( int flag, int pos )
	{
		DWORD style = GetWindowStyle( getHWnd( ) );
		if ( style & WS_CHILD && !( style & WS_CAPTION ) )
		{
			HDC dc = GetWindowDC( getHWnd( ) );
			redrawScrollbar( dc );
			ReleaseDC( getHWnd( ), dc );
		}
	}

	void CNSWindow::onSize( int width, int height, int sizeFlag )
	{
		SIZE szNew;
		szNew.cx = width;
		szNew.cy = height;

		if ( sizeFlag == SIZE_MINIMIZED )
			return;

		HLISTINDEX beginIndex = mChildren.getHead( );
		for ( ; beginIndex != NULL; mChildren.getNext( beginIndex ) )
		{
			CNSWindow* child = mChildren.getValue( beginIndex );
			if ( child->mProcessAnchor == false )
				continue;

			RECT newRect = child->calcSizedRect( szNew, mLastSize );
			MoveWindow( child->getHWnd( ), newRect.left, newRect.top, newRect.right - newRect.left, newRect.bottom - newRect.top, TRUE );

			RECT newClient;
			child->getClientRect( newClient );
			// 计算child需要重绘制区域
		}

		mLastSize = szNew;

		DWORD style = GetWindowStyle( mHWnd );
		callWndProc( WM_SIZE, sizeFlag, MAKELPARAM( width, height ) );
		DWORD newStyle = GetWindowStyle( mHWnd );

		if ( newStyle & WS_CHILD && !( newStyle & WS_CAPTION ) )
		{
			HDC dc = GetWindowDC( mHWnd );
			redrawScrollbar( dc );
			ReleaseDC( mHWnd, dc );

			if ( newStyle != style )
			{
				RECT rcHScroll;
				GetClientRect( mHWnd, &rcHScroll );
				rcHScroll.top = rcHScroll.bottom - scrollWidth;
				InvalidateRect( mHWnd, &rcHScroll, TRUE );

				RECT rcVScroll;
				GetClientRect( mHWnd, &rcVScroll );
				rcVScroll.left = rcVScroll.right - scrollWidth;
				InvalidateRect( mHWnd, &rcVScroll, TRUE );
			}
		}
	}

	bool CNSWindow::onNcPaint( HRGN rgn )
	{
		DWORD style = GetWindowStyle( mHWnd );
		if ( style & WS_CHILD && !( style & WS_CAPTION ) )
		{
			// 只有无标题子窗口才有VS风格title
			HDC dc = NULL;
			if ( rgn == (HRGN) 1 )
				dc = GetWindowDC( mHWnd );
			else
				dc = GetDCEx( mHWnd, rgn, DCX_WINDOW | DCX_INTERSECTRGN | DCX_CACHE );

			if ( mEnableTitle == true )
				drawTitle( dc );

			redrawScrollbar( dc );
			ReleaseDC( getHWnd( ), dc );
			return true;
		}

		return false;
	}

	bool CNSWindow::onNcCalcSize( bool clientArea, NCCALCSIZE_PARAMS* calcSize )
	{
		DWORD style = GetWindowStyle( mHWnd );
		if ( style & WS_CHILD && !( style & WS_CAPTION ) )
		{
			if ( clientArea == true )
			{
				// 改变窗口大小的时候触发
				RECT rcNewWindow = calcSize->rgrc[ 0 ];
				RECT rcOldWindow = calcSize->rgrc[ 1 ];
				RECT rcOldClient = calcSize->rgrc[ 2 ];

				LONG style = GetWindowStyle( mHWnd );
				int curHeight = rcNewWindow.bottom - rcNewWindow.top;
				int curWidth = rcNewWindow.right - rcNewWindow.left;
				if ( style & WS_HSCROLL )
				{
					int newHeight = curHeight - scrollWidth;
					calcSize->rgrc[ 0 ].bottom = calcSize->rgrc[ 0 ].top + newHeight;
				}

				if ( style & WS_VSCROLL )
				{
					int newWidth = curWidth - scrollWidth;
					calcSize->rgrc[ 0 ].right = calcSize->rgrc[ 0 ].left + newWidth;
				}

				if ( mEnableTitle == true )
				{
					calcSize->rgrc[ 0 ].top += titleHeight;
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

		return false;
	}

	void CNSWindow::onScrollBarClicked( POINT& mouse )
	{
		DWORD style = GetWindowStyle( getHWnd( ) );
		HDC dc = GetWindowDC( getHWnd( ) );

		if ( style & WS_VSCROLL )
		{
			RECT rcUpRect;
			getScrollUpRect( rcUpRect );

			RECT rcDownRect;
			getScrollDownRect( rcDownRect );

			RECT rcThumbRect;
			getVScrollThumbRect( rcThumbRect );

			if ( PtInRect( &rcUpRect, mouse ) == TRUE )
			{
				SendMessage( getHWnd( ), WM_VSCROLL, SB_LINEUP, NULL );

				mDownScrollUp = true;
				redrawScrollbar( dc );
				SetCapture( getHWnd( ) );
			}
			else if ( PtInRect( &rcDownRect, mouse ) == TRUE )
			{
				SendMessage( getHWnd( ), WM_VSCROLL, SB_LINEDOWN, NULL );

				mDownScrollDown = true;
				redrawScrollbar( dc );
				SetCapture( getHWnd( ) );
			}
			else if ( PtInRect( &rcThumbRect, mouse ) == TRUE )
			{
				mLastMouse = mouse;
				mDownScrollVThumb = true;

				RECT rcVScrollThumb;
				getVScrollThumbRect( rcVScrollThumb );

				mVScrollPos = rcVScrollThumb.top;
				redrawScrollbar( dc );
				SetCapture( getHWnd( ) );
			}
		}

		if ( style & WS_HSCROLL )
		{
			RECT rcLeftRect;
			getScrollLeftRect( rcLeftRect );

			RECT rcRightRect;
			getScrollRightRect( rcRightRect );

			RECT rcThumbRect;
			getHScrollThumbRect( rcThumbRect );

			if ( PtInRect( &rcLeftRect, mouse ) == TRUE )
			{
				SendMessage( getHWnd( ), WM_HSCROLL, SB_LINELEFT, NULL );

				mDownScrollLeft = true;
				redrawScrollbar( dc );
				SetCapture( getHWnd( ) );
			}
			else if ( PtInRect( &rcRightRect, mouse ) == TRUE )
			{
				SendMessage( getHWnd( ), WM_HSCROLL, SB_LINERIGHT, NULL );

				mDownScrollRight = true;
				redrawScrollbar( dc );
				SetCapture( getHWnd( ) );
			}
			else if ( PtInRect( &rcThumbRect, mouse ) == TRUE )
			{
				mLastMouse = mouse;
				mDownScrollHThumb = true;

				RECT rcHScrollThumb;
				getHScrollThumbRect( rcHScrollThumb );

				mHScrollPos = rcHScrollThumb.left;
				redrawScrollbar( dc );
				SetCapture( getHWnd( ) );
			}
		}

		ReleaseDC( getHWnd( ), dc );
	}

	void CNSWindow::getScrollNcRect( RECT& rc )
	{
		GetWindowRect( getHWnd( ), &rc );
		OffsetRect( &rc, -rc.left, -rc.top );

		rc.left = rc.right - scrollWidth;
		rc.top = rc.bottom - scrollWidth;
	}

	void CNSWindow::getVScrollRect( RECT& rc )
	{
		LONG style = GetWindowStyle( getHWnd( ) );
		if ( style & WS_VSCROLL )
		{
			GetWindowRect( getHWnd( ), &rc );
			OffsetRect( &rc, -rc.left, -rc.top );

			if ( style & WS_HSCROLL )
			{
				rc.bottom = rc.bottom - scrollWidth;
				rc.left = rc.right - scrollWidth;
			}
			else
				rc.left = rc.right - scrollWidth;

			if ( mEnableTitle == true )
				rc.top += titleHeight;
		}
	}

	void CNSWindow::getHScrollRect( RECT& rc )
	{
		LONG style = GetWindowStyle( getHWnd( ) );
		if ( style & WS_HSCROLL )
		{
			GetWindowRect( getHWnd( ), &rc );
			OffsetRect( &rc, -rc.left, -rc.top );

			if ( style & WS_VSCROLL )
			{
				rc.right = rc.right - scrollWidth;
				rc.top = rc.bottom - scrollWidth;
			}
			else
				rc.top = rc.bottom - scrollWidth;
		}
	}

	void CNSWindow::getScrollUpRect( RECT& rc )
	{
		LONG style = GetWindowStyle( getHWnd( ) );
		if ( style & WS_VSCROLL )
		{
			GetWindowRect( getHWnd( ), &rc );
			OffsetRect( &rc, -rc.left, -rc.top );

			rc.left = rc.right - scrollWidth;
			rc.bottom = rc.top + scrollWidth;

			if ( mEnableTitle == true )
			{
				rc.top += titleHeight;
				rc.bottom += titleHeight;
			}
		}
	}

	void CNSWindow::getScrollDownRect( RECT& rc )
	{
		LONG style = GetWindowStyle( getHWnd( ) );
		if ( style & WS_VSCROLL )
		{
			GetWindowRect( getHWnd( ), &rc );
			OffsetRect( &rc, -rc.left, -rc.top );

			if ( style & WS_HSCROLL )
			{
				rc.bottom = rc.bottom - scrollWidth;
				rc.top = rc.bottom - scrollWidth;
				rc.left = rc.right - scrollWidth;
			}
			else
			{
				rc.top = rc.bottom - scrollWidth;
				rc.left = rc.right - scrollWidth;
			}
		}
	}

	void CNSWindow::getScrollLeftRect( RECT& rc )
	{
		LONG style = GetWindowStyle( getHWnd( ) );
		if ( style & WS_HSCROLL )
		{
			GetWindowRect( getHWnd( ), &rc );
			OffsetRect( &rc, -rc.left, -rc.top );

			rc.top = rc.bottom - scrollWidth;
			rc.right = rc.left + scrollWidth;
		}
	}

	void CNSWindow::getScrollRightRect( RECT& rc )
	{
		LONG style = GetWindowStyle( getHWnd( ) );
		if ( style & WS_HSCROLL )
		{
			GetWindowRect( getHWnd( ), &rc );
			OffsetRect( &rc, -rc.left, -rc.top );

			if ( style & WS_VSCROLL )
			{
				rc.right = rc.right - scrollWidth;
				rc.left = rc.right - scrollWidth;
				rc.top = rc.bottom - scrollWidth;
			}
			else
			{
				rc.left = rc.right - scrollWidth;
				rc.top = rc.bottom - scrollWidth;
			}
		}
	}

	void CNSWindow::drawScrollUpButton( HDC dc )
	{
		HPEN	oldPen = NULL;
		HBRUSH	oldBrush = NULL;
		if ( mDownScrollUp == true )
		{
			oldPen = (HPEN) SelectObject( dc, penScrollClickBtn );
			oldBrush = (HBRUSH) SelectObject( dc, brushScrollClickBtn );
		}
		else if ( mHoverScrollUp == true )
		{
			oldPen = (HPEN) SelectObject( dc, penScrollHoverBtn );
			oldBrush = (HBRUSH) SelectObject( dc, brushScrollHoverBtn );
		}
		else
		{
			oldPen = (HPEN) SelectObject( dc, penScrollBtn );
			oldBrush = (HBRUSH) SelectObject( dc, brushScrollBtn );
		}

		RECT rc;
		getScrollUpRect( rc );

		POINT pt[ 3 ];
		pt[ 0 ].x = rc.left + 8;
		pt[ 0 ].y = rc.top + 6;
		pt[ 1 ].x = rc.left + 12;
		pt[ 1 ].y = rc.top + 10;
		pt[ 2 ].x = rc.left + 4;
		pt[ 2 ].y = rc.top + 10;
		Polygon( dc, pt, 3 );

		SelectObject( dc, oldPen );
		SelectObject( dc, oldBrush );
	}

	void CNSWindow::drawScrollDownButton( HDC dc )
	{
		HPEN	oldPen = NULL;
		HBRUSH	oldBrush = NULL;
		if ( mDownScrollDown == true )
		{
			oldPen = (HPEN) SelectObject( dc, penScrollClickBtn );
			oldBrush = (HBRUSH) SelectObject( dc, brushScrollClickBtn );
		}
		else if ( mHoverScrollDown == true )
		{
			oldPen = (HPEN) SelectObject( dc, penScrollHoverBtn );
			oldBrush = (HBRUSH) SelectObject( dc, brushScrollHoverBtn );
		}
		else
		{
			oldPen = (HPEN) SelectObject( dc, penScrollBtn );
			oldBrush = (HBRUSH) SelectObject( dc, brushScrollBtn );
		}

		RECT rc;
		getScrollDownRect( rc );

		POINT pt[ 3 ];
		pt[ 0 ].x = rc.left + 8;
		pt[ 0 ].y = rc.bottom - 6;
		pt[ 1 ].x = rc.left + 12;
		pt[ 1 ].y = rc.bottom - 10;
		pt[ 2 ].x = rc.left + 4;
		pt[ 2 ].y = rc.bottom - 10;
		Polygon( dc, pt, 3 );

		SelectObject( dc, oldPen );
		SelectObject( dc, oldBrush );
	}

	void CNSWindow::drawScrollLeftButton( HDC dc )
	{
		HPEN	oldPen = NULL;
		HBRUSH	oldBrush = NULL;
		if ( mDownScrollLeft == true )
		{
			oldPen = (HPEN) SelectObject( dc, penScrollClickBtn );
			oldBrush = (HBRUSH) SelectObject( dc, brushScrollClickBtn );
		}
		else if ( mHoverScrollLeft == true )
		{
			oldPen = (HPEN) SelectObject( dc, penScrollHoverBtn );
			oldBrush = (HBRUSH) SelectObject( dc, brushScrollHoverBtn );
		}
		else
		{
			oldPen = (HPEN) SelectObject( dc, penScrollBtn );
			oldBrush = (HBRUSH) SelectObject( dc, brushScrollBtn );
		}

		RECT rc;
		getScrollLeftRect( rc );

		POINT pt[ 3 ];
		pt[ 0 ].x = rc.left + 6;
		pt[ 0 ].y = rc.top + 8;
		pt[ 1 ].x = rc.left + 10;
		pt[ 1 ].y = rc.top + 4;
		pt[ 2 ].x = rc.left + 10;
		pt[ 2 ].y = rc.top + 12;
		Polygon( dc, pt, 3 );

		SelectObject( dc, oldPen );
		SelectObject( dc, oldBrush );
	}

	void CNSWindow::drawScrollRightButton( HDC dc )
	{
		HPEN	oldPen = NULL;
		HBRUSH	oldBrush = NULL;
		if ( mDownScrollRight == true )
		{
			oldPen = (HPEN) SelectObject( dc, penScrollClickBtn );
			oldBrush = (HBRUSH) SelectObject( dc, brushScrollClickBtn );
		}
		else if ( mHoverScrollRight == true )
		{
			oldPen = (HPEN) SelectObject( dc, penScrollHoverBtn );
			oldBrush = (HBRUSH) SelectObject( dc, brushScrollHoverBtn );
		}
		else
		{
			oldPen = (HPEN) SelectObject( dc, penScrollBtn );
			oldBrush = (HBRUSH) SelectObject( dc, brushScrollBtn );
		}

		RECT rc;
		getScrollRightRect( rc );

		POINT pt[ 3 ];
		pt[ 0 ].x = rc.right - 6;
		pt[ 0 ].y = rc.bottom - 8;
		pt[ 1 ].x = rc.right - 10;
		pt[ 1 ].y = rc.bottom - 4;
		pt[ 2 ].x = rc.right - 10;
		pt[ 2 ].y = rc.bottom - 12;
		Polygon( dc, pt, 3 );

		SelectObject( dc, oldPen );
		SelectObject( dc, oldBrush );
	}

	void CNSWindow::getVScrollThumbRect( RECT& rc )
	{
		RECT rcVScrollRect;
		getVScrollRect( rcVScrollRect );

		SCROLLINFO si;
		si.cbSize = sizeof( si );
		si.fMask = SIF_RANGE | SIF_TRACKPOS | SIF_PAGE | SIF_POS;
		GetScrollInfo( getHWnd( ), SB_VERT, &si );

		float start = si.nPos / (float) ( si.nMax - si.nMin + 1 );
		float end = start + si.nPage / (float) ( si.nMax - si.nMin + 1 );

		RECT rcRange = rcVScrollRect;
		rcRange.top += scrollWidth;
		rcRange.bottom -= scrollWidth;

		rc = rcRange;
		int height = rc.bottom - rc.top;
		rc.top = rcRange.top + (int) ( height * start );
		rc.bottom = rcRange.top + (int) ( height * end );
		if ( mVScrollPos != -1 )
		{
			height = rc.bottom - rc.top;
			rc.top = mVScrollPos;
			rc.bottom = mVScrollPos + height;
		}
	}

	void CNSWindow::getHScrollThumbRect( RECT& rc )
	{
		RECT rcHScrollRect;
		getHScrollRect( rcHScrollRect );

		SCROLLINFO si;
		si.cbSize = sizeof( si );
		si.fMask = SIF_RANGE | SIF_TRACKPOS | SIF_PAGE | SIF_POS;
		GetScrollInfo( getHWnd( ), SB_HORZ, &si );

		float start = si.nPos / (float) ( si.nMax - si.nMin + 1 );
		float end = start + si.nPage / (float) ( si.nMax - si.nMin + 1 );

		RECT rcRange = rcHScrollRect;
		rcRange.left += scrollWidth;
		rcRange.right -= scrollWidth;

		rc = rcRange;
		int width = rc.right - rc.left;
		rc.left = rcRange.left + (int) ( width * start );
		rc.right = rcRange.left + (int) ( width * end );
	}

	void CNSWindow::getTitleRect( RECT& rc )
	{
		if ( mEnableTitle == true )
		{
			GetWindowRect( mHWnd, &rc );
			OffsetRect( &rc, -rc.left, -rc.top );

			rc.bottom = rc.top + titleHeight;
		}
	}

	void CNSWindow::drawTitle( HDC dc, bool headerLine )
	{
		RECT rcTitle;
		getTitleRect( rcTitle );
		FillRect( dc, &rcTitle, brushNCBack );

		if ( headerLine == true )
		{
			HPEN oldPen = (HPEN) SelectObject( dc, penBorder );
			MoveToEx( dc, rcTitle.left, rcTitle.bottom - 1, NULL );
			LineTo( dc, rcTitle.right, rcTitle.bottom - 1 );
			SelectObject( dc, oldPen );
		}

		rcTitle.bottom -= 4;
		if ( mTitleHighlight == true )
			FillRect( dc, &rcTitle, brushFocusWindow );

		InflateRect( &rcTitle, -3, 0 );
		TCHAR* text = (TCHAR*) CNSString::toTChar( mTitle );
		HFONT oldFont = (HFONT) SelectObject( dc, CNSPreDefine::FB_FONT_BASE );
		int oldBkMode = SetBkMode( dc, TRANSPARENT );
		if ( mTitleHighlight == true )
		{
			COLORREF oldTextColor = SetTextColor( dc, RGB( 255, 255, 255 ) );
			DrawText( dc, text, -1, &rcTitle, DT_LEFT | DT_VCENTER | DT_SINGLELINE );
			SetTextColor( dc, oldTextColor );
		}
		else
		{
			COLORREF oldTextColor = SetTextColor( dc, RGB( 208, 208, 208 ) );
			DrawText( dc, text, -1, &rcTitle, DT_LEFT | DT_VCENTER | DT_SINGLELINE );
			SetTextColor( dc, oldTextColor );
		}

		SIZE sz;
		GetTextExtentPoint( dc, text, lstrlen( text ), &sz );

		int width = rcTitle.right - rcTitle.left - 3 - 3 - sz.cx - 5;
		int left = rcTitle.left + 3 + sz.cx + 5;
		int center = ( rcTitle.bottom - rcTitle.top ) >> 1;
		for ( int i = 0; i <= width; i += 4 )
		{
			if ( mTitleHighlight == true )
			{
				SetPixel( dc, left + i, center - 2, RGB( 89, 168, 222 ) );
				if ( i + 4 <= width )
					SetPixel( dc, left + i + 2, center, RGB( 89, 168, 222 ) );
				SetPixel( dc, left + i, center + 2, RGB( 89, 168, 222 ) );
			}
			else
			{
				SetPixel( dc, left + i, center - 2, RGB( 70, 70, 74 ) );
				if ( i + 4 <= width )
					SetPixel( dc, left + i + 2, center, RGB( 70, 70, 74 ) );
				SetPixel( dc, left + i, center + 2, RGB( 70, 70, 74 ) );
			}
		}

		SelectObject( dc, oldFont );
		SetBkMode( dc, oldBkMode );
	}

	void CNSWindow::drawVScrollThumb( HDC dc )
	{
		RECT rcThumb;
		getVScrollThumbRect( rcThumb );
		InflateRect( &rcThumb, -4, 0 );

		if ( mDownScrollVThumb == true )
			FillRect( dc, &rcThumb, brushScrollClickThumb );
		else if ( mHoverScrollVThumb == true )
			FillRect( dc, &rcThumb, brushScrollHoverThumb );
		else
			FillRect( dc, &rcThumb, brushScrollThumb );
	}

	void CNSWindow::drawHScrollThumb( HDC dc )
	{
		RECT rcThumb;
		getHScrollThumbRect( rcThumb );
		InflateRect( &rcThumb, 0, -4 );

		if ( mDownScrollHThumb == true )
			FillRect( dc, &rcThumb, brushScrollClickThumb );
		else if ( mHoverScrollHThumb == true )
			FillRect( dc, &rcThumb, brushScrollHoverThumb );
		else
			FillRect( dc, &rcThumb, brushScrollThumb );
	}

	LRESULT CALLBACK CNSWindow::windowProc( HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam )
	{
		WNDPROC oldProc = NULL;
		try
		{
			ATOM classAtom = (ATOM) GetClassLongPtr( wnd, GCW_ATOM );
			WNDPROC* oldProcRef = mWndProc.get( classAtom );
			if ( oldProcRef == NULL )
			{
				static CNSString errorDesc;
				errorDesc.format( _UTF8( "收到消息 - 0x%04x时, 没有找到源类型窗口回调, WPARAM - %zd, LPARAM - %zd" ), msg, wParam, lParam );
				NSException( errorDesc );
			}

			oldProc = ( *oldProcRef );
			if ( oldProc == NULL )
			{
				static CNSString errorDesc;
				errorDesc.format( _UTF8( "收到消息 - 0x%04x时, 源类型窗口回调是个空指针, WPARAM - %zd, LPARAM - %zd" ), msg, wParam, lParam );
				NSException( errorDesc );
			}

			switch ( msg )
			{
				case WM_NCCREATE:
				{
					CREATESTRUCT* cs = (CREATESTRUCT*) lParam;
					NSWin32::CNSWindow* window = ( NSWin32::CNSWindow* ) cs->lpCreateParams;
					window->mHWnd = wnd;
					SetWindowLongPtr( wnd, GWLP_USERDATA, (LONG_PTR) window );

					if ( sWindows.get( window->mWindowID ) != NULL )
					{
						static CNSString errorDesc;
						errorDesc.format( _UTF8( "窗口名已经存在, windowID - %s" ), window->mWindowID.getBuffer( ) );
						NSException( errorDesc );
					}

					sWindows.insert( window->mWindowID, window );
					window->onNcCreateWindow( );
					return ::CallWindowProc( oldProc, wnd, msg, wParam, lParam );
				}
				case WM_CREATE:
				{
					CREATESTRUCT* cs = (CREATESTRUCT*) lParam;
					NSWin32::CNSWindow* window = ( NSWin32::CNSWindow* ) cs->lpCreateParams;
					window->onCreateWindow( );
					return ::CallWindowProc( oldProc, wnd, msg, wParam, lParam );
				}
				case WM_GETMINMAXINFO:
				{
					CNSWindow* window = (CNSWindow*) CNSWindow::fromHWnd( wnd );
					if ( window == NULL )
						return ::CallWindowProc( oldProc, wnd, msg, wParam, lParam );
				}
			}

			CNSWindow* window = (CNSWindow*) CNSWindow::fromHWnd( wnd );
			if ( window == NULL )
			{
				static CNSString errorDesc;
				errorDesc.format( _UTF8( "收到消息 - 0x%04x时, 没有找到窗口逻辑对象, WPARAM - %zd, LPARAM - %zd" ), msg, wParam, lParam );
				NSException( errorDesc );
			}

			LRESULT retValue = 0;
			CNSWindow::WindowEventHandler* hander = window->messageHandler.get( msg );
			if ( hander != NULL )
			{
				const CNSString& windowID = window->mWindowID;
				if ( ( *hander )( window, wParam, lParam, &retValue ) == true && msg != WM_NCDESTROY )
					return retValue;

				if ( CNSWindow::sWindows.get( windowID ) == NULL )
					return ::CallWindowProc( oldProc, wnd, msg, wParam, lParam );
			}

			switch ( msg )
			{
				case WM_SYSKEYDOWN:
					return 0;
				case WM_NCCALCSIZE:
				{
					bool clientArea = ( (BOOL) wParam == TRUE ) ? true : false;
					NCCALCSIZE_PARAMS* calcSize = (NCCALCSIZE_PARAMS*) lParam;
					if ( window->onNcCalcSize( clientArea, calcSize ) == true )
						return WVR_VALIDRECTS;

					break;
				}
				case WM_VSCROLL:
				{
					int flag = LOWORD( wParam );
					int pos = HIWORD( wParam );
					window->onVScroll( flag, pos );
					break;
				}
				case WM_HSCROLL:
				{
					int flag = LOWORD( wParam );
					int pos = HIWORD( wParam );
					window->onHScroll( flag, pos );
					break;
				}
				case WM_MOUSEMOVE:
				{
					int flag = (int) wParam;

					POINT ptClient;
					ptClient.x = GET_X_LPARAM( lParam );
					ptClient.y = GET_Y_LPARAM( lParam );
					window->onMouseMove( flag, ptClient );
					break;
				}
				case WM_MOUSEWHEEL:
				{
					int delta = ( (short) HIWORD( wParam ) ) / WHEEL_DELTA;
					int flag = LOWORD( wParam );

					POINT ptClient;
					ptClient.x = GET_X_LPARAM( lParam );
					ptClient.y = GET_Y_LPARAM( lParam );
					window->onMouseWheel( delta, flag, ptClient );
					break;
				}
				case WM_LBUTTONUP:
				{
					int flag = (int) wParam;

					POINT ptClient;
					ptClient.x = GET_X_LPARAM( lParam );
					ptClient.y = GET_Y_LPARAM( lParam );

					window->onLButtonUp( flag, ptClient );
					break;
				}
				case WM_LBUTTONDBLCLK:
				{
					int flag = (int) wParam;

					POINT ptClient;
					ptClient.x = GET_X_LPARAM( lParam );
					ptClient.y = GET_Y_LPARAM( lParam );

					window->onLButtonDblClk( flag, ptClient );
					return 0;
				}
				case WM_LBUTTONDOWN:
				{
					int flag = (int) wParam;

					POINT ptClient;
					ptClient.x = GET_X_LPARAM( lParam );
					ptClient.y = GET_Y_LPARAM( lParam );

					window->onLButtonDown( flag, ptClient );
					return 0;
				}
				case WM_NCMOUSELEAVE:
				{
					if ( window->onNcMouseLeave( ) == true )
						return 0;

					break;
				}
				case WM_NCLBUTTONDBLCLK:
				{
					// WM_NCLBUTTONDOWN得到的是屏幕坐标系，所以需要转换坐标到窗口坐标系
					RECT rcWindow;
					window->getWindowRect( rcWindow );

					POINT ptWindow;
					ptWindow.x = GET_X_LPARAM( lParam ) - rcWindow.left;
					ptWindow.y = GET_Y_LPARAM( lParam ) - rcWindow.top;

					int hitTest = (int) wParam;
					if ( window->onNcLButtonDblClk( hitTest, ptWindow ) == true )
						return 0;

					break;
				}
				case WM_NCLBUTTONDOWN:
				{
					// WM_NCLBUTTONDOWN得到的是屏幕坐标系，所以需要转换坐标到窗口坐标系
					RECT rcWindow;
					window->getWindowRect( rcWindow );

					POINT ptWindow;
					ptWindow.x = GET_X_LPARAM( lParam ) - rcWindow.left;
					ptWindow.y = GET_Y_LPARAM( lParam ) - rcWindow.top;

					int hitTest = (int) wParam;
					if ( window->onNcLButtonDown( hitTest, ptWindow ) == true )
						return 0;

					break;
				}
				case WM_NCHITTEST:
				{
					POINT pt;
					pt.x = LOWORD( lParam );
					pt.y = HIWORD( lParam );
					if ( window->mEnableTitle == true )
					{
						RECT rcTitle;
						window->getTitleRect( rcTitle );

						RECT rc;
						window->getWindowRect( rc );
						pt.x = pt.x - rc.left;
						pt.y = pt.y - rc.top;
						if ( PtInRect( &rcTitle, pt ) == TRUE )
							return HTCAPTION;
					}
					break;
				}
				case WM_NCMOUSEMOVE:
				{
					// WM_NCMOUSEMOVE得到的是屏幕坐标系，所以需要转换坐标到窗口坐标系
					RECT rcWindow;
					window->getWindowRect( rcWindow );

					POINT ptWindow;
					ptWindow.x = GET_X_LPARAM( lParam ) - rcWindow.left;
					ptWindow.y = GET_Y_LPARAM( lParam ) - rcWindow.top;

					int hitTest = (int) wParam;
					if ( window->onNcMouseMove( hitTest, ptWindow ) == true )
						return 0;

					break;
				}
				case WM_KILLFOCUS:
				{
					HWND newFocus = (HWND) wParam;
					window->onKillFocus( newFocus );
					break;
				}
				case WM_SETFOCUS:
				{
					HWND lastFocus = (HWND) wParam;
					window->onSetFocus( lastFocus );
					break;
				}
				case WM_NCPAINT:
				{
					HRGN rgn = (HRGN) wParam;
					if ( window->onNcPaint( rgn ) == true )
						return 0;

					break;
				}
				case WM_SIZE:
				{
					int width = LOWORD( lParam );
					int height = HIWORD( lParam );
					int sizeFlag = (int) wParam;
					window->onSize( width, height, sizeFlag );
					break;
				}
				case WM_NOTIFY:
				{
					NMHDR*		nmhdr = (NMHDR*) lParam;
					CNSWindow*	child = (CNSWindow*) CNSWindow::fromHWnd( nmhdr->hwndFrom );
					if ( child == NULL )
						return ::CallWindowProc( oldProc, wnd, msg, wParam, lParam );

					CNSWindow::WindowEventHandler* hander = child->eventHandler.get( nmhdr->code );
					if ( hander != NULL && ( *hander )( child, wParam, lParam, &retValue ) == true )
						return retValue;

					break;
				}
				case WM_COMMAND:
				{
					int code = HIWORD( wParam );
					if ( (HWND) lParam == NULL )
					{
						int menuID = LOWORD( wParam );
						CNSWindow::WindowEventHandler* hander = window->eventHandler.get( menuID );
						if ( hander != NULL && ( *hander )( window, wParam, lParam, &retValue ) == true )
							return retValue;
					}
					else
					{
						CNSWindow* child = (CNSWindow*) CNSWindow::fromHWnd( (HWND) lParam );
						if ( child == NULL )
							return ::CallWindowProc( oldProc, wnd, msg, wParam, lParam );

						CNSWindow::WindowEventHandler* hander = child->eventHandler.get( code );
						if ( hander != NULL && ( *hander )( child, wParam, lParam, &retValue ) == true )
							return retValue;
					}
					break;
				}
				case WM_CTLCOLOREDIT:
				{
					CNSWindow* child = (CNSWindow*) CNSWindow::fromHWnd( (HWND) lParam );
					if ( child == NULL )
						return ::CallWindowProc( oldProc, wnd, msg, wParam, lParam );

					CNSWindow::WindowEventHandler* hander = child->eventHandler.get( msg );
					if ( hander != NULL && ( *hander )( child, wParam, lParam, &retValue ) == true )
						return retValue;

					break;
				}
				case WM_NCDESTROY:
				{
					::CallWindowProc( oldProc, wnd, msg, wParam, lParam );
					if ( window->mParent != NULL )
						window->mParent->mChildren.erase( window->mWindowID );

					window->mHWnd = NULL;
					HLISTINDEX beginIndex = window->mChildren.getHead( );
					for ( ; beginIndex != NULL; window->mChildren.getNext( beginIndex ) )
					{
						CNSWindow* child = window->mChildren.getValue( beginIndex );
						child->mParent = NULL;
					}

					CNSWindow::sWindows.erase( window->mWindowID );

					NSBase::CNSLuaStack& luaStack = NSBase::CNSLuaStack::getLuaStack( );
					beginIndex = window->mLuaEventHandler.getHead( );
					for ( ; beginIndex != NULL; window->mLuaEventHandler.getNext( beginIndex ) )
					{
						CNSLuaFunction& func = window->mLuaEventHandler.getValue( beginIndex );
						luaStack.clearFunc( func );
					}

					beginIndex = window->mLuaMsgHandler.getHead( );
					for ( ; beginIndex != NULL; window->mLuaMsgHandler.getNext( beginIndex ) )
					{
						CNSLuaFunction& func = window->mLuaMsgHandler.getValue( beginIndex );
						luaStack.clearFunc( func );
					}

					beginIndex = window->mLuaLHotkeyHandler.getHead( );
					for ( ; beginIndex != NULL; window->mLuaLHotkeyHandler.getNext( beginIndex ) )
					{
						CNSLuaFunction& func = window->mLuaLHotkeyHandler.getValue( beginIndex );
						luaStack.clearFunc( func );
					}

					// mLuaRefs是弱引用，被引用对象这里将要被销毁，所以需要让引用指向NULL
					window->cleanUpRef( );
					delete window;
					return 0;
				}
			}

			return ::CallWindowProc( oldProc, wnd, msg, wParam, lParam );
		}
		catch ( CNSException& e )
		{
			NSLog::exception( _UTF8( "函数CNSWindow::windowProc发生异常\n错误描述: \n\t%s\nC++调用堆栈: \n%s" ), e.mErrorDesc, NSBase::NSFunction::getStackInfo( ).getBuffer( ) );
		}

		return 0;
	}

	void CNSWindow::update( )
	{
		unsigned char keyState[ 256 ];
		if ( GetKeyboardState( keyState ) == FALSE )
		{
			DWORD errorCode = GetLastError( );
			static CNSString errorDesc;
			errorDesc.format( _UTF8( "win32函数[GetKeyboardState]调用失败，错误码: %d" ), errorCode );
			NSException( errorDesc );
		}

		CNSWindow* window = CNSWindow::getActive( );
		if ( window != NULL )
		{
			if ( window->mHotkey.processHotkey( keyState ) == true )
				return;

			sGlobalHotkey.processHotkey( keyState );
		}
	}

	CNSWindow::CNSWindow( const CNSString& windowID, const CNSString& windowType ) : mWindowID( windowID ), mWindowType( windowType )
	{
	}

	void CNSWindow::destroy( )
	{
		DestroyWindow( mHWnd );
	}

	void CNSWindow::close( )
	{
		SendMessage( mHWnd, WM_CLOSE, 0, 0 );
	}

	RECT CNSWindow::calcSizedRect( SIZE& szNew, SIZE& szOld )
	{
		RECT rcChild;
		GetWindowRect( mHWnd, &rcChild );
		POINT topLeft = { rcChild.left, rcChild.top };
		ScreenToClient( mParent->getHWnd( ), &topLeft );

		POINT bottomRight = { rcChild.right, rcChild.bottom };
		ScreenToClient( mParent->getHWnd( ), &bottomRight );

		int offsetWidth = szNew.cx - szOld.cx;
		int offsetHeight = szNew.cy - szOld.cy;
		int left = topLeft.x;
		if ( mLeftAnchor == CNSWindow::ANCHOR_LEFT )
			left += 0;
		else if ( mLeftAnchor == CNSWindow::ANCHOR_RIGHT )
			left += offsetWidth;
		else if ( mLeftAnchor == CNSWindow::ANCHOR_CENTER )
			left += offsetWidth >> 1;
		left = max( left, 0 );

		int right = bottomRight.x;
		if ( mRightAnchor == CNSWindow::ANCHOR_LEFT )
			right += 0;
		else if ( mRightAnchor == CNSWindow::ANCHOR_RIGHT )
			right += offsetWidth;
		else if ( mRightAnchor == CNSWindow::ANCHOR_CENTER )
			right += offsetWidth >> 1;
		right = max( left, right );

		int top = topLeft.y;
		if ( mTopAnchor == CNSWindow::ANCHOR_TOP )
			top += 0;
		else if ( mTopAnchor == CNSWindow::ANCHOR_BOTTOM )
			top += offsetHeight;
		else if ( mTopAnchor == CNSWindow::ANCHOR_CENTER )
			top += offsetHeight >> 1;
		top = max( top, 0 );

		int bottom = bottomRight.y;
		if ( mBottomAnchor == CNSWindow::ANCHOR_TOP )
			bottom += 0;
		else if ( mBottomAnchor == CNSWindow::ANCHOR_BOTTOM )
			bottom += offsetHeight;
		else if ( mBottomAnchor == CNSWindow::ANCHOR_CENTER )
			bottom += offsetHeight >> 1;
		bottom = max( top, bottom );

		RECT rcRet = { left, top, right, bottom };
		return rcRet;
	}

	void CNSWindow::onCreateWindow( )
	{
	}

	void CNSWindow::onNcCreateWindow( )
	{
	}

	void CNSWindow::onPostCreateWindow( CNSWindow* parent )
	{
		// 注意：WM_SETFONT, WM_GETFONT 只对CommCtrl32中的控件起作用
		SendMessage( mHWnd, WM_SETFONT, (WPARAM) CNSPreDefine::FB_FONT_BASE, true );

		mParent = parent;
		if ( mParent != NULL )
			mParent->mChildren.insert( mWindowID, this );

		RECT rc;
		GetClientRect( mHWnd, &rc );
		mLastSize.cx = rc.right - rc.left;
		mLastSize.cy = rc.bottom - rc.top;
	}

	CNSVsTab*	CNSWindow::newVsTab( const CNSString& windowID, unsigned int tabType, RECT& rc, CNSWindow* parent )
	{
		CNSVsTab*	window = new CNSVsTab( windowID, ( CNSVsTab::EVsTabStyle ) tabType );
		CreateWindowEx( 0, WC_NS_VSTAB, NULL, WS_VISIBLE | WS_CHILD | WS_CLIPCHILDREN, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
						parent == NULL ? NULL : parent->getHWnd( ), NULL, instance, window );

		if ( window->mHWnd == NULL )
		{
			DWORD errorCode = GetLastError( );
			static CNSString errorDesc;
			errorDesc.format( _UTF8( "win32函数[CreateWindowEx]调用失败，错误码: %d" ), errorCode );
			NSException( errorDesc );
		}

		window->onPostCreateWindow( parent );
		return window;
	}

	CNSEdit* CNSWindow::newEdit( const CNSString& windowID, unsigned int editType, RECT& rc, CNSWindow* parent )
	{
		int style = 0;
		if ( editType == CNSEdit::EEditCtrlType::EDIT_NUMBER )
			style = WS_VISIBLE | WS_CHILD | ES_AUTOHSCROLL | ES_NUMBER;
		else if ( editType == CNSEdit::EEditCtrlType::EDIT_SINGLETEXT )
			style = WS_VISIBLE | WS_CHILD | ES_AUTOHSCROLL;
		else if ( editType == CNSEdit::EEditCtrlType::EDIT_MULTITEXT )
			style = WS_VISIBLE | WS_CHILD | ES_MULTILINE | ES_AUTOHSCROLL | ES_AUTOVSCROLL;

		CNSEdit*	window = new CNSEdit( windowID );
		CreateWindowEx( 0, WC_NS_EDIT, NULL, style, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
						parent == NULL ? NULL : parent->getHWnd( ), NULL, instance, window );

		if ( window->mHWnd == NULL )
		{
			DWORD errorCode = GetLastError( );
			static CNSString errorDesc;
			errorDesc.format( _UTF8( "win32函数[CreateWindowEx]调用失败，错误码: %d" ), errorCode );
			NSException( errorDesc );
		}

		window->onPostCreateWindow( parent );
		return window;
	}

	CNSVsListBox*	CNSWindow::newVsListBox( const CNSString& windowID, unsigned int listType, RECT& rc, CNSWindow* parent )
	{
		int style = WS_VISIBLE | WS_CHILD;
		CNSVsListBox* window = new CNSVsListBox( windowID );
		CreateWindowEx( 0, WC_NS_VSLISTBOX, NULL, style, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
						parent == NULL ? NULL : parent->getHWnd( ), NULL, instance, window );

		if ( window->mHWnd == NULL )
		{
			DWORD errorCode = GetLastError( );
			static CNSString errorDesc;
			errorDesc.format( _UTF8( "win32函数[CreateWindowEx]调用失败，错误码: %d" ), errorCode );
			NSException( errorDesc );
		}

		window->onPostCreateWindow( parent );
		return window;
	}

	CComboBox* CNSWindow::newComboBox( const CNSString& windowID, unsigned int comboType, RECT& rc, CNSWindow* parent )
	{
		int style = 0;
		if ( comboType == CComboBox::EComboType::COMBO_DROPDOWN )
			style = WS_VISIBLE | WS_CHILD | CBS_HASSTRINGS | CBS_DROPDOWN;
		else if ( comboType == CComboBox::EComboType::COMBO_DROPLIST )
			style = WS_VISIBLE | WS_CHILD | CBS_HASSTRINGS | CBS_DROPDOWNLIST;

		CComboBox* window = new CComboBox( windowID );
		CreateWindowEx( 0, WC_NS_COMBOBOX, NULL, style, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
						parent == NULL ? NULL : parent->getHWnd( ), NULL, instance, window );

		if ( window->mHWnd == NULL )
		{
			DWORD errorCode = GetLastError( );
			static CNSString errorDesc;
			errorDesc.format( _UTF8( "win32函数[CreateWindowEx]调用失败，错误码: %d" ), errorCode );
			NSException( errorDesc );
		}

		window->onPostCreateWindow( parent );
		return window;
	}

	CNSVsBtn* CNSWindow::newVsBtn( const CNSString& windowID, unsigned int btnType, RECT& rc, CNSWindow* parent )
	{
		CNSVsBtn*	window = new CNSVsBtn( windowID, ( CNSVsBtn::EVsButtonStyle ) btnType );
		CreateWindowEx( 0, WC_NS_VSBTN, NULL, WS_VISIBLE | WS_CHILD, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
						parent == NULL ? NULL : parent->getHWnd( ), NULL, instance, window );

		if ( window->mHWnd == NULL )
		{
			DWORD errorCode = GetLastError( );
			static CNSString errorDesc;
			errorDesc.format( _UTF8( "win32函数[CreateWindowEx]调用失败，错误码: %d" ), errorCode );
			NSException( errorDesc );
		}

		window->onPostCreateWindow( parent );
		return window;
	}

	CNSVsTree* CNSWindow::newVsTree( const CNSString& windowID, unsigned int treeType, RECT& rc, CNSWindow* parent )
	{
		CNSVsTree* window = new CNSVsTree( windowID );
		CreateWindowEx( 0, WC_NS_VSTREE, NULL, WS_CHILD | WS_VISIBLE | TVS_FULLROWSELECT | TVS_HASBUTTONS | TVS_SHOWSELALWAYS | TVS_LINESATROOT, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
						parent == NULL ? NULL : parent->getHWnd( ), NULL, instance, window );

		if ( window->mHWnd == NULL )
		{
			DWORD errorCode = GetLastError( );
			static CNSString errorDesc;
			errorDesc.format( _UTF8( "win32函数[CreateWindowEx]调用失败，错误码: %d" ), errorCode );
			NSException( errorDesc );
		}

		window->onPostCreateWindow( parent );
		return window;
	}

	CNSVsList* CNSWindow::newVsList( const CNSString& windowID, unsigned int listType, RECT& rc, CNSWindow* parent )
	{
		CNSVsList* window = new CNSVsList( windowID );
		::CreateWindowEx( 0, WC_NS_VSLIST, NULL, WS_VISIBLE | WS_CHILD | LVS_REPORT | LVS_SHOWSELALWAYS, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
						  parent == NULL ? NULL : parent->getHWnd( ), NULL, instance, window );

		if ( window->mHWnd == NULL )
		{
			DWORD errorCode = GetLastError( );
			static CNSString errorDesc;
			errorDesc.format( _UTF8( "win32函数[CreateWindowEx]调用失败，错误码: %d" ), errorCode );
			NSException( errorDesc );
		}

		window->onPostCreateWindow( parent );
		return window;
	}

	CNSVsEdit* CNSWindow::newVsEdit( const CNSString& windowID, unsigned int editType, RECT& rc, CNSWindow* parent )
	{
		CNSVsEdit* window = new CNSVsEdit( windowID );
		::CreateWindowEx( 0, WC_NS_VSEDIT, NULL, WS_VISIBLE | WS_CHILD, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
						  parent == NULL ? NULL : parent->getHWnd( ), NULL, instance, window );

		if ( window->mHWnd == NULL )
		{
			DWORD errorCode = GetLastError( );
			static CNSString errorDesc;
			errorDesc.format( _UTF8( "win32函数[CreateWindowEx]调用失败，错误码: %d" ), errorCode );
			NSException( errorDesc );
		}

		window->onPostCreateWindow( parent );
		return window;
	}

	BOOL CALLBACK EnumThreadWndProc( HWND hwnd, LPARAM lParam )
	{
		CNSWindow* window = CNSWindow::fromHWnd( hwnd );
		if ( window == NULL )
			return TRUE;

		CNSSet< CNSWindow* >* prevent = ( CNSSet< CNSWindow* >* ) lParam;
		if ( prevent != NULL && window->isEnable( ) == true )
			prevent->insert( window );

		return TRUE;
	}

	void CNSWindow::doModal( CNSFrame* frame )
	{
		if ( frame->mIsModalDialog == false )
		{
			static CNSString errorDesc;
			errorDesc.format( _UTF8( "窗口[%s] 不是一个模态对话框" ), frame->getWindowID( ).getBuffer( ) );
			NSException( errorDesc );
		}

		CNSSet< CNSWindow* > prevent;
		EnumThreadWindows( GetCurrentThreadId( ), EnumThreadWndProc, (LPARAM) &prevent );
		prevent.erase( frame );

		HLISTINDEX beginIndex = prevent.getHead( );
		for ( ; beginIndex != NULL; prevent.getNext( beginIndex ) )
			prevent.getKey( beginIndex )->enable( false );

		CNSString windowID = frame->getWindowID( );
		while ( 1 )
		{
			MSG msg;
			if ( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) == TRUE )
			{
				if ( msg.message == WM_KEYDOWN && msg.wParam == VK_RETURN )
				{
					if ( SendMessage( msg.hwnd, msg.message, msg.wParam, msg.lParam ) == 1 )
						continue;
				}

				if ( msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE )
				{
					if ( SendMessage( msg.hwnd, msg.message, msg.wParam, msg.lParam ) == 1 )
						continue;
				}

				if ( msg.message == WM_KEYDOWN && msg.wParam == VK_F10 )
				{
					if ( SendMessage( msg.hwnd, msg.message, msg.wParam, msg.lParam ) == 1 )
						continue;
				}

				if ( IsDialogMessage( frame->getHWnd( ), &msg ) == FALSE )
				{
					TranslateMessage( &msg );
					DispatchMessage( &msg );

					if ( CNSWindow::getWindow( windowID ) == NULL )
					{
						frame = NULL;
						break;
					}
				}
			}
			else
			{
				CNSBaseApp::getApp( )->onIdle( );
			}
		}

		beginIndex = prevent.getHead( );
		for ( ; beginIndex != NULL; prevent.getNext( beginIndex ) )
		{
			prevent.getKey( beginIndex )->enable( true );
			prevent.getKey( beginIndex )->active( );
		}
	}

	CNSCustom* CNSWindow::newCustom( const CNSString& windowID, unsigned int style, unsigned int styleEx, RECT& rc, CNSWindow* parent )
	{
		AdjustWindowRectEx( &rc, style, FALSE, styleEx );
		CNSCustom* window = new CNSCustom( windowID );
		::CreateWindowEx( styleEx, WC_NS_CUSTOM, NULL, style, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
						  parent == NULL ? NULL : parent->getHWnd( ), NULL, instance, window );

		if ( window->mHWnd == NULL )
		{
			DWORD errorCode = GetLastError( );
			static CNSString errorDesc;
			errorDesc.format( _UTF8( "win32函数[CreateWindowEx]调用失败，错误码: %d" ), errorCode );
			NSException( errorDesc );
		}

		window->onPostCreateWindow( parent );
		return window;
	}

	CNSVsStatic* CNSWindow::newVsStatic( const CNSString& windowID, unsigned int staticType, RECT& rc, CNSWindow* parent )
	{
		CNSVsStatic* window = new CNSVsStatic( windowID );
		::CreateWindowEx( 0, WC_NS_VSSTATIC, NULL, WS_VISIBLE | WS_CHILD, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
						  parent == NULL ? NULL : parent->getHWnd( ), NULL, instance, window );

		if ( window->mHWnd == NULL )
		{
			DWORD errorCode = GetLastError( );
			static CNSString errorDesc;
			errorDesc.format( _UTF8( "win32函数[CreateWindowEx]调用失败，错误码: %d" ), errorCode );
			NSException( errorDesc );
		}

		window->onPostCreateWindow( parent );
		return window;
	}

	CVsWizard* CNSWindow::newWizard( const CNSString& windowID, unsigned int wizardType, RECT& rc, CNSWindow* parent )
	{
		CVsWizard* window = new CVsWizard( windowID );
		::CreateWindowEx( 0, WC_NS_VSWIZARD, NULL, WS_VISIBLE | WS_CHILD | WS_CLIPCHILDREN, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
						  parent == NULL ? NULL : parent->getHWnd( ), NULL, instance, window );

		if ( window->mHWnd == NULL )
		{
			DWORD errorCode = GetLastError( );
			static CNSString errorDesc;
			errorDesc.format( _UTF8( "win32函数[CreateWindowEx]调用失败，错误码: %d" ), errorCode );
			NSException( errorDesc );
		}

		window->onPostCreateWindow( parent );
		return window;
	}

	CVsFileBrowser* CNSWindow::newFileBrowser( const CNSString& windowID, unsigned int style, RECT& rc, CNSWindow* parent )
	{
		CVsFileBrowser* window = new CVsFileBrowser( windowID );
		::CreateWindowEx( 0, WC_NS_VSFILEBROWSER, NULL, WS_VISIBLE | WS_CHILD | WS_CLIPCHILDREN, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
						  parent == NULL ? NULL : parent->getHWnd( ), NULL, instance, window );

		if ( window->mHWnd == NULL )
		{
			DWORD errorCode = GetLastError( );
			static CNSString errorDesc;
			errorDesc.format( _UTF8( "win32函数[CreateWindowEx]调用失败，错误码: %d" ), errorCode );
			NSException( errorDesc );
		}

		window->onPostCreateWindow( parent );
		return window;
	}

	CNSFrame*	CNSWindow::newFrame( const CNSString& windowID, unsigned int style, RECT& rc, CNSWindow* parent, bool center, HMENU menu )
	{
		int frameStyle = 0;
		int frameStyleEx = 0;
		if ( style == CNSFrame::EFrameType::STYLE_FULL )
			frameStyle |= WS_VISIBLE | WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN;

		if ( style == CNSFrame::EFrameType::STYLE_POPUP )
		{
			frameStyle |= WS_VISIBLE | WS_POPUP | WS_THICKFRAME | WS_CAPTION | WS_SYSMENU | WS_SIZEBOX | WS_CLIPCHILDREN;
			frameStyleEx = WS_EX_TOOLWINDOW;
		}

		if ( style == CNSFrame::EFrameType::STYLE_MINIPOPUP || style == CNSFrame::EFrameType::STYLE_VSPOPUP )
		{
			frameStyle |= WS_VISIBLE | WS_POPUP | WS_CLIPCHILDREN;
			frameStyleEx = WS_EX_TOOLWINDOW;
		}

		if ( style == CNSFrame::EFrameType::STYLE_CHILD )
		{
			frameStyle |= WS_VISIBLE | WS_CHILD;
			frameStyleEx = 0;
		}

		BOOL hasMenu = FALSE;
		if ( menu != NULL )
			hasMenu = TRUE;

		AdjustWindowRectEx( &rc, frameStyle, hasMenu, frameStyleEx );
		if ( center == true )
		{
			int screenWidth = GetSystemMetrics( SM_CXSCREEN );
			int screenHeight = GetSystemMetrics( SM_CYSCREEN );

			int windowWidth = rc.right - rc.left;
			int windowHeight = rc.bottom - rc.top;
			rc.top = ( screenHeight - windowHeight ) >> 1;
			rc.left = ( screenWidth - windowWidth ) >> 1;
			rc.bottom = rc.top + windowHeight;
			rc.right = rc.left + windowWidth;
		}

		CNSFrame* window = new CNSFrame( windowID, ( CNSFrame::EFrameType ) style );
		HWND wnd = ::CreateWindowEx( frameStyleEx, WC_NS_FRAME, NULL, frameStyle, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
									 parent == NULL ? NULL : parent->getHWnd( ), menu, instance, window );

		if ( wnd == NULL )
		{
			DWORD errorCode = GetLastError( );
			static CNSString errorDesc;
			errorDesc.format( _UTF8( "win32函数[CreateWindowEx]调用失败，错误码: %d" ), errorCode );
			NSException( errorDesc );
		}

		window->onPostCreateWindow( parent );
		return window;
	}

	CNSWindow* CNSWindow::fromHWnd( HWND wnd )
	{
		WNDPROC classProc = (WNDPROC) GetClassLongPtr( wnd, GCLP_WNDPROC );
		WNDPROC wndProc = (WNDPROC) GetWindowLongPtr( wnd, GWLP_WNDPROC );
		if ( classProc == CNSWindow::windowProc || wndProc == CNSWindow::windowProc )
		{
			CNSWindow* window = (CNSWindow*) GetWindowLongPtr( wnd, GWLP_USERDATA );
			if ( window == NULL )
				return NULL;

			return window;
		}

		return NULL;
	}

	void CNSWindow::init( HINSTANCE instance )
	{
		CNSPreDefine::FB_FONT_NOTIFY = CreateFont( 30,	// nHeight 
												   0,							// nWidth 
												   0,							// nEscapement 
												   0,							// nOrientation 
												   FW_BOLD,					// nWeight 
												   FALSE,						// bItalic 
												   FALSE,						// bUnderline 
												   0,							// cStrikeOut 
												   ANSI_CHARSET,				// nCharSet 
												   OUT_DEFAULT_PRECIS,			// nOutPrecision 
												   CLIP_DEFAULT_PRECIS,		// nClipPrecision 
												   DEFAULT_QUALITY,			// nQuality 
												   DEFAULT_PITCH | FF_MODERN,	// nPitchAndFamily 
												   _T( "Arial" ) );				// lpszFac

		CNSPreDefine::FB_FONT_BASE = CreateFont( 18,	// nHeight 
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

		CNSPreDefine::FB_FONT_BASEBOLD = CreateFont( 18,	// nHeight 
													 0,							// nWidth 
													 0,							// nEscapement 
													 0,							// nOrientation 
													 FW_BOLD,					// nWeight 
													 FALSE,						// bItalic 
													 FALSE,						// bUnderline 
													 0,							// cStrikeOut 
													 ANSI_CHARSET,				// nCharSet 
													 OUT_DEFAULT_PRECIS,			// nOutPrecision 
													 CLIP_DEFAULT_PRECIS,		// nClipPrecision 
													 PROOF_QUALITY,				// nQuality 
													 FIXED_PITCH | FF_MODERN,	// nPitchAndFamily 
													 _T( "Microsoft YaHei UI" ) );	// lpszFac

		CNSWindow::brushScrollBack = CreateSolidBrush( RGB( 63, 63, 70 ) );
		CNSWindow::brushNCBack = CreateSolidBrush( RGB( 45, 45, 48 ) );
		CNSWindow::penBorder = CreatePen( PS_SOLID, 1, RGB( 63, 63, 70 ) );
		CNSWindow::brushFocusWindow = CreateSolidBrush( RGB( 0, 122, 204 ) );
		CNSWindow::penScrollBtn = CreatePen( PS_SOLID, 1, RGB( 153, 153, 153 ) );
		CNSWindow::brushScrollBtn = CreateSolidBrush( RGB( 153, 153, 153 ) );
		CNSWindow::penScrollHoverBtn = CreatePen( PS_SOLID, 1, RGB( 28, 151, 234 ) );
		CNSWindow::brushScrollHoverBtn = CreateSolidBrush( RGB( 28, 151, 234 ) );
		CNSWindow::penScrollClickBtn = CreatePen( PS_SOLID, 1, RGB( 0, 122, 204 ) );
		CNSWindow::brushScrollClickBtn = CreateSolidBrush( RGB( 0, 122, 204 ) );
		CNSWindow::brushScrollClickThumb = CreateSolidBrush( RGB( 239, 235, 239 ) );
		CNSWindow::brushScrollThumb = CreateSolidBrush( RGB( 104, 104, 104 ) );
		CNSWindow::brushScrollHoverThumb = CreateSolidBrush( RGB( 158, 158, 158 ) );
		CNSWindow::scrollWidth = 17;
		CNSWindow::titleHeight = 24;

		CNSWindow::instance = instance;
		CNSFrame::init( );
		CNSVsTab::init( );
		CNSVsList::init( );
		CNSEdit::init( );
		CComboBox::init( );
		CNSVsBtn::init( );
		CNSVsTree::init( );
		CNSVsEdit::init( );
		CNSVsStatic::init( );
		CVsFileBrowser::init( );
		CVsFileDialog::init( );
		CVsWizard::init( );
		CNSVsListBox::init( );
		CNSCustom::init( );

		CNSWindow::regLuaLib( );
		CNSFrame::regLuaLib( );
		CNSVsBtn::regLuaLib( );
		CNSEdit::regLuaLib( );
		CVsFileDialog::regLuaLib( );
	}

	void CNSWindow::exit( )
	{
		CNSLuaStack& luaStack = CNSLuaStack::getLuaStack( );
		HLISTINDEX beginIndex = CNSWindow::sLuaGHotkeyHandler.getHead( );
		for ( ; beginIndex != NULL; CNSWindow::sLuaGHotkeyHandler.getNext( beginIndex ) )
		{
			CNSLuaFunction& func = CNSWindow::sLuaGHotkeyHandler.getValue( beginIndex );
			luaStack.clearFunc( func );
		}

		beginIndex = sLuaBrush.getHead( );
		for ( ; beginIndex != NULL; sLuaBrush.getNext( beginIndex ) )
			DeleteObject( sLuaBrush.getValue( beginIndex ) );

		if ( CNSPreDefine::FB_FONT_NOTIFY != NULL )
			DeleteObject( CNSPreDefine::FB_FONT_NOTIFY );

		if ( CNSPreDefine::FB_FONT_BASE != NULL )
			DeleteObject( CNSPreDefine::FB_FONT_BASE );

		if ( CNSPreDefine::FB_FONT_BASEBOLD != NULL )
			DeleteObject( CNSPreDefine::FB_FONT_BASEBOLD );

		if ( CNSWindow::brushScrollBack != NULL )
			DeleteObject( CNSWindow::brushScrollBack );
		
		if ( CNSWindow::brushNCBack != NULL )
			DeleteObject( CNSWindow::brushNCBack );

		if ( CNSWindow::penBorder != NULL )
			DeleteObject( CNSWindow::penBorder );

		if ( CNSWindow::brushFocusWindow != NULL )
			DeleteObject( CNSWindow::brushFocusWindow );

		if ( CNSWindow::penScrollBtn != NULL )
			DeleteObject( CNSWindow::penScrollBtn );

		if ( CNSWindow::brushScrollBtn != NULL )
			DeleteObject( CNSWindow::brushScrollBtn );

		if ( CNSWindow::penScrollHoverBtn != NULL )
			DeleteObject( CNSWindow::penScrollHoverBtn );

		if ( CNSWindow::brushScrollHoverBtn != NULL )
			DeleteObject( CNSWindow::brushScrollHoverBtn );

		if ( CNSWindow::penScrollClickBtn != NULL )
			DeleteObject( CNSWindow::penScrollClickBtn );

		if ( CNSWindow::brushScrollClickBtn != NULL )
			DeleteObject( CNSWindow::brushScrollClickBtn );

		if ( CNSWindow::brushScrollClickThumb != NULL )
			DeleteObject( CNSWindow::brushScrollClickThumb );

		if ( CNSWindow::brushScrollThumb != NULL )
			DeleteObject( CNSWindow::brushScrollThumb );

		if ( CNSWindow::brushScrollHoverThumb != NULL )
			DeleteObject( CNSWindow::brushScrollHoverThumb );

		CNSVsList::exit( );
		CNSCustom::exit( );
		CNSVsTab::exit( );
		CNSEdit::exit( );
		CComboBox::exit( );
		CNSVsBtn::exit( );
		CNSVsTree::exit( );
		CNSVsStatic::exit( );
		CVsWizard::exit( );
		CVsFileBrowser::exit( );
		CVsFileDialog::exit( );
		CNSVsEdit::exit( );
		CNSVsListBox::exit( );
		CNSFrame::exit( );
	}

	void CNSWindow::regLuaLib( )
	{
		NSBase::CNSLuaStack& luaStack = NSBase::CNSLuaStack::getLuaStack( );
		lua_State* lua = luaStack.getLuaState( );
		luaL_newlib( lua, NSWin32::NSWin32 );
		lua_setglobal( lua, "NSWin32" );

		luaStack.newTable( );
		luaStack.pushField( "HOTKEY_NONE", HOTKEY_NONE );
		luaStack.pushField( "HOTKEY_ALT", HOTKEY_ALT );
		luaStack.pushField( "HOTKEY_CTRL", HOTKEY_CTRL );
		luaStack.pushField( "HOTKEY_SHIFT", HOTKEY_SHIFT );
		luaStack.setGlobalTable( "hotkeyFlag" );

		luaStack.newTable( );
		luaStack.pushField( "KEYCODE_NONE", KEYCODE_NONE );
		luaStack.pushField( "KEYCODE_F1", KEYCODE_F1 );
		luaStack.pushField( "KEYCODE_F2", KEYCODE_F2 );
		luaStack.pushField( "KEYCODE_F3", KEYCODE_F3 );
		luaStack.pushField( "KEYCODE_F4", KEYCODE_F4 );
		luaStack.pushField( "KEYCODE_F5", KEYCODE_F5 );
		luaStack.pushField( "KEYCODE_F6", KEYCODE_F6 );
		luaStack.pushField( "KEYCODE_F7", KEYCODE_F7 );
		luaStack.pushField( "KEYCODE_F8", KEYCODE_F8 );
		luaStack.pushField( "KEYCODE_F9", KEYCODE_F9 );
		luaStack.pushField( "KEYCODE_F10", KEYCODE_F10 );
		luaStack.pushField( "KEYCODE_F11", KEYCODE_F11 );
		luaStack.pushField( "KEYCODE_F12", KEYCODE_F12 );

		luaStack.pushField( "KEYCODE_RETURN", KEYCODE_RETURN );
		luaStack.pushField( "KEYCODE_ESC", KEYCODE_ESC );
		luaStack.pushField( "KEYCODE_A", KEYCODE_A );
		luaStack.pushField( "KEYCODE_B", KEYCODE_B );
		luaStack.pushField( "KEYCODE_F", KEYCODE_F );
		luaStack.pushField( "KEYCODE_L", KEYCODE_L );
		luaStack.pushField( "KEYCODE_G", KEYCODE_G );
		luaStack.pushField( "KEYCODE_S", KEYCODE_S );
		luaStack.pushField( "KEYCODE_PLUS", KEYCODE_PLUS );
		luaStack.pushField( "KEYCODE_MINUS", KEYCODE_MINUS );
		luaStack.pushField( "KEYCODE_LEFT", KEYCODE_LEFT );
		luaStack.pushField( "KEYCODE_RIGHT", KEYCODE_RIGHT );
		luaStack.setGlobalTable( "hotkeyKeycode" );

		luaStack.newTable( );
		luaStack.pushField( "NS_VSBTN", NS_VSBTN );
		luaStack.pushField( "NS_VSEDIT", NS_VSEDIT );
		luaStack.pushField( "NS_VSTAB", NS_VSTAB );
		luaStack.pushField( "NS_VSSTATIC", NS_VSSTATIC );
		luaStack.pushField( "NS_VSTREE", NS_VSTREE );
		luaStack.pushField( "NS_VSLIST", NS_VSLIST );
		luaStack.pushField( "NS_VSLISTBOX", NS_VSLISTBOX );
		luaStack.pushField( "NS_VSWIZARD", NS_VSWIZARD );
		luaStack.pushField( "NS_VSFILEBROWSER", NS_VSFILEBROWSER );
		luaStack.pushField( "NS_EDIT", NS_EDIT );
		luaStack.pushField( "NS_FRAME", NS_FRAME );
		luaStack.pushField( "NS_COMBOBOX", NS_COMBOBOX );
		luaStack.setGlobalTable( "windowType" );

		luaStack.newTable( );
		luaStack.pushField( "ANCHOR_LEFT", CNSWindow::ANCHOR_LEFT );
		luaStack.pushField( "ANCHOR_RIGHT", CNSFrame::ANCHOR_RIGHT );
		luaStack.pushField( "ANCHOR_TOP", CNSFrame::ANCHOR_TOP );
		luaStack.pushField( "ANCHOR_BOTTOM", CNSFrame::ANCHOR_BOTTOM );
		luaStack.pushField( "ANCHOR_CENTER", CNSFrame::ANCHOR_CENTER );
		luaStack.setGlobalTable( "anchorStyle" );

		luaStack.newTable( );
		luaStack.pushField( "EDGE_LEFT", 0 );
		luaStack.pushField( "EDGE_RIGHT", 1 );
		luaStack.pushField( "EDGE_TOP", 2 );
		luaStack.pushField( "EDGE_BOTTOM", 3 );
		luaStack.setGlobalTable( "anchorEdge" );

		luaStack.newTable( );
		luaStack.pushField( "DT_TOP", 0x00000000 );
		luaStack.pushField( "DT_LEFT", 0x00000000 );
		luaStack.pushField( "DT_CENTER", 0x00000001 );
		luaStack.pushField( "DT_RIGHT", 0x00000002 );
		luaStack.pushField( "DT_VCENTER", 0x00000004 );
		luaStack.pushField( "DT_BOTTOM", 0x00000008 );
		luaStack.pushField( "DT_WORDBREAK", 0x00000010 );
		luaStack.pushField( "DT_SINGLELINE", 0x00000020 );
		luaStack.pushField( "DT_EXPANDTABS", 0x00000040 );
		luaStack.pushField( "DT_TABSTOP", 0x00000080 );
		luaStack.pushField( "DT_NOCLIP", 0x00000100 );
		luaStack.pushField( "DT_EXTERNALLEADING", 0x00000200 );
		luaStack.pushField( "DT_CALCRECT", 0x00000400 );
		luaStack.pushField( "DT_NOPREFIX", 0x00000800 );
		luaStack.pushField( "DT_INTERNAL", 0x00001000 );
		luaStack.pushField( "DT_EDITCONTROL", 0x00002000 );
		luaStack.pushField( "DT_PATH_ELLIPSIS", 0x00004000 );
		luaStack.pushField( "DT_END_ELLIPSIS", 0x00008000 );
		luaStack.pushField( "DT_MODIFYSTRING", 0x00010000 );
		luaStack.pushField( "DT_RTLREADING", 0x00020000 );
		luaStack.pushField( "DT_WORD_ELLIPSIS", 0x00040000 );
		luaStack.pushField( "DT_NOFULLWIDTHCHARBREAK", 0x00080000 );
		luaStack.pushField( "DT_HIDEPREFIX", 0x00100000 );
		luaStack.pushField( "DT_PREFIXONLY", 0x00200000 );
		luaStack.setGlobalTable( "drawStyle" );
	}

	bool CNSWindow::onMessageTimer( CNSWindow* child, WPARAM wParam, LPARAM lParam, LRESULT* result )
	{
		int timerID = (int) wParam;
		if ( timerID == 100 )
		{
			unsigned int curTick = CNSTimer::getCurTick( );
			static unsigned int lastTick = 0;
			if ( lastTick == 0 )
				lastTick = curTick;

			int tickOffset = curTick - lastTick;
			lastTick = curTick;
			if ( messageState == 0 )
			{
				messageKeepTick = max( 0, messageKeepTick - tickOffset );
				if ( messageKeepTick == 0 )
					messageState = 1;
			}
			else if ( messageState == 1 )
			{
				messageAlpha = max( 0, messageAlpha - tickOffset * 0.1f );

				CNSWindow* message = CNSWindow::getWindow( "debugMessage" );
				if ( message != NULL )
				{
					if ( messageAlpha == 0 )
						message->destroy( );
					else
						SetLayeredWindowAttributes( message->getHWnd( ), RGB( 240, 240, 240 ), (int) messageAlpha, LWA_COLORKEY | LWA_ALPHA );
				}
			}
		}

		return true;
	}

	bool CNSWindow::onMessageRender( CNSWindow* child, WPARAM wParam, LPARAM lParam, LRESULT* result )
	{
		HBRUSH brBack = CreateSolidBrush( RGB( 0, 0, 0 ) );
		PAINTSTRUCT ps;
		HDC dc = BeginPaint( child->getHWnd( ), &ps );
		RECT rc;
		GetClientRect( child->getHWnd( ), &rc );

		HRGN rgn = CreateRoundRectRgn( rc.left, rc.top, rc.right, rc.bottom, 20, 20 );
		FillRgn( dc, rgn, brBack );

		HFONT oldFont = (HFONT) SelectObject( dc, CNSPreDefine::FB_FONT_NOTIFY );
		int oldMode = SetBkMode( dc, TRANSPARENT );
		COLORREF oldClr = SetTextColor( dc, RGB( 255, 255, 255 ) );
		TCHAR* text = (TCHAR*) CNSString::toTChar( messageText );
		DrawText( dc, text, lstrlen( text ), &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE );
		SetBkMode( dc, oldMode );
		SetTextColor( dc, oldClr );
		SelectObject( dc, oldFont );

		DeleteObject( rgn );
		DeleteObject( brBack );
		EndPaint( child->getHWnd( ), &ps );
		return true;
	}

	void CNSWindow::notification( const CNSString& text )
	{
		int screenWidth = ::GetSystemMetrics( SM_CXSCREEN );
		int screenHeight = ::GetSystemMetrics( SM_CYSCREEN );
		int left = ( screenWidth - 300 ) >> 1;
		int top = ( screenHeight - 100 ) >> 1;
		RECT rc = { left, top, left + 300, top + 100 };
		CNSCustom* message = CNSWindow::newCustom( "debugMessage", WS_VISIBLE | WS_POPUP, WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TOOLWINDOW, rc, NULL );
		message->registerMessage( WM_PAINT, onMessageRender );
		message->registerMessage( WM_TIMER, onMessageTimer );
		message->setTimer( 100, 15 );
		messageState = 0;
		messageKeepTick = 1000;
		messageAlpha = 180.0f;
		messageText = text;

		SetLayeredWindowAttributes( message->getHWnd( ), RGB( 240, 240, 240 ), (int) messageAlpha, LWA_COLORKEY | LWA_ALPHA );
	}

	CNSWindow* CNSWindow::getWindow( const CNSString& windowID )
	{
		CNSWindow** windowRef = sWindows.get( windowID );
		if ( windowRef == NULL )
			return NULL;

		return *windowRef;
	}

	void CNSWindow::move( int x, int y, float duration, bool ani )
	{
		RECT rc;
		GetWindowRect( mHWnd, &rc );
		if ( ani == false )
		{
			int width = rc.right - rc.left;
			int height = rc.bottom - rc.top;
			::MoveWindow( mHWnd, x, y, width, height, FALSE );
			return;
		}

		if ( rc.left == x && rc.top == y )
			return;

		CMoveProxy::addMove( mHWnd, CNSVector2( (float) x, (float) y ), duration );
		SetTimer( mHWnd, 101, 10, CNSWindow::MoveTimer );
	}

	void CNSWindow::delayFocus( )
	{
		SetTimer( mHWnd, 100, 10, CNSWindow::FocusTimer );
	}

	CNSWindow* CNSWindow::focus( )
	{
		HWND focusWnd = SetFocus( mHWnd );
		return CNSWindow::fromHWnd( focusWnd );
	}

	CNSWindow* CNSWindow::getFocus( )
	{
		HWND focusWnd = GetFocus( );
		return CNSWindow::fromHWnd( focusWnd );
	}

	CNSWindow* CNSWindow::getActive( )
	{
		HWND activeWnd = GetActiveWindow( );
		return CNSWindow::fromHWnd( activeWnd );
	}

	void CNSWindow::setWindowText( HWND wnd, const CNSString& text )
	{
		SetWindowText( wnd, (TCHAR*) CNSString::toTChar( text ) );
	}

	CNSString& CNSWindow::getWindowText( HWND wnd )
	{
		static CNSString windowName;
		TCHAR buffer[ 256 ];
		if ( GetWindowText( wnd, buffer, 256 ) == 0 )
		{
			if ( GetWindowTextLength( wnd ) == 0 )
			{
				windowName.clear( );
				return windowName;
			}

			DWORD errorCode = GetLastError( );
			static CNSString errorDesc;
			errorDesc.format( _UTF8( "win32函数[GetWindowText]调用失败，错误码: %d" ), errorCode );
			NSException( errorDesc );
		}

		windowName = CNSString::fromTChar( (char*) buffer );
		return windowName;
	}

	void CNSWindow::registerEvent( unsigned int notifyCode, WindowEventHandler handler )
	{
		eventHandler.insert( notifyCode, handler );
	}

	void CNSWindow::registerMessage( unsigned int msg, WindowEventHandler handler )
	{
		messageHandler.insert( msg, handler );
	}

	void CNSWindow::registerGlobalHotkey( int flag, int keyCode, CHotkey::FGlobalHotkeyHandler handler )
	{
		sGlobalHotkey.addHotkey( flag, keyCode, handler );
	}

	void CNSWindow::registerLocalHotkey( int flag, int keyCode, CHotkey::FLocalHotkeyHandler handler )
	{
		mHotkey.addHotkey( flag, keyCode, handler, this );
	}

	CNSWindow* CNSWindow::getParent( )
	{
		HWND parent = GetParent( mHWnd );
		return CNSWindow::fromHWnd( parent );
	}

	HWND CNSWindow::getHWnd( )
	{
		return mHWnd;
	}

	void CNSWindow::setText( const CNSString& text )
	{
		CNSWindow::setWindowText( mHWnd, text );
	}

	CNSString& CNSWindow::getText( )
	{
		return CNSWindow::getWindowText( mHWnd );
	}

	void CNSWindow::refresh( )
	{
		RedrawWindow( mHWnd, NULL, NULL, RDW_UPDATENOW | RDW_INVALIDATE | RDW_ALLCHILDREN );
	}

	void CNSWindow::refreshNc( )
	{
		SetWindowPos( mHWnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOREDRAW | SWP_FRAMECHANGED );
	}

	void CNSWindow::maximized( )
	{
		ShowWindow( mHWnd, SW_SHOWMAXIMIZED );
	}

	void CNSWindow::minimized( )
	{
		ShowWindow( mHWnd, SW_SHOWMINIMIZED );
	}

	void CNSWindow::show( )
	{
		ShowWindow( mHWnd, SW_SHOW );
	}

	void CNSWindow::hide( )
	{
		ShowWindow( mHWnd, SW_HIDE );
	}

	bool CNSWindow::isVisible( )
	{
		return ::IsWindowVisible( mHWnd );
	}

	bool CNSWindow::isEnable( )
	{
		return ::IsWindowEnabled( mHWnd );
	}

	void CNSWindow::enable( bool enable )
	{
		::EnableWindow( mHWnd, enable );
		::InvalidateRect( mHWnd, NULL, FALSE );
	}

	CNSWindow* CNSWindow::active( )
	{
		HWND activeWnd = ::SetActiveWindow( mHWnd );
		return CNSWindow::fromHWnd( activeWnd );
	}

	void CNSWindow::getWindowRect( RECT& rc )
	{
		GetWindowRect( mHWnd, &rc );
	}

	void CNSWindow::getClientRect( RECT& rc )
	{
		GetClientRect( mHWnd, &rc );
	}

	void CNSWindow::setRedraw( BOOL redraw )
	{
		SetWindowRedraw( mHWnd, redraw );
	}

	void CNSWindow::setVisible( bool visible )
	{
		if ( visible == true )
			ShowWindow( mHWnd, SW_SHOW );
		else
			ShowWindow( mHWnd, SW_HIDE );
	}

	void CNSWindow::setTimer( int timerID, int interval )
	{
		::SetTimer( mHWnd, timerID, interval, NULL );
	}

	void CNSWindow::killTimer( int timerID )
	{
		::KillTimer( mHWnd, timerID );
	}

	void CNSWindow::pushUserData( CNSLuaStack& luaStack, CNSWindow* ref )
	{
		if ( ref->mWindowType == NS_VSSTATIC )
			CNSVsStatic::pushUserData( luaStack, ( NSWin32::CNSVsStatic* ) ref );
		else if ( ref->mWindowType == NS_VSTAB )
			CNSVsTab::pushUserData( luaStack, ( NSWin32::CNSVsTab* ) ref );
		else if ( ref->mWindowType == NS_EDIT )
			CNSEdit::pushUserData( luaStack,( NSWin32::CNSEdit* ) ref );
		else if ( ref->mWindowType == NS_VSEDIT )
			CNSVsEdit::pushUserData( luaStack,( NSWin32::CNSVsEdit* ) ref );
		else if ( ref->mWindowType == NS_COMBOBOX )
			CComboBox::pushUserData( luaStack,( NSWin32::CComboBox* ) ref );
		else if ( ref->mWindowType == NS_VSBTN )
			CNSVsBtn::pushUserData( luaStack,( NSWin32::CNSVsBtn* ) ref );
		else if ( ref->mWindowType == NS_VSTREE )
			CNSVsTree::pushUserData( luaStack,( NSWin32::CNSVsTree* ) ref );
		else if ( ref->mWindowType == NS_VSLIST )
			CNSVsList::pushUserData( luaStack,( NSWin32::CNSVsList* ) ref );
		else if ( ref->mWindowType == NS_VSLISTBOX )
			CNSVsListBox::pushUserData( luaStack,( NSWin32::CNSVsListBox* ) ref );
		else if ( ref->mWindowType == NS_VSFILEBROWSER )
			CVsFileBrowser::pushUserData( luaStack,( NSWin32::CVsFileBrowser* ) ref );
		else if ( ref->mWindowType == NS_VSWIZARD )
			CVsWizard::pushUserData( luaStack,( NSWin32::CVsWizard* ) ref );
		else if ( ref->mWindowType == NS_FRAME )
			CNSFrame::pushUserData( luaStack,( NSWin32::CNSFrame* ) ref );
	}

	void CNSWindow::popUserData( const CNSLuaStack& luaStack, CNSWindow*& ref )
	{
		CNSWindow* window = NULL;
		if ( window == NULL )
			CNSVsStatic::popUserData( luaStack, (CNSVsStatic*&) window );
		if ( window == NULL )
			CNSVsTab::popUserData( luaStack, (CNSVsTab*&) window );
		if ( window == NULL )
			CNSEdit::popUserData( luaStack, (CNSEdit*&) window );
		if ( window == NULL )
			CNSVsEdit::popUserData( luaStack, (CNSVsEdit*&) window );
		if ( window == NULL )
			CComboBox::popUserData( luaStack, (CComboBox*&) window );
		if ( window == NULL )
			CNSVsBtn::popUserData( luaStack, (CNSVsBtn*&) window );
		if ( window == NULL )
			CNSVsTree::popUserData( luaStack, (CNSVsTree*&) window );
		if ( window == NULL )
			CNSVsList::popUserData( luaStack, (CNSVsList*&) window );
		if ( window == NULL )
			CNSVsListBox::popUserData( luaStack, (CNSVsListBox*&) window );
		if ( window == NULL )
			CVsFileBrowser::popUserData( luaStack, (CVsFileBrowser*&) window );
		if ( window == NULL )
			CVsWizard::popUserData( luaStack, (CVsWizard*&) window );
		if ( window == NULL )
			CNSFrame::popUserData( luaStack, (CNSFrame*&) window );
		if ( window == NULL )
		{
			static CNSString errorDesc;
			errorDesc.format( _UTF8( "参数[%d], 不是有效的窗口类型" ) );
			NSException( errorDesc );
		}

		ref = window;
	}
}