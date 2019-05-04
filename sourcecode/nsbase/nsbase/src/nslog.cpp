#include <nsbase.h>
using namespace NSWin32;

namespace NSLog
{
	FLogHandler gLogHandler = NULL;
	FExceptionHandler gExceptionHandler = NULL;
	FILE* gLogFileHandle = NULL;
	const CNSString sLogText;
	const CNSString sExceptionText;

	void log( const char* format, ... )
	{
		va_list args;
		va_start( args, format );
		// 计算需要得长度
		int needLen = vsnprintf( NULL, 0, format, args ) + 1;
		static CNSOctets errorBuffer;
		errorBuffer.reserve( needLen );
		errorBuffer.length( ) = needLen;

		// 格式化文本
		vsnprintf( (char*) errorBuffer.begin( ), needLen, format, args );
		va_end( args );

		CNSTimer curTime;
		static CNSString text;
		text.format( "[%s]%s\n", curTime.getTimeText( ).getBuffer( ), (char*) errorBuffer.begin( ) );
		// printf被写入文件了，并且文件是utf8格式，所以这里写入utf8没有问题
		printf( text.getBuffer( ) );

		NSFunction::removeConst( sLogText ).pushback( text );
		if ( sLogText.getLength( ) > 65535 )
			NSFunction::removeConst( sLogText ).clear( );

		if ( gLogHandler != NULL )
			gLogHandler( text );
	}

	void exception( const char* format, ... )
	{
		va_list tArgs;
		va_start( tArgs, format );
		// 计算需要得长度
		int needLen = vsnprintf( NULL, 0, format, tArgs ) + 1;
		static CNSOctets errorBuffer;
		errorBuffer.reserve( needLen );
		errorBuffer.length( ) = needLen;

		// 格式化文本
		vsnprintf( (char*) errorBuffer.begin( ), needLen, format, tArgs );
		va_end( tArgs );

		CNSTimer curTime;
		static CNSString error;
		error.format( "[%s]%s\n", curTime.getTimeText( ).getBuffer( ), (char*) errorBuffer.begin( ) );
		// printf被写入文件了，并且文件是utf8格式，所以这里写入utf8没有问题
		printf( error.getBuffer( ) );

		NSFunction::removeConst( sExceptionText ).pushback( error );
		if ( sExceptionText.getLength( ) > 65535 )
			NSFunction::removeConst( sExceptionText ).clear( );

		if ( gExceptionHandler != NULL )
			gExceptionHandler( error );
	}

	CNSString mkLogDir( )
	{
		CNSString rootPath = "./nosn";
		_mkdir( rootPath );
		_mkdir( rootPath + "/log" );

		CNSString fileName;
		time_t curTime = time( NULL );
		tm* tpTime = ::localtime( &curTime );
		fileName.format( "%s/log/%04d%02d%02d-%02d-%02d", rootPath.getBuffer( ), 1900 + tpTime->tm_year, tpTime->tm_mon + 1, tpTime->tm_mday,
			tpTime->tm_hour, tpTime->tm_min );

		_mkdir( fileName );
		return fileName;
	}

	void setLogHandler( FLogHandler handler )
	{
		// 处理函数只能设置一次
		if ( gLogHandler == NULL )
			gLogHandler = handler;
	}

	void setExceptionHandler( FExceptionHandler handler )
	{
		// 处理函数只能设置一次
		if ( gExceptionHandler == NULL )
			gExceptionHandler = handler;
	}

	void init( const CNSString& logFile )
	{
		CNSString fileName = mkLogDir( ) + "/" + logFile;
		gLogFileHandle = freopen( CNSString::convertUtf8ToMbcs( fileName ).getBuffer( ), "wb+", stdout );
		if ( gLogFileHandle == NULL )
			NSException( logFile + _UTF8( " - 打开失败" ) );

		unsigned int utf8Header = UTF8_BOM;
		fwrite( &utf8Header, 3, 1, gLogFileHandle );
		NSFunction::removeConst( sLogText ).clear( );
		NSFunction::removeConst( sExceptionText ).clear( );
	}

	void exit( )
	{
		if ( gLogFileHandle != NULL )
		{
			fflush( gLogFileHandle );
			fclose( gLogFileHandle );

			// 还原标准输出
			freopen( "CON", "w", stdout );
		}
	}
};
