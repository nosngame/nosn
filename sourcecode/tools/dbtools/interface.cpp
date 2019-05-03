#include "stdafx.h"
#include "dbtools.h"
#include "dbtoolsDlg.h"
#include "progressdlg.h"

#include "interface.h"
#include "win32_interface.h"
#include "treeview_interface.h"
#include "listview_interface.h"
#include "combobox_interface.h"
#include "edit_interface.h"
#include "static_interface.h"
#include "button_interface.h"

#define RT_DIALOG				MAKEINTRESOURCE( 5 )
#define WC_WAITING              _T("Waiting")
#define PI 3.141592653f
#define DEG2RAD( X ) ( ( ( X ) / 180.0f ) * PI )
#define RAD2DEG( X ) ( ( ( X ) / PI ) * 180.0f )

static int messageBox( lua_State* lua );
static int doModal( lua_State* lua );
static int createWindow( lua_State* lua );
static int getWindowData( lua_State* lua );
static int destroyWindow( lua_State* lua );
static int enableWindow( lua_State* lua );
static int getWindowRect( lua_State* lua );
static int clientToScreen( lua_State* lua );
static int moveWindow( lua_State* lua );
static int showWindow( lua_State* lua );
static int setWindowText( lua_State* lua );
static int getWindowText( lua_State* lua );
static int delayActive( lua_State* lua );
static int appendMenu( lua_State* lua );
static int trace( lua_State* lua );
static int progress( lua_State* lua );
static int endProgress( lua_State* lua );
static int registerModule( lua_State* lua );
static int getDlgCtrlID( lua_State* lua );
static int getDBSelection( lua_State* lua );
static int getNMHdr( lua_State* lua );
static int alignWindow( lua_State* lua );
static int getChildHwnd( lua_State* lua );
static int waiting( lua_State* lua );
static int endWaiting( lua_State* lua );
static int getLang( lua_State* lua );

BEGIN_EXPORT( System )
	EXPORT_FUNC( messageBox )
	EXPORT_FUNC( doModal )
	EXPORT_FUNC( createWindow )
	EXPORT_FUNC( getChildHwnd )
	EXPORT_FUNC( getWindowData )
	EXPORT_FUNC( destroyWindow )
	EXPORT_FUNC( enableWindow )
	EXPORT_FUNC( getWindowRect )
	EXPORT_FUNC( clientToScreen )
	EXPORT_FUNC( moveWindow )
	EXPORT_FUNC( showWindow )
	EXPORT_FUNC( setWindowText )
	EXPORT_FUNC( delayActive )
	EXPORT_FUNC( appendMenu )
	EXPORT_FUNC( trace )
	EXPORT_FUNC( registerModule )
	EXPORT_FUNC( progress )
	EXPORT_FUNC( endProgress )
	EXPORT_FUNC( getDlgCtrlID )
	EXPORT_FUNC( getDBSelection )
	EXPORT_FUNC( getNMHdr )
	EXPORT_FUNC( alignWindow )
	EXPORT_FUNC( waiting )
	EXPORT_FUNC( endWaiting )
	EXPORT_FUNC( getLang )
END_EXPORT

class CWaitingData
{
public:
	float x1;
	float y1;
	float x2;
	float y2;
	float angle;
	float speed;
	HDC memDC;
	HBITMAP memBmp;
	HBITMAP preBmp;

public:
	CWaitingData( ) : x1( 0.0f ), y1( 0.0f ), x2( 0.0f ), y2( 0.0f ), angle( 0.0f ), speed( 400.0f ), memDC( NULL ), memBmp( NULL ), preBmp( NULL )
	{
	}

	void update( unsigned int delta )
	{
		angle = angle + speed * delta / 1000.0f;
		x1 = cosf( DEG2RAD( angle ) ) * 30.0f;
		y1 = sinf( DEG2RAD( angle ) ) * 30.0f;
		x2 = cosf( DEG2RAD( angle - 90 ) ) * 30.0f;
		y2 = sinf( DEG2RAD( angle - 90 ) ) * 30.0f;
	}
};

