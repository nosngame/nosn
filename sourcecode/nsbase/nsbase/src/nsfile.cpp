#include <nsbase.h>

namespace NSBase
{
	static int openExist( lua_State* lua );
	static int openAlways( lua_State* lua );
	static int createNew( lua_State* lua );
	static int close( lua_State* lua );
	static int write( lua_State* lua );
	static int read( lua_State* lua );
	static int isEnd( lua_State* lua );
	static int seekBegin( lua_State* lua );
	static int seekEnd( lua_State* lua );
	static int seek( lua_State* lua );
	static int flush( lua_State* lua );

	static const struct luaL_Reg NSFileIo[] =
	{
		{ "openExist", openExist },
		{ "openAlways", openAlways },
		{ "createNew", createNew },
		{ NULL, NULL }
	};

	static const struct luaL_Reg NSFileLib[] =
	{
		{ "close", close },
		{ "read", read },
		{ "write", write },
		{ "seekBegin", seekBegin },
		{ "seekEnd", seekEnd },
		{ "seek", seek },
		{ "isEnd", isEnd },
		{ "flush", flush },
		{ NULL, NULL }
	};

	void regNSFileLib( )
	{
		NSBase::CNSLuaStack& luaStack = NSBase::CNSLuaStack::getLuaStack( );
		luaStack.regLib( "NSFile", NSBase::NSFileIo );
	}

	CNSFile::CNSFile( )
	{
	}

	CNSFile::~CNSFile( )
	{
		close( );
	}

	// 文件必须存在
	void CNSFile::openExist( const CNSString& filename )
	{
		mFileName = filename;
		if ( mpFile != NULL )
		{
			static CNSString errorDesc;
			errorDesc.format( _UTF8( "文件[%s]重复打开" ), mFileName.getBuffer( ) );
			NSException( errorDesc );
		}

		mpFile = fopen( CNSString::convertUtf8ToMbcs( mFileName ).getBuffer( ), "rb+" );
		if ( mpFile == NULL )
		{
			static CNSString errorDesc;
			errorDesc.format( _UTF8( "文件[%s]打开失败, 错误码 - %d" ), mFileName.getBuffer( ), errno );
			NSException( errorDesc );
		}
	}

	// 一定打开文件，文件如果不存在就新建一个
	void CNSFile::openAlways( const CNSString& filename )
	{
		mFileName = filename;
		if ( mpFile != NULL )
		{
			static CNSString errorDesc;
			errorDesc.format( _UTF8( "文件[%s]重复打开" ), mFileName.getBuffer( ) );
			NSException( errorDesc );
		}

		mpFile = fopen( mFileName.getBuffer( ), "rb+" );
		if ( mpFile == NULL )
		{
			if ( errno == ENOENT )
			{
				mpFile = fopen( mFileName.getBuffer( ), "wb+" );
				return;
			}

			static CNSString errorDesc;	
			errorDesc.format( _UTF8( "文件[%s]打开失败, 错误码 - %d" ), mFileName.getBuffer( ), errno );
			NSException( errorDesc );
		}
	}

	// 一定打开文件，文件如果存在就覆盖
	void CNSFile::createNew( const CNSString& filename )
	{
		mFileName = filename;
		if ( mpFile != NULL )
		{
			static CNSString errorDesc;
			errorDesc.format( _UTF8( "文件[%s]重复打开" ), mFileName.getBuffer( ) );
			NSException( errorDesc );
		}

		mpFile = fopen( mFileName.getBuffer( ), "wb+" );
		if ( mpFile == NULL )
		{
			static CNSString errorDesc;
			errorDesc.format( _UTF8( "文件[%s]打开失败, 错误码 - %d" ), mFileName.getBuffer( ), errno );
			NSException( errorDesc );
		}
	}

	void CNSFile::close( )
	{
		if ( mpFile != NULL )
			fclose( mpFile );

		mpFile = NULL;
	}

	bool CNSFile::isEnd( ) const
	{
		if ( mpFile == NULL )
			return true;

		return ::ftell( mpFile ) == length( );
	}

	int CNSFile::length( ) const
	{
		if ( mNeedRefresh == true )
		{
			// 记录文件位置
			long curPos = ::ftell( mpFile );
			// 移动到文件结束
			::fseek( mpFile, 0, SEEK_END );
			mLength = ::ftell( mpFile );

			// 还原文件位置
			::fseek( mpFile, curPos, SEEK_SET );
			mNeedRefresh = false;
		}
		return mLength;
	}

