#pragma once

namespace NSHttp
{
	void returnImage( const CNSString& rName, TSessionID sessionID, CNSOctets imageBuffer );
	void returnMbcs( const CNSString& rName, TSessionID sessionID, const CNSString& rText );
	void returnUtf8( const CNSString& rName, TSessionID sessionID, const CNSString& rText );
	void init( );
	void exit( );
	size_t getHttpData( void* ptr, size_t size, size_t nmemb, void* userdata );
	size_t getGZipData( void* ptr, size_t size, size_t nmemb, void* userdata );
}