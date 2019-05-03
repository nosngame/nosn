#include "stdafx.h"
#include "dbtools.h"
#include "dbtoolsDlg.h"
#include "progressdlg.h"
#include "win32_interface.h"

CProgressDlg*					CDialogBase::progressDlg	= NULL;
CNSMap< HWND, CNSOctetsStream > CDialogBase::wndParams;
HWND lua_tohwnd( lua_State* lua, unsigned int index, const CString& classType )
{
	HWND parent				= (HWND) lua_touserdata( lua, index );
	unsigned int ctrlID		= (unsigned int) luaL_checknumber( lua, index + 1 );
	if ( IsWindow( parent ) == FALSE )
	{
		static CNSString errorDesc;
		errorDesc.Format( _UTF8( "父窗口不是有效的窗口句柄" ) );
		FBException( errorDesc );
	}
	
	HWND wnd				= GetDlgItem( parent, ctrlID );
	if ( wnd == NULL || IsWindow( wnd ) == FALSE )
	{
		static CNSString errorDesc;
		errorDesc.Format( _UTF8( "控件ID[%d] 不存在" ), ctrlID );
		FBException( errorDesc );
	}

	TCHAR className[ 32 ];
	GetClassName( wnd, className, 32 );
	if ( classType.IsEmpty( ) == false && classType != className )
	{
		static CNSString errorDesc;
		errorDesc.Format( _UTF8( "控件ID[%d] 不是[%s]类型" ), ctrlID, classType );
		FBException( errorDesc );
	}

	return wnd;
}

CRect lua_torect( lua_State* lua, unsigned int index )
{
	CRect rc;
	int left, top, width, height = 0;
	lua_pushnil( lua );  // nil 入栈作为初始 key
	while( lua_next( lua, index ) != 0 ) 
	{
		unsigned int value	= (unsigned int) luaL_checknumber( lua, -1 );
		const char* key		= luaL_checkstring( lua, -2 );
		CNSString member( key );
		if ( member == "x" )
			left = value;
		else if ( member == "y" )
			top = value;
		else if ( member == "width" )
			width = value;
		else if ( member == "height" )
			height = value;
		else
		{
			static CNSString errorDesc;
			errorDesc.Format( _UTF8( "索引 - %d, 不是一个有效的Rect表, 无效字段 - %s" ), index, key );
			FBException( errorDesc );
		}

		lua_pop( lua, 1 );
	}
	
	rc.SetRect( left, top, left + width, top + height );
	return rc;
}

void windowText( HWND wnd, TCHAR* text, int size )
{
	int bytesWritten = ::GetWindowText( wnd, text, size );
	if ( bytesWritten == 0 )
	{
		DWORD errorCode = ::GetLastError( );
		if ( errorCode != 0 )
		{
			static CNSString errorDesc;
			errorDesc.Format( _UTF8( "Win32函数[GetWindowText] 调用失败, 错误码 - %d" ), errorCode );
			FBException( errorDesc );
		}
	}
}

CDialogBase::CDialogBase( bool modeless, const CNSString& windowProc, const CNSString& preWinProc, const CNSOctetsStream& stream ) 
	: CDialog( ), mIsModeless( modeless ), mIsFrameWnd( false ), mWindowProc( windowProc ), mPreWinProc( preWinProc ), mStream( stream )
{
}

BOOL CDialogBase::OnInitDialog( )
{
	if ( GetParent( ) == NULL || GetParent( )->GetSafeHwnd( ) == AfxGetMainWnd( )->GetSafeHwnd( ) )
		mIsFrameWnd = true;

	wndParams.Insert( GetSafeHwnd( ), mStream );
	return CDialog::OnInitDialog( );
}

void CDialogBase::OnCancel( )
{
	if ( mIsModeless == true )
	{
		DestroyWindow( );
		return;
	}

	return CDialog::OnCancel( );
}

void CDialogBase::PostNcDestroy( )
{
	if ( mIsFrameWnd == true && progressDlg != NULL )
	{
		delete progressDlg;
		progressDlg = NULL;
	}

	wndParams.Erase( GetSafeHwnd( ) );
	CDialog::PostNcDestroy( );

	if ( mIsModeless == true )
		delete this;
}

BOOL CDialogBase::PreTranslateMessage( MSG* msg )
{
	try
	{
		bool retValue = false;
		if ( mPreWinProc.IsEmpty( ) == false )
		{
			static CNSVector< CFBDataType > arg;
			arg.Clear( );
			arg.PushBack( CFBDataType( (void*) GetSafeHwnd( ) ) );
			arg.PushBack( CFBDataType( (void*) msg->hwnd ) );
			arg.PushBack( CFBDataType( msg->message ) );
			arg.PushBack( CFBDataType( (intptr_t) msg->lParam ) );
			arg.PushBack( CFBDataType( (intptr_t) msg->wParam ) );
			CFBDataType result = luaStack->FireEvent( mPreWinProc, arg );
			if ( result.mDataType == CFBDataType::TYPE_BOOLEAN )
				retValue = result.mBoolValue;
			else
			{
				static CNSString errorDesc;
				errorDesc.Format( _UTF8( "C++ 调用lua 错误:\n \t lua函数[%s] 返回值类型错误，只能是boolean" ), mPreWinProc.GetBuffer( ) );
				FBException( errorDesc );
			}
		}

		if ( retValue == true )
			return TRUE;
	}
	catch( CNSException& e )
	{
		static CNSString errorDesc;
		errorDesc.Format( _UTF8( "error: %s\n行号: %d\n文件: %s\n函数: %s" ), e.mErrorDesc, e.mLineNumber, e.mpFileName, e.mpFuncName );
		NSLog::exception( errorDesc.GetBuffer( ) );
	}

	return CDialog::PreTranslateMessage( msg );
}

LRESULT CDialogBase::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	try
	{
		if ( mWindowProc.IsEmpty( ) == false )
		{
			static CNSVector< CFBDataType > arg;
			arg.Clear( );
			arg.PushBack( CFBDataType( (void*) GetSafeHwnd( ) ) );
			arg.PushBack( CFBDataType( message ) );
			arg.PushBack( CFBDataType( (intptr_t) lParam ) );
			arg.PushBack( CFBDataType( (intptr_t) wParam ) );
			CFBDataType result = luaStack->FireEvent( mWindowProc, arg );
			if ( result.mDataType == CFBDataType::TYPE_NONE )
				return CDialog::WindowProc( message, wParam, lParam );
			else if ( result.mDataType == CFBDataType::TYPE_DOUBLE )
				return (LRESULT) result.mDoubleValue;
			else if ( result.mDataType == CFBDataType::TYPE_BOOLEAN )
				return (LRESULT) result.mBoolValue;
			else
			{
				static CNSString errorDesc;
				errorDesc.Format( _UTF8( "C++ 调用lua 错误:\n \t lua函数[%s] 返回值类型错误，只能是number 或者 boolean" ), mWindowProc.GetBuffer( ) );
				FBException( errorDesc );
			}
		}
	}
	catch( CNSException& e )
	{
		static CNSString errorDesc;
		errorDesc.Format( _UTF8( "error: %s\n行号: %d\n文件: %s\n函数: %s" ), e.mErrorDesc, e.mLineNumber, e.mpFileName, e.mpFuncName );
		NSLog::exception( errorDesc.GetBuffer( ) );
	}

	return CDialog::WindowProc( message, wParam, lParam );
}
