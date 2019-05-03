#pragma once
namespace NSLog
{
	typedef void( *FLogHandler )( const NSBase::CNSString& log );
	typedef void( *FExceptionHandler )( const NSBase::CNSString& exception );
	extern const CNSString sLogText;
	extern const CNSString sExceptionText;

	// ����Log�ı�������
	void setLogHandler( FLogHandler handler );

	// �����쳣�ı�������
	void setExceptionHandler( FExceptionHandler handler );

	// ����log
	void init( const CNSString& logFile );

	// ����log
	void exit( );

	// д����־
	void log( const char* format, ... );

	// д���쳣
	void exception( const char* format, ... );
}