bool sInitWndClass	= false;
HWND waitWnd		= NULL;
HPEN borderPen1		= NULL;
HPEN borderPen2		= NULL;
LRESULT CALLBACK WaitingProc( HWND wnd, UINT message, WPARAM wParam, LPARAM lParam )
{
	switch( message )
	{
	case WM_CREATE:
		{
			CREATESTRUCT* cs = (CREATESTRUCT*) lParam;
			CRect rcBorder;
			GetClientRect( wnd, &rcBorder );

			CWaitingData* waiting = (CWaitingData*) cs->lpCreateParams;
			waiting->memDC	= CreateCompatibleDC( GetDC( wnd ) );
			waiting->memBmp	= CreateCompatibleBitmap( GetDC( wnd ), rcBorder.Width( ), rcBorder.Height( ) );
			waiting->preBmp	= (HBITMAP) SelectObject( waiting->memDC, waiting->memBmp );   
			SetTimer( wnd, 0, 1, NULL );
			SetWindowLongPtr( wnd, GWLP_USERDATA, (LONG_PTR) waiting );
			return ::DefWindowProc( wnd, message, wParam, lParam );
		}
	case WM_DESTROY:
		{
			CWaitingData* waiting = (CWaitingData*) GetWindowLongPtr( wnd, GWLP_USERDATA );
			SelectObject( waiting->memDC, waiting->preBmp);  
			DeleteObject( waiting->memBmp );
			DeleteDC( waiting->memDC );
			delete waiting;
			return ::DefWindowProc( wnd, message, wParam, lParam );
		}
	case WM_TIMER:
		{
			CWaitingData* waiting = (CWaitingData*) GetWindowLongPtr( wnd, GWLP_USERDATA );
			unsigned int thisTick = CNSTimer::getCurTick( );
			static unsigned int mLastTick = thisTick;
			waiting->update( thisTick - mLastTick );
			mLastTick = thisTick;
			::InvalidateRect( wnd, NULL, FALSE );
		}
		break;
	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			BeginPaint( wnd, &ps );
			CRect rcBorder( ps.rcPaint );
			CWaitingData* waiting = (CWaitingData*) GetWindowLongPtr( wnd, GWLP_USERDATA );

			FillRect( waiting->memDC, &rcBorder, GetStockBrush( WHITE_BRUSH ) );
			HBRUSH oldBrush = SelectBrush( waiting->memDC, GetStockBrush( NULL_BRUSH ) );
			float centerX = rcBorder.left + rcBorder.Width( ) / 2.0f;
			float centerY = rcBorder.top + rcBorder.Height( ) / 2.0f;
			HPEN oldPen = SelectPen( waiting->memDC, borderPen1 );
			Ellipse( waiting->memDC, (int) centerX - 15, (int) centerY - 15, (int) centerX + 15, (int) centerY + 15 );
			SelectPen( waiting->memDC, oldPen );

			oldPen = SelectPen( waiting->memDC, borderPen2 );
			Arc( waiting->memDC, (int) centerX - 15, (int) centerY - 15, (int) centerX + 15, (int) centerY + 15, (int)( centerX + waiting->x1 ), (int)( centerY + waiting->y1 ), (int)( centerX + waiting->x2 ), (int)( centerY + waiting->y2 ) );
			SelectPen( waiting->memDC, oldPen );
			SelectBrush( waiting->memDC, oldBrush );

			BitBlt( ps.hdc, 0, 0, rcBorder.right, rcBorder.bottom, waiting->memDC, 0, 0, SRCCOPY );
			EndPaint( wnd, &ps );
			return 0;
		}
	default: //缺省时采用系统消息缺省处理函数
		return DefWindowProc( wnd, message, wParam, lParam );
	}
	return 0;
}

class CWindowMove
{
	HWND		mMoveWindow;
	CFBVector2	mTarget;
	CFBVector2	mPosition;
	float		mCurSpeed;
	float		mCurAcc;
	float		mHalfDur;
	float		mDuration;
	int			mState;
	int			mWidth;
	int			mHeight;
	DWORD		mLastTime;
	static CNSVector< CWindowMove > sMoveList;

public:
	CWindowMove( HWND wnd, const CFBVector2& target, float duration ) : mMoveWindow( wnd ), mTarget( target ), mCurSpeed( 0 ), mHalfDur( duration / 2.0f ), mDuration( duration ), mState( 0 ), mLastTime( CNSTimer::getCurTick( ) )
	{
		CRect rc;
		GetWindowRect( mMoveWindow, rc );
		mPosition.setX( (float) rc.left );
		mPosition.setZ( (float) rc.top );
		mCurAcc = ( mTarget - mPosition ).magnitude( ) / ( ( mDuration / 2.0f ) * mDuration );

		mWidth	= rc.Width( );
		mHeight	= rc.Height( );
	}

	bool update( unsigned int curTick )
	{
		float timeDelta = ( curTick - mLastTime ) / 1000.0f;
		mLastTime = curTick;

		float timeDeltaReal = min( mHalfDur, timeDelta );
		if ( mState == 0 )
		{
			mHalfDur = mHalfDur - timeDeltaReal;
			if ( mHalfDur <= 0.0f )
			{
				mState = 1;
				mHalfDur = mDuration / 2.0f;
			}
		}
		else if ( mState == 1 )
		{	
			mHalfDur = mHalfDur + timeDeltaReal;
			if ( mHalfDur <= 0.0f )
				mState = 2;
		}
		
		mCurSpeed = max( 100.0f, mCurSpeed + mCurAcc * timeDelta );

		CFBVector2 offset = mTarget - mPosition;
		CFBVector2 delta = offset.normalize( ) * mCurSpeed * timeDelta;
		if ( offset.magnitude( ) <= delta.magnitude( ) || mState == 2 )
		{
			::MoveWindow( mMoveWindow, mTarget.getIntX( ), mTarget.getIntZ( ), mWidth, mHeight, FALSE );
			KillTimer( mMoveWindow, 0 );
			return false;
		}

		mPosition = delta + mPosition;
		::MoveWindow( mMoveWindow, mPosition.getIntX( ), mPosition.getIntZ( ), mWidth, mHeight, FALSE );
		return true;
	}

public:
	static void addMove( HWND wnd, const CFBVector2& target, float duration )
	{
		CWindowMove move( wnd, target, duration );
		sMoveList.PushBack( move );
	}

