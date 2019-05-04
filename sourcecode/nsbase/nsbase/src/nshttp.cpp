#include <nsbase.h>
#include <curl.h>

namespace NSHttp
{
#pragma region C++接口
	static int httpGet( lua_State* lua );
	static int httpsGet( lua_State* lua );
	static int httpPost( lua_State* lua );
	static int httpsPost( lua_State* lua );
	static int httpPostWithHeader( lua_State* lua );
	static int httpGZipPost( lua_State* lua );
	static int httpReturn( lua_State* lua );
	static int httpReturnUtf8( lua_State* lua );

	BEGIN_EXPORT( NSHttpLib )
		EXPORT_FUNC( httpGet )
		EXPORT_FUNC( httpsGet )
		EXPORT_FUNC( httpPost )
		EXPORT_FUNC( httpsPost )
		EXPORT_FUNC( httpPostWithHeader )
		EXPORT_FUNC( httpGZipPost )
		EXPORT_FUNC( httpReturn )
		EXPORT_FUNC( httpReturnUtf8 )
	END_EXPORT

	class CHttpResult
	{
	public:
		CNSString		mResult;
		CNSOctets		mPostData;
		CNSOctets		mResultData;
		CURL*			mpCurl;
		CURLM*			mpCurlM;
		int				mRunningHandles;
		CNSLuaFunction	mLuaFunc;

	public:
		CHttpResult( ) : mRunningHandles( 0 )
		{
		}
	};

	CNSMap< CURL*, CHttpResult* >* getHttpReq( )
	{
		static CNSMap< CURL*, CHttpResult* > result;
		return &result;
	}

	size_t getHttpData( void *ptr, size_t size, size_t nmemb, void *userdata )
	{
		CHttpResult* tpData = (CHttpResult*) userdata;
		tpData->mResult.pushback( CNSString( (char*) ptr, size * nmemb ) );
		return ( size * nmemb );
	}

	size_t getGZipData( void *ptr, size_t size, size_t nmemb, void *userdata )
	{
		CHttpResult* result = (CHttpResult*) userdata;
		result->mResultData.insert( result->mResultData.begin( ), ptr, size * nmemb );

		CNSOctets data = result->mResultData.gzipUncompress( 2048 );
		result->mResult = CNSString( (char*) data.begin( ), data.length( ) );
		return ( size * nmemb );
	}

	void nsTimerHttpLogic( int timerID, unsigned int curTick, int remain, void* userdata )
	{
		// 处理curl
		CNSMap< CURL*, CHttpResult* >* req = getHttpReq( );
		static CNSVector< CURL* > tombs;
		tombs.clear( );
		HLISTINDEX beginIndex = req->getHead( );
		for ( ; beginIndex != NULL; req->getNext( beginIndex ) )
		{
			CHttpResult* result = req->getValue( beginIndex );
			curl_multi_perform( result->mpCurlM, &result->mRunningHandles );
			int tSocket = 0;
			if ( result->mRunningHandles > 0 )
			{
				long tWait = 0;
				fd_set tReadFD;
				fd_set tWriteFD;
				fd_set tExcepFD;
				FD_ZERO( &tReadFD );
				FD_ZERO( &tWriteFD );
				FD_ZERO( &tExcepFD );
				curl_multi_fdset( result->mpCurlM, &tReadFD, &tWriteFD, &tExcepFD, &tSocket );
				if ( tSocket != -1 )
				{
					struct timeval timeout = { tWait / 1000, ( tWait % 1000 ) * 1000 };
					if ( select( tSocket + 1, &tReadFD, &tWriteFD, &tExcepFD, &timeout ) < 0 )
						fprintf( stderr, "E: select(%i,,,,%li): %i: %s\n", tSocket + 1, tWait, errno, strerror( errno ) );
				}
			}

			CURLMsg* multiCurl = NULL;
			int msgQueue;
			if ( multiCurl = curl_multi_info_read( result->mpCurlM, &msgQueue ) )
			{
				if ( multiCurl->msg == CURLMSG_DONE )
				{
					NSBase::CNSLuaStack& luaStack = NSBase::CNSLuaStack::getLuaStack( );
					if ( result->mLuaFunc.isValid( ) == true )
					{
						luaStack.preCall( result->mLuaFunc );
						luaStack << result->mResult;
						luaStack << multiCurl->data.result;
						luaStack.call( );
						luaStack.clearFunc( result->mLuaFunc );
					}

					tombs.pushback( result->mpCurl );
					curl_multi_remove_handle( result->mpCurlM, result->mpCurl );
					curl_easy_cleanup( result->mpCurl );
					curl_multi_cleanup( result->mpCurlM );
					delete result;
				}
			}
		}

		for ( unsigned int i = 0; i < tombs.getCount( ); i ++ )
			req->erase( tombs[ i ] );
	}

