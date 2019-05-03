#pragma once
namespace NSBase
{
	namespace NSFunction
	{
		template< typename Ret, typename Arg >
		union UAliasingCast
		{
		public:
			Ret	mRet;
			Arg mArg;

		public:
			UAliasingCast( )
			{
			}

		public:
			Ret operator( ) ( Arg vArg )
			{
				mArg = vArg;
				return mRet;
			}
		};

		template< typename Ret, typename Arg > inline Ret aliasingCast( Arg arg )
		{
			return UAliasingCast< Ret, Arg >( )( arg );
		}

		template< typename T > inline T& removeConst( const T& t )
		{
			return const_cast<T&>( t );
		}

		inline int random( int minValue, int maxValue )
		{
			maxValue = max( minValue, maxValue );
			return (int) ( ( rand( ) / ( (float) RAND_MAX + 1 ) ) * ( maxValue - minValue + 1 ) + minValue );
		}

#ifdef PLATFORM_WIN32
		class CDir
		{
		public:
			CNSVector< CDir* >			mDirs;		// 目录中包含目录
			CNSVector< CNSString >		mFiles;		// 目录中包含文件
			CNSString					mName;		// 目录名称
		};

		void initFAT( );
		CDir* enumFile( const CNSString& rPath );
		bool getFileData( const CNSString& rPath, CNSOctets& rData );
#endif

#if defined BYTE_ORDER_BIG_ENDIAN
#define swapByte16( x )	( x )
#define swapByte32( x ) ( x )
#define swapByte64( x )	( x )
#elif defined BYTE_ORDER_LITTLE_ENDIAN
		inline unsigned short swapByte16( unsigned short data )
		{
			unsigned char hi = ( data & 0xFF00 ) >> 8;
			unsigned char lo = data & 0x00FF;
			return ( ( lo << 8 ) | hi );
		}
		inline unsigned int swapByte32( unsigned int data )
		{
			unsigned char char1 = ( data & 0xFF000000 ) >> 24;
			unsigned char char2 = ( data & 0x00FF0000 ) >> 16;
			unsigned char char3 = ( data & 0x0000FF00 ) >> 8;
			unsigned char char4 = data & 0x000000FF;
			return ( char4 << 24 ) | ( char3 << 16 ) | ( char2 << 8 ) | char1;
		}
		inline unsigned long long swapByte64( unsigned long long data )
		{
			union
			{
				unsigned long long __ll;
				unsigned int __l[ 2 ];
			} src, des;
			src.__ll = data;
			des.__l[ 0 ] = swapByte32( src.__l[ 1 ] );
			des.__l[ 1 ] = swapByte32( src.__l[ 0 ] );
			return des.__ll;
		}
#endif
	}
};