	static void updateMove( unsigned int curTime )
	{
		for ( size_t i = sMoveList.GetCount( ) - 1; i >= 0; i -- )
		{
			if ( sMoveList[ i ].update( curTime ) == false )
				sMoveList.Erase( i );
		}
	}
};

CNSVector< CWindowMove > CWindowMove::sMoveList;
void __stdcall MoveTimer( HWND wnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime )
{
	unsigned int curTick = CNSTimer::getCurTick( );
	CWindowMove::updateMove( curTick );
}

void __stdcall ActiveTimer( HWND wnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime )
{
	::SetActiveWindow( wnd );
	KillTimer( wnd, 1 );
}

static int setWindowText( lua_State* lua )
{
	DECLARE_BEGIN_PROTECTED
	HWND		wnd		= (HWND) lua_touserdata( lua, 1 );
	const char* text	= luaL_checkstring( lua, 2 );
	if ( IsWindow( wnd ) == FALSE )
	{
		static CNSString errorDesc;
		errorDesc.Format( _UTF8( "指定窗口不是有效的窗口句柄" ) );
		FBException( errorDesc );
	}

	SetWindowText( wnd, CNSString::ToTChar( text ) );
	DECLARE_END_PROTECTED
	return 0;
}

static int getWindowText( lua_State* lua )
{
	DECLARE_BEGIN_PROTECTED
	HWND		wnd		= (HWND) lua_touserdata( lua, 1 );
	if ( IsWindow( wnd ) == FALSE )
	{
		static CNSString errorDesc;
		errorDesc.Format( _UTF8( "指定窗口不是有效的窗口句柄" ) );
		FBException( errorDesc );
	}

	TCHAR text[ 256 ];
	windowText( wnd, text, 256 );
	lua_pushstring( lua, CNSString::FromTChar( text ) );
	DECLARE_END_PROTECTED
	return 1;
}

static int getLang( lua_State* lua )
{
	DECLARE_BEGIN_PROTECTED
	unsigned int	textID	= lua_tounsigned( lua, 1 );
	CNSString		text	= Common::getLangText( textID );
	lua_pushstring( lua, text.GetBuffer( ) );
	DECLARE_END_PROTECTED
	return 1;
}

static int showWindow( lua_State* lua )
{
	DECLARE_BEGIN_PROTECTED
	HWND wnd	= (HWND) lua_touserdata( lua, 1 );
	if ( IsWindow( wnd ) == FALSE )
	{
		static CNSString errorDesc;
		errorDesc.Format( _UTF8( "指定窗口不是有效的窗口句柄" ) );
		FBException( errorDesc );
	}

	bool show	= lua_toboolean( lua, 2 );
	if ( show == true )
		ShowWindow( wnd, SW_SHOW );
	else
		ShowWindow( wnd, SW_HIDE );
	DECLARE_END_PROTECTED
	return 0;
}

static int moveWindow( lua_State* lua )
{
	DECLARE_BEGIN_PROTECTED
	HWND wnd		= (HWND) lua_touserdata( lua, 1 );
	int x			= (int) luaL_checkinteger( lua, 2 );
	int y			= (int) luaL_checkinteger( lua, 3 );
	float duration	= (float) luaL_checknumber( lua, 4 );
	bool ani		= lua_toboolean( lua, 5 );
	if ( IsWindow( wnd ) == FALSE )
	{
		static CNSString errorDesc;
		errorDesc.Format( _UTF8( "指定窗口不是有效的窗口句柄" ) );
		FBException( errorDesc );
	}

	CRect rc;
	GetWindowRect( wnd, &rc );
	if ( ani == false )
	{
		::MoveWindow( wnd, x, y, rc.Width( ), rc.Height( ), FALSE );
		return 0;
	}
	
	if ( rc.left == x && rc.top == y )
		return 0;

	CWindowMove::addMove( wnd, CFBVector2( (float) x, (float) y ), duration );
	SetTimer( wnd, 0, 10, MoveTimer );
	DECLARE_END_PROTECTED
	return 0;
}

