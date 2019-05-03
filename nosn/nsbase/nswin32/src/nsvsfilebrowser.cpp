#include <nsbase.h>
#include <Shlwapi.h>
#include <shlobj.h>
namespace NSWin32
{
	HBRUSH CVsFileBrowser::backBrush = NULL;
	HANDLE CVsFileBrowser::queryThread = NULL;
	HANDLE CVsFileBrowser::queryEvent = NULL;
	class CQueryIcon
	{
	public:
		NSWin32::CNSVsList*	mListCtrl;
		TCHAR				mFileName[ MAX_PATH ];
		TCHAR				mFilePath[ MAX_PATH ];
		DWORD				mAttrib;
		FILETIME			mLastWrite;
		long long			mFileSize;
		int					mType;

	public:
		CQueryIcon( CNSVsList* ctrl = NULL, const TCHAR* fileName = NULL, FILETIME& lastWrite = FILETIME( ), long long fileSize = 0, const TCHAR* filePath = NULL, DWORD attrib = 0, int type = 0 )
			: mListCtrl( ctrl ), mLastWrite( lastWrite ), mFileSize( fileSize ), mAttrib( attrib ), mType( type )
		{
			if ( fileName != NULL )
				_tcscpy( mFileName, fileName );

			if ( filePath != NULL )
				_tcscpy( mFilePath, filePath );
		}
	};

	class CQueryResult
	{
	public:
		NSWin32::CNSVsList*	mListCtrl;
		HIMAGELIST			mImageList;
		TCHAR				mFileName[ MAX_PATH ];
		int					mIImage;
		FILETIME			mLastWrite;
		long long			mFileSize;
		DWORD				mAttrib;
		TCHAR				mTypeName[ MAX_PATH ];
		TCHAR				mDisplayName[ MAX_PATH ];
		int					mType;

	public:
		CQueryResult( CNSVsList* ctrl = NULL, HIMAGELIST imageList = NULL, const TCHAR* fileName = NULL, int iImage = 0, FILETIME& lastWrite = FILETIME( ), long long fileSize = 0, DWORD attrib = 0, const TCHAR* typeName = NULL, const TCHAR* displayName = NULL, int type = 0 )
			: mListCtrl( ctrl ), mImageList( imageList ), mIImage( iImage ), mLastWrite( lastWrite ), mFileSize( fileSize ), mAttrib( attrib ), mType( type )
		{
			if ( fileName != NULL )
				_tcscpy( mFileName, fileName );

			if ( typeName != NULL )
				_tcscpy( mTypeName, typeName );

			if ( displayName != NULL )
				_tcscpy( mDisplayName, displayName );
		}
	};

	CRITICAL_SECTION listCS;
	CNSVector< CQueryIcon >		listForQuery;
	CNSVector< CQueryResult >	listForResult;

	DWORD WINAPI iconQueryThread( void* param )
	{
		while ( 1 )
		{
			if ( ::WaitForSingleObject( CVsFileBrowser::queryEvent, 0 ) == WAIT_OBJECT_0 )
				return 0;

			CQueryIcon item;
			EnterCriticalSection( &listCS );
			int count = listForQuery.getCount( );
			if ( count > 0 )
			{
				item = listForQuery[ 0 ];
				listForQuery.erase( 0 );
			}
			LeaveCriticalSection( &listCS );

			if ( count == 0 )
			{
				Sleep( 10 );
				continue;
			}

			if ( item.mAttrib == -1 )
			{
				EnterCriticalSection( &listCS );
				listForResult.pushback( CQueryResult( item.mListCtrl, NULL, NULL, -1 ) );
				LeaveCriticalSection( &listCS );
			}
			else if ( item.mAttrib == -2 )
			{
				EnterCriticalSection( &listCS );
				listForResult.pushback( CQueryResult( item.mListCtrl, NULL, NULL, -2 ) );
				LeaveCriticalSection( &listCS );
			}
			else
			{
				SHFILEINFO sfi = { 0 };
				HIMAGELIST imageList = (HIMAGELIST) SHGetFileInfo( item.mFilePath, item.mAttrib,
																   &sfi, sizeof( sfi ), SHGFI_USEFILEATTRIBUTES | SHGFI_SYSICONINDEX | SHGFI_SMALLICON | SHGFI_TYPENAME | SHGFI_DISPLAYNAME );

				EnterCriticalSection( &listCS );
				listForResult.pushback( CQueryResult( item.mListCtrl, imageList, item.mFileName, sfi.iIcon, item.mLastWrite, item.mFileSize, item.mAttrib, sfi.szTypeName, sfi.szDisplayName, item.mType ) );
				LeaveCriticalSection( &listCS );
			}
		}
	}

