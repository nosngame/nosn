#pragma once
namespace NSMysql
{
	// 注册数据库客户端
	class CProtocolRegisterDataClient : public CNSProtocol
	{
	public:
		enum { PID = 1 };

	public:
		CNSString		mServerName;

	public: 
		CProtocolRegisterDataClient( const CNSString& rServerName = "" ) :
		  CNSProtocol( PID ), mServerName( rServerName )
		  {
		  }

	public:
		virtual CNSOctetsStream& marshal( CNSOctetsStream& stream ) const
		{
			stream << mServerName;
			return stream;
		}

		virtual const CNSOctetsStream& unMarshal( const CNSOctetsStream& stream )
		{
			stream >> mServerName;
			return stream;
		}
	};

	class CProtocolShutdownData : public CNSProtocol
	{
	public:
		enum { PID = 2 };
		unsigned mDuration;

	public:
		CProtocolShutdownData( unsigned int duration = 0 ) : CNSProtocol( PID ), mDuration( duration )
		{
		}

	public:
		virtual CNSOctetsStream& marshal( CNSOctetsStream& stream ) const
		{
			stream << mDuration;
			return stream;
		}

		virtual const CNSOctetsStream& unMarshal( const CNSOctetsStream& stream )
		{
			stream >> mDuration;
			return stream;
		}
	};
	
	class CProtocolExecuteSqlRequest : public CNSProtocol
	{
	public:
		enum	{ PID = 3 };

	public:
		unsigned int		mSessionID;		// 会话ID
		bool				mFullInfo;
		bool				mTransaction;
		CNSString			mSqlCommand;
		const CNSOctets * 	mpInputBuffer;

	public:
		// 建立一个数据库请求消息
		CProtocolExecuteSqlRequest( ) : CNSProtocol( PID ), mSessionID( 0 ), mTransaction( false )
		{
		}

	public:
		CProtocolExecuteSqlRequest( int vSessionID, bool fullInfo, bool transaction, const CNSString& rSqlCommand, const CNSOctets* buffer )
			: CNSProtocol( PID ), mSessionID( vSessionID ), mFullInfo( fullInfo ), mTransaction( transaction ), mSqlCommand( rSqlCommand ), mpInputBuffer( buffer )
		{
		}

	public:
		~CProtocolExecuteSqlRequest( )
		{
		}

	public:
		virtual CNSOctetsStream& marshal( CNSOctetsStream& stream ) const
		{
			stream << mSessionID;
			stream << mFullInfo;
			stream << mTransaction;
			stream << mSqlCommand;
			stream << (*mpInputBuffer);
			return stream;
		}

		virtual const CNSOctetsStream& unMarshal( const CNSOctetsStream& stream )
		{
			static CNSOctets buffer;
			stream >> mSessionID;
			stream >> mFullInfo;
			stream >> mTransaction;
			stream >> mSqlCommand;
			stream >> buffer;
			mpInputBuffer = &buffer;
			return stream;
		}
	};

	class CProtocolExecuteSqlResponse : public CNSProtocol, public CNSLuaWeakRef
	{
	public:
		enum	{ PID = 4 };

	public:
		unsigned int									mDBSessionID;		// 会话ID
		unsigned int									mColumn;
		unsigned int									mResultCode;
		CNSVector< CNSString >							mFieldInfo;
		CNSVector< CNSPair< int, CNSOctets > >			mRecordSets;
		CNSMap< int, CNSMap< CNSString, CNSOctets > >	mBlobData;
		CNSString										mErrorDesc;

	public:
		CProtocolExecuteSqlResponse( unsigned int vDBSessionID, unsigned int column, unsigned int vResultCode, CNSVector< CNSString >& fieldInfo, const CNSVector< CNSPair< int, CNSOctets > >& rRecordSets, const CNSString& errorDesc ) 
			: CNSProtocol( PID ), mDBSessionID( vDBSessionID ), mColumn( column ), mResultCode( vResultCode ), mFieldInfo( fieldInfo ), mRecordSets( rRecordSets ), mErrorDesc( errorDesc )
		{
		}

	public:
		CProtocolExecuteSqlResponse( ) : CNSProtocol( PID ), mDBSessionID( 0 ), mColumn( 0 ), mResultCode( 0 )
		{
		}

	public:
		~CProtocolExecuteSqlResponse( )
		{
		}

	public:
		int getRecordValue( CNSLuaStack& luaStack, unsigned int rowIndex );
		void getValue( CNSLuaStack& luaStack );
		unsigned int getRecordCount( );
		bool cellValid( unsigned int colIndex, unsigned int rowIndex );
		unsigned int GetCellSize( unsigned int colIndex, unsigned int rowIndex );
		CNSOctets GetRaw( unsigned int colIndex, unsigned int rowIndex );	
		CNSMap< CNSString, CNSOctets >& GetBlobData( unsigned int colIndex, unsigned int rowIndex );
		int GetRawType( unsigned int vColIndex, unsigned int vRowIndex );
		int getIntValue( unsigned int vColIndex, unsigned int vRowIndex );
		long long getInt64Value( unsigned int vColIndex, unsigned int vRowIndex );
		unsigned int getUIntValue( unsigned int vColIndex, unsigned int vRowIndex );
		float getFloatValue( unsigned int vColIndex, unsigned int vRowIndex );
		double getDoubleValue( unsigned int vColIndex, unsigned int vRowIndex );
		CNSString getStringValue( unsigned int vColIndex, unsigned int vRowIndex );
		CNSString getHexString( unsigned int vColIndex, unsigned int vRowIndex );

	public:
		virtual CNSOctetsStream& marshal( CNSOctetsStream& stream ) const
		{
			stream << mDBSessionID;
			stream << mColumn;
			stream << mResultCode;
			stream << mRecordSets;
			stream << mFieldInfo;
			stream << mErrorDesc;
			return stream;
		}

		virtual const CNSOctetsStream& unMarshal( const CNSOctetsStream& stream )
		{
			mBlobData.clear( );
			stream >> mDBSessionID;
			stream >> mColumn;
			stream >> mResultCode;
			stream >> mRecordSets;
			stream >> mFieldInfo;
			stream >> mErrorDesc;
			return stream;
		}

	public:
		static void pushUserData( CNSLuaStack& luaStack, CProtocolExecuteSqlResponse* ref );
		static void popUserData( const CNSLuaStack& luaStack, CProtocolExecuteSqlResponse*& ref );

	};

	void init( );
	void createMysql( const CNSString& name, const CNSString& address, unsigned int interval, unsigned int bufferSize, unsigned int streamSize );
	void execScriptSql( const CNSString& name, const CNSLuaFunction& funcRef, const CNSOctets& inputBuffer, const CNSString& sql, bool transfaction = false, bool fullInfo = false );
	void killMysql( const CNSString& name, int duration );
	void exit( );
}
