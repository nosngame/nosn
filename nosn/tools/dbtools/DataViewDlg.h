#pragma once


// CDataViewDlg �Ի���

class CDataViewDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CDataViewDlg)
	CString mData;

public:
	CDataViewDlg( const CString& data, CWnd* pParent = NULL );   // ��׼���캯��
	virtual ~CDataViewDlg();

// �Ի�������
	enum { IDD = IDD_DIALOG_DATAVIEW };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��
	virtual BOOL OnInitDialog( );

	DECLARE_MESSAGE_MAP()
};