	void init( )
	{
		curl_global_init( CURL_GLOBAL_ALL );

		NSBase::CNSLuaStack& luaStack = NSBase::CNSLuaStack::getLuaStack( );
		luaStack.regLib( "NSHttp", NSHttp::NSHttpLib );
		CNSTimer::createTimer( nsTimerHttpLogic, 10, NULL );
	}

	void exit( )
	{
		curl_global_cleanup( );
	}

	void returnImage( const CNSString& name, TSessionID sessionID, CNSOctets imageBuffer )
	{
		CNSString response;
		response.pushback( "HTTP/1.1 200 OK\n" );
		response.pushback( "Date: Wed, 22 Jan 2014 12:46:20 GMT\n" );
		response.pushback( "Server: Apache/2.4.7 (Win32) PHP/5.5.8\n" );
		response.pushback( "Last-Modified: Mon, 11 Jun 2007 18:53:14 GMT\n" );
		response.pushback( "ETag: \"2e-432a5e4a73a80\"\n" );
		response.pushback( "Accept-Ranges: bytes\n" );

		CNSString tContextLength;
		tContextLength.format( "Content-Length: %d\n", imageBuffer.length( ) );
		response.pushback( tContextLength );
		response.pushback( "Connection: close\n" );
		response.pushback( "Content-Type: image/x-icon\n" );
		response.pushback( "\n" );
		response.pushback( "\n" );
		CNSOctets buffer( response.getBuffer( ), response.getLength( ) );
		NSNet::send( name, buffer, sessionID );
	}

	void returnMbcs( const CNSString& name, TSessionID sessionID, const CNSString& text )
	{
		CNSString tData;
		if ( text.isTextUtf8( ) == true )
			tData = CNSString::convertUtf8ToMbcs( text );
		else
			tData = text;

		CNSString response;
		response.pushback( "HTTP/1.1 200 OK\n" );
		response.pushback( "Date: Wed, 22 Jan 2014 12:46:20 GMT\n" );
		response.pushback( "Server: Apache/2.4.7 (Win32) PHP/5.5.8\n" );
		response.pushback( "Last-Modified: Mon, 11 Jun 2007 18:53:14 GMT\n" );
		response.pushback( "ETag: \"2e-432a5e4a73a80\"\n" );
		response.pushback( "Accept-Ranges: bytes\n" );

		CNSString tContextLength;
		tContextLength.format( "Content-Length: %d\n", tData.getLength( ) );
		response.pushback( tContextLength );
		response.pushback( "Connection: close\n" );
		response.pushback( "Content-Type: text/html\n" );
		response.pushback( "\n" );
		response.pushback( tData.getBuffer( ) );
		response.pushback( "\n" );
		CNSOctets buffer( response.getBuffer( ), response.getLength( ) );
		NSNet::send( name, buffer, sessionID );
	}

	void returnUtf8( const CNSString& name, TSessionID sessionID, const CNSString& text )
	{
		CNSString tData;
		if ( text.isTextUtf8( ) == true )
			tData = text;
		else
			tData = CNSString::convertMbcsToUtf8( text );

		CNSString response;
		response.pushback( "HTTP/1.1 200 OK\n" );
		response.pushback( "Date: Wed, 22 Jan 2014 12:46:20 GMT\n" );
		response.pushback( "Server: Apache/2.4.7 (Win32) PHP/5.5.8\n" );
		response.pushback( "Last-Modified: Mon, 11 Jun 2007 18:53:14 GMT\n" );
		response.pushback( "ETag: \"2e-432a5e4a73a80\"\n" );
		response.pushback( "Accept-Ranges: bytes\n" );

		CNSString tContextLength;
		tContextLength.format( "Content-Length: %d\n", tData.getLength( ) );
		response.pushback( tContextLength );
		response.pushback( "Connection: close\n" );
		response.pushback( "Content-Type: text/html;charset=utf-8\n" );
		response.pushback( "\n" );
		response.pushback( tData.getBuffer( ) );
		response.pushback( "\n" );
		CNSOctets buffer( response.getBuffer( ), response.getLength( ) );
		NSNet::send( name, buffer, sessionID );
	}
#pragma endregion

#pragma region Lua接口		
	static int httpGet( lua_State* lua )
	{
		DECLARE_BEGIN_PROTECTED
		static CNSString url;
		CNSLuaFunction func( __FUNCTION__ );
		luaStack >> url;
		luaStack >> func;

		CURLM* multiCurl = curl_multi_init( );
		CURL* easyCurl = curl_easy_init( );

		NSHttp::CHttpResult* result = new NSHttp::CHttpResult;
		result->mpCurl = easyCurl;
		result->mLuaFunc = func;
		result->mpCurlM = multiCurl;

		NSHttp::getHttpReq( )->insert( easyCurl, result );
		curl_easy_setopt( easyCurl, CURLOPT_URL, url.getBuffer( ) );
		curl_easy_setopt( easyCurl, CURLOPT_WRITEFUNCTION, NSHttp::getHttpData );
		curl_easy_setopt( easyCurl, CURLOPT_WRITEDATA, result );
		curl_multi_add_handle( multiCurl, easyCurl );
		DECLARE_END_PROTECTED
	}

