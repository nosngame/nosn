#pragma once


// CConsoleDlg �Ի���

class CConsoleDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CConsoleDlg)

public:
	CConsoleDlg(CWnd* pParent = NULL);   // ��׼���캯��
	virtual ~CConsoleDlg();

// �Ի�������
	enum { IDD = IDD_DIALOG_CONSOLE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��
	virtual void OnCancel( );
	DECLARE_MESSAGE_MAP()
};