static int delayActive( lua_State* lua )
{
	DECLARE_BEGIN_PROTECTED
	HWND wnd	= (HWND) lua_touserdata( lua, 1 );
	if ( IsWindow( wnd ) == FALSE )
	{
		static CNSString errorDesc;
		errorDesc.Format( _UTF8( "指定窗口不是有效的窗口句柄" ) );
		FBException( errorDesc );
	}

	SetTimer( wnd, 1, 10, ActiveTimer );
	DECLARE_END_PROTECTED
	return 0;
}

static int enableWindow( lua_State* lua )
{
	DECLARE_BEGIN_PROTECTED
	HWND wnd	= (HWND) lua_touserdata( lua, 1 );
	bool enable = lua_toboolean( lua, 2 );
	if ( IsWindow( wnd ) == FALSE )
	{
		static CNSString errorDesc;
		errorDesc.Format( _UTF8( "指定窗口不是有效的窗口句柄" ) );
		FBException( errorDesc );
	}

	EnableWindow( wnd, enable );
	DECLARE_END_PROTECTED
	return 0;
}

static int destroyWindow( lua_State* lua )
{
	DECLARE_BEGIN_PROTECTED
	HWND wnd	= (HWND) lua_touserdata( lua, 1 );
	if ( IsWindow( wnd ) == FALSE )
	{
		static CNSString errorDesc;
		errorDesc.Format( _UTF8( "指定窗口不是有效的窗口句柄" ) );
		FBException( errorDesc );
	}

	PostMessage( wnd, WM_CLOSE, 0, 0 );
	DECLARE_END_PROTECTED
	return 0;
}

static int getWindowRect( lua_State* lua )
{
	DECLARE_BEGIN_PROTECTED
	HWND wnd	= (HWND) lua_touserdata( lua, 1 );
	if ( IsWindow( wnd ) == FALSE )
	{
		static CNSString errorDesc;
		errorDesc.Format( _UTF8( "指定窗口不是有效的窗口句柄" ) );
		FBException( errorDesc );
	}

	CRect rc;
	GetWindowRect( wnd, &rc );
	CNSLuaStack*	luaStack	= CNSLuaStack::FromState( lua );
	luaStack->BeginTable( );
	luaStack->LuaPushField( "x", CFBDataType( (int) rc.left ) );
	luaStack->LuaPushField( "y", CFBDataType( (int) rc.top ) );
	luaStack->LuaPushField( "width", CFBDataType( (int) rc.Width( ) ) );
	luaStack->LuaPushField( "height", CFBDataType( (int) rc.Height( ) ) );
	DECLARE_END_PROTECTED
	return 1;
}

static int clientToScreen( lua_State* lua )
{
	DECLARE_BEGIN_PROTECTED
	HWND wnd	= (HWND) lua_touserdata( lua, 1 );
	if ( IsWindow( wnd ) == FALSE )
	{
		static CNSString errorDesc;
		errorDesc.Format( _UTF8( "指定窗口不是有效的窗口句柄" ) );
		FBException( errorDesc );
	}

	CRect rc = lua_torect( lua, 2 );
	::ClientToScreen( wnd, &rc.TopLeft( ) );
	::ClientToScreen( wnd, &rc.BottomRight( ) );

	CNSLuaStack*	luaStack	= CNSLuaStack::FromState( lua );
	luaStack->BeginTable( );
	luaStack->LuaPushField( "x", CFBDataType( (int) rc.left ) );
	luaStack->LuaPushField( "y", CFBDataType( (int) rc.top ) );
	luaStack->LuaPushField( "width", CFBDataType( (int) rc.Width( ) ) );
	luaStack->LuaPushField( "height", CFBDataType( (int) rc.Height( ) ) );
	DECLARE_END_PROTECTED
	return 1;
}

static int waiting( lua_State* lua )
{
	DECLARE_BEGIN_PROTECTED
	HWND parent	= (HWND) lua_touserdata( lua, 1 );
	if ( IsWindow( parent ) == FALSE )
	{
		static CNSString errorDesc;
		errorDesc.Format( _UTF8( "指定窗口不是有效的窗口句柄" ) );
		FBException( errorDesc );
	}

	if ( sInitWndClass == false )
	{
		HBRUSH waitBrush	= CreateSolidBrush( RGB( 255, 255, 255 ) );
		borderPen1			= CreatePen( PS_SOLID, 5, RGB( 203, 203, 203 ) );
		borderPen2			= CreatePen( PS_SOLID, 5, RGB( 0, 0, 78 ) );

		WNDCLASS wc;
		wc.style			= CS_CLASSDC;
		wc.lpfnWndProc		= WaitingProc;
		wc.cbClsExtra		= 0;
		wc.cbWndExtra		= 0; //窗口实例无扩展
		wc.hInstance		= ::AfxGetInstanceHandle( );
		wc.hIcon			= NULL;
		wc.hCursor			= LoadCursor( NULL, IDC_ARROW );
		wc.hbrBackground	= waitBrush;
		wc.lpszMenuName		= NULL;
		wc.lpszClassName	= WC_WAITING;
		RegisterClass( &wc );
		sInitWndClass = true;
	}

	waitWnd = CreateWindow( WC_WAITING, _T(""), WS_VISIBLE | WS_CHILD, 0, 0, 48, 48, parent, NULL, AfxGetInstanceHandle( ), new CWaitingData( ) );

	CRect parentRect;
	GetClientRect( parent, &parentRect );

	int x = ( parentRect.Width( ) - 48 ) / 2;
	int y = ( parentRect.Height( ) - 48 ) / 2;
	::MoveWindow( waitWnd, x, y, 48, 48, TRUE );
	
	EnableWindow( parent, FALSE );
	DECLARE_END_PROTECTED
	return 0;
}

