// DataViewDlg.cpp : 实现文件
//
#include "stdafx.h"
#include "dbtools.h"
#include "DataViewDlg.h"
#include "afxdialogex.h"


// CDataViewDlg 对话框

IMPLEMENT_DYNAMIC(CDataViewDlg, CDialogEx)

CDataViewDlg::CDataViewDlg( const CString& data, CWnd* pParent /*=NULL*/)
: CDialogEx(CDataViewDlg::IDD, pParent), mData( data )
{
}

CDataViewDlg::~CDataViewDlg()
{
}

void CDataViewDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CDataViewDlg, CDialogEx)
END_MESSAGE_MAP()

BOOL CDataViewDlg::OnInitDialog( )
{
	CDialogEx::OnInitDialog( );

	CEdit* valueEdit = (CEdit*) GetDlgItem( IDC_EDIT_VALUE );
	valueEdit->SetWindowText( mData );
	return TRUE;
}

// CDataViewDlg 消息处理程序
