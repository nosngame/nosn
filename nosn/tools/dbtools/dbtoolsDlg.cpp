
// dbtoolsDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "dbtools.h"
#include "afxdialogex.h"
#include "interface.h"
#include "dbtoolsDlg.h"
#include "ExecSqlDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif
CNSLuaStack*		luaStack = NULL;
void CDBToolsLuaHandler::onError( const CNSString& errorText )
{
	NSLog::exception( errorText.GetBuffer( ) );
}

void CDBToolsLuaHandler::onOpenScript( const CNSString& fileName, const char* buffer, unsigned int size )
{
	CNSString text( buffer, size );
	NSLog::addLuaFile( fileName, text );
}

void CDBToolsLuaHandler::onRegister( CNSLuaStack* luaStack )
{
	NSLog::FBLogInitDebug( luaStack->GetLuaState( ), AfxGetMainWnd( )->GetSafeHwnd( ), NSLog::EConsoleType::TYPE_GUI );
	NSLog::FBLogSwitchDebug( true );
	NSLog::hookDebug( );
	Register( luaStack->GetLuaState( ) );
	NSLog::log( _UTF8( "注册lua函数成功" ) );
}

void CDBToolEvent::OnAddSession( CDBClient* dbClient, const CNSString& rName, const CNSSockAddr& rLocal, const CNSSockAddr& rPeer, TSessionID vSessionID )
{
	CGroupData* group = theApp.mGroupData.Get( rName );
	if ( group == NULL )
		return;

	CdbtoolsDlg* mainDlg = (CdbtoolsDlg*) AfxGetMainWnd( );
	if ( mainDlg == NULL )
		return;

	CListCtrl* list = (CListCtrl*) mainDlg->GetDlgItem( IDC_SERVER_LIST );
	list->SetItem( group->mIndex, 5, LVIF_TEXT, CdbtoolsApp::getUnicodeLangText( 9 ), 0, 0, 0, 0 );
	static CNSVector< CFBDataType > arg;
	arg.Clear( );
	arg.PushBack( CFBDataType( dbClient->mName.GetBuffer( ) ) );
	luaStack->FireEvent( "onCompleteDB", arg );
}

void CDBToolEvent::OnAddSessionFault( CDBClient* dbClient, const CNSString& rName, int vCode )
{
	CGroupData* group = theApp.mGroupData.Get( rName );
	if ( group == NULL )
		return;

	group->mRetryCount ++;
	CString text;
	text.Format( CdbtoolsApp::getUnicodeLangText( 10 ), group->mRetryCount );

	CdbtoolsDlg* mainDlg = (CdbtoolsDlg*) AfxGetMainWnd( );
	if ( mainDlg == NULL )
		return;

	CListCtrl* list = (CListCtrl*) mainDlg->GetDlgItem( IDC_SERVER_LIST );
	list->SetItem( group->mIndex, 5, LVIF_TEXT, text, 0, 0, 0, 0 );
}

void CDBToolEvent::OnDelSession( CDBClient* dbClient, const CNSString& rName, TSessionID vSessionID )
{
	CGroupData* group = theApp.mGroupData.Get( rName );
	if ( group == NULL )
		return;

	group->mRetryCount ++;
	CString text;
	text.Format( CdbtoolsApp::getUnicodeLangText( 11 ), group->mRetryCount );
	CdbtoolsDlg* mainDlg = (CdbtoolsDlg*) AfxGetMainWnd( );
	if ( mainDlg == NULL )
		return;

	CListCtrl* list = (CListCtrl*) mainDlg->GetDlgItem( IDC_SERVER_LIST );
	list->SetItem( group->mIndex, 5, LVIF_TEXT, text, 0, 0, 0, 0 );
}

void CDBToolEvent::OnError( CDBClient* dbClient, const CNSString& error )
{
	NSLog::log( error );
}

void CDBToolEvent::OnPreSql( CDBClient* dbClient, const CNSString& sqlCommand, bool fullInfo )
{
	static CNSString sqlLog;
	if ( fullInfo == true )
		sqlLog.Format( _UTF8("执行SQL：\n\t%s\n\t是否请求完整信息: 是"), sqlCommand.GetBuffer( ) );
	else
		sqlLog.Format( _UTF8("执行SQL：\n\t%s\n\t是否请求完整信息: 否"), sqlCommand.GetBuffer( ) );
	NSLog::log( sqlLog );
}

