#include <nsbase.h>

namespace NSConsole
{
	enum EDebugStatus
	{
		DEBUG_NONE,
		DEBUG_STEP,
		DEBUG_INTO
	};

	enum EMarkerType
	{
		MARKER_BREAK,
		MARKER_INTERRUPT,
		MARKER_LINETIP,
	};

	NSWin32::CNSBaseApp*				baseApp = NULL;
	int									PAGE_LOG = -1;
	int									PAGE_EXCEPTION = -1;
	int									PAGE_DEBUG = -1;
	bool								searchFrameActive = false;
	float								pos = 0;
	unsigned int						lastTick = 0;
	HDC									memDC = NULL;
	HBITMAP								memBmp = NULL;

	CNSMap< CNSString, CNSString >		luaFiles;
	CNSSet< CNSString >					luaBreaks;
	NSWin32::CNSFrame*					console = NULL;
	CNSString							mConsoleTitle = _UTF8( "NoSN - Lua调试器" );
	CNSString							curLuaFile;

	int									breakStack = 0;
	bool								searchWord = false;
	bool								searchCase = false;
	bool								blockFinding = false;
	int									lastFindStart = -1;
	int									oldTipLine = -1;
	CNSString							searchText;
	HWND								wndHost = NULL;

	int									luaDebugStatus = EDebugStatus::DEBUG_NONE;	// 是否单步跟踪

	CNSMap< int, CNSString >			openFileIndex;
	CNSMap< CNSString, int >			openFileList;
	int									openFileID = 1;

	// 当前中断信息
	CNSString							curIntrputFile;
	int									curIntrputLine = -1;
	bool								inBreak = false;

	void CreateConsoleWindow( );
	void showFile( const CNSString& filePath, const CNSString& buffer );

	void DebugReset( )
	{
		curIntrputLine = -1;
		curIntrputFile.clear( );
		luaBreaks.clear( );
		luaDebugStatus = EDebugStatus::DEBUG_NONE;
	}

	void DebugAddBreak( const CNSString& fileName, int line )
	{
		NSWin32::CNSVsEdit* editorCtrl = ( NSWin32::CNSVsEdit* )NSWin32::CNSWindow::getWindow( "consoleEditor" );
		editorCtrl->showMargin( line, EMarkerType::MARKER_BREAK );
		luaBreaks.insert( fileName + ":" + CNSString::number2String( line ) );
	}

	void DebugRemoveBreak( const CNSString& fileName, int line )
	{
		NSWin32::CNSVsEdit* editorCtrl = ( NSWin32::CNSVsEdit* )NSWin32::CNSWindow::getWindow( "consoleEditor" );
		editorCtrl->hideMargin( line, EMarkerType::MARKER_BREAK );
		luaBreaks.erase( fileName + ":" + CNSString::number2String( line ) );
	}

	void DebugRemoveAllBreaks( )
	{
		luaBreaks.clear( );
	}

	bool DebugHasBreak( const CNSString& fileName, int line )
	{
		return luaBreaks.find( fileName + ":" + CNSString::number2String( line ) );
	}

	void DebugGoto( const CNSString& fileName, int line )
	{
		NSWin32::CNSVsTab* tabCtrl = ( NSWin32::CNSVsTab* )NSWin32::CNSWindow::getWindow( "consoleTab" );
		NSWin32::CNSVsTree* treeCtrl = ( NSWin32::CNSVsTree* )NSWin32::CNSWindow::getWindow( "consoleTree" );
		NSWin32::CNSVsEdit* editorCtrl = ( NSWin32::CNSVsEdit* )NSWin32::CNSWindow::getWindow( "consoleEditor" );

		if ( tabCtrl->getCurSel( ) != PAGE_DEBUG )
			tabCtrl->setCurSel( PAGE_DEBUG );

		HTREEITEM item = treeCtrl->findItem( fileName );
		if ( item != NULL )
			treeCtrl->setCurSel( item );

		CNSString* textRef = (CNSString*) treeCtrl->getItemData( item );
		if ( textRef != NULL )
			showFile( fileName, *textRef );

		if ( NSWin32::CNSWindow::getFocus( ) != editorCtrl )
			editorCtrl->focus( );

		// 设置需要跳转的中断页边标记
		editorCtrl->showMargin( line, EMarkerType::MARKER_INTERRUPT );

		// 让中断行号在屏幕内显示出来
		editorCtrl->gotoLine( line );

		// 重新记录当前中断信息
		curIntrputLine = line;
		curIntrputFile = fileName;
	}

	void refreshListStack( NSWin32::CNSVsList* list )
	{
		NSBase::CNSLuaStack& luaStack = NSBase::CNSLuaStack::getLuaStack( );
		lua_State* lua = luaStack.getLuaState( );
		list->clear( );
		int level = 0;
		while ( 1 )
		{
			lua_Debug ar;
			if ( lua_getstack( lua, level, &ar ) == 0 )
				break;

			lua_getinfo( lua, "nSl", &ar );
			CNSString funcName;
			CNSString fileName = ar.source;
			CNSString line = CNSString::number2String( ar.currentline );
			if ( ar.name != NULL )
				funcName = ar.name;
			else
				funcName = ar.source;

			list->addItem( CNSString::number2String( level ) );
			list->setItem( 1, level, fileName );
			list->setItem( 2, level, funcName );
			list->setItem( 3, level, line );
			level ++;
		}
	}