	void CNSFile::readAllBytes( const CNSOctets& value ) const
	{
		if ( mpFile == NULL )
		{
			static CNSString errorDesc;
			errorDesc.format( _UTF8( "文件[%s]已经关闭" ), mFileName.getBuffer( ) );
			NSException( errorDesc );
		}

		int len = length( );
		NSFunction::removeConst( value ).resize( len );
		if ( fread( NSFunction::removeConst( value ).begin( ), sizeof( char ), len, mpFile ) != len )
		{
			static CNSString errorDesc;
			errorDesc.format( _UTF8( "crt函数[fread]文件[%s]读取失败, 错误码 - %d" ), mFileName.getBuffer( ), errno );
			NSException( errorDesc );
		}

		unsigned int header = 0;
		NSFunction::memcpy_fast( &header, value.begin( ), 3 );
		if ( header == UTF8_BOM )
		{
			char* begin = (char*) NSFunction::removeConst( value ).begin( );
			NSFunction::removeConst( value ).erase( begin, begin + 3 );
		}
	}

	void CNSFile::writeAllBytes( const CNSOctets value, bool widthBOM )
	{
		if ( mpFile == NULL )
		{
			static CNSString errorDesc;
			errorDesc.format( _UTF8( "文件[%s]已经关闭" ), mFileName.getBuffer( ) );
			NSException( errorDesc );
		}

		if ( widthBOM == true )
		{
			unsigned int header = UTF8_BOM;
			if ( fwrite( &header, 1, 3, mpFile ) != 3 )
			{
				static CNSString errorDesc;
				errorDesc.format( _UTF8( "crt函数[fwrite]文件[%s]读取失败, 错误码 - %d" ), mFileName.getBuffer( ), errno );
				NSException( errorDesc );
			}
		}

		if ( fwrite( value.begin( ), sizeof( char ), value.length( ), mpFile ) != value.length( ) )
		{
			static CNSString errorDesc;
			errorDesc.format( _UTF8( "crt函数[fread]文件[%s]读取失败, 错误码 - %d" ), mFileName.getBuffer( ), errno );
			NSException( errorDesc );
		}
	}

	void CNSFile::clear( )
	{
		fclose( mpFile );
		mpFile = NULL;
		createNew( mFileName );
	}

	template< typename T > T CNSFile::popBytes( ) const
	{
		if ( mpFile == NULL )
		{
			static CNSString errorDesc;
			errorDesc.format( _UTF8( "文件[%s]已经关闭" ), mFileName.getBuffer( ) );
			NSException( errorDesc );
		}

		T value;
		if ( fread( &value, sizeof( T ), 1, mpFile ) != 1 )
		{
			static CNSString errorDesc;
			errorDesc.format( _UTF8( "crt函数[fread]文件[%s]读取失败, 错误码 - %d" ), mFileName.getBuffer( ), errno );
			NSException( errorDesc );
		}

		return value;
	}

	template< typename T >
	void CNSFile::pushBytes( const T& data )
	{
		if ( mpFile == NULL )
		{
			static CNSString errorDesc;
			errorDesc.format( _UTF8( "文件[%s]已经关闭" ), mFileName.getBuffer( ) );
			NSException( errorDesc );
		}

		if ( fwrite( &data, sizeof( T ), 1, mpFile ) != 1 )
		{
			static CNSString errorDesc;
			errorDesc.format( _UTF8( "crt函数[fwrite]文件[%s]写入失败, 错误码 - %d" ), mFileName.getBuffer( ), errno );
			NSException( errorDesc );
		}

		mNeedRefresh = true;
	}

	void CNSFile::seekBegin( )
	{
		if ( mpFile == NULL )
		{
			static CNSString errorDesc;
			errorDesc.format( _UTF8( "文件[%s]已经关闭" ), mFileName.getBuffer( ) );
			NSException( errorDesc );
		}

		::fseek( mpFile, 0, SEEK_SET );
	}

	void CNSFile::seekEnd( )
	{
		if ( mpFile == NULL )
		{
			static CNSString errorDesc;
			errorDesc.format( _UTF8( "文件[%s]已经关闭" ), mFileName.getBuffer( ) );
			NSException( errorDesc );
		}

		::fseek( mpFile, 0, SEEK_END );
	}

	void CNSFile::seek( int offset )
	{
		if ( mpFile == NULL )
		{
			static CNSString errorDesc;
			errorDesc.format( _UTF8( "文件[%s]已经关闭" ), mFileName.getBuffer( ) );
			NSException( errorDesc );
		}

		::fseek( mpFile, offset, SEEK_CUR );
	}

	void CNSFile::flush( )
	{
		::fflush( mpFile );
	}

	const CNSFile& CNSFile::operator >> ( ETransaction trans ) const
	{
		switch ( trans )
		{
		case NSBase::EBegin:
			mTranPos = ftell( mpFile );
			break;
		case NSBase::ERollback:
			fseek( mpFile, mTranPos, SEEK_SET );
			break;
		case NSBase::ECommit:
			break;
		}
		return *this;
	}