	CVsFileBrowser::CVsFileBrowser( const CNSString& windowID ) : CNSWindow( windowID, NS_VSFILEBROWSER ), mFilterType( EFilterType::FilterDirectory )
	{
	}

	void CVsFileBrowser::init( )
	{
		CoInitialize( NULL );
		backBrush = CreateSolidBrush( RGB( 45, 45, 48 ) );
		WNDCLASSEX wcex;
		wcex.cbSize = sizeof( WNDCLASSEX );
		wcex.style = CS_HREDRAW | CS_VREDRAW;
		wcex.lpfnWndProc = CVsFileBrowser::windowProc;
		wcex.cbClsExtra = 0;
		wcex.cbWndExtra = 0;
		wcex.hInstance = CNSWindow::instance;
		wcex.hIcon = NULL;
		wcex.hCursor = LoadCursor( NULL, IDC_ARROW );
		wcex.hbrBackground = backBrush;
		wcex.lpszMenuName = NULL;
		wcex.lpszClassName = _T( "VSFileBrowserInner" );
		wcex.hIconSm = NULL;
		ATOM classAtom = RegisterClassEx( &wcex );
		if ( classAtom == NULL )
		{
			DWORD errorCode = GetLastError( );
			static CNSString errorDesc;
			errorDesc.format( _UTF8( "win32函数[RegisterClassEx]调用失败，错误码: %d" ), errorCode );
			NSException( errorDesc );
		}

		CNSWindow::superClass( _T( "VSFileBrowserInner" ), WC_NS_VSFILEBROWSER );
		InitializeCriticalSection( &listCS );

		queryEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
		DWORD threadID;
		queryThread = CreateThread( NULL, 0, (LPTHREAD_START_ROUTINE) iconQueryThread, NULL, 0, &threadID );
	}

	void CVsFileBrowser::exit( )
	{
		CoUninitialize( );
		UnregisterClass( _T( "VSFileBrowserInner" ), CNSWindow::instance );
		UnregisterClass( WC_NS_VSFILEBROWSER, CNSWindow::instance );

		if ( queryEvent != NULL )
			SetEvent( queryEvent );

		if ( queryThread != NULL )
			WaitForSingleObject( queryThread, INFINITE );

		if ( queryEvent != NULL )
			CloseHandle( queryEvent );

		if ( backBrush != NULL )
			DeleteObject( backBrush );

		DeleteCriticalSection( &listCS );
	}

	// Visual studio Style Wizard 窗口回调函数
	LRESULT CVsFileBrowser::windowProc( HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam )
	{
		try
		{
			CVsFileBrowser* vsFileList = (CVsFileBrowser*) CNSWindow::fromHWnd( wnd );
			if ( vsFileList == NULL )
				return DefWindowProc( wnd, msg, wParam, lParam );

			return DefWindowProc( wnd, msg, wParam, lParam );
		}
		catch ( CNSException& e )
		{
			NSLog::exception( _UTF8( "函数CVsFileBrowser::windowProc发生异常\n错误描述: \n\t%s\nC++调用堆栈: \n%s" ), e.mErrorDesc, NSBase::NSFunction::getStackInfo( ).getBuffer( ) );
		}

		return 0;
	}

	bool CVsFileBrowser::onListCtrlClicked( CNSWindow* child, WPARAM wParam, LPARAM lParam, LRESULT* reuslt )
	{
		CNSVsList* list = (CNSVsList*) child;
		CVsFileBrowser* fileList = (CVsFileBrowser*) list->getParent( );
		if ( fileList == NULL )
			return true;

		fileList->notifyParent( EVsFileBrowserEvent::EventSelect );
		return true;
	}

