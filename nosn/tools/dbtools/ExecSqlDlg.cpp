// ExecSqlDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "dbtools.h"
#include "afxdialogex.h"
#include "ExecSqlDlg.h"
#include "DataViewDlg.h"

// CExecSqlDlg 对话框

IMPLEMENT_DYNAMIC(CExecSqlDlg, CDialogEx)

CExecSqlDlg::CExecSqlDlg(const CNSString& groupName, const CNSString& groupID, CWnd* pParent /*=NULL*/)
	: CDialogEx(CExecSqlDlg::IDD, pParent), mGroupName( groupName ), mGroupID( groupID ), mSqlTick( 0 )
{

}

CExecSqlDlg::~CExecSqlDlg()
{
}

void CExecSqlDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CExecSqlDlg, CDialogEx)
	ON_BN_CLICKED(IDC_BUTTON_EXESQL, &CExecSqlDlg::OnBnClickedButtonExesql)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST_RESULT, &CExecSqlDlg::OnNMDblclkListResult)
END_MESSAGE_MAP()

BOOL CExecSqlDlg::OnInitDialog( )
{
	CDialogEx::OnInitDialog( );
	CString title;
	CString groupName	= CNSString::ToTChar( mGroupName );
	CString groupID		= CNSString::ToTChar( mGroupID );
	title.Format( CdbtoolsApp::getUnicodeLangText( 13 ), (LPCTSTR) groupName, (LPCTSTR) groupID );
	SetWindowText( title );
	return TRUE;
}

// CExecSqlDlg 消息处理程序
void CExecSqlDlg::OnExecSqlCommand( const CNSString& name, TSessionID sessionID, CProtocolExecuteSqlResponse* pProtocol, CNSOctetsStream& stream )
{
	unsigned int dlgValue = 0;
	stream >> (unsigned int) dlgValue;
	CExecSqlDlg* dlg = (CExecSqlDlg*) dlgValue;
	if ( pProtocol->mResultCode == 0 )
	{
		dlg->MessageBox( CNSString::ToTChar( pProtocol->mErrorDesc ), CdbtoolsApp::getUnicodeLangText( 12 ), MB_OK | MB_ICONERROR );
		return;
	}

	unsigned int cost = CNSTimer::getCurTick( ) - dlg->mSqlTick;
	CStatic* costWnd = (CStatic*) dlg->GetDlgItem( IDC_STATIC_COST );
	CString execCost;
	execCost.Format( CdbtoolsApp::getUnicodeLangText( 14 ), cost );
	costWnd->SetWindowText( execCost );

	CListCtrl* list = (CListCtrl*) dlg->GetDlgItem( IDC_LIST_RESULT );
	list->DeleteAllItems( );
	while ( list->DeleteColumn( 0 ) );

	list->SetExtendedStyle( LVS_EX_FULLROWSELECT );

	for ( size_t i = 0; i < pProtocol->mFieldInfo.GetCount( ); i ++ )
		list->InsertColumn( i, CNSString::ToTChar( pProtocol->mFieldInfo[ i ] ), LVCFMT_RIGHT, 100 );

	for ( size_t row = 0; row < pProtocol->GetRowCount( ); row ++ )
	{
		for ( size_t col = 0; col < pProtocol->mFieldInfo.GetCount( ); col ++ )
		{
			int dataType = pProtocol->GetRawType( col, row );
			CNSString value;
			switch( dataType )
			{
				case TYPE_LONGLONG:
					value = CNSString::Number2String( pProtocol->GetInt64Value( col, row ) );
					break;
				case TYPE_LONG:
					value = CNSString::Number2String( pProtocol->GetIntValue( col, row ) );
					break;
				case TYPE_FLOAT:
					value = CNSString::Number2String( pProtocol->GetFloatValue( col, row ) );
					break;
				case TYPE_DOUBLE:
					value = CNSString::Number2String( pProtocol->GetDoubleValue( col, row ) );
					break;
				case TYPE_STRING:
				case TYPE_VAR_STRING:
				case TYPE_VARCHAR:
					value = pProtocol->GetStringValue( col, row );
					break;
				case TYPE_BLOB:
				case TYPE_TINY_BLOB:
				case TYPE_MEDIUM_BLOB:
				case TYPE_LONG_BLOB:
				{
					const CNSOctets data = pProtocol->GetRaw( col, row );
					for ( size_t i = 0; i < data.Length( ); i ++ )
					{
						CNSString dataValue;
						dataValue.Format( "%02x", *( (unsigned char*) data.Begin( ) + i ) );
						if ( i < data.Length( ) - 1 )
							dataValue += "-";
						value += dataValue;
					}
					break;
				}
			}

			if ( col == 0 )
				list->InsertItem( row, CNSString::ToTChar( value ), 0 );
			else
				list->SetItem( row, col, LVIF_TEXT, CNSString::ToTChar( value ), 0, 0, 0, 0 );
		}
	}
}

void CExecSqlDlg::OnBnClickedButtonExesql( )
{
	CEdit* edit = (CEdit*) GetDlgItem( IDC_EDIT_SQLCOMMAND );
	CEdit* editTimes = (CEdit*) GetDlgItem( IDC_EDIT_TIMES );
	CString times;
	editTimes->GetWindowText( times );

	CString sqlCmd;
	edit->GetWindowText( sqlCmd );

	// 傻逼MFC的CString, 没有获得字节长度函数
	CNSString tSqlCommand = CNSString::FromTChar( sqlCmd.GetBuffer( ) );

	static CNSOctetsStream stream;
	stream << (size_t) this;

	static CNSOctets buffer;
	mSqlTick = CNSTimer::getCurTick( );
	for ( int i = 0; i < _wtoi( times.GetString( ) ); i ++ )
		ExecuteSqlEx( CNSString( "dbtools:" ) + mGroupID, CSession( CExecSqlDlg::OnExecSqlCommand, stream ), buffer, tSqlCommand, false, true );
}

void CExecSqlDlg::OnNMDblclkListResult(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	*pResult = 0;

	CListCtrl* list = (CListCtrl*) GetDlgItem( IDC_LIST_RESULT );
	CString data = list->GetItemText( pNMItemActivate->iItem, pNMItemActivate->iSubItem );
	CDataViewDlg dlg( data );
	dlg.DoModal( );
}
