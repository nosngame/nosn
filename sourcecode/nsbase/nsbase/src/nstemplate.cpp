#include <nsbase.h>

namespace NSBase
{
	unsigned int CNSCallCounter::mCounter[ 8 ] = { 0 };
	unsigned int CNSCallCounter::mTickCounter[ 8 ] = { 0 };
	CNSCallCounter::CNSCallCounter( int index, unsigned int duration, const char* name )
	{
		if ( mTickCounter[ index ] == 0 )
			mTickCounter[ index ] = CNSTimer::getCurTick( );

		mCounter[ index ] ++;
		unsigned int thisTick = CNSTimer::getCurTick( );
		if ( thisTick - mTickCounter[ index ] >= duration )
		{
			time_t curTime = time( NULL );
			tm* tpTime = ::localtime( &curTime );
			printf( "[%d-%d-%d %d:%d:%d]%s - %1.2f / sec\n", 1900 + tpTime->tm_year, tpTime->tm_mon + 1, tpTime->tm_mday,
					tpTime->tm_hour, tpTime->tm_min, tpTime->tm_sec, name, 1000 * mCounter[ index ] / (float) duration );
			mTickCounter[ index ] += duration;
			mCounter[ index ] = 0;
		}
	}

	CNSString CNSString::convertMbcsToUtf8( const CNSString& text )
	{
		return convertMbcsToUtf8( text.getBuffer( ) );
	}

	CNSString CNSString::convertMbcsToUtf8( const char* mbcs )
	{
		const char *toCode = "UTF-8", *fromCode = "GB2312";
		static iconv_t ic = NULL;
		if ( ic == NULL )
			ic = iconv_open( toCode, fromCode );

		static char* utf8 = NULL;
		if ( utf8 == NULL )
			utf8 = new char[ 4096 ];

		char* utf8Buf = utf8;
		size_t inBytesLeft = strlen( mbcs );
		size_t outBytesLeft = 4096;
		int ret = (int) iconv( ic, &mbcs, &inBytesLeft, &utf8, &outBytesLeft );
		if ( ret != 0 )
		{
			static CNSString errorDesc;
			errorDesc.format( _UTF8("函数[iconv]错误, 错误码[%d]"), ret );
			NSException( errorDesc )
		}
		utf8[ 0 ] = 0;
		utf8 = utf8Buf;
		return CNSString( utf8 );
	}

	CNSString CNSString::convertUtf8ToMbcs( const CNSString& text )
	{
		return convertUtf8ToMbcs( text.getBuffer( ) );
	}

	CNSString CNSString::convertUtf8ToMbcs( const char* utf8 )
	{
		const char *toCode = "GB2312", *fromCode = "UTF-8";
		static iconv_t ic = NULL;
		if ( ic == NULL )
			ic = iconv_open( toCode, fromCode );

		static char* mbcs = NULL;
		if ( mbcs == NULL )
			mbcs = new char[ 4096 ];

		char* mbcsBuf = mbcs;
		size_t inBytesLeft = strlen( utf8 );
		size_t outBytesLeft = 4096;
		int ret = (int) iconv( ic, &utf8, &inBytesLeft, &mbcs, &outBytesLeft );
		if ( ret != 0 )
		{
			static CNSString errorDesc;
			errorDesc.format( _UTF8("函数[iconv]错误, 错误码[%d]"), ret );
			NSException( errorDesc )
		}
		mbcs[ 0 ] = 0;
		mbcs = mbcsBuf;
		return CNSString( mbcs );
	}
#ifdef PLATFORM_WIN32
	CNSString CNSString::convertUni16ToUtf8( const char* uni )
	{
		const char *toCode = "UTF-8", *fromCode = "UNICODELITTLE";
		static iconv_t ic = NULL;
		if ( ic == NULL )
			ic = iconv_open( toCode, fromCode );

		static char* utf8 = NULL;
		if ( utf8 == NULL )
			utf8 = new char[ 4096 ];

		char* utf8Buf = utf8;
		size_t inBytesLeft = wcslen( (wchar_t*) uni ) * sizeof( wchar_t );
		size_t outBytesLeft = 4096;
		int ret = iconv( ic, &uni, &inBytesLeft, &utf8, &outBytesLeft );
		if ( ret != 0 )
		{
			static CNSString errorDesc;
			errorDesc.format( _UTF8("函数[iconv]错误, 错误码[%d]"), ret );
			NSException( errorDesc )
		}

		utf8[ 0 ] = 0;
		utf8 = utf8Buf;
		return CNSString( utf8 );
	}

	CNSOctets CNSString::convertUtf8ToUni16( const CNSString& text )
	{
		return convertUtf8ToUni16( text.getBuffer( ) );
	}

	CNSOctets CNSString::convertUtf8ToUni16( const char* utf8 )
	{
		const char *toCode = "UNICODELITTLE", *fromCode = "UTF-8";
		static iconv_t ic = NULL;
		if ( ic == NULL )
			ic = iconv_open( toCode, fromCode );

		static char* uni = NULL;
		if ( uni == NULL )
			uni = new char[ 4096 ];

		char* begin = uni;
		size_t inBytesLeft = strlen( utf8 );
		size_t outBytesLeft = 4096;
		int ret = iconv( ic, &utf8, &inBytesLeft, &uni, &outBytesLeft );
		if ( ret != 0 )
		{
			static CNSString errorDesc;
			errorDesc.format( _UTF8("函数[iconv]错误, 错误码[%d]"), ret );
			NSException( errorDesc )
		}
		( (wchar_t*) uni )[ 0 ] = 0;
		char* end = uni + 2;
		return CNSOctets( begin, end );
	}

	CNSString CNSString::fromTChar( char* text )
	{
		static CNSString utf8String;
#ifdef _UNICODE
		utf8String = CNSString::convertUni16ToUtf8( text );
#elif _MBCS
		utf8String = CNSString::convertMbcsToUtf8( text );
#endif
		return utf8String;
	}

	// 这个函数假定参数text 是utf8类型
	char* CNSString::toTChar( const CNSString& text )
	{
		static CNSOctets charBuffer;
#ifdef _UNICODE
		charBuffer = CNSString::convertUtf8ToUni16( text );
#elif _MBCS
		charBuffer = CNSString::convertUtf8ToMbcs( text );
#endif
		return (char*) charBuffer.begin( );
	}

	// 这个函数假定参数text 是unicode(wchar_t类型) 或者 mbcs(char类型)，不能传入utf8文本
	CNSOctets CNSString::toTCharOctets( const CNSString& text )
	{
		CNSOctets charBuffer;
#ifdef _UNICODE
		charBuffer = CNSString::convertUtf8ToUni16( text );
#elif _MBCS
		charBuffer = CNSString::convertUtf8ToMbcs( text );
#endif
		return charBuffer;
	}
#endif
}