	static int httpsGet( lua_State* lua )
	{
		DECLARE_BEGIN_PROTECTED
		static CNSString url;
		CNSLuaFunction func( __FUNCTION__ );
		luaStack >> url;
		luaStack >> func;

		CURLM* multiCurl = curl_multi_init( );
		CURL* easyCurl = curl_easy_init( );

		NSHttp::CHttpResult* result = new NSHttp::CHttpResult;
		result->mpCurl = easyCurl;
		result->mLuaFunc = func;
		result->mpCurlM = multiCurl;

		NSHttp::getHttpReq( )->insert( easyCurl, result );
		curl_easy_setopt( easyCurl, CURLOPT_URL, url.getBuffer( ) );
		curl_easy_setopt( easyCurl, CURLOPT_SSL_VERIFYPEER, 0 ); // 对认证证书来源的检查
		curl_easy_setopt( easyCurl, CURLOPT_SSL_VERIFYHOST, 1 ); // 从证书中检查SSL加密算法是否存在
		curl_easy_setopt( easyCurl, CURLOPT_WRITEFUNCTION, NSHttp::getHttpData );
		curl_easy_setopt( easyCurl, CURLOPT_WRITEDATA, result );
		curl_multi_add_handle( multiCurl, easyCurl );
		DECLARE_END_PROTECTED
	}

	static int httpGZipPost( lua_State* lua )
	{
		DECLARE_BEGIN_PROTECTED
		static CNSString url;
		static CNSString postData;
		CNSLuaFunction func( __FUNCTION__ );
		luaStack >> url;
		luaStack >> postData;
		luaStack >> func;

		CURLM* multiCurl = curl_multi_init( );
		CURL* easyCurl = curl_easy_init( );

		NSHttp::CHttpResult* result = new NSHttp::CHttpResult;
		result->mpCurl = easyCurl;
		result->mLuaFunc = func;
		result->mpCurlM = multiCurl;
		result->mPostData = CNSOctets( postData, strlen( postData ) ).gzipCompress( );

		NSHttp::getHttpReq( )->insert( easyCurl, result );
		curl_easy_setopt( easyCurl, CURLOPT_URL, url.getBuffer( ) );
		curl_easy_setopt( easyCurl, CURLOPT_POST, 1 );
		curl_easy_setopt( easyCurl, CURLOPT_ACCEPT_ENCODING, "gzip" );
		curl_easy_setopt( easyCurl, CURLOPT_POSTFIELDS, result->mPostData.begin( ) );
		curl_easy_setopt( easyCurl, CURLOPT_POSTFIELDSIZE, result->mPostData.length( ) );
		curl_easy_setopt( easyCurl, CURLOPT_WRITEFUNCTION, NSHttp::getGZipData );
		curl_easy_setopt( easyCurl, CURLOPT_WRITEDATA, result );
		curl_multi_add_handle( multiCurl, easyCurl );
		DECLARE_END_PROTECTED
	}

	static int httpPost( lua_State* lua )
	{
		DECLARE_BEGIN_PROTECTED
		static CNSString url;
		static CNSString postData;
		CNSLuaFunction func( __FUNCTION__ );
		luaStack >> url;
		luaStack >> postData;
		luaStack >> func;

		CURLM* multiCurl = curl_multi_init( );
		CURL* easyCurl = curl_easy_init( );

		NSHttp::CHttpResult* result = new NSHttp::CHttpResult;
		result->mpCurl = easyCurl;
		result->mLuaFunc = func;
		result->mpCurlM = multiCurl;
		result->mPostData = CNSOctets( postData, strlen( postData ) );

		NSHttp::getHttpReq( )->insert( easyCurl, result );
		curl_easy_setopt( easyCurl, CURLOPT_URL, url.getBuffer( ) );
		curl_easy_setopt( easyCurl, CURLOPT_POST, 1 );
		curl_easy_setopt( easyCurl, CURLOPT_POSTFIELDS, result->mPostData.begin( ) );
		curl_easy_setopt( easyCurl, CURLOPT_POSTFIELDSIZE, result->mPostData.length( ) );
		curl_easy_setopt( easyCurl, CURLOPT_WRITEFUNCTION, NSHttp::getHttpData );
		curl_easy_setopt( easyCurl, CURLOPT_WRITEDATA, result );
		curl_multi_add_handle( multiCurl, easyCurl );
		DECLARE_END_PROTECTED
	}

