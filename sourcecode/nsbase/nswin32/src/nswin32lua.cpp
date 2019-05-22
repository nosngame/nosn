#include <nsbase.h>
#include "nswin32lua.h"
#include "nsgdilua.h"
#include "nswindowlua.h"
#include "toolhelp.h"

namespace NSWin32
{
	CNSPaintStruct::CNSPaintStruct( NSWin32::CNSWindow* window ) : mLastFont( NULL ), mLastBKMode( -1 ), mpWindow( window )
	{
		::BeginPaint( mpWindow->getHWnd( ), &mPaintStruct );
		setBKMode( TRANSPARENT );
		setFont( NSWin32::CNSPreDefine::FB_FONT_BASE );
	}

	CNSPaintStruct::~CNSPaintStruct( )
	{
		if ( mLastFont != NULL )
			SelectFont( mPaintStruct.hdc, mLastFont );

		if ( mLastBKMode != -1 )
			::SetBkMode( mPaintStruct.hdc, mLastBKMode );

		::EndPaint( mpWindow->getHWnd( ), &mPaintStruct );

		// mLuaRefs是弱引用，被引用对象这里将要被销毁，所以需要让引用指向NULL
		cleanUpRef( );
	}

	void CNSPaintStruct::setBKMode( int mode )
	{
		mLastBKMode = ::SetBkMode( mPaintStruct.hdc, mode );
	}

	void CNSPaintStruct::setFont( HFONT font )
	{
		mLastFont = SelectFont( mPaintStruct.hdc, font );
	}

	void CNSPaintStruct::fillRect( const CNSRect& rc, HBRUSH br )
	{
		::FillRect( mPaintStruct.hdc, &( (RECT) rc ), br );
	}

	void CNSPaintStruct::drawText( const CNSString& text, CNSRect& rc, int drawStyle, COLORREF textClr )
	{
		COLORREF lastClr = ::SetTextColor( mPaintStruct.hdc, textClr );
		::DrawText( mPaintStruct.hdc, (TCHAR*) CNSString::toTChar( text ), -1, &( (RECT) rc ), drawStyle );
		::SetTextColor( mPaintStruct.hdc, lastClr );
	}

	void CNSPaintStruct::pushUserData( CNSLuaStack& luaStack, CNSPaintStruct* ref )
	{
		static CNSVector< const luaL_Reg* > reg;
		reg.clear( );
		reg.pushback( NSWin32::NSDc );
		luaStack.pushNSWeakRef( ref, "nsPs", reg );
	}

	void CNSPaintStruct::popUserData( const CNSLuaStack& luaStack, CNSPaintStruct*& ref )
	{
		ref = (CNSPaintStruct*) luaStack.popNSWeakRef( "nsPs" );
	}

	CNSRect CNSPaintStruct::getPSRect( ) const
	{
		return mPaintStruct.rcPaint;
	}

	CPoint CSize::operator - ( const CPoint& rPoint ) const
	{
		return rPoint - *this;
	}

	CPoint CSize::operator + ( const CPoint& rPoint ) const
	{
		return rPoint + *this;
	}

	void CPoint::Offset( const CSize& rSize )
	{
		mX += rSize.mCX; mY += rSize.mCY;
	}

	void CPoint::operator += ( const CSize& rSize )
	{
		mX += rSize.mCX; mY += rSize.mCY;
	}

	void CPoint::operator -= ( const CSize& rSize )
	{
		mX -= rSize.mCX; mY -= rSize.mCY;
	}

	CPoint CPoint::operator - ( const CSize& rSize ) const
	{
		return CPoint( mX - rSize.mCX, mY - rSize.mCY );
	}

	CPoint CPoint::operator + ( const CSize& rSize ) const
	{
		return CPoint( mX + rSize.mCX, mY + rSize.mCY );
	}
}