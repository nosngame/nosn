// ProgressDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "dbtools.h"
#include "ProgressDlg.h"
#include "ProgressDlg.h"

// CProgressDlg 对话框

IMPLEMENT_DYNAMIC(CProgressDlg, CDialogEx)

CProgressDlg::CProgressDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CProgressDlg::IDD, pParent)
{

}

CProgressDlg::~CProgressDlg()
{
}

void CProgressDlg::setProgress( const CString& title, float percent )
{
	SetWindowText( title );
	SendDlgItemMessage( IDC_PROGRESS_VALUE, PBM_SETPOS, (WPARAM)( percent * 100 ), 0 );
}

void CProgressDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BOOL CProgressDlg::PreTranslateMessage( MSG* pMsg )
{
	if ( pMsg->message == WM_KEYDOWN && ( pMsg->wParam == VK_ESCAPE || pMsg->wParam == VK_RETURN ) )
		return TRUE;

	return CDialogEx::PreTranslateMessage( pMsg );	
}

BEGIN_MESSAGE_MAP(CProgressDlg, CDialogEx)
END_MESSAGE_MAP()


// CProgressDlg 消息处理程序
