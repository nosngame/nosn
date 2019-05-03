#pragma once
namespace NSWin32
{
	class CNSPreDefine
	{
	public:
		static HFONT	FB_FONT_NOTIFY;
		static HFONT	FB_FONT_BASE;
		static HFONT	FB_FONT_BASEBOLD;
	};

	enum EHotkeyFlag
	{
		HOTKEY_NONE = 0,
		HOTKEY_ALT = 0x01,
		HOTKEY_CTRL = 0x02,
		HOTKEY_SHIFT = 0x04,
	};

	enum EHotkeyKeycode
	{
		KEYCODE_NONE = 0,
		KEYCODE_F1 = VK_F1,
		KEYCODE_F2 = VK_F2,
		KEYCODE_F3 = VK_F3,
		KEYCODE_F4 = VK_F4,
		KEYCODE_F5 = VK_F5,
		KEYCODE_F6 = VK_F6,
		KEYCODE_F7 = VK_F7,
		KEYCODE_F8 = VK_F8,
		KEYCODE_F9 = VK_F9,
		KEYCODE_F10 = VK_F10,
		KEYCODE_F11 = VK_F11,
		KEYCODE_F12 = VK_F12,
		KEYCODE_RETURN = VK_RETURN,
		KEYCODE_ESC = VK_ESCAPE,
		KEYCODE_A = 'A',
		KEYCODE_B = 'B',
		KEYCODE_C = 'C',
		KEYCODE_F = 'F',
		KEYCODE_L = 'L',
		KEYCODE_G = 'G',
		KEYCODE_S = 'S',
		KEYCODE_PLUS = VK_OEM_PLUS,
		KEYCODE_MINUS = VK_OEM_MINUS,
		KEYCODE_LEFT = VK_LEFT,
		KEYCODE_RIGHT = VK_RIGHT,
	};

	class CNSBaseApp
	{
		friend class CNSWindow;
	private:
		static CNSBaseApp* app;

	private:
		class CIPAddress
		{
		public:
			CNSString mInner;
			CNSString mOuter;
			CNSString mClient;

		public:
			CIPAddress( const CNSString& inner = "", const CNSString& outer = "", const CNSString& client = "" ) : mInner( inner ), mOuter( outer ), mClient( client )
			{
			}
		};

	protected:
		CNSString mName;			// 进程唯一标识(不要用中文)
		HINSTANCE mInstance = NULL;
		bool mEnableDebug = true;
		bool mUseNSHttpDebugger = false;
		bool mUseNSHttp = false;
		bool mUseNSMysql = false;
		bool mUseIPName = false;
		bool mUseNSLocal = false;
		CNSMap< CNSString, CIPAddress > mMachineTable;
		CNSMap< CNSString, CNSString > mCmdList;

	public:
		CNSBaseApp( );
		CNSBaseApp( const CNSString& name, bool enableDebug );

	public:
		~CNSBaseApp( );

	private:
		//生产DUMP文件  
		static int GenerateMiniDump( HANDLE hFile, PEXCEPTION_POINTERS pExceptionPointers );
		static LONG WINAPI RedirectedSetUnhandledExceptionFilter( EXCEPTION_POINTERS* /*ExceptionInfo*/ );
		static LONG WINAPI OurSetUnhandledExceptionFilter( EXCEPTION_POINTERS* pExceptionInfo );

	protected:
		void parseCommandLine( );

	protected:
		virtual void onInitApp( ) = 0;
		virtual void onExitApp( ) = 0;
		virtual bool onPreTranslateMessage( MSG& msg ) { return false; }
		virtual void onIdle( );

	public:
		virtual void pumpMessage( );

	public:
		void run( );
		HINSTANCE getInstance( ) const
		{
			return mInstance;
		}

		bool getCmdBool( const CNSString& key, bool defaultValue );
		int getCmdInt32( const CNSString& key, int defaultValue );
		double getCmdNumber( const CNSString& key, double defaultValue );
		CNSString getCmdString( const CNSString& key, const CNSString& defaultValue );

		const CNSString& name2IPPort( const CNSString& machineIP );
		const CNSString& name2IPAddress( const CNSString& machineIP );
		void loadIPName( );

		void useIPName( bool enable ) { mUseIPName = enable; }
		void useNSHttpDebugger( bool use ) { mUseNSHttpDebugger = use; }
		void useNSHttp( bool use ) { mUseNSHttp = use; }
		void useNSMysql( bool use ) { mUseNSMysql = use; }
		void useNSLocal( bool use ) { mUseNSLocal = use; }
		bool isEnableDebug( ) const
		{
			return mEnableDebug;
		}

