#pragma once
#include <math.h>

namespace NSMath
{
	class CNSVector2
	{
	public:
		static CNSVector2 zero;
		static CNSVector2 one;

	private:
		float x;
		float z;
		mutable float m;
		mutable float d;
		mutable float c;

	public:
		CNSVector2( float xValue = 0, float zValue = 0 ) : x( xValue ), z( zValue ), m( FLT_MIN ), d( FLT_MIN ), c( FLT_MIN )
		{
		}

		void setX( float xValue )
		{
			x = xValue;
			m = FLT_MIN;
			d = FLT_MIN;
			c = FLT_MIN;
		}

		void setZ( float zValue )
		{
			z = zValue;
			m = FLT_MIN;
			d = FLT_MIN;
			c = FLT_MIN;
		}

		float getX( ) const
		{
			return x;
		}

		float getZ( ) const
		{
			return z;
		}

		int getIntX( ) const
		{
			return (int)x;
		}

		int getIntZ( ) const
		{
			return (int)z;
		}

		CNSVector2 operator - ( const CNSVector2& v ) const
		{
			return CNSVector2( x - v.x, z - v.z );
		}

		CNSVector2 operator + ( const CNSVector2& v ) const
		{
			return CNSVector2( x + v.x, z + v.z );
		}

		CNSVector2 operator * ( float length ) const
		{
			return CNSVector2( x * length, z * length );
		}

		bool operator == ( const CNSVector2& v ) const
		{
			return x == v.x && z == v.z;
		}

		float magnitude( ) const
		{
			if (m == FLT_MIN)
				m = sqrtf( x * x + z * z );

			return m;
		}

		CNSVector2 normalize( ) const
		{
			CNSVector2 normal;
			normal.x = x / magnitude( );
			normal.z = z / magnitude( );
			return normal;
		}

		float cross( const CNSVector2& v ) const
		{
			if (c == FLT_MIN)
				c = x * v.z - z * v.x;

			return c;
		}

		float dot( const CNSVector2& v )  const
		{
			if (d == FLT_MIN)
				d = x * v.x + z * v.z;

			return d;
		}
	};

	class CNSRect
	{
	public:
		float x;
		float y;
		float w;
		float h;

	public:
		CNSRect( float xValue = 0.0f, float yValue = 0.0f, float wValue = 0.0f, float hValue = 0.0f ) : x( xValue ), y( yValue ), w( wValue ), h( hValue )
		{
		}

	public:
		void splitRect( CNSVector< CNSRect >& rcList )
		{
			float halfW = w / 2;
			float halfH = h / 2;
			float centerX = x + halfW;
			float centerY = y + halfH;
			float topX = x;
			float topY = y;
			rcList.pushback( CNSRect( topX, topY, halfW, halfH ) );
			rcList.pushback( CNSRect( centerX, topY, halfW, halfH ) );
			rcList.pushback( CNSRect( topX, centerY, halfW, halfH ) );
			rcList.pushback( CNSRect( centerX, centerY, halfW, halfH ) );
		}

		bool ptInRect( const CNSVector2& p ) const
		{
			if (p.getX( ) >= x && p.getX( ) < ( x + w ) && p.getZ( ) >= y && p.getZ( ) < ( y + h ))
				return true;

			return false;
		}
	};

	class CNSSegment
	{
	public:
		CNSVector2			p[ 2 ];
		mutable CNSVector2	mCenter;
		mutable bool		mCenterDirty;

	public:
		CNSSegment( ) : mCenterDirty( true )
		{
		}

		CNSSegment( const CNSVector2& p1, const CNSVector2& p2 ) : mCenterDirty( true )
		{
			p[ 0 ] = p1;
			p[ 1 ] = p2;
		}

		CNSVector2 getCenter( )
		{
			if (mCenterDirty == true)
			{
				mCenter = CNSVector2( ( p[ 0 ].getX( ) + p[ 1 ].getX( ) ) / 2.0f, ( p[ 0 ].getZ( ) + p[ 1 ].getZ( ) ) / 2.0f );
				mCenterDirty = true;
			}

			return mCenter;
		}

		CNSVector2 findCloset( const CNSVector2& src, const CNSVector2& des )
		{
			float t1 = ( p[ 0 ] - src ).magnitude( ) + ( p[ 0 ] - des ).magnitude( );
			float t2 = ( p[ 1 ] - src ).magnitude( ) + ( p[ 1 ] - des ).magnitude( );
			return t1 < t2 ? p[ 0 ] : p[ 1 ];
		}

		bool intersectLine( const CNSSegment& seg ) const
		{
			float d1 = direction( seg.p[ 0 ], seg.p[ 1 ], p[ 0 ] );
			float d2 = direction( seg.p[ 0 ], seg.p[ 1 ], p[ 1 ] );
			float d3 = direction( p[ 0 ], p[ 1 ], seg.p[ 0 ] );
			float d4 = direction( p[ 0 ], p[ 1 ], seg.p[ 1 ] );

			if (d1 * d2 < 0 && d3 * d4 < 0)
				return true;
			if (d1 == 0 && seg.onSegment( p[ 0 ] ))
				return true;
			if (d2 == 0 && seg.onSegment( p[ 1 ] ))
				return true;
			if (d3 == 0 && onSegment( seg.p[ 0 ] ))
				return true;
			if (d4 == 0 && onSegment( seg.p[ 1 ] ))
				return true;

			return false;
		}

	private:
		bool onSegment( const CNSVector2& pos ) const
		{
			if (min( p[ 0 ].getX( ), p[ 1 ].getX( ) ) <= pos.getX( ) && pos.getX( ) <= max( p[ 0 ].getX( ), p[ 1 ].getX( ) ))
			{
				if (min( p[ 0 ].getZ( ), p[ 1 ].getZ( ) ) <= pos.getZ( ) && pos.getZ( ) <= max( p[ 0 ].getZ( ), p[ 1 ].getZ( ) ))
				{
					return true;
				}
			}
			return false;
		}

		static float direction( const CNSVector2& pi, const CNSVector2& pj, const CNSVector2& pk )
		{
			CNSVector2 v1 = pi - pk;
			CNSVector2 v2 = pi - pj;
			return v1.cross( v2 );
		}
	};
};
