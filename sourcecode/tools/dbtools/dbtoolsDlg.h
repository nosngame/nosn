#pragma once
class CdbtoolsDlg;
class CDBToolsLuaHandler : public CFBLuaEvent
{
public:
	CDBToolsLuaHandler( )
	{
	}

public:
	virtual void onError( const CNSString& errorText );
	virtual void onOpenScript( const CNSString& fileName, const char* buffer, unsigned int size );
	virtual void onRegister( CNSLuaStack* luaStack );
};

class CDBToolEvent : public CDBEvent
{
public:
	CDBToolEvent( )
	{
	}

public:
	virtual void OnAddSession( CDBClient* dbClient, const CNSString& rName, const CNSSockAddr& rLocal, const CNSSockAddr& rPeer, TSessionID vSessionID );
	virtual void OnAddSessionFault( CDBClient* dbClient, const CNSString& rName, int vCode );
	virtual void OnDelSession( CDBClient* dbClient, const CNSString& rName, TSessionID vSessionID );
	virtual void OnError( CDBClient* dbClient, const CNSString& error );
	virtual void OnPreSql( CDBClient* dbClient, const CNSString& sqlCommand, bool fullInfo );
	virtual void OnConnectDB( CDBClient* dbClient );
};

// CdbtoolsDlg 对话框
class CdbtoolsDlg : public CDialogEx
{
	friend class CDBToolEvent;

// 构造
public:
	CdbtoolsDlg(CWnd* pParent = NULL);	// 标准构造函数
	virtual ~CdbtoolsDlg( );
// 对话框数据
	enum { IDD = IDD_DBTOOLS_DIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持

public:
	void AppendMainMenu( unsigned int menuID, const CNSString& text, const CNSString& callback, CNSOctetsStream& stream );
	void RegisterModule( const CNSString& name, const CNSString& desc );

// 实现
protected:
	CMenu*				mpPopup;
	HICON				m_hIcon;
	CMenu				appMenu;
	CMenu				systemMenu;
	CMenu				langMenu;
	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	virtual BOOL OnCmdMsg( UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo );
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnInitMenu( CMenu* menu );

	void RefreshUILang( );
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnNMRClickServerList(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnExecSql( );
	afx_msg void OnBnClickedButtonClearlog();
};

extern CNSLuaStack*		luaStack;
