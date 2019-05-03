#pragma once
class CExecSqlDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CExecSqlDlg)
public:
	CNSString		mGroupID;
	CNSString		mGroupName;
	unsigned int	mSqlTick;

public:
	CExecSqlDlg( const CNSString& groupName, const CNSString& groupID, CWnd* pParent = NULL );   // 标准构造函数
	virtual ~CExecSqlDlg( );

// 对话框数据
	enum { IDD = IDD_DIALOG_EXECSQL };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
	virtual BOOL OnInitDialog( );

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedButtonExesql();

public:
	static void OnExecSqlCommand( const CNSString& name, TSessionID sessionID, CProtocolExecuteSqlResponse* pProtocol, CNSOctetsStream& stream );
	afx_msg void OnNMDblclkListResult(NMHDR *pNMHDR, LRESULT *pResult);
};