	bool CVsFileBrowser::onListCtrlDblClk( CNSWindow* child, WPARAM wParam, LPARAM lParam, LRESULT* reuslt )
	{
		CNSVsList* list = (CNSVsList*) child;
		CVsFileBrowser* fileList = (CVsFileBrowser*) list->getParent( );
		if ( fileList == NULL )
			return true;

		CNSVector< int > selItems = list->getCurSel( );
		if ( selItems.getCount( ) == 1 )
		{
			int index = selItems[ 0 ];
			CNSString fileName = list->getItemText( 0, index );
			CNSString tempPath = fileList->mCurPath + fileName;
			DWORD attrib = GetFileAttributes( CNSString::toTChar( tempPath ) );
			if ( attrib & FILE_ATTRIBUTE_DIRECTORY )
				fileList->setCurPath( tempPath );
			else
				fileList->notifyParent( EVsFileBrowserEvent::EventDBlClkFile );
		}

		return true;
	}

	bool CVsFileBrowser::onDirInputEditKeydown( CNSWindow* child, WPARAM wParam, LPARAM lParam, LRESULT* reuslt )
	{
		CNSEdit* edit = (CNSEdit*) child;
		CVsFileBrowser* fileList = (CVsFileBrowser*) edit->getParent( );
		if ( fileList == NULL )
			return false;

		if ( wParam == VK_RETURN )
		{
			CNSString input = edit->getText( );
			if ( PathIsDirectory( CNSString::toTChar( input ) ) == FALSE )
			{
				static CNSString tips;
				tips.format( _UTF8( "%s - 不是一个合法路径" ), input.getBuffer( ) );
				MessageBox( fileList->getHWnd( ), CNSString::toTChar( tips ), _T( "FBLuaIde" ), MB_OK | MB_ICONERROR );
				return true;
			}

			// 如果最后一个字符不是\, 那么需要补一个
			fileList->setCurPath( input );
			*reuslt = 1;
			return true;
		}

		return false;
	}

	bool CVsFileBrowser::onUpButtonClicked( CNSWindow* child, WPARAM wParam, LPARAM lParam, LRESULT* reuslt )
	{
		CNSVsBtn* btn = (CNSVsBtn*) child;
		CVsFileBrowser* fileList = (CVsFileBrowser*) btn->getParent( );
		if ( fileList == NULL )
			return true;

		if ( fileList->mCurPath.isEmpty( ) == true )
			return true;

		CNSVector< CNSString > pathSet;
		fileList->mCurPath.split( "/", pathSet );
		if ( pathSet.getCount( ) == 2 )
		{
			fileList->mCurPath.clear( );
			fileList->displayDrivers( );
			return true;
		}

		CNSString newPath;
		for ( unsigned int i = 0; i < pathSet.getCount( ) - 2; i ++ )
			newPath += pathSet[ i ] + "/";

		fileList->setCurPath( newPath );
		return true;
	}

	void __stdcall CVsFileBrowser::onQueryTimer( HWND wnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime )
	{
		if ( idEvent == 100 )
		{
			CVsFileBrowser* fileList = (CVsFileBrowser*) CNSWindow::fromHWnd( wnd );
			CQueryResult item;
			EnterCriticalSection( &listCS );
			for ( unsigned int i = 0; i < listForResult.getCount( ); i ++ )
			{
				item = listForResult[ i ];
				if ( item.mIImage == -1 )
				{
					item.mListCtrl->setRedraw( TRUE );
				}
				else if ( item.mIImage == -2 )
				{
					item.mListCtrl->clear( );
				}
				else
				{
					if ( item.mType == 1 )
					{
						int index = item.mListCtrl->addItem( CNSString::fromTChar( item.mFileName ) );
						item.mListCtrl->setImageList( item.mImageList );
						item.mListCtrl->setItemImage( 0, index, item.mIImage );
						item.mListCtrl->setItem( 2, index, CNSString::fromTChar( item.mTypeName ) );
					}
					else if ( item.mType == 0 )
					{
						int index = item.mListCtrl->addItem( CNSString::fromTChar( item.mFileName ) );
						item.mListCtrl->setImageList( item.mImageList );
						item.mListCtrl->setItemImage( 0, index, item.mIImage );

						SYSTEMTIME stUTC, stLocal;
						FileTimeToSystemTime( &item.mLastWrite, &stUTC );
						SystemTimeToTzSpecificLocalTime( NULL, &stUTC, &stLocal );

						CNSString lastWrite;
						lastWrite.format( "%d/%02d/%02d %02d:%02d", stLocal.wYear, stLocal.wMonth, stLocal.wDay,
										  stLocal.wHour, stLocal.wMinute );
						item.mListCtrl->setItem( 1, index, lastWrite );

						item.mListCtrl->setItem( 2, index, CNSString::fromTChar( item.mTypeName ) );
						if ( !( item.mAttrib & FILE_ATTRIBUTE_DIRECTORY ) )
						{
							CNSString fileSize;
							fileSize.format( "%d KB", NSFunction::floor( ( (double) item.mFileSize / 1024 ) ) );
							item.mListCtrl->setItem( 3, index, fileSize );
						}
					}
				}
			}
			listForResult.clear( );
			LeaveCriticalSection( &listCS );
		}
	}