	static int httpPostWithHeader( lua_State* lua )
	{
		DECLARE_BEGIN_PROTECTED
		static CNSString url;
		CNSVector< CNSString > header;
		static CNSString postData;
		CNSLuaFunction func( __FUNCTION__ );
		luaStack >> url;
		luaStack >> header;
		luaStack >> postData;
		luaStack >> func;

		CURLM* multiCurl = curl_multi_init( );
		CURL* easyCurl = curl_easy_init( );

		NSHttp::CHttpResult* result = new NSHttp::CHttpResult;
		result->mpCurl = easyCurl;
		result->mLuaFunc = func;
		result->mpCurlM = multiCurl;
		result->mPostData = CNSOctets( postData, strlen( postData ) );;

		/* init to NULL is important */
		struct curl_slist* httpHeader = NULL;
		for ( unsigned int i = 0; i < header.getCount( ); i ++ )
			httpHeader = curl_slist_append( httpHeader, header[ i ] );

		NSHttp::getHttpReq( )->insert( easyCurl, result );
		curl_easy_setopt( easyCurl, CURLOPT_URL, url.getBuffer( ) );
		curl_easy_setopt( easyCurl, CURLOPT_POST, 1 );
		curl_easy_setopt( easyCurl, CURLOPT_HTTPHEADER, httpHeader );
		curl_easy_setopt( easyCurl, CURLOPT_POSTFIELDS, result->mPostData.begin( ) );
		curl_easy_setopt( easyCurl, CURLOPT_POSTFIELDSIZE, result->mPostData.length( ) );
		curl_easy_setopt( easyCurl, CURLOPT_WRITEFUNCTION, NSHttp::getHttpData );
		curl_easy_setopt( easyCurl, CURLOPT_WRITEDATA, result );
		curl_multi_add_handle( multiCurl, easyCurl );
		DECLARE_END_PROTECTED
	}

	static int httpsPost( lua_State* lua )
	{
		DECLARE_BEGIN_PROTECTED
		static CNSString url;
		static CNSString postData;
		CNSLuaFunction func( __FUNCTION__ );
		luaStack >> url;
		luaStack >> postData;
		luaStack >> func;

		CURLM* multiCurl = curl_multi_init( );
		CURL* easyCurl = curl_easy_init( );
		NSHttp::CHttpResult* result = new NSHttp::CHttpResult;
		result->mpCurl = easyCurl;
		result->mLuaFunc = func;
		result->mpCurlM = multiCurl;
		result->mPostData = CNSOctets( postData, strlen( postData ) );

		NSHttp::getHttpReq( )->insert( easyCurl, result );
		curl_easy_setopt( easyCurl, CURLOPT_URL, url.getBuffer( ) );
		curl_easy_setopt( easyCurl, CURLOPT_POST, 1 );
		curl_easy_setopt( easyCurl, CURLOPT_POSTFIELDS, result->mPostData.begin( ) );
		curl_easy_setopt( easyCurl, CURLOPT_POSTFIELDSIZE, result->mPostData.length( ) );
		curl_easy_setopt( easyCurl, CURLOPT_SSL_VERIFYPEER, 0 ); // 对认证证书来源的检查
		curl_easy_setopt( easyCurl, CURLOPT_SSL_VERIFYHOST, 1 ); // 从证书中检查SSL加密算法是否存在
		curl_easy_setopt( easyCurl, CURLOPT_WRITEFUNCTION, NSHttp::getHttpData );
		curl_easy_setopt( easyCurl, CURLOPT_WRITEDATA, result );
		curl_multi_add_handle( multiCurl, easyCurl );
		DECLARE_END_PROTECTED
	}

	static int httpReturnUtf8( lua_State* lua )
	{
		DECLARE_BEGIN_PROTECTED
		static CNSString name;
		TSessionID sessionID = 0;
		static CNSString text;
		luaStack >> name;
		luaStack >> sessionID;
		luaStack >> text;
		NSHttp::returnUtf8( name, sessionID, text );
		DECLARE_END_PROTECTED
	}

	static int httpReturn( lua_State* lua )
	{
		DECLARE_BEGIN_PROTECTED
		static CNSString name;
		TSessionID sessionID = 0;
		static CNSString text;
		luaStack >> name;
		luaStack >> sessionID;
		luaStack >> text;
		NSHttp::returnMbcs( name, sessionID, text );
		DECLARE_END_PROTECTED
	}
#pragma endregion
}