#pragma once

namespace NSWin32
{
	class CPoint : public CNSMarshal
	{
	public:
		int	mX;
		int	mY;

	public:
		CPoint( ) : mX( 0 ), mY( 0 ) {}
		CPoint( int vX, int vY ) : mX( vX ), mY( vY ) {}
	public:
		~CPoint( ) {}

	public:
		void Offset( int vXOffset, int vYOffset )
		{
			mX += vXOffset; mY += vYOffset;
		}

		void Offset( const CPoint& rPoint )
		{
			mX += rPoint.mX; mY += rPoint.mY;
		}

		void Offset( const CSize& rSize );
		float length( const CPoint& rDesPos )
		{
			CPoint tLength = *this - rDesPos;
			return sqrtf( float( tLength.mX * tLength.mX + tLength.mY * tLength.mY ) );
		}

	public:
		virtual CNSOctetsStream& marshal( CNSOctetsStream& stream ) const
		{
			stream << mX;
			stream << mY;
			return stream;
		}

		virtual const CNSOctetsStream& unMarshal( const CNSOctetsStream& stream )
		{
			stream >> mX;
			stream >> mY;
			return stream;
		}

	public:
		bool operator != ( const CPoint& rPoint ) const
		{
			return mX != rPoint.mX || mY != rPoint.mY;
		}

		bool operator == ( const CPoint& rPoint ) const
		{
			return mX == rPoint.mX && mY == rPoint.mY;
		}

		void operator += ( const CPoint& rPoint )
		{
			mX += rPoint.mX; mY += rPoint.mY;
		}

		void operator -= ( const CPoint& rPoint )
		{
			mX -= rPoint.mX; mY -= rPoint.mY;
		}

		CPoint operator - ( const CPoint& rPoint ) const
		{
			return CPoint( mX - rPoint.mX, mY - rPoint.mY );
		}

		CPoint operator - ( const CSize& rSize ) const;
		void operator -= ( const CSize& rSize );
		void operator += ( const CSize& rSize );
		CPoint operator - ( ) const
		{
			return CPoint( -mX, -mY );
		}

		CPoint operator + ( const CPoint& rPoint ) const
		{
			return CPoint( mX + rPoint.mX, mY + rPoint.mY );
		}

		CPoint operator + ( const CSize& rSize ) const;
	};

	class CSize : public CNSMarshal
	{
	public:
		int	mCX;			//! 横坐标
		int	mCY;			//! 纵坐标

	public:
		//! CSize 构造函数, 默认构造一个 CSize( 0, 0 ) 对象
		CSize( ) : mCX( 0 ), mCY( 0 ) {}

		//! CSize 构造函数, 通过两个整数构造一个 CSize 对象
		/*! \param int vCX 横坐标
			\param int vCY 纵坐标 */
		CSize( int vCX, int vCY ) : mCX( vCX ), mCY( vCY ) {}

	public:
		//! CSize 析造函数
		~CSize( ) {}
	public:
		virtual CNSOctetsStream& marshal( CNSOctetsStream& stream ) const
		{
			stream << mCX;
			stream << mCY;
			return stream;
		}

		virtual const CNSOctetsStream& unMarshal( const CNSOctetsStream& stream )
		{
			stream >> mCX;
			stream >> mCY;
			return stream;
		}

	public:
		bool operator != ( const CSize& rSize ) const
		{
			return mCX != rSize.mCX || mCY != rSize.mCY;
		}

		bool operator == ( const CSize& rSize ) const
		{
			return mCX == rSize.mCX && mCY == rSize.mCY;
		}

		void operator -= ( const CSize& rSize )
		{
			mCX -= rSize.mCX; mCY -= rSize.mCY;
		}

		void operator += ( const CSize& rSize )
		{
			mCX += rSize.mCX; mCY += rSize.mCY;
		}

		CPoint operator - ( const CPoint& rPoint ) const;

		CSize operator - ( const CSize& rSize ) const
		{
			return CSize( mCX - rSize.mCX, mCY - rSize.mCY );
		}
		CSize operator - ( ) const
		{
			return CSize( -mCX, -mCY );
		}