	const CNSFile& CNSFile::operator >> ( const char& value ) const
	{
		NSFunction::removeConst( value ) = popBytes< char >( );
		return *this;
	}

	const CNSFile& CNSFile::operator >> ( const unsigned char& value ) const
	{
		NSFunction::removeConst( value ) = popBytes< unsigned char >( );
		return *this;
	}

	const CNSFile& CNSFile::operator >> ( const short& value ) const
	{
		NSFunction::removeConst( value ) = popBytes< short >( );
		return *this;
	}

	const CNSFile& CNSFile::operator >> ( const unsigned short& value ) const
	{
		NSFunction::removeConst( value ) = popBytes< unsigned short >( );
		return *this;
	}

	const CNSFile& CNSFile::operator >> ( const int& value ) const
	{
		NSFunction::removeConst( value ) = popBytes< int >( );
		return *this;
	}

	const CNSFile& CNSFile::operator >> ( const unsigned int& value ) const
	{
		NSFunction::removeConst( value ) = popBytes< unsigned int >( );
		return *this;
	}

	const CNSFile& CNSFile::operator >> ( const long long& value ) const
	{
		NSFunction::removeConst( value ) = popBytes< long long >( );
		return *this;
	}

	const CNSFile& CNSFile::operator >> ( const unsigned long long& value ) const
	{
		NSFunction::removeConst( value ) = popBytes< unsigned long long >( );
		return *this;
	}

	const CNSFile& CNSFile::operator >> ( const float& value ) const
	{
		NSFunction::removeConst( value ) = popBytes< float >( );
		return *this;
	}

	const CNSFile& CNSFile::operator >> ( const double& value ) const
	{
		NSFunction::removeConst( value ) = popBytes< double >( );
		return *this;
	}

	const CNSFile& CNSFile::operator >> ( const bool& value ) const
	{
		NSFunction::removeConst( value ) = popBytes< bool >( );
		return *this;
	}

	const CNSFile& CNSFile::operator >> ( const CNSString& value ) const
	{
		unsigned short len;
		operator >> ( len );
		NSFunction::removeConst( value ).clear( );

		CNSOctets buffer( len, len );
		if ( fread( buffer.begin( ), sizeof( char ), len, mpFile ) != len )
		{
			static CNSString errorDesc;
			errorDesc.format( _UTF8( "crt函数[fread]文件[%s]读取失败, 错误码 - %d" ), mFileName.getBuffer( ), errno );
			NSException( errorDesc );
		}

		NSFunction::removeConst( value ).pushback( (char*) buffer.begin( ), len );
		return *this;
	}

	const CNSFile& CNSFile::operator >> ( const CNSOctets& value ) const
	{
		unsigned int len;
		operator >> ( len );
		NSFunction::removeConst( value ).resize( len );

		if ( fread( (void*) value.begin( ), sizeof( char ), len, mpFile ) != len )
		{
			static CNSString errorDesc;
			errorDesc.format( _UTF8( "crt函数[fread]文件[%s]读取失败, 错误码 - %d" ), mFileName.getBuffer( ), errno );
			NSException( errorDesc );
		}
		return *this;
	}

	CNSFile& CNSFile::operator << ( const char value )
	{
		pushBytes( value );
		return *this;
	}

	CNSFile& CNSFile::operator << ( const unsigned char value )
	{
		pushBytes( value );
		return *this;
	}

	CNSFile& CNSFile::operator << ( const short value )
	{
		pushBytes( value );
		return *this;
	}

	CNSFile& CNSFile::operator << ( const unsigned short value )
	{
		pushBytes( value );
		return *this;
	}

	CNSFile& CNSFile::operator << ( const int value )
	{
		pushBytes( value );
		return *this;
	}

	CNSFile& CNSFile::operator << ( const unsigned int value )
	{
		pushBytes( value );
		return *this;
	}

	CNSFile& CNSFile::operator << ( const long long value )
	{
		pushBytes( value );
		return *this;
	}

	CNSFile& CNSFile::operator << ( const unsigned long long value )
	{
		pushBytes( value );
		return *this;
	}

	CNSFile& CNSFile::operator << ( const float value )
	{
		pushBytes( value );
		return *this;
	}

	CNSFile& CNSFile::operator << ( const double value )
	{
		pushBytes( value );
		return *this;
	}

	CNSFile& CNSFile::operator << ( const bool value )
	{
		pushBytes( value );
		return *this;
	}