	void CVsFileBrowser::onPostCreateWindow( CNSWindow* parent )
	{
		CNSWindow::onPostCreateWindow( parent );
		RECT rc;
		GetClientRect( mHWnd, &rc );

		RECT rcList = rc;
		rcList.top += 21;
		NSWin32::CNSVsList* fileList = NSWin32::CNSWindow::newVsList( mWindowID + "_list", 0, rcList, this );
		fileList->addColumn( _UTF8( "文件" ), 155 );
		fileList->addColumn( _UTF8( "修改日期" ), 110 );
		fileList->addColumn( _UTF8( "类型" ), 150 );
		fileList->addColumn( _UTF8( "大小" ), 130 );
		fileList->registerEvent( NSWin32::CNSVsList::EListCtrlEvent::EventDblClicked, onListCtrlDblClk );
		fileList->registerEvent( NSWin32::CNSVsList::EListCtrlEvent::EventClicked, onListCtrlClicked );

		RECT rcPrevBtn = rc;
		rcPrevBtn.left = 0;
		rcPrevBtn.top = 1;
		rcPrevBtn.right = rcPrevBtn.left + 30;
		rcPrevBtn.bottom = 19;
		NSWin32::CNSVsBtn* prevBtn = NSWin32::CNSWindow::newVsBtn( mWindowID + "_prevbtn", NSWin32::CNSVsBtn::EVsButtonStyle::STYLE_PUSHBUTTON | NSWin32::CNSVsBtn::EVsButtonStyle::STYLE_IMAGE, rcPrevBtn, this );
		prevBtn->mLeftAnchor = EAnchor::ANCHOR_LEFT;
		prevBtn->mRightAnchor = EAnchor::ANCHOR_LEFT;
		prevBtn->mTopAnchor = EAnchor::ANCHOR_TOP;
		prevBtn->mBottomAnchor = EAnchor::ANCHOR_TOP;

		CImage* imagePrev = CImage::loadPngImageBGRA( "prev.png" );
		prevBtn->setImage( imagePrev );

		RECT rcNextBtn = rc;
		rcNextBtn.left = 32;
		rcNextBtn.top = 1;
		rcNextBtn.right = rcNextBtn.left + 30;
		rcNextBtn.bottom = 19;
		NSWin32::CNSVsBtn* nextBtn = NSWin32::CNSWindow::newVsBtn( mWindowID + "_nextbtn", NSWin32::CNSVsBtn::EVsButtonStyle::STYLE_PUSHBUTTON | NSWin32::CNSVsBtn::EVsButtonStyle::STYLE_IMAGE, rcNextBtn, this );
		nextBtn->mLeftAnchor = EAnchor::ANCHOR_LEFT;
		nextBtn->mRightAnchor = EAnchor::ANCHOR_LEFT;
		nextBtn->mTopAnchor = EAnchor::ANCHOR_TOP;
		nextBtn->mBottomAnchor = EAnchor::ANCHOR_TOP;

		CImage* imageNext = CImage::loadPngImageBGRA( "next.png" );
		nextBtn->setImage( imageNext );

		RECT rcUpBtn = rc;
		rcUpBtn.left = 64;
		rcUpBtn.top = 1;
		rcUpBtn.right = rcUpBtn.left + 24;
		rcUpBtn.bottom = 19;
		NSWin32::CNSVsBtn* upBtn = NSWin32::CNSWindow::newVsBtn( mWindowID + "_upbtn", NSWin32::CNSVsBtn::EVsButtonStyle::STYLE_PUSHBUTTON | NSWin32::CNSVsBtn::EVsButtonStyle::STYLE_IMAGE, rcUpBtn, this );
		upBtn->mLeftAnchor = EAnchor::ANCHOR_LEFT;
		upBtn->mRightAnchor = EAnchor::ANCHOR_LEFT;
		upBtn->mTopAnchor = EAnchor::ANCHOR_TOP;
		upBtn->mBottomAnchor = EAnchor::ANCHOR_TOP;

		CImage* imageUp = CImage::loadPngImageBGRA( "up.png" );
		upBtn->setImage( imageUp );
		upBtn->registerEvent( NSWin32::CNSVsBtn::EVsButtonEvent::EventClicked, onUpButtonClicked );

		RECT rcEdit;
		rcEdit.left = 90;
		rcEdit.right = rc.right;
		rcEdit.top = 1;
		rcEdit.bottom = 19;
		NSWin32::CNSEdit* edit = NSWin32::CNSWindow::newEdit( mWindowID + "_edit", NSWin32::CNSEdit::EEditCtrlType::EDIT_SINGLETEXT, rcEdit, this );
		edit->mLeftAnchor = EAnchor::ANCHOR_LEFT;
		edit->mRightAnchor = EAnchor::ANCHOR_RIGHT;
		edit->mTopAnchor = EAnchor::ANCHOR_TOP;
		edit->mBottomAnchor = EAnchor::ANCHOR_TOP;
		edit->registerMessage( WM_KEYDOWN, onDirInputEditKeydown );
		SetTimer( mHWnd, 100, 10, onQueryTimer );

		TCHAR* path = NULL;
		SHGetKnownFolderPath( FOLDERID_Desktop, KF_FLAG_DEFAULT, NULL, &path );
		setCurPath( CNSString::fromTChar( path ) );
		CoTaskMemFree( path );
	}

