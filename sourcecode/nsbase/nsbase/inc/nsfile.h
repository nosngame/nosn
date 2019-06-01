#pragma once

namespace NSBase
{
	// 需要序列化到文件的类，从这里派生
	class CNSFileMarshal
	{
	public:
		virtual ~CNSFileMarshal( )
		{
		}

	public:
		virtual CNSFile& marshal( CNSFile& ) const = 0;
		virtual const CNSFile& unMarshal( const CNSFile& ) = 0;
	};

	class CNSFile : public CNSLuaObject
	{
	protected:
		FILE* mpFile = NULL;
		CNSString mFileName;
		mutable long mLength = 0;
		mutable int mTranPos = 0;
		mutable bool mNeedRefresh = true;

	public:
		CNSFile( );

	public:
		~CNSFile( );

	protected:
		template< typename T > T popBytes( ) const;
		template< typename T > void pushBytes( const T& data );

	public:
		void openExist( const CNSString& fileName, const char* mode = "rb+" );
		void openAlways( const CNSString& fileName );
		void createNew( const CNSString& fileName );
		void close( );
		bool isEnd( ) const;
		int length( ) const;
		void readAllBytes( const CNSOctets& value ) const;
		void writeAllBytes( const CNSOctets value, bool widthBOM = false );
		const CNSString& getFileName( ) const
		{
			return mFileName;
		}

		void seekBegin( );
		void seekEnd( );
		void seek( int offset );
		void flush( );
		const CNSFile& operator >> ( ETransaction trans ) const;
		const CNSFile& operator >> ( const char& value ) const;
		const CNSFile& operator >> ( const unsigned char& value ) const;
		const CNSFile& operator >> ( const short& value ) const;
		const CNSFile& operator >> ( const unsigned short& value ) const;
		const CNSFile& operator >> ( const int& value ) const;
		const CNSFile& operator >> ( const unsigned int& value ) const;
		const CNSFile& operator >> ( const long long& value ) const;
		const CNSFile& operator >> ( const unsigned long long& value ) const;
		const CNSFile& operator >> ( const float& value ) const;
		const CNSFile& operator >> ( const double& value ) const;
		const CNSFile& operator >> ( const bool& value ) const;
		const CNSFile& operator >> ( const CNSString& value ) const;
		const CNSFile& operator >> ( const CNSOctets& value ) const;
		const CNSFile& operator >> ( const CNSFileMarshal& value ) const
		{
			return NSFunction::removeConst( value ).unMarshal( *this );
		}

		CNSFile& operator << ( const char value );
		CNSFile& operator << ( const unsigned char value );
		CNSFile& operator << ( const short value );
		CNSFile& operator << ( const unsigned short value );
		CNSFile& operator << ( const int value );
		CNSFile& operator << ( const unsigned int value );
		CNSFile& operator << ( const long long value );
		CNSFile& operator << ( const unsigned long long value );
		CNSFile& operator << ( const float value );
		CNSFile& operator << ( const double value );
		CNSFile& operator << ( const bool value );
		CNSFile& operator << ( const CNSString& value );
		CNSFile& operator << ( const CNSOctets value );
		CNSFile& operator << ( const CNSFileMarshal& value )
		{
			return value.marshal( *this );
		}

	public:
		static void pushUserData( CNSLuaStack& luaStack, CNSFile* ref );
		static void popUserData( const CNSLuaStack& luaStack, CNSFile*& ref );
	};

	void regNSFileLib( );
}
