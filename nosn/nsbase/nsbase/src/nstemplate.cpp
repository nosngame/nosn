#include <nsbase.h>

namespace NSBase
{
	unsigned int CNSCallCounter::mCounter[ 8 ] = { 0 };
	unsigned int CNSCallCounter::mTickCounter[ 8 ] = { 0 };
	CNSCallCounter::CNSCallCounter( int index, unsigned int duration, const char* name )
	{
		if (mTickCounter[ index ] == 0)
			mTickCounter[ index ] = CNSTimer::getCurTick( );

		mCounter[ index ] ++;
		unsigned int thisTick = CNSTimer::getCurTick( );
		if (thisTick - mTickCounter[ index ] >= duration)
		{
			time_t curTime = time( NULL );
			tm* tpTime = ::localtime( &curTime );
			printf( "[%d-%d-%d %d:%d:%d]%s - %1.2f / sec\n", 1900 + tpTime->tm_year, tpTime->tm_mon + 1, tpTime->tm_mday,
					tpTime->tm_hour, tpTime->tm_min, tpTime->tm_sec, name, 1000 * mCounter[ index ] / (float)duration );
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
		// 返回的uniLen包含了0結束
		int uniLen = MultiByteToWideChar( CP_ACP, 0, mbcs, -1, NULL, 0 );
		if (uniLen == -1)
		{
			static CNSString error( "mbcs -> unicode error" );
			return error;
		}

		// 这里需要扩大一倍
		static CNSOctets uniBuffer;
		uniBuffer.reserve( uniLen << 1 );
		uniBuffer.length( ) = uniLen << 1;

		// 0結束也被转换了
		MultiByteToWideChar( CP_ACP, 0, mbcs, -1, (LPWSTR)uniBuffer.begin( ), uniLen );
		return CNSString::convertUni16ToUtf8( (wchar_t*)uniBuffer.begin( ), uniLen - 1 );
	}

	CNSString CNSString::convertUtf8ToMbcs( const CNSString& text )
	{
		return convertUtf8ToMbcs( text.getBuffer( ) );
	}

	CNSString CNSString::convertUtf8ToMbcs( const char* utf8 )
	{
		int utf8Len = strlen( utf8 );
		// unicode16的字节长度，uniByteLen包含了0结束
		int uniByteLen = CNSString::_convertUtf8ToUni16( utf8, utf8Len, NULL );

		// unicode16的字符长度
		int uniLen = uniByteLen >> 1;
		static CNSOctets uniBuffer;
		uniBuffer.reserve( uniByteLen );
		uniBuffer.length( ) = uniByteLen;
		CNSString::_convertUtf8ToUni16( utf8, utf8Len, (unsigned short*)uniBuffer.begin( ) );

		// mbcsLen包含了0结束
		int mbcsLen = WideCharToMultiByte( CP_ACP, 0, (LPCWCH)uniBuffer.begin( ), uniLen, 0, 0, 0, 0 );
		if (mbcsLen == -1)
		{
			static CNSString error( "unicode -> mbcs error" );
			return error;
		}

		static CNSOctets mbcsBuffer;
		mbcsBuffer.reserve( mbcsLen );
		mbcsBuffer.length( ) = mbcsLen;
		WideCharToMultiByte( CP_ACP, 0, (wchar_t*)uniBuffer.begin( ), uniLen, (char*)mbcsBuffer.begin( ), mbcsLen, NULL, NULL );

		static CNSString value;
		value.clear( );
		value.pushback( (char*)mbcsBuffer.begin( ), mbcsLen - 1 );
		return value;
	}

#ifdef PLATFORM_WIN32
	CNSOctets CNSString::convertUtf8ToUni16( const CNSString& text )
	{
		return convertUtf8ToUni16( text.getBuffer( ) );
	}

	CNSOctets CNSString::convertUtf8ToUni16( const char* text )
	{
		int utf8Len = strlen( text );
		// unicode16的字节长度，uniByteLen包含了0结束
		int uniByteLen = CNSString::_convertUtf8ToUni16( text, utf8Len, NULL );

		// unicode16的字符长度
		int uniLen = uniByteLen >> 1;
		static CNSOctets uniBuffer;
		uniBuffer.reserve( uniByteLen );
		uniBuffer.length( ) = uniByteLen;
		CNSString::_convertUtf8ToUni16( text, utf8Len, (unsigned short*)uniBuffer.begin( ) );
		return uniBuffer;
	}

	CNSOctets CNSString::convertUtf8ToUni16( const char* text, size_t start, size_t end )
	{
		static CNSOctets uniBuffer;
		size_t len = strlen( text );
		if (end == -1)
		{
			if (len == 0)
			{
				uniBuffer.clear( );
				unsigned short null = 0;
				uniBuffer.insert( uniBuffer.end( ), &null, sizeof( null ) );
				return uniBuffer;
			}

			end = len - 1;
		}

		if (start >= len || end >= len)
		{
			uniBuffer.clear( );
			unsigned short null = 0;
			uniBuffer.insert( uniBuffer.end( ), &null, sizeof( null ) );
			return uniBuffer;
		}

		size_t utf8Len = end - start + 1;
		// unicode16的字节长度，uniByteLen包含了0结束
		int uniByteLen = CNSString::_convertUtf8ToUni16( (char*)text + start, utf8Len, NULL );

		// unicode16的字符长度
		int uniLen = uniByteLen >> 1;
		uniBuffer.reserve( uniByteLen );
		uniBuffer.length( ) = uniByteLen;
		CNSString::_convertUtf8ToUni16( text + start, utf8Len, (unsigned short*)uniBuffer.begin( ) );
		return uniBuffer;
	}

	// 这个函数假定参数text 是utf8类型
	TCHAR* CNSString::toTChar( const CNSString& text )
	{
		static CNSOctets charBuffer;
#ifdef _UNICODE
		charBuffer = CNSString::convertUtf8ToUni16( text );
#elif _MBCS
		charBuffer = CNSString::convertUtf8ToMbcs( text );
#endif

		return (TCHAR*) charBuffer.begin( );
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
	// 这个函数假定参数text 是utf8，返回TCHAR
	TCHAR* CNSString::toTChar( const CNSString& text, int start, int end )
	{
		static CNSOctets tCharBuffer;
#ifdef _UNICODE
		tCharBuffer = CNSString::convertUtf8ToUni16( text, start, end );
#elif _MBCS
		tCharBuffer = CNSString::convertUtf8ToMbcs( text, start, end );
#endif

		return (TCHAR*)tCharBuffer.begin( );
	}
#endif
}