		const CNSString& getName( ) const
		{
			return mName;
		}

	public:
		static CNSBaseApp* getApp( )
		{
			return app;
		}
	};

	template< typename T >
	class CConsoleApp : public CNSBaseApp
	{
	protected:
		T mLogic;
		DWORD mNumReaded;
		INPUT_RECORD* mpInputBuffer = NULL;

	public:
		CConsoleApp( )
		{
		}

		~CConsoleApp( )
		{
		}

	public:
		static T* getLogic( )
		{
			CConsoleApp* app = (CConsoleApp*) CNSBaseApp::getApp( );
			return &app->mLogic;
		}

		static void setConsoleTitle( const CNSString& title )
		{
			// 设置窗口标题
			HWND wnd = ::GetConsoleWindow( );
			if ( wnd == NULL )
			{
				int errorCode = GetLastError( );
				CNSString errorDesc;
				errorDesc.format( _UTF8( "错误[GetConsoleWindow 函数调用失败，错误码 - %d]" ), errorCode );
				NSException( errorDesc );
			}

			NSWin32::CNSWindow::setWindowText( wnd, title );
		}

	protected:
		virtual void onInitApp( )
		{
			CNSString randText( "unnamed_" );
			for ( int i = 0; i < 10; i++ )
			{
				int value = NSFunction::random( 0, 9 );
				randText.pushback( CNSString::number2String( value ) );
			}
			parseCommandLine( );
			mName = getCmdString( "name", randText );
			mEnableDebug = getCmdBool( "debug", true );
			CNSLocal::getNSLocal( ).setLang( getCmdString( "lang", "ch" ) );

			CNSBaseApp::onInitApp( );
			// 设置窗口标题
			HWND wnd = ::GetConsoleWindow( );
			if ( wnd == NULL )
			{
				int errorCode = GetLastError( );
				CNSString errorDesc;
				errorDesc.format( _UTF8( "错误[GetConsoleWindow 函数调用失败，错误码 - %d]" ), errorCode );
				NSException( errorDesc );
			}

			static CNSString title;
#ifdef _M_IX86
			title.format( _UTF8( "%s 32bit" ), mName.getBuffer( ) );
#elif _M_X64
			title.format( _UTF8( "%s 64bit" ), mName.getBuffer( ) );
#endif
			NSWin32::CNSWindow::setWindowText( wnd, title );
			NSConsole::setHostWnd( wnd );
			mLogic.init( this );
		}

		virtual void onExitApp( )
		{
			CNSBaseApp::onExitApp( );
			mLogic.exit( );
		}

		void readConsoleInput( )
		{
			HANDLE stdHandle = ::GetStdHandle( STD_INPUT_HANDLE );
			DWORD numEvent;
			::GetNumberOfConsoleInputEvents( stdHandle, &numEvent );

			mpInputBuffer = new INPUT_RECORD[ numEvent ];
			if ( ::ReadConsoleInput( stdHandle, mpInputBuffer, numEvent, &mNumReaded ) == FALSE )
			{
				int errorCode = GetLastError( );
				CNSString errorDesc;
				errorDesc.format( _UTF8( "错误[ReadConsoleInput 函数调用失败，错误码 - %d]" ), errorCode );
				NSException( errorDesc );
			}
		}

		void clearConsoleInput( )
		{
			delete[] mpInputBuffer;
		}

		bool isConsoleKeyPressed( int controlKey, NSWin32::EHotkeyKeycode virtualKey )
		{
			bool trigger = false;
			for ( DWORD i = 0; i < mNumReaded; i ++ )
			{
				if ( mpInputBuffer[ i ].EventType == KEY_EVENT )
				{
					KEY_EVENT_RECORD* keyEvent = &mpInputBuffer[ i ].Event.KeyEvent;
					if ( controlKey & NSWin32::EHotkeyFlag::HOTKEY_CTRL )
					{
						if ( keyEvent->dwControlKeyState & LEFT_CTRL_PRESSED || keyEvent->dwControlKeyState & RIGHT_CTRL_PRESSED )
						{
							if ( keyEvent->bKeyDown == TRUE && keyEvent->wVirtualKeyCode == virtualKey )
							{
								trigger = true;
								break;
							}
						}
					}
					else if ( controlKey & NSWin32::EHotkeyFlag::HOTKEY_ALT )
					{
						if ( keyEvent->dwControlKeyState & LEFT_ALT_PRESSED || keyEvent->dwControlKeyState & RIGHT_ALT_PRESSED )
						{
							if ( keyEvent->bKeyDown == TRUE && keyEvent->wVirtualKeyCode == virtualKey )
							{
								trigger = true;
								break;
							}
						}
					}
					else if ( controlKey & NSWin32::EHotkeyFlag::HOTKEY_SHIFT )
					{
						if ( keyEvent->dwControlKeyState & SHIFT_PRESSED )
						{
							if ( keyEvent->bKeyDown == TRUE && keyEvent->wVirtualKeyCode == virtualKey )
							{
								trigger = true;
								break;
							}
						}
					}
					else
					{
						if ( keyEvent->bKeyDown == TRUE && keyEvent->wVirtualKeyCode == virtualKey )
						{
							trigger = true;
							break;
						}
					}
				}
			}

			return trigger;
		}