void CDBToolEvent::OnConnectDB( CDBClient* dbClient )
{
	CGroupData* group = theApp.mGroupData.Get( dbClient->GetNetworkIO( )->GetName( ) );
	if ( group == NULL )
		return;

	CdbtoolsDlg* mainDlg = (CdbtoolsDlg*) AfxGetMainWnd( );
	if ( mainDlg == NULL )
		return;

	CString unicode = CNSString::ToTChar( dbClient->mAddress );

	static CString text;
	text.Format( CdbtoolsApp::getUnicodeLangText( 8 ), unicode.GetBuffer( ) );
	CListCtrl* list = (CListCtrl*) mainDlg->GetDlgItem( IDC_SERVER_LIST );
	list->SetItem( group->mIndex, 5, LVIF_TEXT, text, 0, 0, 0, 0 );
}

// 用于应用程序“关于”菜单项的 CAboutDlg 对话框
class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()

// CdbtoolsDlg 对话框
CdbtoolsDlg::CdbtoolsDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CdbtoolsDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

CdbtoolsDlg::~CdbtoolsDlg( )
{
}

void CdbtoolsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CdbtoolsDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_NOTIFY(NM_RCLICK, IDC_SERVER_LIST, &CdbtoolsDlg::OnNMRClickServerList)
	ON_COMMAND(ID_EXECSQL, &CdbtoolsDlg::OnExecSql)
	ON_WM_INITMENU( )
END_MESSAGE_MAP()

void CdbtoolsDlg::RefreshUILang( )
{
	CString title;
#ifdef _M_IX86
	title.Format( _T("%s - 32bit"), CdbtoolsApp::getUnicodeLangText( 15 ).GetBuffer( ) );
#elif _M_X64
	title.Format( _T("%s - 64bit"), CdbtoolsApp::getUnicodeLangText( 15 ).GetBuffer( ) );
#endif
	SetWindowText( title.GetString( ) );
	appMenu.ModifyMenu( 0, MF_POPUP | MF_STRING | MF_BYPOSITION, (UINT_PTR) systemMenu.GetSafeHmenu( ), CdbtoolsApp::getUnicodeLangText( 16 ) + L"(&S)" );
	systemMenu.ModifyMenu( 0, MF_STRING| MF_BYPOSITION, ID_MENU_SHOWCONSOLE, CdbtoolsApp::getUnicodeLangText( 17 ) + L"(&C)...\tCtrl+F1" );
	systemMenu.ModifyMenu( 1, MF_POPUP | MF_STRING| MF_BYPOSITION, (UINT_PTR) langMenu.GetSafeHmenu( ), CdbtoolsApp::getUnicodeLangText( 18 ) );

	CListCtrl* list = (CListCtrl*) GetDlgItem( IDC_SERVER_LIST );

	for ( int i = 0; i < 6; i ++ )
	{
		CString unicode = CdbtoolsApp::getUnicodeLangText( i + 1 );
		LVCOLUMN col;
		col.mask	= LVCF_TEXT;
		col.pszText = unicode.GetBuffer( );
		list->SetColumn( i, &col );
		unicode.ReleaseBuffer( );
	}

	RedrawWindow( NULL, NULL, RDW_INVALIDATE | RDW_FRAME );
}

void CdbtoolsDlg::OnInitMenu( CMenu* menu )
{
	CDialogEx::OnInitMenu( menu );
	if ( mpPopup == menu )
		return;

	CMenu*		systemMenu	= menu->GetSubMenu( 0 );
	CMenu*		langMenu	= systemMenu->GetSubMenu( 1 );
	CNSString	curLang		= Common::getLang( );
	HLISTINDEX beginIndex	= theApp.mLangMenuID.GetHead( );
	for ( int i = 0; beginIndex != NULL; theApp.mLangMenuID.GetNext( beginIndex ), i ++ )
	{
		UINT menuID = langMenu->GetMenuItemID( i );
		if ( menuID == 0xFFFFFFFF )
			break;

		CNSString menuLang = theApp.mLangMenuID.GetValue( beginIndex );
		if ( menuLang == curLang )
			langMenu->CheckMenuItem( menuID, MF_BYCOMMAND | MF_CHECKED );
		else
			langMenu->CheckMenuItem( menuID, MF_BYCOMMAND | MF_UNCHECKED );
	}
}