	void DebugBreak( const CNSString& fileName, int line )
	{
		// 刷新调用堆栈
		NSWin32::CNSVsList* listStack = ( NSWin32::CNSVsList* ) NSWin32::CNSWindow::getWindow( "consoleListStack" );
		if ( listStack != NULL )
			refreshListStack( listStack );

		inBreak = true;
		// 打开调试窗口
		CreateConsoleWindow( );

		// 定位到中断文件和中断行号
		DebugGoto( fileName, line );

		// 开启消息循环维持住当前窗口逻辑
		if ( baseApp == NULL )
			NSException( _UTF8( "没有调用NSConsole::init" ) );

		while ( 1 )
		{
			try
			{
				MSG msg;
				if ( ::PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) == TRUE )
				{
					if ( msg.message == WM_QUIT )
						break;

					NSWin32::CNSWindow* varWatch = NSWin32::CNSWindow::getWindow( "varWatchFrame" );
					NSWin32::CNSWindow* fileList = NSWin32::CNSWindow::getWindow( "consoleFileListFrame" );
					NSWin32::CNSWindow* listStack = NSWin32::CNSWindow::getWindow( "consoleListStackFrame" );
					if ( console != NULL && GetAncestor( msg.hwnd, GA_ROOTOWNER ) == console->getHWnd( ) ||
						fileList != NULL && GetAncestor( msg.hwnd, GA_ROOTOWNER ) == fileList->getHWnd( ) ||
						listStack != NULL && GetAncestor( msg.hwnd, GA_ROOTOWNER ) == listStack->getHWnd( ) ||
						varWatch != NULL && GetAncestor( msg.hwnd, GA_ROOTOWNER ) == varWatch->getHWnd( ) )
					{
						// 只处理这四个窗口及其子窗口的消息
						::TranslateMessage( &msg );
						::DispatchMessage( &msg );
					}
					else
					{
						// 需要处理一下WM_PAINT, 因为WM_PAINT不会删除这个消息
						if ( msg.message == WM_PAINT )
						{
							PAINTSTRUCT ps;
							::BeginPaint( msg.hwnd, &ps );
							::EndPaint( msg.hwnd, &ps );
						}
					}
				}
				else
				{
					NSWin32::CNSWindow::update( );
					CNSTimer::updateTimer( );
					::Sleep( 1 );
				}
			}
			catch ( CNSException& e )
			{
				NSLog::exception( _UTF8( "程序调试断点阻塞主循环发生异常\n错误描述: \n\t%s\nC++调用堆栈:\n%s" ), e.mErrorDesc, NSBase::NSFunction::getStackInfo( ).getBuffer( ) );
			}
		}
		inBreak = false;
	}

	int getCurStackDeep( )
	{
		NSBase::CNSLuaStack& luaStack = NSBase::CNSLuaStack::getLuaStack( );
		lua_State* lua = luaStack.getLuaState( );
		int stackLevel = 0;
		while ( 1 )
		{
			lua_Debug ar;
			if ( lua_getstack( lua, stackLevel, &ar ) == 0 )
				break;

			stackLevel ++;
		}

		return stackLevel;
	}

	static void luaDebugCallback( lua_State* L, lua_Debug* ar )
	{
		try
		{
			static CNSString lastBreakFile;
			static int lastBreakLine = -1;
			switch ( ar->event )
			{
			case LUA_HOOKRET:
				break;
			case LUA_HOOKCALL:
				break;
			case LUA_HOOKLINE:
			{
				if ( luaDebugStatus == EDebugStatus::DEBUG_INTO )
				{
					// 如果是进入跟踪
					lua_getinfo( L, "nSl", ar );

					static CNSString fileName;
					fileName = ar->source;
					fileName.replace( "\\", "/" );
					if ( lastBreakFile != fileName || lastBreakLine != ar->currentline )
					{
						breakStack = getCurStackDeep( );
						DebugBreak( fileName, ar->currentline );
						lastBreakFile = fileName;
						lastBreakLine = ar->currentline;
					}
					break;
				}
				else if ( luaDebugStatus == EDebugStatus::DEBUG_STEP )
				{
					// 如果是单步跟踪
					int curStackDeep = getCurStackDeep( );
					if ( curStackDeep <= breakStack )
					{
						// 如果当前栈深度小于上次断点中断时的深度，那么可以中断
						lua_getinfo( L, "nSl", ar );

						static CNSString fileName;
						fileName = ar->source;
						fileName.replace( "\\", "/" );

						if ( lastBreakFile != fileName || lastBreakLine != ar->currentline )
						{
							breakStack = curStackDeep;
							DebugBreak( fileName, ar->currentline );
							lastBreakFile = fileName;
							lastBreakLine = ar->currentline;
						}
						break;
					}
				}

				// 单步跟踪可能会跳过很多代码，这些代码也需要检查断点，只有步入跟踪不用再次检查断点
				if ( luaBreaks.getCount( ) > 0 )
				{
					lua_getinfo( L, "nSl", ar );
					static CNSString srcType;
					srcType = ar->what;

					static CNSString fileName;
					fileName = ar->source;
					fileName.replace( "\\", "/" );

					if ( luaBreaks.find( fileName + ":" + CNSString::number2String( ar->currentline ) ) == true && srcType == "Lua" )
					{
						breakStack = getCurStackDeep( );
						DebugBreak( fileName, ar->currentline );
						lastBreakFile = fileName;
						lastBreakLine = ar->currentline;
					}
				}
				break;
			}
			default:
				break;
			}
		}
		catch ( CNSException& e )
		{
			NSLog::exception( _UTF8( "lua调试过程发生异常\n错误描述: \n\t%s\nC++调用堆栈:\n%s" ), e.mErrorDesc, NSBase::NSFunction::getStackInfo( ).getBuffer( ) );
		}
	}

	void onDebugStep( int flag, int keycode, NSWin32::CNSWindow* window )
	{
		if ( inBreak == false )
			return;

		// 清除当前断点
		if ( curLuaFile == curIntrputFile )
		{
			NSWin32::CNSVsEdit* luaEditor = ( NSWin32::CNSVsEdit* ) NSWin32::CNSWindow::getWindow( "consoleEditor" );
			if ( luaEditor != NULL )
				luaEditor->hideMargin( curIntrputLine, EMarkerType::MARKER_INTERRUPT );
		}

		curIntrputLine = -1;
		curIntrputFile.clear( );

		// 单步跟踪
		PostQuitMessage( 0 );
		luaDebugStatus = EDebugStatus::DEBUG_STEP;
	}

	void onDebugContinue( int flag, int keycode, NSWin32::CNSWindow* window )
	{
		if ( inBreak == false )
			return;

		// 清除当前断点
		if ( curLuaFile == curIntrputFile )
		{
			NSWin32::CNSVsEdit* luaEditor = ( NSWin32::CNSVsEdit* ) NSWin32::CNSWindow::getWindow( "consoleEditor" );
			if ( luaEditor != NULL )
				luaEditor->hideMargin( curIntrputLine, EMarkerType::MARKER_INTERRUPT );
		}
		curIntrputLine = -1;
		curIntrputFile.clear( );

		// 继续
		PostQuitMessage( 0 );
		luaDebugStatus = EDebugStatus::DEBUG_NONE;
	}

	void onDebugInto( int flag, int keycode, NSWin32::CNSWindow* window )
	{
		if ( inBreak == false )
			return;

		// 清除当前断点
		if ( curLuaFile == curIntrputFile )
		{
			NSWin32::CNSVsEdit* luaEditor = ( NSWin32::CNSVsEdit* ) NSWin32::CNSWindow::getWindow( "consoleEditor" );
			if ( luaEditor != NULL )
				luaEditor->hideMargin( curIntrputLine, EMarkerType::MARKER_INTERRUPT );
		}
		curIntrputLine = -1;
		curIntrputFile.clear( );

		// 步入跟踪
		PostQuitMessage( 0 );
		luaDebugStatus = EDebugStatus::DEBUG_INTO;
	}

	bool onConsoleListStackClicked( NSWin32::CNSWindow* child, WPARAM wParam, LPARAM lParam, LRESULT* result )
	{
		NSWin32::CNSVsList* listStack = ( NSWin32::CNSVsList* ) child;
		NSWin32::CNSVsTab* tabCtrl = ( NSWin32::CNSVsTab* ) NSWin32::CNSWindow::getWindow( "consoleTab" );
		NSWin32::CNSVsTree* treeCtrl = ( NSWin32::CNSVsTree* ) NSWin32::CNSWindow::getWindow( "consoleTree" );
		NSWin32::CNSVsEdit* editorCtrl = ( NSWin32::CNSVsEdit* ) NSWin32::CNSWindow::getWindow( "consoleEditor" );

		CNSVector< int > curSel = listStack->getCurSel( );
		if ( curSel.getCount( ) == 0 )
			return true;

		int	level = curSel[ 0 ];
		CNSString fileName = listStack->getItemText( 1, level );
		CNSString line = listStack->getItemText( 3, level );
		if ( line == "-1" )
			return true;

		if ( tabCtrl->getCurSel( ) != PAGE_DEBUG )
			tabCtrl->setCurSel( PAGE_DEBUG );

		HTREEITEM item = treeCtrl->findItem( fileName );
		if ( item != NULL )
			treeCtrl->setCurSel( item );

		CNSString* textRef = (CNSString*) treeCtrl->getItemData( item );
		if ( textRef != NULL )
			showFile( fileName, *textRef );

		if ( NSWin32::CNSWindow::getFocus( ) != editorCtrl )
			editorCtrl->focus( );

		// 让中断行号在屏幕内显示出来
		editorCtrl->visibleLine( line.toInteger( ) );
		editorCtrl->gotoLine( line.toInteger( ) );
		return true;
	}

	void openSelectFile( )
	{
		NSWin32::CNSVsList*	fileList = ( NSWin32::CNSVsList* ) NSWin32::CNSWindow::getWindow( "consoleFileList" );
		NSWin32::CNSVsTree*	treeCtrl = ( NSWin32::CNSVsTree* ) NSWin32::CNSWindow::getWindow( "consoleTree" );
		if ( treeCtrl == NULL )
			return;

		CNSVector< int >& curSel = fileList->getCurSel( );
		if ( curSel.getCount( ) > 0 )
		{
			CNSString	filePath = fileList->getItemText( 1, curSel[ 0 ] );
			HTREEITEM	item = treeCtrl->findItem( filePath );
			if ( item != NULL )
				treeCtrl->setCurSel( item );

			CNSString* textRef = (CNSString*) treeCtrl->getItemData( item );
			if ( textRef != NULL )
				showFile( filePath, *textRef );

			NSWin32::CNSFrame* fileFrame = ( NSWin32::CNSFrame* ) NSWin32::CNSWindow::getWindow( "consoleFileListFrame" );
			if ( fileFrame != NULL )
				fileFrame->destroy( );
		}
	}

	bool onFileListSearchInputKeydown( NSWin32::CNSWindow* child, WPARAM wParam, LPARAM lParam, LRESULT* result )
	{
		NSWin32::CNSEdit*	input = ( NSWin32::CNSEdit* ) child;
		NSWin32::CNSVsList* fileList = ( NSWin32::CNSVsList* ) NSWin32::CNSWindow::getWindow( "consoleFileList" );
		if ( wParam == VK_UP || wParam == VK_NEXT || wParam == VK_PRIOR || wParam == VK_DOWN )
		{
			SendMessage( fileList->getHWnd( ), WM_KEYDOWN, wParam, lParam );
			return true;
		}
		if ( wParam == VK_RETURN )
		{
			openSelectFile( );
			return true;
		}

		return false;
	}

	bool onFileListSearchInputKillFocus( NSWin32::CNSWindow* child, WPARAM wParam, LPARAM lParam, LRESULT* result )
	{
		NSWin32::CNSEdit*	input = ( NSWin32::CNSEdit* ) child;
		HWND		newHwnd = (HWND) wParam;
		NSWin32::CNSWindow*	searchBtn = NSWin32::CNSWindow::getWindow( "consoleFileListSearchButton" );
		if ( searchBtn->getHWnd( ) != newHwnd )
		{
			input->focus( );

			// 返回true，阻止默认处理(保持绘制光标)，默认处理将取消光标绘制
			return true;
		}

		// 如果不需要重新获得焦点，那么需要继续默认处理
		return false;
	}

	bool onFileListSearchInputChanged( NSWin32::CNSWindow* child, WPARAM wParam, LPARAM lParam, LRESULT* result )
	{
		NSWin32::CNSEdit* input = ( NSWin32::CNSEdit* ) child;
		CNSString& text = input->getText( );
		NSWin32::CNSVsList* fileList = ( NSWin32::CNSVsList* ) NSWin32::CNSWindow::getWindow( "consoleFileList" );
		if ( fileList == NULL )
			return true;

		fileList->clear( );
		HLISTINDEX beginIndex = luaFiles.getHead( );
		for ( ; beginIndex != NULL; luaFiles.getNext( beginIndex ) )
		{
			CNSString& filePath = luaFiles.getKey( beginIndex );
			CNSString& fileName = filePath.getFileName( '/' );
			if ( fileName.nocaseFindFirstOf( text ) != -1 )
			{
				int index = fileList->addItem( fileName );
				fileList->setItem( 1, index, filePath );
			}
		}
		fileList->setCurSel( 0 );
		return true;
	}

	bool onFileListDblClicked( NSWin32::CNSWindow* child, WPARAM wParam, LPARAM lParam, LRESULT* result )
	{
		openSelectFile( );
		return true;
	}

	bool onFileCommitClicked( NSWin32::CNSWindow* child, WPARAM wParam, LPARAM lParam, LRESULT* result )
	{
		openSelectFile( );
		return true;
	}

	void onDebugOpenFileList( int flag, int keycode )
	{
		NSWin32::CNSFrame* fileFrame = ( NSWin32::CNSFrame* ) NSWin32::CNSWindow::getWindow( "consoleFileListFrame" );
		if ( fileFrame != NULL )
		{
			fileFrame->destroy( );
			return;
		}

		RECT rcFileListFrame;
		rcFileListFrame.left = 0;
		rcFileListFrame.right = 600;
		rcFileListFrame.top = 0;
		rcFileListFrame.bottom = 305;
		fileFrame = ( NSWin32::CNSFrame* ) NSWin32::CNSWindow::newFrame( "consoleFileListFrame", NSWin32::CNSFrame::EFrameType::STYLE_POPUP, rcFileListFrame, NULL, true );
		fileFrame->setText( _UTF8( "文件查找" ) );
		fileFrame->show( );

		RECT rcFileList;
		rcFileList.left = 2;
		rcFileList.right = 598;
		rcFileList.top = 2;
		rcFileList.bottom = 250;
		NSWin32::CNSVsList* fileList = NSWin32::CNSWindow::newVsList( "consoleFileList", 0, rcFileList, fileFrame );
		fileList->addColumn( _UTF8( "文件名" ), 200 );
		fileList->addColumn( _UTF8( "文件路径" ), 396 );
		fileList->registerEvent( NSWin32::CNSVsList::EListCtrlEvent::EventDblClicked, onFileListDblClicked );

		fileList->setRedraw( false );
		HLISTINDEX beginIndex = luaFiles.getHead( );
		for ( ; beginIndex != NULL; luaFiles.getNext( beginIndex ) )
		{
			CNSString& filePath = luaFiles.getKey( beginIndex );
			int index = fileList->addItem( filePath.getFileName( '/' ) );
			fileList->setItem( 1, index, filePath );
		}
		fileList->setCurSel( 0 );
		fileList->setRedraw( true );

		RECT rcSearch;
		rcSearch.left = 3;
		rcSearch.right = 500;
		rcSearch.top = 257;
		rcSearch.bottom = rcSearch.top + 18;
		NSWin32::CNSEdit* fileSearch = NSWin32::CNSWindow::newEdit( "consoleFileListSearchEdit",
			NSWin32::CNSEdit::EEditCtrlType::EDIT_SINGLETEXT, rcSearch, fileFrame );
		fileSearch->mTopAnchor = NSWin32::CNSWindow::ANCHOR_BOTTOM;
		fileSearch->registerEvent( NSWin32::CNSEdit::EventChanged, onFileListSearchInputChanged );
		fileSearch->registerMessage( WM_KILLFOCUS, onFileListSearchInputKillFocus );
		fileSearch->registerMessage( WM_KEYDOWN, onFileListSearchInputKeydown );
		fileSearch->focus( );

		RECT rcCommit;
		rcCommit.left = 505;
		rcCommit.right = 595;
		rcCommit.top = 257;
		rcCommit.bottom = rcCommit.top + 18;
		NSWin32::CNSVsBtn* fileCommit = NSWin32::CNSWindow::newVsBtn( "consoleFileListSearchButton",
			NSWin32::CNSVsBtn::EVsButtonStyle::STYLE_PUSHBUTTON | NSWin32::CNSVsBtn::EVsButtonStyle::STYLE_TEXT, rcCommit, fileFrame );
		fileCommit->setText( _UTF8( "确定" ) );
		fileCommit->mTopAnchor = NSWin32::CNSWindow::ANCHOR_BOTTOM;
		fileCommit->mLeftAnchor = NSWin32::CNSWindow::ANCHOR_RIGHT;
		fileCommit->registerEvent( NSWin32::CNSVsBtn::EventClicked, onFileCommitClicked );
	}

	void onDebugOpenStack( int flag, int keycode )
	{
		POINT pt;
		pt.x = 10;
		pt.y = 10;
		ClientToScreen( console->getHWnd( ), &pt );

		RECT rcStack;
		rcStack.left = pt.x;
		rcStack.right = pt.x + 600;
		rcStack.top = pt.y;
		rcStack.bottom = pt.y + 300;
		NSWin32::CNSFrame* stackFrame = ( NSWin32::CNSFrame* ) NSWin32::CNSWindow::getWindow( "consoleListStackFrame" );
		if ( stackFrame != NULL )
		{
			stackFrame->destroy( );
			return;
		}

		stackFrame = ( NSWin32::CNSFrame* ) NSWin32::CNSWindow::newFrame( "consoleListStackFrame", NSWin32::CNSFrame::EFrameType::STYLE_POPUP, rcStack, console, false );
		stackFrame->setText( _UTF8( "调用堆栈" ) );
		stackFrame->show( );

		RECT rcStackList;
		rcStackList.left = 0;
		rcStackList.right = 600;
		rcStackList.top = 0;
		rcStackList.bottom = 300;
		NSWin32::CNSVsList* list = NSWin32::CNSWindow::newVsList( "consoleListStack", 0, rcStackList, stackFrame );
		list->addColumn( _UTF8( "层级" ), 50 );
		list->addColumn( _UTF8( "文件" ), 250 );
		list->addColumn( _UTF8( "函数" ), 250 );
		list->addColumn( _UTF8( "行号" ), 50 );
		list->registerEvent( NSWin32::CNSVsList::EListCtrlEvent::EventClicked, onConsoleListStackClicked );
		refreshListStack( list );
	}

	void searchHelper( )
	{
		NSWin32::CNSVsEdit* editorCtrl = ( NSWin32::CNSVsEdit* ) NSWin32::CNSWindow::getWindow( "consoleEditor" );
		if ( editorCtrl == NULL )
			return;

		int findStart = editorCtrl->getCaret( );
		if ( lastFindStart == findStart )
		{
			findStart = 0;
			lastFindStart = -1;
		}

		int	findPos = editorCtrl->search( searchText, findStart, -1, searchCase, searchWord );
		if ( findPos == -1 )
		{
			NSWin32::CNSFrame* frame = ( NSWin32::CNSFrame* ) NSWin32::CNSWindow::getWindow( "consoleFrameSearch" );
			if ( frame != NULL )
				frame->enable( false );

			MessageBox( console->getHWnd( ), _T( "文档搜索完成" ), _T( "系统提示" ), MB_OK | MB_ICONINFORMATION );
			lastFindStart = findStart;
			if ( frame != NULL )
				frame->enable( true );

			NSWin32::CNSEdit*		input = ( NSWin32::CNSEdit* ) NSWin32::CNSWindow::getWindow( "searchInput" );
			if ( input != NULL )
				input->focus( );
			return;
		}

		editorCtrl->select( findPos, findPos + searchText.getLength( ) );
	}

	bool onSearchInputChanged( NSWin32::CNSWindow* child, WPARAM wParam, LPARAM lParam, LRESULT* result )
	{
		NSWin32::CNSEdit* input = ( NSWin32::CNSEdit* ) child;
		searchText = input->getText( );
		return true;
	}

	bool onGotoInputChanged( NSWin32::CNSWindow* child, WPARAM wParam, LPARAM lParam, LRESULT* result )
	{
		NSWin32::CNSVsEdit* editorCtrl = ( NSWin32::CNSVsEdit* ) NSWin32::CNSWindow::getWindow( "consoleEditor" );
		if ( editorCtrl == NULL )
			return true;

		NSWin32::CNSEdit* input = ( NSWin32::CNSEdit* ) child;
		CNSString& text = input->getText( );
		if ( text.toInteger( ) > editorCtrl->getLineCount( ) )
		{
			MessageBox( console->getHWnd( ), _T( "超出文档范围" ), _T( "系统提示" ), MB_OK | MB_ICONINFORMATION );
			input->focus( );
			return true;
		}

		editorCtrl->visibleLine( text.toInteger( ) );
		editorCtrl->gotoLine( text.toInteger( ) );
		if ( oldTipLine != -1 )
			editorCtrl->hideMargin( oldTipLine, EMarkerType::MARKER_LINETIP );

		editorCtrl->showMargin( text.toInteger( ), EMarkerType::MARKER_LINETIP );
		oldTipLine = text.toInteger( );
		return true;
	}

	bool onClickSearchBtn( NSWin32::CNSWindow* child, WPARAM wParam, LPARAM lParam, LRESULT* result )
	{
		searchHelper( );
		return true;
	}

	bool onClickSearchBtnCase( NSWin32::CNSWindow* child, WPARAM wParam, LPARAM lParam, LRESULT* result )
	{
		NSWin32::CNSVsBtn* btnCase = ( NSWin32::CNSVsBtn* ) child;
		searchCase = !searchCase;
		btnCase->setCheck( searchCase );
		return true;
	}

	bool onClickSearchBtnWord( NSWin32::CNSWindow* child, WPARAM wParam, LPARAM lParam, LRESULT* result )
	{
		NSWin32::CNSVsBtn* btnWord = ( NSWin32::CNSVsBtn* ) child;
		searchWord = !searchWord;
		btnWord->setCheck( searchWord );
		return true;
	}

	void onDebugSearchNext( int flag, int keycode )
	{
		searchHelper( );
	}

	void onFrameSearchReturn( int flag, int keycode, NSWin32::CNSWindow* window )
	{
		searchHelper( );
	}

	void onConsoleEscape( int flag, int keycode )
	{
		NSWin32::CNSFrame*		frameSearch = ( NSWin32::CNSFrame* ) NSWin32::CNSWindow::getWindow( "consoleFrameSearch" );
		if ( frameSearch != NULL )
		{
			frameSearch->destroy( );
			return;
		}

		NSWin32::CNSFrame*		frameGoto = ( NSWin32::CNSFrame* ) NSWin32::CNSWindow::getWindow( "consoleFrameGoto" );
		if ( frameGoto != NULL )
		{
			NSWin32::CNSVsEdit* editorCtrl = ( NSWin32::CNSVsEdit* ) NSWin32::CNSWindow::getWindow( "consoleEditor" );
			if ( editorCtrl != NULL && oldTipLine != -1 )
			{
				editorCtrl->hideMargin( oldTipLine, EMarkerType::MARKER_LINETIP );
				oldTipLine = -1;
			}

			frameGoto->destroy( );
			return;
		}

		NSWin32::CNSFrame*	stackFrame = ( NSWin32::CNSFrame* ) NSWin32::CNSWindow::getWindow( "consoleListStackFrame" );
		if ( stackFrame != NULL )
		{
			stackFrame->destroy( );
			return;
		}

		NSWin32::CNSFrame*		fileFrame = ( NSWin32::CNSFrame* ) NSWin32::CNSWindow::getWindow( "consoleFileListFrame" );
		if ( fileFrame != NULL )
		{
			fileFrame->destroy( );
			return;
		}

		if ( console != NULL )
			console->close( );
	}

	bool onSearchInputStateDestroy( NSWin32::CNSWindow* child, WPARAM wParam, LPARAM lParam, LRESULT* result )
	{
		pos = 0;
		lastTick = 0;
		return false;
	}

	bool onFrameSearchActive( NSWin32::CNSWindow* child, WPARAM wParam, LPARAM lParam, LRESULT* result )
	{
		RECT rc;
		GetClientRect( child->getHWnd( ), &rc );
		rc.top = rc.bottom - 5;
		InvalidateRect( child->getHWnd( ), &rc, FALSE );
		return false;
	}

	bool onSearchInputStateTimer( NSWin32::CNSWindow* child, WPARAM wParam, LPARAM lParam, LRESULT* result )
	{
		int timerID = (int) wParam;
		if ( timerID == 100 )
		{
			unsigned int		thisTick = CNSTimer::getCurTick( );
			if ( lastTick == 0 )
				lastTick = thisTick;

			unsigned int		tickOffset = thisTick - lastTick;
			lastTick = thisTick;
			RECT rc;
			GetClientRect( child->getHWnd( ), &rc );
			if ( searchFrameActive == true )
			{
				pos = pos + ( tickOffset / 1000.0f ) * 100.0f;
				if ( pos >= rc.right )
					pos = -70;
			}

			InvalidateRect( child->getHWnd( ), NULL, FALSE );
		}

		return false;
	}

	bool onSearchInputStateRender( NSWin32::CNSWindow* child, WPARAM wParam, LPARAM lParam, LRESULT* result )
	{
		NSWin32::CNSWindow* active = NSWin32::CNSWindow::getActive( );
		HBRUSH brForeActive = CreateSolidBrush( RGB( 0, 120, 215 ) );
		HBRUSH brBack = CreateSolidBrush( RGB( 60, 60, 73 ) );
		HBRUSH brForeInActive = CreateSolidBrush( RGB( 78, 78, 87 ) );
		PAINTSTRUCT ps;
		HDC dc = BeginPaint( child->getHWnd( ), &ps );
		RECT rc;
		GetClientRect( child->getHWnd( ), &rc );

		int bottom = rc.bottom;
		int right = rc.right;
		int top = rc.top;
		int left = rc.left;

		if ( memDC == NULL )
		{
			memDC = CreateCompatibleDC( dc );
			memBmp = CreateCompatibleBitmap( dc, right - left, bottom - top );
		}

		HBITMAP oldBmp = (HBITMAP) SelectObject( memDC, memBmp );
		FillRect( memDC, &rc, brBack );

		rc.left = (int) max( left, pos );
		rc.right = (int) pos + min( 70, right - (int) max( left, pos ) );
		if ( searchFrameActive == true )
			FillRect( memDC, &rc, brForeActive );
		else
			FillRect( memDC, &rc, brForeInActive );

		BitBlt( dc, left, top, right - left, bottom - top, memDC, 0, 0, SRCCOPY );
		SelectObject( memDC, oldBmp );
		DeleteObject( brForeActive );
		DeleteObject( brForeInActive );
		DeleteObject( brBack );
		EndPaint( child->getHWnd( ), &ps );
		return false;
	}

	bool onFrameSearchRender( NSWin32::CNSWindow* child, WPARAM wParam, LPARAM lParam, LRESULT* result )
	{
		NSWin32::CNSWindow* active = NSWin32::CNSWindow::getActive( );
		HBRUSH br = NULL;
		if ( active == child )
		{
			searchFrameActive = true;
			br = CreateSolidBrush( RGB( 0, 120, 215 ) );
		}
		else
		{
			searchFrameActive = false;
			br = CreateSolidBrush( RGB( 60, 60, 73 ) );
		}

		PAINTSTRUCT ps;
		BeginPaint( child->getHWnd( ), &ps );

		RECT rc;
		GetClientRect( child->getHWnd( ), &rc );

		rc.top = rc.bottom - 5;
		FillRect( ps.hdc, &rc, br );
		EndPaint( child->getHWnd( ), &ps );
		DeleteObject( br );
		return false;
	}

	void onDebugOpenGotoLine( int flag, int keycode )
	{
		NSWin32::CNSFrame*		frameSearch = ( NSWin32::CNSFrame* ) NSWin32::CNSWindow::getWindow( "consoleFrameSearch" );
		if ( frameSearch != NULL )
			frameSearch->destroy( );

		NSWin32::CNSFrame*		frame = ( NSWin32::CNSFrame* ) NSWin32::CNSWindow::getWindow( "consoleFrameGoto" );
		if ( frame != NULL )
		{
			NSWin32::CNSWindow* input = NSWin32::CNSWindow::getWindow( "searchInput" );
			if ( input != NULL )
				input->focus( );

			frame->active( );
			return;
		}

		NSWin32::CNSVsTab* tab = ( NSWin32::CNSVsTab* ) NSWin32::CNSWindow::getWindow( "consoleTab" );
		if ( tab == NULL )
			return;

		NSWin32::CNSVsEdit* editorCtrl = ( NSWin32::CNSVsEdit* ) NSWin32::CNSWindow::getWindow( "consoleEditor" );
		if ( editorCtrl == NULL )
			return;

		if ( editorCtrl->isVisible( ) == false )
			return;

		RECT rcEditor;
		editorCtrl->getClientRect( rcEditor );

		POINT pt;
		pt.x = ( ( rcEditor.right - rcEditor.left ) - 300 ) / 2;
		pt.y = rcEditor.top;
		ClientToScreen( editorCtrl->getHWnd( ), &pt );

		RECT rcSearch;
		rcSearch.left = pt.x;
		rcSearch.right = pt.x + 300;
		rcSearch.top = pt.y;
		rcSearch.bottom = pt.y + 33;
		frame = ( NSWin32::CNSFrame* ) NSWin32::CNSWindow::newFrame( "consoleFrameGoto", NSWin32::CNSFrame::EFrameType::STYLE_MINIPOPUP, rcSearch, tab, false );
		frame->registerMessage( WM_PAINT, onFrameSearchRender );
		frame->registerMessage( WM_ACTIVATE, onFrameSearchActive );
		frame->mLeftAnchor = NSWin32::CNSWindow::ANCHOR_RIGHT;
		frame->mBottomAnchor = NSWin32::CNSWindow::ANCHOR_TOP;
		frame->show( );

		RECT rcInput;
		rcInput.left = 5;
		rcInput.right = 300 - 5;
		rcInput.top = 6;
		rcInput.bottom = 24;
		NSWin32::CNSEdit* input = ( NSWin32::CNSEdit* ) NSWin32::CNSWindow::newEdit( "gotoInput", NSWin32::CNSEdit::EEditCtrlType::EDIT_NUMBER, rcInput, frame );
		input->mRightAnchor = NSWin32::CNSWindow::ANCHOR_LEFT;
		input->mBottomAnchor = NSWin32::CNSWindow::ANCHOR_TOP;
		input->setText( searchText );
		input->setSel( );
		input->focus( );
		input->registerEvent( NSWin32::CNSEdit::EventChanged, onGotoInputChanged );
	}

	void onDebugOpenSearch( int flag, int keycode )
	{
		NSWin32::CNSFrame*		frameGoto = ( NSWin32::CNSFrame* ) NSWin32::CNSWindow::getWindow( "consoleFrameGoto" );
		if ( frameGoto != NULL )
			frameGoto->destroy( );

		NSWin32::CNSFrame*		frame = ( NSWin32::CNSFrame* ) NSWin32::CNSWindow::getWindow( "consoleFrameSearch" );
		if ( frame != NULL )
		{
			NSWin32::CNSWindow* input = NSWin32::CNSWindow::getWindow( "searchInput" );
			if ( input != NULL )
				input->focus( );

			frame->active( );
			return;
		}

		NSWin32::CNSVsTab* tab = ( NSWin32::CNSVsTab* ) NSWin32::CNSWindow::getWindow( "consoleTab" );
		if ( tab == NULL )
			return;

		NSWin32::CNSVsEdit* editorCtrl = ( NSWin32::CNSVsEdit* ) NSWin32::CNSWindow::getWindow( "consoleEditor" );
		if ( editorCtrl == NULL )
			return;

		if ( editorCtrl->isVisible( ) == false )
			return;

		RECT rcEditor;
		editorCtrl->getClientRect( rcEditor );

		POINT pt;
		pt.x = rcEditor.right - 300;
		pt.y = rcEditor.top;
		ClientToScreen( editorCtrl->getHWnd( ), &pt );

		RECT rcSearch;
		rcSearch.left = pt.x;
		rcSearch.right = pt.x + 300;
		rcSearch.top = pt.y;
		rcSearch.bottom = pt.y + 60;

		frame = ( NSWin32::CNSFrame* ) NSWin32::CNSWindow::newFrame( "consoleFrameSearch", NSWin32::CNSFrame::EFrameType::STYLE_MINIPOPUP, rcSearch, tab, false );
		frame->registerLocalHotkey( NSWin32::HOTKEY_NONE, NSWin32::KEYCODE_RETURN, onFrameSearchReturn );
		frame->registerMessage( WM_PAINT, onFrameSearchRender );
		frame->registerMessage( WM_ACTIVATE, onFrameSearchActive );
		frame->show( );

		RECT rcInput;
		rcInput.left = 5;
		rcInput.right = 300 - 80;
		rcInput.top = 6;
		rcInput.bottom = 24;
		NSWin32::CNSEdit* input = ( NSWin32::CNSEdit* ) NSWin32::CNSWindow::newEdit( "searchInput", NSWin32::CNSEdit::EEditCtrlType::EDIT_SINGLETEXT, rcInput, frame );
		input->mRightAnchor = NSWin32::CNSWindow::ANCHOR_LEFT;
		input->mBottomAnchor = NSWin32::CNSWindow::ANCHOR_TOP;

		input->setText( searchText );
		input->setSel( );
		input->focus( );
		input->registerEvent( NSWin32::CNSEdit::EventChanged, onSearchInputChanged );

		RECT rcInputState;
		rcInputState.left = 5;
		rcInputState.right = 300 - 80;
		rcInputState.top = 24;
		rcInputState.bottom = 26;
		NSWin32::CNSCustom* inputState = ( NSWin32::CNSCustom* ) NSWin32::CNSWindow::newCustom( "searchInputState", WS_VISIBLE | WS_CHILD, 0, rcInputState, frame );
		inputState->registerMessage( WM_PAINT, onSearchInputStateRender );
		inputState->registerMessage( WM_TIMER, onSearchInputStateTimer );
		inputState->registerMessage( WM_NCDESTROY, onSearchInputStateDestroy );
		inputState->setTimer( 100, 15 );

		RECT rcBtn;
		rcBtn.left = 300 - 75;
		rcBtn.right = rcBtn.left + 70;
		rcBtn.top = 6;
		rcBtn.bottom = rcBtn.top + 20;
		NSWin32::CNSVsBtn* btnCommit = ( NSWin32::CNSVsBtn* ) NSWin32::CNSWindow::newVsBtn( "searchCommit",
			NSWin32::CNSVsBtn::EVsButtonStyle::STYLE_PUSHBUTTON | NSWin32::CNSVsBtn::EVsButtonStyle::STYLE_TEXT, rcBtn, frame );
		btnCommit->mRightAnchor = NSWin32::CNSWindow::ANCHOR_LEFT;
		btnCommit->mBottomAnchor = NSWin32::CNSWindow::ANCHOR_TOP;
		btnCommit->setText( _UTF8( "走你" ) );
		btnCommit->registerEvent( NSWin32::CNSVsBtn::EventClicked, onClickSearchBtn );

		RECT rcCheck1;
		rcCheck1.left = 10;
		rcCheck1.right = rcCheck1.left + 80;
		rcCheck1.top = 30;
		rcCheck1.bottom = rcCheck1.top + 20;
		NSWin32::CNSVsBtn* btnCheck1 = ( NSWin32::CNSVsBtn* ) NSWin32::CNSWindow::newVsBtn( "searchCase",
			NSWin32::CNSVsBtn::EVsButtonStyle::STYLE_CHECK | NSWin32::CNSVsBtn::EVsButtonStyle::STYLE_TEXT, rcCheck1, frame );
		btnCheck1->mRightAnchor = NSWin32::CNSWindow::ANCHOR_LEFT;
		btnCheck1->mBottomAnchor = NSWin32::CNSWindow::ANCHOR_TOP;
		btnCheck1->setText( _UTF8( "区分大小写" ) );
		btnCheck1->registerEvent( NSWin32::CNSVsBtn::EventClicked, onClickSearchBtnCase );
		btnCheck1->setCheck( searchCase );

		RECT rcCheck2;
		rcCheck2.left = 100;
		rcCheck2.right = rcCheck2.left + 80;
		rcCheck2.top = 30;
		rcCheck2.bottom = rcCheck2.top + 20;
		NSWin32::CNSVsBtn* btnCheck2 = ( NSWin32::CNSVsBtn* ) NSWin32::CNSWindow::newVsBtn( "searchWord",
			NSWin32::CNSVsBtn::EVsButtonStyle::STYLE_CHECK | NSWin32::CNSVsBtn::EVsButtonStyle::STYLE_TEXT, rcCheck2, frame );
		btnCheck2->mRightAnchor = NSWin32::CNSWindow::ANCHOR_LEFT;
		btnCheck2->mBottomAnchor = NSWin32::CNSWindow::ANCHOR_TOP;
		btnCheck2->setText( _UTF8( "全字匹配" ) );
		btnCheck2->registerEvent( NSWin32::CNSVsBtn::EventClicked, onClickSearchBtnWord );
		btnCheck2->setCheck( searchWord );
	}

	void setConsoleTitle( const CNSString& title )
	{
		mConsoleTitle = title;
	}

	void enableDebug( bool enable )
	{
		NSBase::CNSLuaStack& luaStack = NSBase::CNSLuaStack::getLuaStack( );
		if ( enable == true )
			lua_sethook( luaStack.getLuaState( ), luaDebugCallback, LUA_MASKLINE | LUA_MASKCALL | LUA_MASKRET, 0 );
		else
			lua_sethook( luaStack.getLuaState( ), NULL, 0, 0 );
	}

	void addLuaFile( const CNSString& fileName, const CNSString& buffer )
	{
		CNSString name = fileName;
		name.replace( "\\", "/" );
		CNSString& luaText = luaFiles.insert( name, buffer );
		if ( name == curLuaFile )
		{
			// 如果当前打开的文件被修改了
			curLuaFile.clear( );
			showFile( name, buffer );
		}

		NSWin32::CNSVsTree* treeCtrl = ( NSWin32::CNSVsTree* ) NSWin32::CNSWindow::getWindow( "consoleTree" );
		if ( treeCtrl != NULL )
		{
			HTREEITEM item = treeCtrl->findItem( name );
			if ( item != NULL )
			{
				treeCtrl->setItemData( item, &luaText );
				return;
			}

			static CNSVector< CNSString > pathList( 16 );
			pathList.clear( );
			name.split( "/", pathList );
			HTREEITEM parentItem = TVI_ROOT;
			CNSString parentKey;
			for ( size_t i = 0; i < pathList.getCount( ); i ++ )
			{
				parentKey = parentKey + pathList[ i ];
				HTREEITEM item = treeCtrl->findItem( parentKey );
				if ( item == NULL )
				{
					for ( size_t t = i; t < pathList.getCount( ); t ++ )
					{
						if ( t == pathList.getCount( ) - 1 )
							parentItem = treeCtrl->addItem( parentItem, TVI_SORT, pathList[ t ], &luaText );
						else
							parentItem = treeCtrl->addItem( parentItem, TVI_LAST, pathList[ t ], NULL );
					}
					break;
				}

				parentItem = item;
				parentKey += "/";
			}
		}
	}

	void interrupt( )
	{
		if ( curLuaFile == curIntrputFile )
		{
			// 清除当前断点
			NSWin32::CNSVsEdit* luaEditor = ( NSWin32::CNSVsEdit* ) NSWin32::CNSWindow::getWindow( "consoleEditor" );
			if ( luaEditor != NULL )
				luaEditor->hideMargin( curIntrputLine, EMarkerType::MARKER_INTERRUPT );
		}

		curIntrputLine = -1;
		curIntrputFile.clear( );
		breakStack = getCurStackDeep( );
		luaDebugStatus = EDebugStatus::DEBUG_STEP;
	}

	void exit( )
	{
		if ( console != NULL )
			console->destroy( );

		baseApp = NULL;
		PAGE_LOG = -1;
		PAGE_EXCEPTION = -1;
		PAGE_DEBUG = -1;
		searchFrameActive = false;
		pos = 0;
		lastTick = 0;
		memDC = NULL;
		memBmp = NULL;
		luaBreaks.clear( );
		console = NULL;
		mConsoleTitle = _UTF8( "NoSN - Lua调试器" );
		curLuaFile.clear( );
		breakStack = 0;
		searchWord = false;
		searchCase = false;
		blockFinding = false;
		lastFindStart = -1;
		oldTipLine = -1;
		searchText.clear( );
		wndHost = NULL;
		luaDebugStatus = EDebugStatus::DEBUG_NONE;
		openFileIndex.clear( );
		openFileList.clear( );
		openFileID = 1;
		curIntrputFile.clear( );
		curIntrputLine = -1;
		inBreak = false;
		luaFiles.clear( );
	}

	void consoleLogHandler( const NSBase::CNSString& log )
	{
		NSWin32::CNSVsTab* tabCtrl = ( NSWin32::CNSVsTab* ) NSWin32::CNSWindow::getWindow( "consoleTab" );
		NSWin32::CNSVsEdit* logEdit = ( NSWin32::CNSVsEdit* ) NSWin32::CNSWindow::getWindow( "consoleEdit" );
		if ( tabCtrl != NULL && logEdit != NULL && tabCtrl->getCurSel( ) == PAGE_LOG )
		{
			logEdit->append( log );

			if ( logEdit->getTextLength( ) >= 65535 )
				logEdit->clear( );
		}
	}

	void consoleExceptionHandler( const NSBase::CNSString& exception )
	{
		NSWin32::CNSVsTab* tabCtrl = ( NSWin32::CNSVsTab* ) NSWin32::CNSWindow::getWindow( "consoleTab" );
		NSWin32::CNSVsEdit* logEdit = ( NSWin32::CNSVsEdit* ) NSWin32::CNSWindow::getWindow( "consoleEdit" );
		if ( tabCtrl != NULL && logEdit != NULL && tabCtrl->getCurSel( ) == PAGE_EXCEPTION )
		{
			logEdit->append( exception );
			if ( logEdit->getTextLength( ) >= 65535 )
				logEdit->clear( );
		}

		if ( wndHost != NULL )
		{
			FlashWindow( wndHost, TRUE );
			static CNSString windowName;
			if ( windowName.isEmpty( ) == true )
				windowName = NSWin32::CNSWindow::getWindowText( wndHost );

			NSWin32::CNSWindow::setWindowText( wndHost, windowName + _UTF8( " - 有异常" ) );
		}
	}

	void init( )
	{
		baseApp = NSWin32::CNSBaseApp::getApp( );
		if ( baseApp == NULL )
			NSException( _UTF8( "没有创建App对象" ) );

		// 如果console已经打开，那么先销毁，保证console的界面重新构造
		NSWin32::CNSVsTree* treeCtrl = ( NSWin32::CNSVsTree* ) NSWin32::CNSWindow::getWindow( "consoleTree" );
		if ( treeCtrl != NULL )
			treeCtrl->clear( );

		NSWin32::CNSVsEdit* luaEditor = ( NSWin32::CNSVsEdit* ) NSWin32::CNSWindow::getWindow( "consoleEditor" );
		if ( luaEditor != NULL )
			luaEditor->clear( );

		NSLog::setLogHandler( consoleLogHandler );
		NSLog::setExceptionHandler( consoleExceptionHandler );
	}

	void setHostWnd( HWND wnd )
	{
		wndHost = wnd;
	}

	bool onConsoleEditorFocusIn( NSWin32::CNSWindow* child, WPARAM wParam, LPARAM lParam, LRESULT* result )
	{
		NSWin32::CNSFrame* tips = ( NSWin32::CNSFrame* ) NSWin32::CNSWindow::getWindow( "varWatchFrame" );
		if ( tips != NULL )
			tips->destroy( );

		return true;
	}

	HTREEITEM parent = TVI_ROOT;
	void stackTraceProc( CNSLuaStack::EnumData* data, CNSLuaStack::EnumData* key, CNSLuaStack::EnumData* value, void* userdata )
	{
		bool hasChildren = false;
		NSWin32::CNSVsTree* tipsTree = ( NSWin32::CNSVsTree* ) userdata;
		static CNSString dataText;
		if ( data->type == LUA_TNUMBER )
			dataText.format( "%s[number]\n", CNSString::number2String( *(double*) data->value ).getBuffer( ) );
		else if ( data->type == LUA_TSTRING )
			dataText.format( "%s[string]\n", (char*) data->value );
		else if ( data->type == LUA_TBOOLEAN )
			dataText.format( "%s[boolean]\n", CNSString::bool2String( *(bool*) data->value ).getBuffer( ) );
		else if ( data->type == LUA_TNIL )
			dataText.format( "[nil]\n" );
		else if ( data->type == LUA_TNONE )
			dataText.format( "[none]\n" );
		else if ( data->type == LUA_TFUNCTION )
			dataText.format( "0x%016x[function]\n", data->value );
		else if ( data->type == LUA_TTHREAD )
			dataText.format( "0x%016x[thread]\n", data->value );
		else if ( data->type == LUA_TUSERDATA )
			dataText.format( "0x%016x[userdata]\n", data->value );
		else if ( data->type == LUA_TLIGHTUSERDATA )
			dataText.format( "0x%016x[lightuserdata]\n", data->value );
		else if ( data->type == LUA_TTABLE )
		{
			if ( (ptrdiff_t) data->value == 1 )
			{
				if ( key != NULL )
				{
					if ( key->type == LUA_TNUMBER )
						dataText.format( "%s[number]", CNSString::number2String( *(double*) key->value ).getBuffer( ) );
					else if ( key->type == LUA_TSTRING )
						dataText.format( "%s", key->value );
					hasChildren = true;
				}
				else
				{
					dataText.clear( );
					tipsTree->clear( );
					return;
				}
			}
			else if ( (ptrdiff_t) data->value == 0 )
			{
				parent = tipsTree->getParentItem( parent );
				dataText.clear( );
			}
			else if ( (ptrdiff_t) data->value == 2 )
			{
				static CNSString keyText;
				static CNSString valueText;
				if ( key->type == LUA_TNUMBER )
					keyText.format( "%s[number]", CNSString::number2String( *(double*) key->value ).getBuffer( ) );
				else if ( key->type == LUA_TSTRING )
					keyText.format( "%s[string]", (char*) key->value );

				if ( value->type == LUA_TNUMBER )
					valueText.format( "%s[number]", CNSString::number2String( *(double*) value->value ).getBuffer( ) );
				else if ( value->type == LUA_TSTRING )
					valueText.format( "%s[string]", (char*) value->value );
				else if ( value->type == LUA_TBOOLEAN )
					valueText.format( "%s[boolean]", CNSString::bool2String( *(bool*) value->value ).getBuffer( ) );
				else if ( value->type == LUA_TNIL )
					valueText.format( "[nil]" );
				else if ( value->type == LUA_TNONE )
					valueText.format( "[none]" );
				else if ( value->type == LUA_TFUNCTION )
					valueText.format( "0x%016x[function]", value->value );
				else if ( value->type == LUA_TTHREAD )
					valueText.format( "0x%016x[thread]", value->value );
				else if ( value->type == LUA_TUSERDATA )
					valueText.format( "0x%016x[userdata]", value->value );
				else if ( value->type == LUA_TLIGHTUSERDATA )
					valueText.format( "0x%016x[lightuserdata]", value->value );
				dataText.format( "%s = %s", keyText.getBuffer( ), valueText.getBuffer( ) );
			}
		}

		if ( dataText.isEmpty( ) == true )
			return;

		HTREEITEM subItem = NULL;
		subItem = tipsTree->addItem( parent, TVI_SORT, dataText, NULL );
		if ( hasChildren == true )
			parent = subItem;
	}

	bool onConsoleEditorDwellStart( NSWin32::CNSWindow* child, WPARAM wParam, LPARAM lParam, LRESULT* result )
	{
		NSWin32::CNSVsEdit* luaEditor = ( NSWin32::CNSVsEdit* ) child;
		SCNotification* notify = (SCNotification*) lParam;
		unsigned int start, end = -1;
		CNSString& word = luaEditor->getWordByPosition( (unsigned int) notify->position, start, end );
		// 如果不在调试中断状态，不能查看变量
		if ( inBreak == false )
			return true;

		CNSLuaStack& luaStack = CNSLuaStack::getLuaStack( );
		int stackIndex = -1;
		int varType = luaStack.queryVar( word, stackIndex );
		NSWin32::CNSFrame* tips = ( NSWin32::CNSFrame* ) NSWin32::CNSWindow::getWindow( "varWatchFrame" );
		if ( varType == 0 )
		{
			if ( tips != NULL )
				tips->destroy( );

			return true;
		}

		POINT pt;
		pt.x = notify->x;
		pt.y = notify->y;
		ClientToScreen( luaEditor->getHWnd( ), &pt );

		RECT rcWatch;
		rcWatch.left = pt.x;
		rcWatch.top = max( 0, pt.y - 300 );
		rcWatch.right = rcWatch.left + 400;
		rcWatch.bottom = rcWatch.top + 250;

		parent = TVI_ROOT;
		if ( tips == NULL )
		{
			tips = NSWin32::CNSWindow::newFrame( "varWatchFrame", NSWin32::CNSFrame::EFrameType::STYLE_POPUP, rcWatch, NULL, false );
			tips->setText( CNSString( _UTF8( "变量查看器 - " ) ) + word );
			tips->show( );

			RECT rcWatchTree;
			rcWatchTree.left = 0;
			rcWatchTree.right = 400;
			rcWatchTree.top = 0;
			rcWatchTree.bottom = 250;
			NSWin32::CNSVsTree* tipsTree = NSWin32::CNSWindow::newVsTree( "varWatch", 0, rcWatchTree, tips );
			if ( tipsTree != NULL )
			{
				tipsTree->clear( );
				luaStack.enumStack( stackIndex, stackTraceProc, tipsTree );
				lua_pop( luaStack.getLuaState( ), 1 );
			}
		}
		else
		{
			tips->move( rcWatch.left, rcWatch.top, 0.5f, true );
			NSWin32::CNSVsTree* tipsTree = ( NSWin32::CNSVsTree* ) NSWin32::CNSWindow::getWindow( "varWatch" );
			if ( tipsTree != NULL )
			{
				tipsTree->clear( );
				luaStack.enumStack( stackIndex, stackTraceProc, tipsTree );
				lua_pop( luaStack.getLuaState( ), 1 );
			}
		}

		return true;
	}

	bool onConsoleEditorMarginClicked( NSWin32::CNSWindow* child, WPARAM wParam, LPARAM lParam, LRESULT* result )
	{
		NSWin32::CNSVsTree*		luaTree = ( NSWin32::CNSVsTree* ) NSWin32::CNSWindow::getWindow( "consoleTree" );
		NSWin32::CNSVsEdit*		luaEditor = ( NSWin32::CNSVsEdit* ) child;
		SCNotification*		notify = (SCNotification*) lParam;
		HTREEITEM			selItem = luaTree->getCurSel( );

		CNSString& fileName = curLuaFile;
		int line = luaEditor->lineFromPosition( (unsigned int) notify->position );
		if ( line == -1 )
			return 0;

		if ( DebugHasBreak( fileName, line ) == true )
			DebugRemoveBreak( fileName, line );
		else
			DebugAddBreak( fileName, line );

		return true;
	}

	void showFile( const CNSString& filePath, const CNSString& buffer )
	{
		if ( curLuaFile == filePath )
			return;

		NSWin32::CNSVsEdit* luaEditor = ( NSWin32::CNSVsEdit* ) NSWin32::CNSWindow::getWindow( "consoleEditor" );
		if ( luaEditor == NULL )
			return;

		NSWin32::CNSVsTab*	vsTab = ( NSWin32::CNSVsTab* ) NSWin32::CNSWindow::getWindow( "consoleFileTab" );
		if ( vsTab == NULL )
			return;

		luaEditor->setVisible( true );
		int* openIDRef = openFileList.get( filePath );
		if ( openIDRef != NULL )
		{
			NSBase::CNSVector< int > itemIDs;
			vsTab->getItems( itemIDs );
			for ( unsigned int i = 0; i < itemIDs.getCount( ); i ++ )
			{
				long long tabOpenID = (long long) vsTab->getItemData( itemIDs[ i ] );
				if ( ( *openIDRef ) == (int) tabOpenID )
				{
					vsTab->setCurSel( itemIDs[ i ] );
					break;
				}
			}
		}
		else
		{
			CNSString& fileName = filePath.getFileName( '/' );
			int index = vsTab->addItem( fileName );
			vsTab->setItemData( index, (intptr_t) openFileID );
			vsTab->setCurSel( index );
			openFileIndex.insert( openFileID, filePath );
			openFileList.insert( filePath, openFileID );
			openFileID ++;
		}

		luaEditor->setEditorText( buffer );
		HLISTINDEX beginIndex = luaBreaks.getHead( );
		for ( ; beginIndex != NULL; luaBreaks.getNext( beginIndex ) )
		{
			CNSString& breakInfo = luaBreaks.getKey( beginIndex );

			static CNSVector< CNSString > infoSet;
			infoSet.clear( );
			breakInfo.split( ":", infoSet );
			if ( filePath == infoSet[ 0 ] )
			{
				int line = infoSet[ 1 ].toInteger( );
				luaEditor->showMargin( line, EMarkerType::MARKER_BREAK );
			}
		}

		if ( filePath == curIntrputFile )
		{
			luaEditor->showMargin( curIntrputLine, EMarkerType::MARKER_INTERRUPT );
			luaEditor->visibleLine( curIntrputLine );
			luaEditor->gotoLine( curIntrputLine );
			luaEditor->delayFocus( );
		}

		curLuaFile = filePath;
	}

	bool onConsoleTreeSelectChanged( NSWin32::CNSWindow* child, WPARAM wParam, LPARAM lParam, LRESULT* result )
	{
		NSWin32::CNSVsTree*	tree = ( NSWin32::CNSVsTree* ) child;
		NMTREEVIEW*		nmhdr = (NMTREEVIEW*) lParam;
		HTREEITEM		item = nmhdr->itemNew.hItem;
		CNSString*		textRef = (CNSString*) tree->getItemData( item );
		if ( textRef == NULL )
			return true;

		CNSString& filePath = tree->getItemPath( item );
		showFile( filePath, *textRef );
		return true;
	}

	bool onConsoleFileTabCloseItem( NSWin32::CNSWindow* child, WPARAM wParam, LPARAM lParam, LRESULT* result )
	{
		NSWin32::CNSVsTab::VSTabClose*	closeItemHdr = ( NSWin32::CNSVsTab::VSTabClose* ) lParam;
		NSWin32::CNSVsTab*				vsTab = ( NSWin32::CNSVsTab* ) child;
		int									tabOpenID = (int) (long long) vsTab->getItemData( closeItemHdr->deleteItemID );
		NSBase::CNSString*					filePathRef = openFileIndex.get( tabOpenID );
		if ( filePathRef == NULL )
			return true;

		vsTab->deleteItem( closeItemHdr->deleteItemID );
		openFileList.erase( ( *filePathRef ) );
		openFileIndex.erase( tabOpenID );
		return true;
	}

	bool onConsoleFileTabSelectChanged( NSWin32::CNSWindow* child, WPARAM wParam, LPARAM lParam, LRESULT* result )
	{
		NSWin32::CNSVsTab* vsTab = ( NSWin32::CNSVsTab* ) child;
		NSWin32::CNSVsTree* tree = ( NSWin32::CNSVsTree* ) NSWin32::CNSWindow::getWindow( "consoleTree" );
		if ( tree == NULL )
			return true;

		NSWin32::CNSVsEdit* luaEditor = ( NSWin32::CNSVsEdit* ) NSWin32::CNSWindow::getWindow( "consoleEditor" );
		if ( luaEditor == NULL )
			return true;

		NSWin32::CNSVsTab::VSTabSelectChanged* nmhdrTabSelect = ( NSWin32::CNSVsTab::VSTabSelectChanged* ) lParam;
		int index = nmhdrTabSelect->newItemID;
		if ( index == -1 )
		{
			curLuaFile.clear( );
			luaEditor->setVisible( false );
			return true;
		}

		int	newOpenID = (int) (long long) vsTab->getItemData( index );
		int oldOpenID = (int) (long long) vsTab->getItemData( nmhdrTabSelect->oldItemID );
		CNSString* filePathRef = openFileIndex.get( newOpenID );
		if ( filePathRef == NULL )
			return true;

		CNSString&	filePath = ( *filePathRef );
		HTREEITEM	item = tree->findItem( filePath );
		CNSString*	textRef = (CNSString*) tree->getItemData( item );
		if ( textRef == NULL )
			return true;

		// 保存住之前的观察
		showFile( filePath, ( *textRef ) );
		return true;
	}

	bool onConsoleTabSelectChanged( NSWin32::CNSWindow* child, WPARAM wParam, LPARAM lParam, LRESULT* result )
	{
		NSWin32::CNSVsTab* tab = ( NSWin32::CNSVsTab* ) child;
		NSWin32::CNSVsEdit* logEdit = ( NSWin32::CNSVsEdit* ) NSWin32::CNSWindow::getWindow( "consoleEdit" );
		NSWin32::CNSVsTree* luaTree = ( NSWin32::CNSVsTree* ) NSWin32::CNSWindow::getWindow( "consoleTree" );
		NSWin32::CNSVsEdit* luaEditor = ( NSWin32::CNSVsEdit* ) NSWin32::CNSWindow::getWindow( "consoleEditor" );
		NSWin32::CNSFrame* searchFrame = ( NSWin32::CNSFrame* ) NSWin32::CNSWindow::getWindow( "consoleFrameSearch" );
		NSWin32::CNSFrame* debugFrame = ( NSWin32::CNSFrame* ) NSWin32::CNSWindow::getWindow( "consoleDebugFrame" );

		NSWin32::CNSVsTab::VSTabSelectChanged* nmhdrTabSelect = ( NSWin32::CNSVsTab::VSTabSelectChanged* ) lParam;
		int			index = nmhdrTabSelect->newItemID;

		const CNSString* textOutput = NULL;
		if ( index == PAGE_LOG )
		{
			textOutput = &NSLog::sLogText;
			logEdit->setVisible( true );
			debugFrame->setVisible( false );
			logEdit->setEditorText( *textOutput );

			if ( searchFrame != NULL )
				searchFrame->hide( );
		}
		else if ( index == PAGE_EXCEPTION )
		{
			textOutput = &NSLog::sExceptionText;
			logEdit->setVisible( true );
			debugFrame->setVisible( false );
			logEdit->setEditorText( *textOutput );

			if ( searchFrame != NULL )
				searchFrame->hide( );
		}
		else if ( index == PAGE_DEBUG )
		{
			logEdit->setVisible( false );
			debugFrame->setVisible( true );
			if ( searchFrame != NULL )
			{
				searchFrame->show( );
				luaEditor->focus( );
			}
		}

		return true;
	}

	bool onConsoleGetMinMaxInfo( NSWin32::CNSWindow* window, WPARAM wParam, LPARAM lParam, LRESULT* result )
	{
		MINMAXINFO* minmaxInfo = (MINMAXINFO*) lParam;
		minmaxInfo->ptMinTrackSize.x = 300;
		minmaxInfo->ptMinTrackSize.y = 200;
		return true;
	}

	bool onEditorSize( NSWin32::CNSWindow* window, WPARAM wParam, LPARAM lParam, LRESULT* result )
	{
		NSWin32::CNSFrame* searchFrame = ( NSWin32::CNSFrame* ) NSWin32::CNSWindow::getWindow( "consoleFrameSearch" );
		if ( searchFrame != NULL )
		{
			NSWin32::CNSVsEdit* editorCtrl = ( NSWin32::CNSVsEdit* ) window;
			if ( editorCtrl == NULL )
				return 0;

			RECT rcEditor;
			editorCtrl->getClientRect( rcEditor );

			POINT pt;
			pt.x = rcEditor.right - 300;
			pt.y = rcEditor.top;
			ClientToScreen( editorCtrl->getHWnd( ), &pt );
			searchFrame->move( pt.x, pt.y, 0.5, false );
		}

		NSWin32::CNSFrame* gotoFrame = ( NSWin32::CNSFrame* ) NSWin32::CNSWindow::getWindow( "consoleFrameGoto" );
		if ( gotoFrame != NULL )
		{
			NSWin32::CNSVsEdit* editorCtrl = ( NSWin32::CNSVsEdit* ) window;
			if ( editorCtrl == NULL )
				return 0;

			RECT rcEditor;
			editorCtrl->getClientRect( rcEditor );

			POINT pt;
			pt.x = ( ( rcEditor.right - rcEditor.left ) - 300 ) / 2;
			pt.y = rcEditor.top;
			ClientToScreen( editorCtrl->getHWnd( ), &pt );
			gotoFrame->move( pt.x, pt.y, 0.5, false );
		}
		return false;
	}

	bool onConsoleClose( NSWin32::CNSWindow* window, WPARAM wParam, LPARAM lParam, LRESULT* result )
	{
		if ( MessageBox( console->getHWnd( ), _T( "关闭窗口将会重置调试状态" ), _T( "系统提示" ), MB_YESNO | MB_ICONWARNING ) == IDYES )
			window->destroy( );

		return true;
	}

	bool onConsoleDestroy( NSWin32::CNSWindow* window, WPARAM wParam, LPARAM lParam, LRESULT* result )
	{
		openFileIndex.clear( );
		openFileList.clear( );
		curLuaFile.clear( );
		openFileID = 1;

		console = NULL;
		if ( memDC != NULL )
			DeleteDC( memDC );

		if ( memBmp != NULL )
			DeleteObject( memBmp );

		DebugReset( );
		if ( inBreak == true )
		{
			PostQuitMessage( 0 );
			inBreak = false;
		}

		return false;
	}

	bool onConsoleMove( NSWin32::CNSWindow* window, WPARAM wParam, LPARAM lParam, LRESULT* result )
	{
		NSWin32::CNSVsEdit* editorCtrl = ( NSWin32::CNSVsEdit* ) NSWin32::CNSWindow::getWindow( "consoleEditor" );
		if ( editorCtrl == NULL )
			return true;

		NSWin32::CNSFrame* frame = ( NSWin32::CNSFrame* ) NSWin32::CNSWindow::getWindow( "consoleFrameSearch" );
		if ( frame != NULL )
		{
			RECT rcEditor;
			editorCtrl->getClientRect( rcEditor );

			POINT pt;
			pt.x = rcEditor.right - 300;
			pt.y = rcEditor.top;
			ClientToScreen( editorCtrl->getHWnd( ), &pt );
			frame->move( pt.x, pt.y, 0.5, false );
		}

		NSWin32::CNSFrame* gotoFrame = ( NSWin32::CNSFrame* ) NSWin32::CNSWindow::getWindow( "consoleFrameGoto" );
		if ( gotoFrame != NULL )
		{
			RECT rcEditor;
			editorCtrl->getClientRect( rcEditor );

			POINT pt;
			pt.x = ( ( rcEditor.right - rcEditor.left ) - 300 ) / 2;
			pt.y = rcEditor.top;
			ClientToScreen( editorCtrl->getHWnd( ), &pt );
			gotoFrame->move( pt.x, pt.y, 0.5, false );
		}
		return true;
	}

	void buildFileTree( NSWin32::CNSVsTree* tree )
	{
		tree->clear( );
		CNSMap< CNSString, HTREEITEM > treeInfo;
		HLISTINDEX beginIndex = luaFiles.getHead( );
		for ( ; beginIndex != NULL; luaFiles.getNext( beginIndex ) )
		{
			const CNSString& filePath = luaFiles.getKey( beginIndex );
			CNSString& luaText = luaFiles.getValue( beginIndex );
			static CNSVector< CNSString > pathList( 16 );
			pathList.clear( );
			filePath.split( "/", pathList );
			HTREEITEM parentItem = TVI_ROOT;
			CNSString parentKey;
			for ( unsigned int i = 0; i < pathList.getCount( ); i ++ )
			{
				parentKey = parentKey + pathList[ i ];
				HTREEITEM* itemRef = treeInfo.get( parentKey );
				if ( itemRef != NULL )
				{
					parentItem = *itemRef;
					continue;
				}

				if ( i == pathList.getCount( ) - 1 )
					parentItem = tree->addItem( parentItem, TVI_SORT, pathList[ i ], &luaText );
				else
					parentItem = tree->addItem( parentItem, TVI_LAST, pathList[ i ], NULL );
				treeInfo.insert( parentKey, parentItem );
			}
		}
	}

	void onCreateConsole( NSWin32::CNSFrame* console )
	{
		lastFindStart = -1;
		oldTipLine = -1;
		console->registerMessage( WM_GETMINMAXINFO, onConsoleGetMinMaxInfo );
		console->registerMessage( WM_CLOSE, onConsoleClose );
		console->registerMessage( WM_NCDESTROY, onConsoleDestroy );
		console->registerMessage( WM_MOVE, onConsoleMove );

		RECT rcTab;
		rcTab.left = 0;
		rcTab.right = 800;
		rcTab.top = 0;
		rcTab.bottom = 600;
		NSWin32::CNSVsTab* tab = NSWin32::CNSWindow::newVsTab( "consoleTab", NSWin32::CNSVsTab::TAB_FILETYPE, rcTab, console );
		tab->registerEvent( NSWin32::CNSVsTab::EventSelectChanged, onConsoleTabSelectChanged );

		RECT rcEdit;
		tab->getWorkRect( rcEdit );
		NSWin32::CNSVsEdit* edit = NSWin32::CNSWindow::newVsEdit( "consoleEdit", 0, rcEdit, tab );
		edit->setLuaLexer( );
		edit->enableReadonly( true );

		RECT rcFrame;
		tab->getWorkRect( rcFrame );
		NSWin32::CNSFrame* debugFrame = NSWin32::CNSWindow::newFrame( "consoleDebugFrame", NSWin32::CNSFrame::EFrameType::STYLE_CHILD, rcFrame, tab, false );
		debugFrame->split( NSWin32::CNSFrame::ESplitType::SPLIT_HORZ, 0.3f );
		debugFrame->hide( );

		RECT rcTree;
		NSWin32::CNSVsTree* tree = NSWin32::CNSWindow::newVsTree( "consoleTree", 0, rcTree, debugFrame );
		tree->registerEvent( NSWin32::CNSVsTree::CTreeCtrlEvent::EventSelectChanged, onConsoleTreeSelectChanged );
		tree->enableTitle( true );
		tree->setTitle( _UTF8( "工作空间" ) );
		debugFrame->bindLeft( tree );

		RECT rcVsTab;
		NSWin32::CNSVsTab* vsTab = NSWin32::CNSWindow::newVsTab( "consoleFileTab", 0, rcVsTab, debugFrame );
		vsTab->registerEvent( NSWin32::CNSVsTab::EventSelectChanged, onConsoleFileTabSelectChanged );
		vsTab->registerEvent( NSWin32::CNSVsTab::EventClose, onConsoleFileTabCloseItem );
		debugFrame->bindRight( vsTab );

		RECT rcEditor;
		vsTab->getWorkRect( rcEditor );
		NSWin32::CNSVsEdit* editor = NSWin32::CNSWindow::newVsEdit( "consoleEditor", 0, rcEditor, vsTab );
		editor->setLuaLexer( );
		editor->registerEvent( NSWin32::CNSVsEdit::EventMarginClicked, onConsoleEditorMarginClicked );
		editor->registerEvent( NSWin32::CNSVsEdit::EventDwellStart, onConsoleEditorDwellStart );
		editor->registerEvent( NSWin32::CNSVsEdit::EventFocusIn, onConsoleEditorFocusIn );
		editor->registerMessage( WM_SIZE, onEditorSize );
		editor->setVisible( false );

		PAGE_LOG = tab->addItem( _UTF8( "日志" ) );
		PAGE_EXCEPTION = tab->addItem( _UTF8( "异常" ) );
		PAGE_DEBUG = tab->addItem( _UTF8( "调试" ) );
		tab->setCurSel( PAGE_LOG );

		edit->setEditorText( NSLog::sLogText );
		buildFileTree( tree );

		editor->enableLineNumber( true );
		// 开启1号页边，16个像素宽，只接受0, 1, 2号页边符号, 设置断点页边
		editor->setMargin( 1, 16, 0x00000007, RGB( 51, 51, 51 ), true );

		// 设置1号页边符号图形为4( SC_MARK_SHORTARROW )
		editor->setMarginSymbol( NSConsole::EMarkerType::MARKER_INTERRUPT, SC_MARK_SHORTARROW, RGB( 99, 98, 90 ), RGB( 245, 208, 55 ) );

		// 设置0号页边符号图形为0( SC_MARK_CIRCLE )
		editor->setMarginSymbol( NSConsole::EMarkerType::MARKER_BREAK, SC_MARK_CIRCLE, RGB( 255, 255, 255 ), RGB( 228, 20, 0 ) );

		// 设置0号页边符号图形为0( SC_MARK_BACKGROUND  )
		editor->setMarginSymbol( NSConsole::EMarkerType::MARKER_LINETIP, SC_MARK_BACKGROUND, RGB( 255, 255, 255 ), RGB( 78, 78, 87 ) );

		NSWin32::CNSWindow::registerGlobalHotkey( NSWin32::HOTKEY_ALT, NSWin32::KEYCODE_L, onDebugOpenFileList );
		NSWin32::CNSWindow::registerGlobalHotkey( NSWin32::HOTKEY_NONE, NSWin32::KEYCODE_F6, onDebugOpenStack );
		NSWin32::CNSWindow::registerGlobalHotkey( NSWin32::HOTKEY_CTRL, NSWin32::KEYCODE_F, onDebugOpenSearch );
		NSWin32::CNSWindow::registerGlobalHotkey( NSWin32::HOTKEY_CTRL, NSWin32::KEYCODE_G, onDebugOpenGotoLine );
		NSWin32::CNSWindow::registerGlobalHotkey( NSWin32::HOTKEY_NONE, NSWin32::KEYCODE_ESC, onConsoleEscape );
		NSWin32::CNSWindow::registerGlobalHotkey( NSWin32::HOTKEY_NONE, NSWin32::KEYCODE_F3, onDebugSearchNext );

		console->registerLocalHotkey( NSWin32::HOTKEY_NONE, NSWin32::KEYCODE_F10, onDebugStep );
		console->registerLocalHotkey( NSWin32::HOTKEY_NONE, NSWin32::KEYCODE_F5, onDebugContinue );
		console->registerLocalHotkey( NSWin32::HOTKEY_NONE, NSWin32::KEYCODE_F11, onDebugInto );

		DebugReset( );
		console->maximized( );
	}

	void CreateConsoleWindow( )
	{
		if ( console != NULL )
		{
			// 如果窗口是激活窗口什么都不用做
			if ( NSWin32::CNSWindow::getActive( ) == console )
				return;

			// 如果窗口不是激活窗口，重新显示，并且重新加载文件树
			console->active( );
			console->maximized( );

			NSWin32::CNSVsTree* treeCtrl = ( NSWin32::CNSVsTree* ) NSWin32::CNSWindow::getWindow( "consoleTree" );
			if ( treeCtrl != NULL )
			{
				HLISTINDEX beginIndex = luaFiles.getHead( );
				for ( ; beginIndex != NULL; luaFiles.getNext( beginIndex ) )
				{
					CNSString& filePath = luaFiles.getKey( beginIndex );
					if ( treeCtrl->findItem( filePath ) == NULL )
					{
						// 如果文件列表发生变动，需要重新构建文件树
						MessageBox( console->getHWnd( ), _T( "检测到文件列表发生变动，点击确定将刷新" ), _T( "系统提示" ), MB_OK | MB_ICONINFORMATION );
						buildFileTree( treeCtrl );
						break;
					}
				}
			}

			return;
		}

		RECT rc = { 0, 0, 800, 600 };
		console = NSWin32::CNSWindow::newFrame( "consoleWindow", NSWin32::CNSFrame::EFrameType::STYLE_FULL, rc, NULL, true );
		onCreateConsole( console );
		console->setText( mConsoleTitle );
	}

	void showConsole( )
	{
		// 通过程序自身消息循环驱动
		CreateConsoleWindow( );
	}
};
