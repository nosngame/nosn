// ConsoleDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "dbtools.h"
#include "ConsoleDlg.h"
#include "afxdialogex.h"


// CConsoleDlg �Ի���

IMPLEMENT_DYNAMIC(CConsoleDlg, CDialogEx)

CConsoleDlg::CConsoleDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CConsoleDlg::IDD, pParent)
{

}

CConsoleDlg::~CConsoleDlg()
{
}

void CConsoleDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CConsoleDlg, CDialogEx)
	ON_WM_CLOSE( )
END_MESSAGE_MAP()

void CConsoleDlg::OnCancel( )
{
	DestroyWindow( );
}