	void CVsFileBrowser::displayDrivers( )
	{
		NSWin32::CNSVsList* list = ( NSWin32::CNSVsList* ) NSWin32::CNSWindow::getWindow( mWindowID + "_list" );
		if ( list == NULL )
			return;

		NSWin32::CNSEdit* edit = ( NSWin32::CNSEdit* ) NSWin32::CNSWindow::getWindow( mWindowID + "_edit" );
		if ( edit == NULL )
			return;

		edit->setText( _UTF8( "我的电脑" ) );
		list->clear( );
		for ( char i = 'C'; i <= 'Z'; i ++ )
		{
			CNSString driver;
			driver.pushback( i );
			driver.pushback( ':' );
			DWORD v1, v2, v3, v4;
			if ( GetDiskFreeSpace( CNSString::toTChar( driver ), &v1, &v2, &v3, &v4 ) == TRUE )
			{
				TCHAR* driverText = CNSString::toTChar( driver );
				EnterCriticalSection( &listCS );
				listForQuery.pushback( CQueryIcon( list, driverText, FILETIME( ), 0, driverText, 0, 1 ) );
				LeaveCriticalSection( &listCS );
			}
		}
		EnterCriticalSection( &listCS );
		listForQuery.pushback( CQueryIcon( list, NULL, FILETIME( ), 0, NULL, -1 ) );
		LeaveCriticalSection( &listCS );

		list->setRedraw( FALSE );
	}