static int endWaiting( lua_State* lua )
{
	DECLARE_BEGIN_PROTECTED
	HWND parent	= (HWND) lua_touserdata( lua, 1 );
	if ( IsWindow( parent ) == FALSE )
	{
		static CNSString errorDesc;
		errorDesc.Format( _UTF8( "指定窗口不是有效的窗口句柄" ) );
		FBException( errorDesc );
	}

	if ( DestroyWindow( waitWnd ) == FALSE )
	{
		static CNSString errorDesc;
		errorDesc.Format( _UTF8( "Win32函数 DestroyWindow 调用失败, 错误码%d" ), GetLastError( ) );
		FBException( errorDesc );
	}

	EnableWindow( parent, TRUE );
	DECLARE_END_PROTECTED
	return 0;
}

static int getChildHwnd( lua_State* lua )
{
	DECLARE_BEGIN_PROTECTED
	HWND wnd	= lua_tohwnd( lua, 1, NULL );
	lua_pushlightuserdata( lua, (void*) wnd );
	DECLARE_END_PROTECTED
	return 1;
}

static int alignWindow( lua_State* lua )
{
	enum EALIGNTYPE
	{
		ALIGN_BOTTOM = 0,
		ALIGN_TOP,
		ALIGN_LEFT,
		ALIGN_RIGHT
	};

	DECLARE_BEGIN_PROTECTED
	HWND			windowAlign	= (HWND) lua_touserdata( lua, 1 );
	HWND			wndAlignBy	= (HWND) lua_touserdata( lua, 2 );
	unsigned int	alignType	= (unsigned int) lua_tounsigned( lua, 3 );
	if ( IsWindow( windowAlign ) == FALSE )
	{
		static CNSString errorDesc;
		errorDesc.Format( _UTF8( "参数1不是有效的窗口句柄" ) );
		FBException( errorDesc );
	}

	if ( IsWindow( wndAlignBy ) == FALSE )
	{
		static CNSString errorDesc;
		errorDesc.Format( _UTF8( "参数2不是有效的窗口句柄" ) );
		FBException( errorDesc );
	}

	CRect rcAlign;
	GetWindowRect( windowAlign, &rcAlign );

	CRect rcAlignBy;
	GetWindowRect( wndAlignBy, &rcAlignBy );

	HWND		parent	= GetParent( windowAlign );
	LONG_PTR	style	= GetWindowLongPtr( windowAlign, GWL_STYLE );
	if ( parent == NULL || style & WS_POPUP )
		parent = GetDesktopWindow( );

	ScreenToClient( parent, &rcAlignBy.TopLeft( ) );
	ScreenToClient( parent, &rcAlignBy.BottomRight( ) );

	if ( alignType == EALIGNTYPE::ALIGN_BOTTOM )
		MoveWindow( windowAlign, rcAlignBy.left, rcAlignBy.bottom, rcAlign.Width( ), rcAlign.Height( ), TRUE );
	else if ( alignType == EALIGNTYPE::ALIGN_TOP )
		MoveWindow( windowAlign, rcAlignBy.left, rcAlignBy.top - rcAlign.Height( ), rcAlign.Width( ), rcAlign.Height( ), TRUE );
	else if ( alignType == EALIGNTYPE::ALIGN_LEFT )
		MoveWindow( windowAlign, rcAlignBy.left - rcAlign.Width( ), rcAlignBy.top, rcAlign.Width( ), rcAlign.Height( ), TRUE );
	else if ( alignType == EALIGNTYPE::ALIGN_RIGHT )
		MoveWindow( windowAlign, rcAlignBy.right, rcAlignBy.top, rcAlign.Width( ), rcAlign.Height( ), TRUE );

	DECLARE_END_PROTECTED
	return 0;
}