	CNSFile& CNSFile::operator << ( const CNSString& value )
	{
		pushBytes( (unsigned short) value.getLength( ) );
		if ( fwrite( value.getBuffer( ), sizeof( char ), value.getLength( ), mpFile ) != value.getLength( ) )
		{
			static CNSString errorDesc;
			errorDesc.format( _UTF8( "crt函数[fwrite]文件[%s]写入失败, 错误码 - %d" ), mFileName.getBuffer( ), errno );
			NSException( errorDesc );
		}

		mNeedRefresh = true;
		return *this;
	}

	CNSFile& CNSFile::operator << ( const CNSOctets value )
	{
		pushBytes( (unsigned int) value.length( ) );
		if ( fwrite( value.begin( ), sizeof( char ), value.length( ), mpFile ) != value.length( ) )
		{
			static CNSString errorDesc;
			errorDesc.format( _UTF8( "crt函数[fwrite]文件[%s]写入失败, 错误码 - %d" ), mFileName.getBuffer( ), errno );
			NSException( errorDesc );
		}

		mNeedRefresh = true;
		return *this;
	}

	void CNSFile::pushUserData( CNSLuaStack& luaStack, CNSFile* ref )
	{
		luaStack.pushNSObject( ref, "nsFile", NSBase::NSFileLib );
	}

	void CNSFile::popUserData( const CNSLuaStack& luaStack, CNSFile*& ref )
	{
		ref = (CNSFile*) luaStack.popNSObject( "nsFile" );
	}

	static int openExist( lua_State* lua )
	{
		DECLARE_BEGIN_PROTECTED
		const char* filename = luaL_checkstring( lua, 1 );
		CNSFile* file = new CNSFile( );
		luaStack << file;
		file->openExist( filename );
		DECLARE_END_PROTECTED
	}

	static int openAlways( lua_State* lua )
	{
		DECLARE_BEGIN_PROTECTED
		const char* filename = luaL_checkstring( lua, 1 );
		CNSFile* file = new CNSFile( );
		luaStack << file;
		file->openAlways( filename );
		DECLARE_END_PROTECTED
	}

	static int createNew( lua_State* lua )
	{
		DECLARE_BEGIN_PROTECTED
		const char* filename = luaL_checkstring( lua, 1 );
		CNSFile* file = new CNSFile( );
		luaStack << file;
		file->createNew( filename );
		DECLARE_END_PROTECTED
	}

	static int close( lua_State* lua )
	{
		DECLARE_BEGIN_PROTECTED
		CNSFile* file = NULL;
		luaStack >> file;
		file->close( );
		DECLARE_END_PROTECTED
	}

	static int read( lua_State* lua )
	{
		DECLARE_BEGIN_PROTECTED
		CNSFile* file = NULL;
		luaStack >> file;
		if ( file->isEnd( ) == true )
			return 0;

		try
		{
			luaStack >> NSBase::EBegin;
			luaStack << *file;
			luaStack >> NSBase::ECommit;
		}
		catch ( CNSException& e )
		{
			luaStack >> NSBase::ERollback;
			static CNSString errorDesc;
			errorDesc.format( _UTF8( "文件[%s] %s" ), file->getFileName( ).getBuffer( ), e.mErrorDesc );
			NSException( errorDesc )
		}
		DECLARE_END_PROTECTED
	}
	
	static int write( lua_State* lua )
	{
		DECLARE_BEGIN_PROTECTED
		CNSFile* file = NULL;
		luaStack >> file;
		for ( ; luaStack.isEnd( ) == false; )
			luaStack >> *file;
		DECLARE_END_PROTECTED
	}

	static int isEnd( lua_State* lua )
	{
		DECLARE_BEGIN_PROTECTED
		CNSFile* file = NULL;
		luaStack >> file;
		lua_pushboolean( lua, file->isEnd( ) );
		DECLARE_END_PROTECTED
	}

	int seekBegin( lua_State* lua )
	{
		DECLARE_BEGIN_PROTECTED
		CNSFile* file = NULL;
		luaStack >> file;
		file->seekBegin( );
		DECLARE_END_PROTECTED
	}

	int seekEnd( lua_State* lua )
	{
		DECLARE_BEGIN_PROTECTED
		CNSFile* file = NULL;
		luaStack >> file;
		file->seekEnd( );
		DECLARE_END_PROTECTED
	}

	int seek( lua_State* lua )
	{
		DECLARE_BEGIN_PROTECTED
		CNSFile* file = NULL;
		int pos = 0;
		luaStack >> file;
		luaStack >> pos;
		file->seek( pos );
		DECLARE_END_PROTECTED
	}

	int flush( lua_State* lua )
	{
		DECLARE_BEGIN_PROTECTED
		CNSFile* file = NULL;
		luaStack >> file;
		int pos = 0;
		file->flush( );
		DECLARE_END_PROTECTED
	}
}