		virtual void pumpMessage( )
		{
			while ( 1 )
			{
				try
				{
					if ( isEnableDebug( ) == true )
					{
						readConsoleInput( );
						if ( isConsoleKeyPressed( NSWin32::EHotkeyFlag::HOTKEY_CTRL, NSWin32::EHotkeyKeycode::KEYCODE_F1 ) == true )
							NSConsole::showConsole( );

						if ( isConsoleKeyPressed( NSWin32::EHotkeyFlag::HOTKEY_NONE, NSWin32::EHotkeyKeycode::KEYCODE_ESC ) == true )
							break;
						clearConsoleInput( );

						if ( NSWin32::CNSWindow::getWindow( "consoleWindow" ) == NULL )
							onIdle( );
						else
						{
							MSG msg;
							if ( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) == TRUE )
							{
								if ( msg.message == WM_QUIT )
									break;

								if ( onPreTranslateMessage( msg ) == false )
								{
									TranslateMessage( &msg );
									DispatchMessage( &msg );
								}
							}
							else
							{
								onIdle( );
							}
						}
					}
					else
					{
						onIdle( );
					}
				}
				catch ( CNSException& e )
				{
					NSLog::exception( _UTF8( "程序主循环发生异常\n错误描述: \n\t%s\nC++调用堆栈:\n%s" ), NSLog::sExceptionText.getBuffer( ), 
									  e.mErrorDesc, NSBase::NSFunction::getStackInfo( ).getBuffer( ) );
				}
			}
		}
	};

	class CHotkey
	{
	public:
		typedef void( *FGlobalHotkeyHandler )( int flag, int keycode );
		typedef void( *FLocalHotkeyHandler )( int flag, int keycode, NSWin32::CNSWindow* window );

		class CHotkeyData
		{
		public:
			int	mHotkeyFlag = 0;
			int	mKeycode = 0;
			bool mTriggered = false;
			FGlobalHotkeyHandler mGlobalHandler = NULL;
			FLocalHotkeyHandler mLocalHandler = NULL;
			NSWin32::CNSWindow* mpWindow = NULL;
		public:
			CHotkeyData( )
			{
			}

			CHotkeyData( int flag, int keyCode, FGlobalHotkeyHandler handler )
				: mHotkeyFlag( flag ), mKeycode( keyCode ), mGlobalHandler( handler ), mTriggered( false )
			{
			}
			CHotkeyData( int flag, int keyCode, FLocalHotkeyHandler handler, CNSWindow* window )
				: mHotkeyFlag( flag ), mKeycode( keyCode ), mLocalHandler( handler ), mTriggered( false ), mpWindow( window )
			{
			}
		};

	public:
		CNSMap< int, CHotkeyData > mHotkeyData;

	public:
		CHotkey( )
		{
		}

		void addHotkey( int flag, int keyCode, FGlobalHotkeyHandler handler )
		{
			mHotkeyData.insert( ( flag << 24 ) | keyCode, CHotkeyData( flag, keyCode, handler ) );
		}

		void addHotkey( int flag, int keyCode, FLocalHotkeyHandler handler, CNSWindow* window )
		{
			mHotkeyData.insert( ( flag << 24 ) | keyCode, CHotkeyData( flag, keyCode, handler, window ) );
		}

		bool processHotkey( unsigned char* keyState )
		{
			bool processed = false;
			for ( HLISTINDEX beginIndex = mHotkeyData.getHead( ); beginIndex != NULL; mHotkeyData.getNext( beginIndex ) )
			{
				CHotkeyData& keyData = mHotkeyData.getValue( beginIndex );
				unsigned char state = keyState[ keyData.mKeycode ];
				if ( keyData.mTriggered == true )
				{
					if ( keyData.mHotkeyFlag & HOTKEY_ALT )
					{
						if ( ( keyState[ VK_LMENU ] & 0x80 ) != 0x80 && ( keyState[ VK_RMENU ] & 0x80 ) != 0x80 )
							keyData.mTriggered = false;
					}
					else
					{
						if ( ( keyState[ VK_LMENU ] & 0x80 ) == 0x80 || ( keyState[ VK_RMENU ] & 0x80 ) == 0x80 )
							keyData.mTriggered = false;
					}

					if ( keyData.mHotkeyFlag & HOTKEY_CTRL )
					{
						if ( ( keyState[ VK_LCONTROL ] & 0x80 ) != 0x80 && ( keyState[ VK_RCONTROL ] & 0x80 ) != 0x80 )
							keyData.mTriggered = false;
					}
					else
					{
						if ( ( keyState[ VK_LCONTROL ] & 0x80 ) == 0x80 || ( keyState[ VK_RCONTROL ] & 0x80 ) == 0x80 )
							keyData.mTriggered = false;
					}

					if ( keyData.mHotkeyFlag & HOTKEY_SHIFT )
					{
						if ( ( keyState[ VK_LSHIFT ] & 0x80 ) != 0x80 && ( keyState[ VK_RSHIFT ] & 0x80 ) != 0x80 )
							keyData.mTriggered = false;
					}
					else
					{
						if ( ( keyState[ VK_LSHIFT ] & 0x80 ) == 0x80 || ( keyState[ VK_RSHIFT ] & 0x80 ) == 0x80 )
							keyData.mTriggered = false;
					}

					if ( ( state & 0x80 ) != 0x80 )
						keyData.mTriggered = false;
				}
				else
				{
					if ( keyData.mHotkeyFlag & HOTKEY_ALT )
					{
						if ( ( keyState[ VK_LMENU ] & 0x80 ) != 0x80 && ( keyState[ VK_RMENU ] & 0x80 ) != 0x80 )
							continue;
					}
					else
					{
						if ( ( keyState[ VK_LMENU ] & 0x80 ) == 0x80 || ( keyState[ VK_RMENU ] & 0x80 ) == 0x80 )
							continue;
					}

					if ( keyData.mHotkeyFlag & HOTKEY_CTRL )
					{
						if ( ( keyState[ VK_LCONTROL ] & 0x80 ) != 0x80 && ( keyState[ VK_RCONTROL ] & 0x80 ) != 0x80 )
							continue;
					}
					else
					{
						if ( ( keyState[ VK_LCONTROL ] & 0x80 ) == 0x80 || ( keyState[ VK_LCONTROL ] & 0x80 ) == 0x80 )
							continue;
					}

					if ( keyData.mHotkeyFlag & HOTKEY_SHIFT )
					{
						if ( ( keyState[ VK_LSHIFT ] & 0x80 ) != 0x80 && ( keyState[ VK_RSHIFT ] & 0x80 ) != 0x80 )
							continue;
					}
					else
					{
						if ( ( keyState[ VK_LSHIFT ] & 0x80 ) == 0x80 || ( keyState[ VK_RSHIFT ] & 0x80 ) == 0x80 )
							continue;
					}

					if ( ( state & 0x80 ) != 0x80 )
						continue;

					keyData.mTriggered = true;
					if ( keyData.mGlobalHandler != NULL )
						keyData.mGlobalHandler( keyData.mHotkeyFlag, keyData.mKeycode );

					if ( keyData.mLocalHandler != NULL )
						keyData.mLocalHandler( keyData.mHotkeyFlag, keyData.mKeycode, keyData.mpWindow );
				}

				processed = true;
			}

			return processed;
		}
	};

	class CNSWindow : public NSBase::CNSLuaWeakRef
	{
	protected:
		class CMoveProxy
		{
			HWND		mMoveWindow;
			CNSVector2	mTarget;
			CNSVector2	mPosition;
			float		mCurSpeed;
			float		mCurAcc;
			float		mHalfDur;
			float		mDuration;
			int			mState;
			int			mWidth;
			int			mHeight;
			DWORD		mLastTime;
			static CNSVector< CMoveProxy > sMoveList;

		public:
			CMoveProxy( HWND wnd, const CNSVector2& target, float duration )
				: mMoveWindow( wnd ), mTarget( target ), mCurSpeed( 0 ), mHalfDur( duration / 2.0f ), mDuration( duration ), mState( 0 ), mLastTime( CNSTimer::getCurTick( ) )
			{
				RECT rc;
				GetWindowRect( mMoveWindow, &rc );
				mPosition.setX( (float) rc.left );
				mPosition.setZ( (float) rc.top );
				mCurAcc = ( mTarget - mPosition ).magnitude( ) / ( ( mDuration / 2.0f ) * mDuration );

				mWidth = rc.right - rc.left;
				mHeight = rc.bottom - rc.top;
			}

			bool update( unsigned int curTick )
			{
				float timeDelta = ( curTick - mLastTime ) / 1000.0f;
				mLastTime = curTick;

				float timeDeltaReal = min( mHalfDur, timeDelta );
				if ( mState == 0 )
				{
					mHalfDur = mHalfDur - timeDeltaReal;
					if ( mHalfDur <= 0.0f )
					{
						mState = 1;
						mHalfDur = mDuration / 2.0f;
					}
				}
				else if ( mState == 1 )
				{
					mHalfDur = mHalfDur + timeDeltaReal;
					if ( mHalfDur <= 0.0f )
						mState = 2;
				}

				mCurSpeed = max( 100.0f, mCurSpeed + mCurAcc * timeDelta );

				CNSVector2 offset = mTarget - mPosition;
				CNSVector2 delta = offset.normalize( ) * mCurSpeed * timeDelta;
				if ( offset.magnitude( ) <= delta.magnitude( ) || mState == 2 )
				{
					::MoveWindow( mMoveWindow, mTarget.getIntX( ), mTarget.getIntZ( ), mWidth, mHeight, FALSE );
					KillTimer( mMoveWindow, 101 );
					return false;
				}

				mPosition = delta + mPosition;
				::MoveWindow( mMoveWindow, mPosition.getIntX( ), mPosition.getIntZ( ), mWidth, mHeight, FALSE );
				return true;
			}

		public:
			static void addMove( HWND wnd, const CNSVector2& target, float duration )
			{
				CMoveProxy move( wnd, target, duration );
				sMoveList.pushback( move );
			}

			static void updateMove( unsigned int curTime )
			{
				for ( int i = sMoveList.getCount( ) - 1; i >= 0; i -- )
				{
					if ( sMoveList[ i ].update( curTime ) == false )
						sMoveList.erase( i );
				}
			}
		};

	public:
		typedef bool( *WindowEventHandler )( CNSWindow* child, WPARAM wParam, LPARAM lParam, LRESULT* result );

	public:
		enum EAnchor
		{
			ANCHOR_LEFT = 0,
			ANCHOR_RIGHT,
			ANCHOR_TOP,
			ANCHOR_BOTTOM,
			ANCHOR_CENTER,
		};

		unsigned char							mLeftAnchor = ANCHOR_LEFT;
		unsigned char							mRightAnchor = ANCHOR_RIGHT;
		unsigned char							mTopAnchor = ANCHOR_TOP;
		unsigned char							mBottomAnchor = ANCHOR_BOTTOM;
		// 是否处理Anchor
		bool									mProcessAnchor = true;
		static HINSTANCE						instance;
		CNSString								mWindowID;
		CNSString								mWindowType;

	protected:
		HWND									mHWnd = NULL;
		CHotkey									mHotkey;
		intptr_t								mUserData = 0;
		SIZE									mLastSize;

		static float							messageAlpha;
		static int								messageState;
		static int								messageKeepTick;
		static CNSString						messageText;
		static CHotkey							sGlobalHotkey;
		static CNSMap< CNSString, CNSWindow* >	sWindows;
		static CNSMap< ATOM, WNDPROC >			mWndProc;

		static HBRUSH							brushScrollBack;
		static HBRUSH							brushNCBack;
		static HBRUSH							brushFocusWindow;
		static HPEN								penBorder;
		static HBRUSH							brushScrollBtn;
		static HPEN								penScrollBtn;
		static HBRUSH							brushScrollHoverBtn;
		static HPEN								penScrollHoverBtn;
		static HBRUSH							brushScrollClickBtn;
		static HPEN								penScrollClickBtn;
		static HBRUSH							brushScrollThumb;
		static HBRUSH							brushScrollHoverThumb;
		static HBRUSH							brushScrollClickThumb;
		static int								scrollWidth;
		static int								titleHeight;
		CNSMap< int, WindowEventHandler >		eventHandler;
		CNSMap< int, WindowEventHandler >		messageHandler;

	protected:
		bool			mDownScrollUp = false;
		bool			mDownScrollDown = false;
		bool			mDownScrollLeft = false;
		bool			mDownScrollRight = false;
		bool			mDownScrollVThumb = false;
		bool			mDownScrollHThumb = false;
		bool			mHoverScrollUp = false;
		bool			mHoverScrollDown = false;
		bool			mHoverScrollLeft = false;
		bool			mHoverScrollRight = false;
		bool			mHoverScrollVThumb = false;
		bool			mHoverScrollHThumb = false;
		bool			mNcTracking = false;
		POINT			mLastMouse = { 0, 0 };
		int				mVScrollPos = -1;
		int				mHScrollPos = -1;
		bool			mEnableTitle = false;
		bool			mTitleHighlight = false;
		CNSString		mTitle;
		CNSWindow*		mParent = NULL;
		CNSMap< CNSString, CNSWindow* > mChildren;
		CNSMap< unsigned int, CNSLuaFunction > mLuaEventHandler;
		CNSMap< unsigned int, CNSLuaFunction > mLuaMsgHandler;
		CNSMap< unsigned int, CNSLuaFunction > mLuaLHotkeyHandler;

		static CNSMap< unsigned int, CNSLuaFunction > sLuaGHotkeyHandler;

	protected:
		// 都是窗口坐标系
		void getScrollUpRect( RECT& rc );
		void getScrollDownRect( RECT& rc );
		void getVScrollRect( RECT& rc );
		void getVScrollThumbRect( RECT& rc );
		void drawScrollUpButton( HDC dc );
		void drawScrollDownButton( HDC dc );
		void drawVScrollThumb( HDC dc );

		void getScrollLeftRect( RECT& rc );
		void getScrollRightRect( RECT& rc );
		void getHScrollRect( RECT& rc );
		void getHScrollThumbRect( RECT& rc );
		void drawScrollLeftButton( HDC dc );
		void drawScrollRightButton( HDC dc );
		void drawHScrollThumb( HDC dc );
		void getScrollNcRect( RECT& rc );
		virtual void getTitleRect( RECT& rc );
		// 都是窗口坐标系

		void onScrollBarClicked( POINT& mouse );
		virtual void scrollVert( int pos );
		virtual void scrollHorz( int pos );

		void redrawScrollbar( HDC dc );
		virtual void drawTitle( HDC dc, bool headerLine = true );

	protected:
		static void __stdcall FocusTimer( HWND wnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime );
		static void __stdcall MoveTimer( HWND wnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime );
		static bool onMessageRender( CNSWindow* child, WPARAM wParam, LPARAM lParam, LRESULT* result );
		static bool onMessageTimer( CNSWindow* child, WPARAM wParam, LPARAM lParam, LRESULT* result );

	public:
		CNSWindow( const CNSString& windowID, const CNSString& windowType );
		RECT calcSizedRect( SIZE& rcNew, SIZE& rcOld );
		static void init( HINSTANCE instance );
		static void exit( );
		static void regLuaLib( );
	public:
		static void pushUserData( CNSLuaStack& luaStack, CNSWindow* ref );
		static void popUserData( const CNSLuaStack& luaStack, CNSWindow*& ref );
		static LRESULT CALLBACK windowProc( HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam );

		// 超类化指定类，从srcClass->desClass
		static void superClass( TCHAR* srcClass, TCHAR* desClass )
		{
			WNDCLASSEX wcex;
			wcex.cbSize = sizeof( WNDCLASSEX );
			GetClassInfoEx( CNSWindow::instance, srcClass, &wcex );

			WNDPROC oldProc = wcex.lpfnWndProc;
			wcex.lpfnWndProc = CNSWindow::windowProc;
			wcex.lpszClassName = desClass;
			ATOM classAtom = RegisterClassEx( &wcex );
			if ( classAtom == NULL )
			{
				DWORD errorCode = GetLastError( );
				static CNSString errorDesc;
				errorDesc.format( _UTF8( "win32函数[RegisterClassEx]调用失败，错误码: %d" ), errorCode );
				NSException( errorDesc );
			}

			CNSWindow::mWndProc.insert( classAtom, oldProc );
		}

	public:
		LRESULT callWndProc( UINT msg, WPARAM wParam, LPARAM lParam )
		{
			ATOM classAtom = (ATOM) GetClassLongPtr( mHWnd, GCW_ATOM );
			WNDPROC* oldProcRef = mWndProc.get( classAtom );
			if ( oldProcRef == NULL )
			{
				static CNSString errorDesc;
				errorDesc.format( "callWndProc消息 - 0x%04x时, 没有找到源类型窗口回调, WPARAM - %zd, LPARAM - %zd", msg, wParam, lParam );
				NSException( errorDesc );
			}

			WNDPROC oldProc = ( *oldProcRef );
			return ::CallWindowProc( oldProc, mHWnd, msg, wParam, lParam );
		}

	public:
		template< typename T >
		static T* newDialog( const CNSString& windowID, unsigned int style, RECT& rc, CNSWindow* parent, bool center )
		{
			int dialogStyle = WS_VISIBLE | WS_CAPTION | WS_POPUPWINDOW | WS_THICKFRAME | WS_CLIPCHILDREN;
			int dialogStyleEx = WS_EX_TOOLWINDOW;
			AdjustWindowRectEx( &rc, dialogStyle, FALSE, dialogStyleEx );
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

			T* window = new T( windowID, style );
			window->mIsModalDialog = true;
			::CreateWindowEx( dialogStyleEx, WC_NS_FRAME, NULL, dialogStyle, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
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

		static CNSVsTab*		newVsTab( const CNSString& windowID, unsigned int tabType, RECT& rc, CNSWindow* parent );
		static CNSVsEdit*		newVsEdit( const CNSString& windowID, unsigned int editType, RECT& rc, CNSWindow* parent );
		static CNSVsList*		newVsList( const CNSString& windowID, unsigned int listType, RECT& rc, CNSWindow* parent );
		static CNSVsBtn*		newVsBtn( const CNSString& windowID, unsigned int btnType, RECT& rc, CNSWindow* parent );
		static CNSVsTree*		newVsTree( const CNSString& windowID, unsigned int treeType, RECT& rc, CNSWindow* parent );
		static CVsWizard*		newWizard( const CNSString& windowID, unsigned int wizardType, RECT& rc, CNSWindow* parent );
		static CVsFileBrowser*	newFileBrowser( const CNSString& windowID, unsigned int style, RECT& rc, CNSWindow* parent );
		static CNSVsStatic*	newVsStatic( const CNSString& windowID, unsigned int staticType, RECT& rc, CNSWindow* parent );
		static CNSEdit*		newEdit( const CNSString& windowID, unsigned int editType, RECT& rc, CNSWindow* parent );
		static CNSVsListBox*		newVsListBox( const CNSString& windowID, unsigned int editType, RECT& rc, CNSWindow* parent );
		static CComboBox*		newComboBox( const CNSString& windowID, unsigned int comboType, RECT& rc, CNSWindow* parent );
		static CNSFrame*			newFrame( const CNSString& windowID, unsigned int frameType, RECT& rc, CNSWindow* parent, bool center = false, HMENU menu = NULL );
		static CNSCustom*		newCustom( const CNSString& windowID, unsigned int style, unsigned int styleEx, RECT& rc, CNSWindow* parent );

		static void				doModal( CNSFrame* frame );
		static CNSWindow*			getWindow( const CNSString& windowID );
		static void				update( );
		static CNSWindow*			getFocus( );
		static CNSWindow*			getActive( );
		static CNSString&		getWindowText( HWND wnd );
		static void				setWindowText( HWND wnd, const CNSString& text );
		static CNSWindow*			fromHWnd( HWND wnd );
		static void				notification( const CNSString& text );
		static void				registerGlobalHotkey( int flag, int keyCode, CHotkey::FGlobalHotkeyHandler handler );

		void registerEvent( unsigned int code, WindowEventHandler handler );
		void registerMessage( unsigned int msg, WindowEventHandler handler );
		void registerLocalHotkey( int flag, int keyCode, CHotkey::FLocalHotkeyHandler handler );
		CNSWindow* focus( );
		CNSWindow* getParent( );
		HWND getHWnd( );
		const CNSString& getWindowID( ) const
		{
			return mWindowID;
		}

		void setVisible( bool visible );
		void move( int x, int y, float duration, bool ani );
		void delayFocus( );
		void destroy( );
		void close( );
		void setText( const CNSString& text );
		CNSString& getText( );
		void refresh( );
		void refreshNc( );
		void maximized( );
		void minimized( );
		void show( );
		void hide( );
		void enable( bool enable );
		CNSWindow* active( );
		bool isVisible( );
		bool isEnable( );
		void setTimer( int timerID, int interval );
		void killTimer( int timerID );
		void getWindowRect( RECT& rc );
		void getClientRect( RECT& rc );
		void setRedraw( BOOL redraw );
		void enableTitle( bool enable )
		{
			mEnableTitle = enable;
			if ( this == getFocus( ) )
				mTitleHighlight = true;
		}

		void setTitle( const CNSString& title )
		{
			mTitle = title;
		}

		void processAnchor( bool enable )
		{
			mProcessAnchor = enable;
		}

		static void setLuaGHotkeyRef( int flag, int keycode, CNSLuaFunction& func )
		{
			sLuaGHotkeyHandler.insert( ( flag << 24 ) | keycode, func );
		}

		static const CNSLuaFunction& getLuaGHotkeyRef( int flag, int keycode )
		{
			const CNSLuaFunction* funcRef = sLuaGHotkeyHandler.get( ( flag << 24 ) | keycode );
			if ( funcRef == NULL )
			{
				static CNSLuaFunction luaFunc;
				return luaFunc;
			}

			return *funcRef;
		}

		void setLuaLHotkeyRef( int flag, int keycode, const CNSLuaFunction& func )
		{
			mLuaLHotkeyHandler.insert( ( flag << 24 ) | keycode, func );
		}

		const CNSLuaFunction& getLuaLHotkeyRef( int flag, int keycode )
		{
			const CNSLuaFunction* funcRef = mLuaLHotkeyHandler.get( ( flag << 24 ) | keycode );
			if ( funcRef == NULL )
			{
				static CNSLuaFunction luaFunc;
				return luaFunc;
			}

			return *funcRef;
		}

		void setLuaEventHandler( unsigned int notifyCode, const CNSLuaFunction& func )
		{
			mLuaEventHandler.insert( notifyCode, func );
		}

		const CNSLuaFunction& getLuaEventHandler( unsigned int notifyCode ) const
		{
			const CNSLuaFunction* funcRef = mLuaEventHandler.get( notifyCode );
			if ( funcRef == NULL )
			{
				static CNSLuaFunction luaFunc;
				return luaFunc;
			}

			return *funcRef;
		}

		void setLuaMsgHandler( unsigned int notifyCode, const CNSLuaFunction& func )
		{
			mLuaMsgHandler.insert( notifyCode, func );
		}

		const CNSLuaFunction& getLuaMsgHandler( unsigned int notifyCode ) const
		{
			const CNSLuaFunction* funcRef = mLuaMsgHandler.get( notifyCode );
			if ( funcRef == NULL )
			{
				static CNSLuaFunction luaFunc;
				return luaFunc;
			}

			return *funcRef;
		}

		void setUserData( intptr_t data )
		{
			mUserData = data;
		}

		intptr_t getUserData( ) const
		{
			return mUserData;
		}

	protected:
		virtual void onSize( int width, int height, int sizeFlag );
		virtual void onSetFocus( HWND lastFocus );
		virtual void onKillFocus( HWND newFocus );
		virtual bool onNcCalcSize( bool clientArea, NCCALCSIZE_PARAMS* calcSize );
		virtual bool onNcPaint( HRGN rgn );
		virtual bool onNcMouseMove( int hitTest, POINT pt );
		virtual bool onNcLButtonDown( int hitTest, POINT pt );
		virtual bool onNcLButtonDblClk( int hitTest, POINT pt );
		virtual bool onNcMouseLeave( );
		virtual void onLButtonDown( int flag, POINT pt );
		virtual void onLButtonDblClk( int flag, POINT pt );
		virtual void onLButtonUp( int flag, POINT pt );
		virtual void onMouseWheel( int delta, int flag, POINT pt );
		virtual void onMouseMove( int flag, POINT pt );
		virtual void onHScroll( int flag, int pos );
		virtual void onVScroll( int flag, int pos );

	public:
		virtual void onCreateWindow( );
		virtual void onPostCreateWindow( CNSWindow* parent );
		virtual void onNcCreateWindow( );

	protected:
		void notifyParent( UINT code )
		{
			CNSWindow* parent = mParent;
			if ( parent != NULL )
			{
				NMHDR hdr;
				hdr.code = code;
				hdr.idFrom = GetDlgCtrlID( mHWnd );
				hdr.hwndFrom = mHWnd;
				SendMessage( parent->getHWnd( ), WM_NOTIFY, hdr.idFrom, (LPARAM) &hdr );
			}
		}
	};
}