static int getNMHdr( lua_State* lua )
{
	DECLARE_BEGIN_PROTECTED
	intptr_t		lParam		= (intptr_t) luaL_checknumber( lua, 1 );
	NMHDR*			nmhdr		= (NMHDR*) lParam;
	CNSLuaStack*	luaStack	= CNSLuaStack::FromState( lua );
	luaStack->BeginTable( );
	if ( nmhdr->code == NM_CLICK || nmhdr->code == NM_DBLCLK )
	{
		NMITEMACTIVATE* nmClick = (NMITEMACTIVATE*) nmhdr;
		luaStack->LuaPushField( "hwnd",			CFBDataType( (void*) nmClick->hdr.hwndFrom ) );
		luaStack->LuaPushField( "notifyCode",	CFBDataType( nmClick->hdr.code ) );
		luaStack->LuaPushField( "ctrlID",		CFBDataType( (long long) nmClick->hdr.idFrom ) );
		luaStack->LuaPushField( "row",			CFBDataType( nmClick->iItem ) );
		luaStack->LuaPushField( "col",			CFBDataType( nmClick->iSubItem ) );
		luaStack->LuaPushField( "newState",		CFBDataType( nmClick->uNewState ) );
		luaStack->LuaPushField( "oldState",		CFBDataType( nmClick->uOldState ) );
		luaStack->LuaPushField( "changed",		CFBDataType( nmClick->uChanged ) );
		luaStack->LuaPushTableField( "ptAction" );
			luaStack->LuaPushField( "x",		CFBDataType( nmClick->ptAction.x ) );
			luaStack->LuaPushField( "y",		CFBDataType( nmClick->ptAction.y ) );
		luaStack->LuaPushFieldEnd( );
		luaStack->LuaPushField( "lParam",		CFBDataType( nmClick->lParam ) );
		luaStack->LuaPushField( "keyFlags",		CFBDataType( nmClick->uKeyFlags ) );
	}
	else if ( nmhdr->code == LVN_KEYDOWN )
	{
		NMLVKEYDOWN* nmKeydown = (NMLVKEYDOWN*) nmhdr;
		luaStack->LuaPushField( "hwnd",			CFBDataType( (void*) nmKeydown->hdr.hwndFrom ) );
		luaStack->LuaPushField( "notifyCode",	CFBDataType( nmKeydown->hdr.code ) );
		luaStack->LuaPushField( "ctrlID",		CFBDataType( (long long) nmKeydown->hdr.idFrom ) );
		luaStack->LuaPushField( "virtualKey",	CFBDataType( nmKeydown->wVKey ) );
		luaStack->LuaPushField( "flags",		CFBDataType( nmKeydown->flags ) );
	}
	else if ( nmhdr->code == TVN_ITEMEXPANDED || nmhdr->code == TVN_SELCHANGED )
	{
		NMTREEVIEW* nmTreeView = (NMTREEVIEW*) nmhdr;
		luaStack->LuaPushField( "hwnd",			CFBDataType( (void*) nmTreeView->hdr.hwndFrom ) );
		luaStack->LuaPushField( "notifyCode",	CFBDataType( nmTreeView->hdr.code ) );
		luaStack->LuaPushField( "ctrlID",		CFBDataType( (long long) nmTreeView->hdr.idFrom ) );
		luaStack->LuaPushField( "action",		CFBDataType( nmTreeView->action ) );
		luaStack->LuaPushField( "item",			CFBDataType( (long long) nmTreeView->itemNew.hItem ) );
		luaStack->LuaPushTableField( "ptDrag" );
			luaStack->LuaPushField( "x",		CFBDataType( nmTreeView->ptDrag.x ) );
			luaStack->LuaPushField( "y",		CFBDataType( nmTreeView->ptDrag.y ) );
		luaStack->LuaPushFieldEnd( );
	}
	else if ( nmhdr->code == TVN_BEGINLABELEDIT )
	{
		TV_DISPINFO* nmDispInfo = (TV_DISPINFO*) nmhdr;
		luaStack->LuaPushField( "hwnd",			CFBDataType( (void*) nmDispInfo->hdr.hwndFrom ) );
		luaStack->LuaPushField( "notifyCode",	CFBDataType( nmDispInfo->hdr.code ) );
		luaStack->LuaPushField( "ctrlID",		CFBDataType( (long long) nmDispInfo->hdr.idFrom ) );
		luaStack->LuaPushField( "item",			CFBDataType( (long long) nmDispInfo->item.hItem ) );
	}
	else if ( nmhdr->code == TVN_ENDLABELEDIT )
	{
		TV_DISPINFO* nmDispInfo = (TV_DISPINFO*) nmhdr;
		luaStack->LuaPushField( "hwnd",			CFBDataType( (void*) nmDispInfo->hdr.hwndFrom ) );
		luaStack->LuaPushField( "notifyCode",	CFBDataType( nmDispInfo->hdr.code ) );
		luaStack->LuaPushField( "ctrlID",		CFBDataType( (long long) nmDispInfo->hdr.idFrom ) );
		luaStack->LuaPushField( "item",			CFBDataType( (long long) nmDispInfo->item.hItem ) );
		luaStack->LuaPushField( "editText",		CFBDataType( nmDispInfo->item.pszText ) );
	}
	else
	{
		luaStack->LuaPushField( "hwnd",			CFBDataType( (void*) nmhdr->hwndFrom ) );
		luaStack->LuaPushField( "notifyCode",	CFBDataType( nmhdr->code ) );
		luaStack->LuaPushField( "ctrlID",		CFBDataType( (long long) nmhdr->idFrom ) );
	}

	DECLARE_END_PROTECTED
	return 1;
}

