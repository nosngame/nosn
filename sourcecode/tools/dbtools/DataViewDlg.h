#pragma once


// CDataViewDlg 对话框

class CDataViewDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CDataViewDlg)
	CString mData;

public:
	CDataViewDlg( const CString& data, CWnd* pParent = NULL );   // 标准构造函数
	virtual ~CDataViewDlg();

// 对话框数据
	enum { IDD = IDD_DIALOG_DATAVIEW };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
	virtual BOOL OnInitDialog( );

	DECLARE_MESSAGE_MAP()
};