BOOL CdbtoolsDlg::OnCmdMsg( UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo )
{
	if ( nCode == CN_COMMAND )
	{
		CNSString* langRef = theApp.mLangMenuID.Get( nID );
		if ( langRef != NULL )
		{
			Common::setLang( *langRef );
			RefreshUILang( );
			return TRUE;
		}

		CMenuCallback* callback = theApp.mMenuCallback.Get( nID );
		if ( callback != NULL )
		{
			static CNSVector< CFBDataType > arg;
			arg.Clear( );
			arg.PushBack( CFBDataType( GetSafeHwnd( ) ) );
			arg.PushBack( CFBDataType( nID ) );
			luaStack->FireEvent( callback->mCallback, arg, callback->mStream );
			return TRUE;
		}
	}

	return CDialogEx::OnCmdMsg( nID, nCode, pExtra, pHandlerInfo );
}

// CdbtoolsDlg 消息处理程序
BOOL CdbtoolsDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	LPTSTR cmdLine = GetCommandLine( );
	CNSString cmd = CNSString::FromTChar( cmdLine );
	Common::ParseCommandLine( cmd );
	CNSString port = Common::GetCmdString( "port", "12009" );
	try
	{
		Common::LoadMachineTable( "" );
		Common::loadLocalization( "dbtoolsloc.xml" );
		Common::setLang( "ch" );

		CNSMap< CNSString, CNSString >& langInfo = Common::getLangInfo( );
		langMenu.CreatePopupMenu( );
		HLISTINDEX beginIndex = langInfo.GetHead( );
		for ( int i = 0; beginIndex != NULL; langInfo.GetNext( beginIndex ), i ++ )
		{
			CNSString name	= langInfo.GetKey( beginIndex );
			CNSString label	= langInfo.GetValue( beginIndex );

			langMenu.AppendMenu( MF_STRING, 35000 + i, CNSString::ToTChar( label ) );
			theApp.mLangMenuID.Insert( 35000 + i, name );
		}

		systemMenu.CreatePopupMenu( );
		systemMenu.AppendMenu( MF_STRING, ID_MENU_SHOWCONSOLE, _T("控制台(&C)...\tCtrl+F1") );
		systemMenu.AppendMenu( MF_POPUP | MF_STRING, (UINT_PTR) langMenu.GetSafeHmenu( ), _T("选择语言(&L)") );

		appMenu.CreateMenu( );
		appMenu.AppendMenu( MF_POPUP | MF_STRING, (UINT_PTR) systemMenu.GetSafeHmenu( ), _T("系统(&S)") );
		SetMenu( &appMenu );

		CListCtrl* list = (CListCtrl*) GetDlgItem( IDC_SERVER_LIST );
		list->SetExtendedStyle( LVS_EX_FULLROWSELECT );

		list->InsertColumn( 0, CdbtoolsApp::getUnicodeLangText( 1 ), LVCFMT_RIGHT, 100 );
		list->InsertColumn( 1, CdbtoolsApp::getUnicodeLangText( 2 ), LVCFMT_RIGHT, 100 );
		list->InsertColumn( 2, CdbtoolsApp::getUnicodeLangText( 3 ), LVCFMT_RIGHT, 100 );
		list->InsertColumn( 3, CdbtoolsApp::getUnicodeLangText( 4 ), LVCFMT_RIGHT, 100 );
		list->InsertColumn( 4, CdbtoolsApp::getUnicodeLangText( 5 ), LVCFMT_RIGHT, 100 );
		list->InsertColumn( 5, CdbtoolsApp::getUnicodeLangText( 6 ), LVCFMT_RIGHT, 300 );

		static CDBToolsLuaHandler	mLuaError;
		luaStack = InitLua( "script/dbtools", 0, &mLuaError );

		TiXmlDocument doc;
		if ( doc.LoadFile( "serverlist.xml" ) == false )
		{
			CString errorText;
			errorText.Format( CdbtoolsApp::getUnicodeLangText( 7 ), (LPCTSTR) CNSString::ToTChar( doc.ErrorDesc( ) ) );
			MessageBox( errorText );
			EndDialog( 0 );
			return TRUE;
		}

		TiXmlElement* region = doc.FirstChildElement( "Region" );
		for ( ; region != NULL; region = region->NextSiblingElement( "Region" ) )
		{
			CNSString regionName = region->Attribute( "Name" );
			TiXmlElement* group = region->FirstChildElement( "Group" );
			for ( int index = 0; group != NULL; group = group->NextSiblingElement( "Group" ), index ++ )
			{
				CNSString groupName = group->Attribute( "Name" );
				CNSString groupID	= group->Attribute( "GroupID" );
				list->InsertItem( index, CNSString::ToTChar( regionName ) );
				Common::setGroupID( groupID );
				CNSString innerIP = Common::Machine2IPAddress( "group:inner" );
				CNSString outerIP = Common::Machine2IPAddress( "group:outer" );
				list->SetItem( index, 1, LVIF_TEXT, CNSString::ToTChar( groupName ), 0, 0, 0, 0 );
				list->SetItem( index, 2, LVIF_TEXT, CNSString::ToTChar( groupID ), 0, 0, 0, 0 );
				list->SetItem( index, 3, LVIF_TEXT, CNSString::ToTChar( outerIP ), 0, 0, 0, 0 );
				list->SetItem( index, 4, LVIF_TEXT, CNSString::ToTChar( innerIP ), 0, 0, 0, 0 );
				list->SetItem( index, 5, LVIF_TEXT, _T(""), 0, 0, 0, 0 );
				CNSString dbName = CNSString( "dbtools:" ) + groupID;
				CGroupData& groupData = theApp.mGroupData.Insert( dbName, CGroupData( index, groupName, groupID ) );

				list->SetItemData( index, (DWORD_PTR) &groupData );
				static CDBToolEvent	dbEvent;
				InitializeMysqlEx( luaStack, dbName, innerIP + ":" + port, &dbEvent );
			}
		}
		RefreshUILang( );

		CNSVector< CFBDataType > arg;
		luaStack->FireEvent( "onLaunchApplication", arg );
	}
	catch( CNSException& e )
	{
		static CNSString errorDesc;
		errorDesc.Format( _UTF8( "error: %s\n行号: %d\n文件: %s\n函数: %s" ), e.mErrorDesc, e.mLineNumber, e.mpFileName, e.mpFuncName );
		MessageBox( CNSString::ToTChar( errorDesc ), _T("异常"), MB_OK | MB_ICONERROR );
		NSLog::FBLogFinialize( );
		NSNet::FBNetFinalize( );
		FinializeMysql( );
		PostQuitMessage( 0 );
		return TRUE;
	}
	// 将“关于...”菜单项添加到系统菜单中。
	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CdbtoolsDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CdbtoolsDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CdbtoolsDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CdbtoolsDlg::AppendMainMenu( unsigned int menuID, const CNSString& text, const CNSString& callback, CNSOctetsStream& stream )
{
	mpPopup->AppendMenu( MF_STRING | MF_BYCOMMAND, menuID, CNSString::ToTChar( text ) );
	CMenuCallback menuCallback;
	menuCallback.mCallback = callback;
	menuCallback.mStream = stream;
	theApp.mMenuCallback.Insert( menuID, menuCallback );
}