static int getDBSelection( lua_State* lua )
{
	DECLARE_BEGIN_PROTECTED
	CdbtoolsDlg*	dlg			= (CdbtoolsDlg*) AfxGetMainWnd( );
	CListCtrl*		list		= (CListCtrl*) dlg->GetDlgItem( IDC_SERVER_LIST );
	CNSLuaStack*	luaStack	= CNSLuaStack::FromState( lua );

	POSITION pos = list->GetFirstSelectedItemPosition( );
	int index = list->GetNextSelectedItem( pos );
	CGroupData* group = (CGroupData*) list->GetItemData( index );
	luaStack->BeginTable( );
	luaStack->LuaPushField( "mDBName",	( CNSString( "dbtools:" ) + group->mGroupID ).GetBuffer( ) );
	luaStack->LuaPushField( "mGroupName", group->mGroupName.GetBuffer( ) );
	luaStack->LuaPushField( "mGroupID",	group->mGroupID.GetBuffer( ) );
	DECLARE_END_PROTECTED
	return 1;
}

static int getDlgCtrlID( lua_State* lua )
{
	DECLARE_BEGIN_PROTECTED
	HWND hwnd			= (HWND) lua_touserdata( lua, 1 );
	if ( IsWindow( hwnd ) == FALSE )
	{
		static CNSString errorDesc;
		errorDesc.Format( _UTF8( "不是有效的窗口句柄" ) );
		FBException( errorDesc );
	}
	
	int ctrlID = GetDlgCtrlID( hwnd );
	lua_pushnumber( lua, ctrlID );
	DECLARE_END_PROTECTED
	return 1;
}

static int progress( lua_State* lua )
{
	DECLARE_BEGIN_PROTECTED
	HWND parent			= (HWND) lua_touserdata( lua, 1 );
	const char* title	= luaL_checkstring( lua, 2 );
	float percent		= (float) luaL_checknumber( lua, 3 );
	if ( IsWindow( parent ) == FALSE )
	{
		static CNSString errorDesc;
		errorDesc.Format( _UTF8( "指定窗口不是有效的窗口句柄" ) );
		FBException( errorDesc );
	}

	if ( CDialogBase::progressDlg == NULL )
	{
		CDialogBase::progressDlg = new CProgressDlg( );
		CDialogBase::progressDlg->Create( IDD_DIALOG_PROGRESS, CWnd::FromHandle( parent ) );
		CDialogBase::progressDlg->ShowWindow( SW_SHOW );
		EnableWindow( parent, FALSE );
	}

	CDialogBase::progressDlg->setProgress( CNSString::ToTChar( title ), percent );
	DECLARE_END_PROTECTED
	return 0;
}

static int endProgress( lua_State* lua )
{
	DECLARE_BEGIN_PROTECTED
	if ( CDialogBase::progressDlg == NULL )
		return 0;

	CDialogBase::progressDlg->GetParent( )->EnableWindow( TRUE );
	CDialogBase::progressDlg->DestroyWindow( );
	delete CDialogBase::progressDlg;
	CDialogBase::progressDlg = NULL;
	DECLARE_END_PROTECTED
	return 0;
}

static int registerModule( lua_State* lua )
{
	DECLARE_BEGIN_PROTECTED
	const char* name		= luaL_checkstring( lua, 1 );
	const char* desc		= luaL_checkstring( lua, 2 );
	CdbtoolsDlg* dlg		= (CdbtoolsDlg*) AfxGetMainWnd( );
	dlg->RegisterModule( name, desc );
	DECLARE_END_PROTECTED
	return 0;
}

static int trace( lua_State* lua )
{
	DECLARE_BEGIN_PROTECTED
	const char* text		= luaL_checkstring( lua, 1 );
	NSLog::log( text );
	DECLARE_END_PROTECTED
	return 0;
}

static int appendMenu( lua_State* lua )
{
	DECLARE_BEGIN_PROTECTED
	int top					= lua_gettop( lua );
	unsigned int menuID		= luaL_checkunsigned( lua, 1 );
	const char* text		= luaL_checkstring( lua, 2 );
	const char* callback	= luaL_checkstring( lua, 3 );
	CNSLuaStack* luaStack	= CNSLuaStack::FromState( lua );
	CNSOctetsStream stream;
	for ( int i = 4; i <= top; i ++ )
		luaStack->LuaPushData( stream, i );

	CdbtoolsDlg* dlg		= (CdbtoolsDlg*) AfxGetMainWnd( );
	dlg->AppendMainMenu( menuID, text, callback, stream );
	DECLARE_END_PROTECTED
	return 0;
}

