#pragma once


// CConsoleDlg 对话框

class CConsoleDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CConsoleDlg)

public:
	CConsoleDlg(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CConsoleDlg();

// 对话框数据
	enum { IDD = IDD_DIALOG_CONSOLE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
	virtual void OnCancel( );
	DECLARE_MESSAGE_MAP()
};
