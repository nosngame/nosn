#pragma once
namespace NSLog
{
	typedef void( *FLogHandler )( const NSBase::CNSString& log );
	typedef void( *FExceptionHandler )( const NSBase::CNSString& exception );
	extern const CNSString sLogText;
	extern const CNSString sExceptionText;

	// 设置Log文本处理函数
	void setLogHandler( FLogHandler handler );

	// 设置异常文本处理函数
	void setExceptionHandler( FExceptionHandler handler );

	// 构造log
	void init( const CNSString& logFile );

	// 析构log
	void exit( );

	// 写入日志
	void log( const char* format, ... );

	// 写入异常
	void exception( const char* format, ... );
}