static int messageBox( lua_State* lua )
{
	DECLARE_BEGIN_PROTECTED
	HWND wnd			= (HWND) lua_touserdata( lua, 1 );
	const char* text	= luaL_checkstring( lua, 2 );
	const char* title	= luaL_checkstring( lua, 3 );
	unsigned int type	= luaL_checkunsigned( lua, 4 );
	if ( IsWindow( wnd ) == FALSE )
	{
		static CNSString errorDesc;
		errorDesc.Format( _UTF8( "父窗口不是有效的窗口句柄" ) );
		FBException( errorDesc );
	}

	CString textString	= CNSString::ToTChar( text );
	CString titleString = CNSString::ToTChar( title );
	int result = MessageBox( wnd, textString, titleString, type );
	lua_pushnumber( lua, result );
	DECLARE_END_PROTECTED
	return 1;
}

static int createWindow( lua_State* lua )
{
	DECLARE_BEGIN_PROTECTED
	int top			= lua_gettop( lua );
	HWND parent		= (HWND) lua_touserdata( lua, 1 );
	if ( parent != NULL && IsWindow( parent ) == FALSE )
	{
		static CNSString errorDesc;
		errorDesc.Format( _UTF8( "父窗口不是有效的窗口句柄" ) );
		FBException( errorDesc );
	}

	CWnd* parentWnd	= NULL;
	if ( parent != NULL )
		parentWnd = CWnd::FromHandle( parent );

	const char* dialog		= luaL_checkstring( lua, 2 );
	const char* windowProc	= lua_tostring( lua, 3 );
	const char* preWinProc	= lua_tostring( lua, 4 );
	CNSLuaStack* luaStack	= CNSLuaStack::FromState( lua );
	CNSOctetsStream stream;
	for ( int i = 5; i <= top; i ++ )
		luaStack->LuaPushData( stream, i );

	HGLOBAL resHandle	= theApp.GetResource( dialog, RT_DIALOG );
	CDialogBase* window = new CDialogBase( true, windowProc, preWinProc, stream );
	window->CreateIndirect( resHandle, parentWnd );

	lua_pushlightuserdata( lua, (void*) window->GetSafeHwnd( ) );
	DECLARE_END_PROTECTED
	return 1;
}

static int doModal( lua_State* lua )
{
	DECLARE_BEGIN_PROTECTED
	int top					= lua_gettop( lua );
	const char* dialog		= luaL_checkstring( lua, 1 );
	const char* windowProc	= lua_tostring( lua, 2 );
	const char* preWinProc	= lua_tostring( lua, 3 );
	CNSLuaStack* luaStack	= CNSLuaStack::FromState( lua );
	CNSOctetsStream stream;
	for ( int i = 3; i <= top; i ++ )
		luaStack->LuaPushData( stream, i );

	CDialogBase dlg( false, windowProc, preWinProc, stream );
	HGLOBAL resHandle	= theApp.GetResource( dialog, RT_DIALOG );
	dlg.InitModalIndirect( resHandle );
	dlg.DoModal( );
	DECLARE_END_PROTECTED
	return 0;
}

static int getWindowData( lua_State* lua )
{
	int retCount = 0;
	DECLARE_BEGIN_PROTECTED
	HWND wnd				= (HWND) lua_touserdata( lua, 1 );
	if ( IsWindow( wnd ) == FALSE )
	{
		static CNSString errorDesc;
		errorDesc.Format( _UTF8( "指定窗口不是有效的窗口句柄" ) );
		FBException( errorDesc );
	}

	CNSOctetsStream* data	= CDialogBase::wndParams.Get( wnd );
	if ( data == NULL )
		return 0;

	(*data) >> CNSOctetsStream::EBegin;
	CNSLuaStack* luaStack	= CNSLuaStack::FromState( lua );
	for ( ; data->IsEnd( ) == false; retCount ++ )
		luaStack->LuaPopData( *data );
	(*data) >> CNSOctetsStream::ERollback;
	DECLARE_END_PROTECTED
	return retCount;
}

void Register( lua_State* lua )
{
	luaL_newlib( lua, System );
	lua_setglobal( lua, "System" );

	luaL_newlib( lua, ComboBox );
	lua_setglobal( lua, "ComboBox" );

	luaL_newlib( lua, ListView );
	lua_setglobal( lua, "ListView" );

	luaL_newlib( lua, TreeView );
	lua_setglobal( lua, "TreeView" );

	luaL_newlib( lua, Edit );
	lua_setglobal( lua, "Edit" );

	luaL_newlib( lua, Static );
	lua_setglobal( lua, "Static" );

	luaL_newlib( lua, Button );
	lua_setglobal( lua, "Button" );

	luaL_newlib( lua, Mysql::Mysql );
	lua_setglobal( lua, "Mysql" );

	luaL_newlib( lua, Misc );
	lua_setglobal( lua, "Misc" );
}