		CPoint operator + ( const CPoint& rPoint ) const;
		CSize operator + ( const CSize& rSize ) const
		{
			return CSize( mCX + rSize.mCX, mCY + rSize.mCY );
		}
	};

	class CNSRect : public CNSMarshal, public CNSLuaMarshal
	{
	protected:
		RECT rc;

	public:
		CNSRect( int x = 0, int y = 0, int width = 0, int height = 0 )
		{
			rc.left = x;
			rc.top = y;
			rc.right = x + width;
			rc.bottom = y + height;
		}

		CNSRect( const RECT& r )
		{
			rc.left = r.left;
			rc.top = r.top;
			rc.right = r.right;
			rc.bottom = r.bottom;
		}
	public:
		~CNSRect( )
		{
		}

	public:
		virtual CNSOctetsStream& marshal( CNSOctetsStream& stream ) const
		{
			stream << (int) rc.left;
			stream << (int) rc.top;
			stream << (int) rc.right;
			stream << (int) rc.bottom;
			return stream;
		}

		virtual const CNSOctetsStream& unMarshal( const CNSOctetsStream& stream )
		{
			stream >> (int) rc.left;
			stream >> (int) rc.top;
			stream >> (int) rc.right;
			stream >> (int) rc.bottom;
			return stream;
		}

		virtual CNSLuaStack& marshal( CNSLuaStack& luaStack ) const
		{
			luaStack.pushField( "left", (int) rc.left );
			luaStack.pushField( "top", (int) rc.top );
			luaStack.pushField( "width", (int) rc.right - (int) rc.left );
			luaStack.pushField( "height", (int) rc.bottom - (int) rc.top );
			return luaStack;
		}

		virtual const CNSLuaStack& unmarshal( const CNSLuaStack& luaStack )
		{
			int width = 0;
			int height = 0;
			luaStack.popField( "left", (int&) rc.left );
			luaStack.popField( "top", (int&) rc.top );
			luaStack.popField( "width", (int&) width );
			luaStack.popField( "height", (int&) height );
			rc.right = rc.left + width;
			rc.bottom = rc.top + height;
			return luaStack;
		}

	public:
		operator RECT( )
		{
			return rc;
		}

		operator const RECT( ) const
		{
			return rc;
		}

		operator RECT&( )
		{
			return rc;
		}

		operator const RECT&( ) const
		{
			return rc;
		}
	};

	class CNSColor : public CNSLuaMarshal
	{
	public:
		unsigned char r = 0;
		unsigned char g = 0;
		unsigned char b = 0;
		unsigned char a = 0;
	public:
		CNSColor( )
		{
		}

		CNSColor( unsigned char cr, unsigned char cg, unsigned char cb, unsigned char ca = 255 ) : r( cr ), g( cg ), b( cb ), a( ca )
		{
		}

	public:
		operator COLORREF( )
		{
			return RGB( r, g, b );
		}

	public:
		virtual CNSLuaStack& marshal( CNSLuaStack& luaStack ) const
		{
			luaStack.pushField( "r", r );
			luaStack.pushField( "g", g );
			luaStack.pushField( "b", b );
			return luaStack;
		}

		virtual const CNSLuaStack& unmarshal( const CNSLuaStack& luaStack )
		{
			luaStack.popField( "r", r );
			luaStack.popField( "g", g );
			luaStack.popField( "b", b );
			return luaStack;
		}
	};

	class CNSPaintStruct : public NSBase::CNSLuaWeakRef
	{
	private:
		PAINTSTRUCT mPaintStruct;
		int mLastBKMode;
		HFONT mLastFont;
		NSWin32::CNSWindow* mpWindow;

	public:
		CNSPaintStruct( NSWin32::CNSWindow* window );
	public:
		~CNSPaintStruct( );

	public:
		void setBKMode( int mode );
		void setFont( HFONT font );
		void fillRect( const CNSRect& rc, HBRUSH br );
		void drawText( const CNSString & text, CNSRect& rc, int drawStyle, COLORREF textClr );
		CNSRect getPSRect( ) const;

	public:
		static void pushUserData( CNSLuaStack& luaStack, CNSPaintStruct* ref );
		static void popUserData( const CNSLuaStack& luaStack, CNSPaintStruct*& ref );
	};
}