	void CVsFileBrowser::display( )
	{
		NSWin32::CNSEdit* edit = ( NSWin32::CNSEdit* ) NSWin32::CNSWindow::getWindow( mWindowID + "_edit" );
		if ( edit == NULL )
			return;

		edit->setText( getCurPath( ) );
		WIN32_FIND_DATA ffd;
		HANDLE findHandle = FindFirstFile( CNSString::toTChar( mCurPath + "*.*" ), &ffd );
		if ( findHandle == NULL )
			return;

		NSWin32::CNSVsList* list = ( NSWin32::CNSVsList* ) NSWin32::CNSWindow::getWindow( mWindowID + "_list" );
		if ( list == NULL )
			return;

		EnterCriticalSection( &listCS );
		listForQuery.clear( );
		listForResult.clear( );
		listForQuery.pushback( CQueryIcon( list, NULL, FILETIME( ), 0, NULL, -2 ) );
		LeaveCriticalSection( &listCS );
		for ( BOOL find = true; find == TRUE; find = FindNextFile( findHandle, &ffd ) )
		{
			CNSString findFile = CNSString::fromTChar( ffd.cFileName );
			if ( findFile == ".." )
				continue;

			if ( findFile == "." )
				continue;

			if ( findFile == ".svn" )
				continue;

			if ( findFile == ".vscode" )
				continue;

			if ( findFile == ".vs" )
				continue;

			if ( mFilterType == EFilterType::FilterDirectory )
			{
				if ( ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY && !( ffd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN ) )
				{
					long long fileSize = ( (long long) ffd.nFileSizeHigh << 32 ) | ffd.nFileSizeLow;
					EnterCriticalSection( &listCS );
					listForQuery.pushback( CQueryIcon( list, ffd.cFileName, ffd.ftLastWriteTime, fileSize, CNSString::toTChar( mCurPath + findFile ), ffd.dwFileAttributes ) );
					LeaveCriticalSection( &listCS );
				}
			}
			else if ( mFilterType == EFilterType::FilterFile )
			{
				if ( ( ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY || findFile.getFileExtent( ) == mFilter || mFilter.isEmpty( ) == true ) && !( ffd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN ) )
				{
					long long fileSize = ( (long long) ffd.nFileSizeHigh << 32 ) | ffd.nFileSizeLow;
					EnterCriticalSection( &listCS );
					listForQuery.pushback( CQueryIcon( list, ffd.cFileName, ffd.ftLastWriteTime, fileSize, CNSString::toTChar( mCurPath + findFile ), ffd.dwFileAttributes ) );
					LeaveCriticalSection( &listCS );
				}
			}
		}

		EnterCriticalSection( &listCS );
		listForQuery.pushback( CQueryIcon( list, NULL, FILETIME( ), 0, NULL, -1 ) );
		LeaveCriticalSection( &listCS );

		list->setRedraw( FALSE );
		FindClose( findHandle );
	}

	void CVsFileBrowser::getSelectFile( CNSVector< CNSString >& result ) const
	{
		NSWin32::CNSVsList* list = ( NSWin32::CNSVsList* ) NSWin32::CNSWindow::getWindow( mWindowID + "_list" );
		if ( list == NULL )
			return;

		result.clear( );
		CNSVector< int >& curSel = list->getCurSel( );
		for ( unsigned int i = 0; i < curSel.getCount( ); i ++ )
		{
			CNSString& item = list->getItemText( 0, curSel[ i ] );
			result.pushback( item );
		}

		return;
	}

	CNSString& CVsFileBrowser::getCurPath( ) const
	{
		static CNSString result;
		result = mCurPath;
		result.replace( "/", "\\" );
		if ( PathIsRoot( CNSString::toTChar( result ) ) == FALSE )
			result.erase( result.getLength( ) - 1 );

		return result;
	}

	void CVsFileBrowser::setCurPath( const CNSString& path )
	{
		mCurPath = path;
		if ( PathIsDirectory( CNSString::toTChar( mCurPath ) ) == FALSE )
			return;

		unsigned int len = mCurPath.getLength( );
		if ( len > 0 && mCurPath[ len - 1 ] != '\\' && mCurPath[ len - 1 ] != '/' )
			mCurPath.pushback( "\\" );

		mCurPath.replace( "\\", "/" );
		display( );
		notifyParent( EVsFileBrowserEvent::EventChanged );
	}

	void CVsFileBrowser::setFilter( EFilterType type, const CNSString& filter )
	{
		mFilterType = type;
		mFilter = filter;
	}

	void CVsFileBrowser::refresh( )
	{
		CNSString curPath = getCurPath( );
		setCurPath( curPath );
	}

	void CVsFileBrowser::pushUserData( CNSLuaStack& luaStack, CVsFileBrowser* ref )
	{
		static CNSVector< const luaL_Reg* > reg;
		reg.clear( );
		reg.pushback( NSWin32::NSWindow );
		reg.pushback( NSWin32::NSVsFileBrowser );
		luaStack.pushNSWeakRef( ref, NS_VSFILEBROWSER, reg );
	}

	void CVsFileBrowser::popUserData( const CNSLuaStack& luaStack, CVsFileBrowser*& ref )
	{
		ref = (CVsFileBrowser*) luaStack.popNSWeakRef( NS_VSFILEBROWSER );
	}
}