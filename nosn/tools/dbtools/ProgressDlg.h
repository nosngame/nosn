#pragma once


// CProgressDlg �Ի���

class CProgressDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CProgressDlg)

public:
	CProgressDlg(CWnd* pParent = NULL);   // ��׼���캯��
	virtual ~CProgressDlg();

// �Ի�������
	enum { IDD = IDD_DIALOG_PROGRESS };
	void setProgress( const CString& title, float percent );

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��
	virtual BOOL PreTranslateMessage( MSG* pMsg );
	DECLARE_MESSAGE_MAP()
};