void CdbtoolsDlg::RegisterModule( const CNSString& name, const CNSString& desc )
{
	CModule mod;
	mod.mName = name;
	mod.mDesc = desc;
	theApp.mModuleList.PushBack( mod );
}

void CdbtoolsDlg::OnNMRClickServerList(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	*pResult = 0;

	if ( pNMItemActivate->iItem == -1 )
		return;

	CMenu menu;  
    menu.LoadMenu( IDR_MENU_MAIN );  
	mpPopup = menu.GetSubMenu( 0 );
	theApp.mMenuCallback.Clear( );

	static CNSVector< CFBDataType > arg;
	arg.Clear( );
	arg.PushBack( CFBDataType( GetSafeHwnd( ) ) );
	luaStack->FireEvent( "onPopupMainMenu", arg );

	CListCtrl* list = (CListCtrl*) GetDlgItem( IDC_SERVER_LIST );
	CPoint ptAction( pNMItemActivate->ptAction );
	list->ClientToScreen( &ptAction );

	CGroupData* data = (CGroupData*) list->GetItemData( pNMItemActivate->iItem );
	mpPopup->TrackPopupMenu( TPM_LEFTALIGN | TPM_LEFTBUTTON, ptAction.x, ptAction.y, this );
}

void CdbtoolsDlg::OnExecSql()
{
	CListCtrl* list = (CListCtrl*) GetDlgItem( IDC_SERVER_LIST );
	POSITION pos = list->GetFirstSelectedItemPosition( );
	int index = list->GetNextSelectedItem( pos );

	CGroupData* data = (CGroupData*) list->GetItemData( index );
	CExecSqlDlg dlg( data->mGroupName, data->mGroupID );
	dlg.DoModal( );
}

