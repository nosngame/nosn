
// dbtools.cpp : ����Ӧ�ó��������Ϊ��
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


// CdbtoolsApp ����

CdbtoolsApp::CdbtoolsApp( ) : resModule( NULL )
{
	resModule = LoadLibrary( _T("dbtoolres.dll") );
	// ֧����������������
	m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_RESTART;

	// TODO: �ڴ˴���ӹ�����룬
	// ��������Ҫ�ĳ�ʼ�������� InitInstance ��
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
		errorDesc.Format( _UTF8( "����[ImageList_LoadImageA]����ʧ��, ������: %d" ), GetLastError( ) );
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
		NSLog::exception( _UTF8( "error: %s\n�к�: %d\n�ļ�: %s\n����: %s" ), e.mErrorDesc, e.mLineNumber, e.mpFileName, e.mpFuncName );
	}
}

// Ψһ��һ�� CdbtoolsApp ����

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
	// ���һ�������� Windows XP �ϵ�Ӧ�ó����嵥ָ��Ҫ
	// ʹ�� ComCtl32.dll �汾 6 ����߰汾�����ÿ��ӻ���ʽ��
	//����Ҫ InitCommonControlsEx()�����򣬽��޷��������ڡ�
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// ��������Ϊ��������Ҫ��Ӧ�ó�����ʹ�õ�
	// �����ؼ��ࡣ
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinApp::InitInstance();
	SetTimer( NULL, 0, 1, OnTimer );

	AfxEnableControlContainer();
	mAccel = LoadAccelerators ( AfxGetInstanceHandle( ), MAKEINTRESOURCE( IDR_ACCELERATOR ) );

	// ���� shell ���������Է��Ի������
	// �κ� shell ����ͼ�ؼ��� shell �б���ͼ�ؼ���
	CShellManager *pShellManager = new CShellManager;

	// ���Windows Native���Ӿ����������Ա��� MFC �ؼ�����������
	CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows));

	// ��׼��ʼ��
	// ���δʹ����Щ���ܲ�ϣ����С
	// ���տ�ִ���ļ��Ĵ�С����Ӧ�Ƴ�����
	// ����Ҫ���ض���ʼ������
	// �������ڴ洢���õ�ע�����
	// TODO: Ӧ�ʵ��޸ĸ��ַ�����
	// �����޸�Ϊ��˾����֯��
	SetRegistryKey(_T("Ӧ�ó��������ɵı���Ӧ�ó���"));

	CdbtoolsDlg dlg;
	m_pMainWnd = &dlg;
	INT_PTR nResponse = dlg.DoModal();
	if (nResponse == IDOK)
	{
		// TODO: �ڴ˷��ô����ʱ��
		//  ��ȷ�������رնԻ���Ĵ���
	}
	else if (nResponse == IDCANCEL)
	{
		// TODO: �ڴ˷��ô����ʱ��
		//  ��ȡ�������رնԻ���Ĵ���
	}
	else if (nResponse == -1)
	{
		TRACE(traceAppMsg, 0, "����: �Ի��򴴽�ʧ�ܣ�Ӧ�ó���������ֹ��\n");
		TRACE(traceAppMsg, 0, "����: ������ڶԻ�����ʹ�� MFC �ؼ������޷� #define _AFX_NO_MFC_CONTROLS_IN_DIALOGS��\n");
	}

	// ɾ�����洴���� shell ��������
	if (pShellManager != NULL)
	{
		delete pShellManager;
	}

	// ���ڶԻ����ѹرգ����Խ����� FALSE �Ա��˳�Ӧ�ó���
	//  ����������Ӧ�ó������Ϣ�á�
	return FALSE;
}

