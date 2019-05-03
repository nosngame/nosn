
// dbtools.cpp : 定义应用程序的类行为。
//

#include "stdafx.h"

#include "dbtools.h"
#include "dbtoolsDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CdbtoolsApp

BEGIN_MESSAGE_MAP(CdbtoolsApp, CWinApp)
	ON_COMMAND(ID_ACC_SHOWCONSOLE,	&CdbtoolsApp::OnShowConsole)
	ON_COMMAND(ID_MENU_SHOWCONSOLE, &CdbtoolsApp::OnShowConsole)
END_MESSAGE_MAP()


// CdbtoolsApp 构造

CdbtoolsApp::CdbtoolsApp( ) : resModule( NULL )
{
	resModule = LoadLibrary( _T("dbtoolres.dll") );
	// 支持重新启动管理器
	m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_RESTART;

	// TODO: 在此处添加构造代码，
	// 将所有重要的初始化放置在 InitInstance 中
}

HIMAGELIST CdbtoolsApp::GetImageList( const CNSString& resName )
{
	HIMAGELIST* imageListRef = mImageListCache.Get( resName );
	if ( imageListRef != NULL )
		return *imageListRef;

	HIMAGELIST imageList = ImageList_LoadImageA( resModule, resName.GetBuffer( ), 16, 4, RGB( 255, 0, 255 ), IMAGE_BITMAP, 0 );
	if ( imageList == NULL )
	{
		static CNSString errorDesc;
		errorDesc.Format( _UTF8( "函数[ImageList_LoadImageA]调用失败, 错误码: %d" ), GetLastError( ) );
		FBException( errorDesc );
	}
	return mImageListCache.Insert( resName, imageList );
}

HGLOBAL CdbtoolsApp::GetResource( const CNSString& resName, LPTSTR resType )
{
	static CNSString resID;
	resID.Format( "%s%lld", resName.GetBuffer( ), (size_t) resType );
	HGLOBAL* resRef = mResCache.Get( resID );
	if ( resRef != NULL )
		return *resRef;

	CString unicode		= CNSString::ToTChar( resName );
	HRSRC	src			= ::FindResource( resModule, unicode.GetBuffer( ), resType );
	HGLOBAL resMem		= ::LoadResource( resModule, src );
	return mResCache.Insert( resID, resMem );
}

HBITMAP CdbtoolsApp::GetBitmap( const CNSString& resName )
{
	HBITMAP* bitmapRef = mBitmapCache.Get( resName );
	if ( bitmapRef != NULL )
		return *bitmapRef;

	HBITMAP bitmap = ::LoadBitmapA( resModule, resName.GetBuffer( ) );
	return mBitmapCache.Insert( resName, bitmap );
}

HICON CdbtoolsApp::GetIcon( const CNSString& resName )
{
	HICON* iconRef = mIconCache.Get( resName );
	if ( iconRef != NULL )
		return *iconRef;

	HICON icon = ::LoadIconA( resModule, resName.GetBuffer( ) );
	return mIconCache.Insert( resName, icon );
}

HCURSOR CdbtoolsApp::GetCursor( const CNSString& resName )
{
	HCURSOR* cursorRef = mCursorCache.Get( resName );
	if ( cursorRef != NULL )
		return *cursorRef;

	HCURSOR cursor = ::LoadCursorA( resModule, resName.GetBuffer( ) );
	return mCursorCache.Insert( resName, cursor );
}

BOOL CdbtoolsApp::PreTranslateMessage( MSG* msg )
{
	if ( mAccel != NULL && TranslateAccelerator( AfxGetMainWnd( )->GetSafeHwnd( ), mAccel, msg ) )   
		return TRUE;

	return CWinApp::PreTranslateMessage( msg );
}

void CdbtoolsApp::OnShowConsole( )
{
	NSLog::showConsole( );
}

void __stdcall OnTimer( HWND wnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime )
{
	try
	{
		NSLog::FBLogUpdate( );
		NSNet::FBNetPollEvent( );
	}
	catch( CNSException& e )
	{
		NSLog::exception( _UTF8( "error: %s\n行号: %d\n文件: %s\n函数: %s" ), e.mErrorDesc, e.mLineNumber, e.mpFileName, e.mpFuncName );
	}
}

// 唯一的一个 CdbtoolsApp 对象

CdbtoolsApp theApp;

BOOL CdbtoolsApp::ExitInstance( )
{
	NSLog::FBLogFinialize( );
	KillTimer( NULL, 0 );
	FBNetFinalize( );
	FinializeMysql( );

	return CWinApp::ExitInstance( );
}

BOOL CdbtoolsApp::InitInstance( )
{
	FBNetInitialize( );
	// 如果一个运行在 Windows XP 上的应用程序清单指定要
	// 使用 ComCtl32.dll 版本 6 或更高版本来启用可视化方式，
	//则需要 InitCommonControlsEx()。否则，将无法创建窗口。
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// 将它设置为包括所有要在应用程序中使用的
	// 公共控件类。
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinApp::InitInstance();
	SetTimer( NULL, 0, 1, OnTimer );

	AfxEnableControlContainer();
	mAccel = LoadAccelerators ( AfxGetInstanceHandle( ), MAKEINTRESOURCE( IDR_ACCELERATOR ) );

	// 创建 shell 管理器，以防对话框包含
	// 任何 shell 树视图控件或 shell 列表视图控件。
	CShellManager *pShellManager = new CShellManager;

	// 激活“Windows Native”视觉管理器，以便在 MFC 控件中启用主题
	CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows));

	// 标准初始化
	// 如果未使用这些功能并希望减小
	// 最终可执行文件的大小，则应移除下列
	// 不需要的特定初始化例程
	// 更改用于存储设置的注册表项
	// TODO: 应适当修改该字符串，
	// 例如修改为公司或组织名
	SetRegistryKey(_T("应用程序向导生成的本地应用程序"));

	CdbtoolsDlg dlg;
	m_pMainWnd = &dlg;
	INT_PTR nResponse = dlg.DoModal();
	if (nResponse == IDOK)
	{
		// TODO: 在此放置处理何时用
		//  “确定”来关闭对话框的代码
	}
	else if (nResponse == IDCANCEL)
	{
		// TODO: 在此放置处理何时用
		//  “取消”来关闭对话框的代码
	}
	else if (nResponse == -1)
	{
		TRACE(traceAppMsg, 0, "警告: 对话框创建失败，应用程序将意外终止。\n");
		TRACE(traceAppMsg, 0, "警告: 如果您在对话框上使用 MFC 控件，则无法 #define _AFX_NO_MFC_CONTROLS_IN_DIALOGS。\n");
	}

	// 删除上面创建的 shell 管理器。
	if (pShellManager != NULL)
	{
		delete pShellManager;
	}

	// 由于对话框已关闭，所以将返回 FALSE 以便退出应用程序，
	//  而不是启动应用程序的消息泵。
	return FALSE;
}

