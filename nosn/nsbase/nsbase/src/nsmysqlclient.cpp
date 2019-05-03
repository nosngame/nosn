#include <nsbase.h>

namespace NSMysql
{
	enum EMysqlDataType
	{
		TYPE_DECIMAL, TYPE_TINY,
		TYPE_SHORT, TYPE_LONG,
		TYPE_FLOAT, TYPE_DOUBLE,
		TYPE_NULL, TYPE_TIMESTAMP,
		TYPE_LONGLONG, TYPE_INT24,
		TYPE_DATE, TYPE_TIME,
		TYPE_DATETIME, TYPE_YEAR,
		TYPE_NEWDATE, TYPE_VARCHAR,
		TYPE_BIT, TYPE_NEWDECIMAL = 246,
		TYPE_ENUM = 247,
		TYPE_SET = 248,
		TYPE_TINY_BLOB = 249,
		TYPE_MEDIUM_BLOB = 250,
		TYPE_LONG_BLOB = 251,
		TYPE_BLOB = 252,
		TYPE_VAR_STRING = 253,
		TYPE_STRING = 254,
		TYPE_GEOMETRY = 255
	};

	static int sSessionID = 0;

	// 连接成功触发逻辑是否执行过，防止数据库服务器掉线重连，脚本连接成功触发逻辑(onCompleteDB被执行多次)
	static bool mConnectTriggered = false;

#pragma region lua接口
	static int getValue( lua_State* lua );
	static int getRecordValue( lua_State* lua );
	static int getRecordCount( lua_State* lua );
	static int getFieldInfo( lua_State* lua );

	BEGIN_EXPORT( NSMysqlResult )
		EXPORT_FUNC( getValue )
		EXPORT_FUNC( getRecordValue )
		EXPORT_FUNC( getRecordCount )
		EXPORT_FUNC( getFieldInfo )
	END_EXPORT

	static int getFieldInfo( lua_State* lua )
	{
		DECLARE_BEGIN_PROTECTED
		NSMysql::CProtocolExecuteSqlResponse* result = NULL;
		luaStack >> result;
		luaStack << result->mFieldInfo;
		DECLARE_END_PROTECTED
	}

	static int getValue( lua_State* lua )
	{
		DECLARE_BEGIN_PROTECTED
		NSMysql::CProtocolExecuteSqlResponse* result = NULL;
		luaStack >> result;
		result->getValue( luaStack );
		DECLARE_END_PROTECTED
	}

	static int getRecordValue( lua_State* lua )
	{
		DECLARE_BEGIN_PROTECTED
		NSMysql::CProtocolExecuteSqlResponse* result = NULL;
		unsigned int rowIndex = 0;
		luaStack >> result;
		luaStack >> rowIndex;
		if ( result->cellValid( 0, rowIndex ) == false )
		{
			static CNSString errorDesc;
			errorDesc.format( _UTF8( "数据库字段访问越界 rowIndex = %d" ), rowIndex );
			NSException( errorDesc );
		}

		result->getRecordValue( luaStack, rowIndex );
		DECLARE_END_PROTECTED
	}

	static int getRecordCount( lua_State* lua )
	{
		DECLARE_BEGIN_PROTECTED
		NSMysql::CProtocolExecuteSqlResponse* result = NULL;
		luaStack >> result;
		luaStack << result->getRecordCount( );
		DECLARE_END_PROTECTED
	}

	static int getIntValue( lua_State* lua )
	{
		DECLARE_BEGIN_PROTECTED
		unsigned int colIndex = 0;
		unsigned int rowIndex = 0;
		NSMysql::CProtocolExecuteSqlResponse* result = NULL;
		luaStack >> result;
		luaStack >> colIndex;
		luaStack >> rowIndex;
		if ( result->cellValid( colIndex, rowIndex ) == false )
		{
			static CNSString errorDesc;
			errorDesc.format( _UTF8( "数据库字段访问越界 colInex = %d, rowIndex = %d" ), colIndex, rowIndex );
			NSException( errorDesc );
		}

		lua_pushnumber( lua, result->getIntValue( colIndex, rowIndex ) );
		DECLARE_END_PROTECTED
	}

	static int execSqlWithData( lua_State* lua );
	static int execSqlNoData( lua_State* lua );

	BEGIN_EXPORT( NSMysqlLib )
		EXPORT_FUNC( execSqlWithData )
		EXPORT_FUNC( execSqlNoData )
	END_EXPORT

	static int execSqlNoData( lua_State* lua )
	{
		DECLARE_BEGIN_PROTECTED
		static CNSString dbName;
		static CNSString sqlCmd;
		CNSLuaFunction func;
		luaStack >> dbName;
		luaStack >> sqlCmd;
		luaStack >> func;

		static CNSOctets buffer;
		NSMysql::execScriptSql( dbName, func, buffer, sqlCmd );
		DECLARE_END_PROTECTED
	}

	static int execSqlWithData( lua_State* lua )
	{
		DECLARE_BEGIN_PROTECTED
		static CNSString dbName; 
		static CNSString sqlCmd;
		luaStack >> dbName;
		luaStack >> sqlCmd;
		
		static CNSMap< CNSString, CNSOctets > blobData;
		luaStack >> blobData;

		CNSLuaFunction func;
		luaStack >> func;

		static CNSOctetsStream stream;
		stream.clear( );
		stream << blobData;

		CNSOctets buffer = stream.mBuffer.compress( );
		NSMysql::execScriptSql( dbName, func, buffer, sqlCmd );
		DECLARE_END_PROTECTED
	}
#pragma endregion
	int CProtocolExecuteSqlResponse::getRecordValue( CNSLuaStack& luaStack, unsigned int rowIndex )
	{
		for ( unsigned int colIndex = 0; colIndex < mColumn; colIndex ++ )
		{
			int index = rowIndex * mColumn + colIndex;
			CNSPair< int, CNSOctets >& cellData = mRecordSets[ index ];
			switch ( cellData.mFirst )
			{
				case TYPE_LONGLONG:
					luaStack << *(long long*) cellData.mSecond.begin( );
					break;
				case TYPE_LONG:
					luaStack << *(int*) cellData.mSecond.begin( );
					break;
				case TYPE_FLOAT:
					luaStack << *(float*) cellData.mSecond.begin( );
					break;
				case TYPE_DOUBLE:
					luaStack << *(double*) cellData.mSecond.begin( );
					break;
				case TYPE_STRING:
				case TYPE_VAR_STRING:
				case TYPE_VARCHAR:
				{
					static CNSString stringValue;
					stringValue.clear( );
					stringValue.pushback( (char*) cellData.mSecond.begin( ), cellData.mSecond.length( ) );
					luaStack << stringValue;
					break;
				}
				case TYPE_BLOB:
				case TYPE_TINY_BLOB:
				case TYPE_MEDIUM_BLOB:
				case TYPE_LONG_BLOB:
				{
					try
					{
						CNSMap< CNSString, CNSOctets > data;
						CNSOctets buffer = cellData.mSecond.uncompress( );

						CNSOctetsStream stream( buffer );
						stream >> data;
						luaStack << data;
					}
					catch ( CNSException& )
					{
						luaStack << cellData.mSecond;
					}
					break;
				}
			}
		}

		return mColumn;
	}

	void CProtocolExecuteSqlResponse::getValue( CNSLuaStack& luaStack )
	{
		luaStack.newTable( );
		for ( unsigned int rowIndex = 0; rowIndex < getRecordCount( ); rowIndex ++ )
		{
			luaStack.newTableField( rowIndex + 1 );
			for ( unsigned int colIndex = 0; colIndex < mColumn; colIndex ++ )
			{
				int index = rowIndex * mColumn + colIndex;
				CNSPair< int, CNSOctets >& cellData = mRecordSets[ index ];
				switch ( cellData.mFirst )
				{
					case TYPE_LONGLONG:
						luaStack.pushField( colIndex + 1, *(long long*) cellData.mSecond.begin( ) );
						break;
					case TYPE_LONG:
						luaStack.pushField( colIndex + 1, *(int*) cellData.mSecond.begin( ) );
						break;
					case TYPE_FLOAT:
						luaStack.pushField( colIndex + 1, *(float*) cellData.mSecond.begin( ) );
						break;
					case TYPE_DOUBLE:
						luaStack.pushField( colIndex + 1, *(double*) cellData.mSecond.begin( ) );
						break;
					case TYPE_STRING:
					case TYPE_VAR_STRING:
					case TYPE_VARCHAR:
					{
						static CNSString stringValue;
						stringValue.clear( );
						stringValue.pushback( (char*) cellData.mSecond.begin( ), cellData.mSecond.length( ) );
						luaStack.pushField( colIndex + 1, stringValue );
						break;
					}
					case TYPE_BLOB:
					case TYPE_TINY_BLOB:
					case TYPE_MEDIUM_BLOB:
					case TYPE_LONG_BLOB:
					{
						try
						{
							CNSMap< CNSString, CNSOctets > data;
							CNSOctets buffer = cellData.mSecond.uncompress( );

							CNSOctetsStream stream( buffer );
							stream >> data;
							luaStack.pushField( colIndex + 1, data );
						}
						catch ( CNSException& )
						{
							luaStack.newTableField( colIndex + 1 );
							luaStack.setFieldTable( );
						}
					}
				}
			}
			luaStack.setFieldTable( );
		}
	}

	unsigned int CProtocolExecuteSqlResponse::getRecordCount( )
	{
		if ( mColumn == 0 )
			return 0;

		// 如果结果个数不是数据列数的整数倍
		if ( mRecordSets.getCount( ) % mColumn != 0 )
			NSException( _UTF8( "数据库异常" ) );

		return mRecordSets.getCount( ) / mColumn;
	}

	bool CProtocolExecuteSqlResponse::cellValid( unsigned int colIndex, unsigned int rowIndex )
	{
		return ( rowIndex * mColumn + colIndex ) < mRecordSets.getCount( );
	}

	unsigned int CProtocolExecuteSqlResponse::GetCellSize( unsigned int colIndex, unsigned int rowIndex )
	{
		int index = rowIndex * mColumn + colIndex;
		return mRecordSets[ index ].mSecond.length( );
	}

	CNSOctets CProtocolExecuteSqlResponse::GetRaw( unsigned int colIndex, unsigned int rowIndex )
	{
		int index = rowIndex * mColumn + colIndex;
		return mRecordSets[ index ].mSecond;
	}

	CNSMap< CNSString, CNSOctets >& CProtocolExecuteSqlResponse::GetBlobData( unsigned int colIndex, unsigned int rowIndex )
	{
		int rawType = GetRawType( colIndex, rowIndex );
		if ( rawType != TYPE_BLOB && rawType != TYPE_TINY_BLOB && rawType != TYPE_MEDIUM_BLOB && rawType != TYPE_LONG_BLOB )
			NSException( _UTF8( "数据库字段类型不匹配" ) );

		int tIndex = rowIndex * mColumn + colIndex;
		CNSMap< CNSString, CNSOctets >* tpData = mBlobData.get( tIndex );
		if ( tpData == NULL )
		{
			CNSOctets rawData = mRecordSets[ tIndex ].mSecond;
			if ( rawData.length( ) == 0 )
			{
				static CNSMap< CNSString, CNSOctets > sNull;
				return sNull;
			}

			static CNSMap< CNSString, CNSOctets > data;
			CNSOctets buffer = rawData.uncompress( );

			CNSOctetsStream stream( buffer );
			stream >> data;
			tpData = &mBlobData.insert( tIndex, data );
		}

		return ( *tpData );
	}

	int CProtocolExecuteSqlResponse::GetRawType( unsigned int vColIndex, unsigned int vRowIndex )
	{
		int tIndex = vRowIndex * mColumn + vColIndex;
		return mRecordSets[ tIndex ].mFirst;
	}

	int CProtocolExecuteSqlResponse::getIntValue( unsigned int vColIndex, unsigned int vRowIndex )
	{
		if ( GetCellSize( vColIndex, vRowIndex ) < sizeof( int ) )
			NSException( _UTF8( "数据库字段类型不匹配" ) );

		if ( GetRawType( vColIndex, vRowIndex ) != TYPE_LONG )
			NSException( _UTF8( "数据库字段类型不匹配" ) );

		return *(int*) GetRaw( vColIndex, vRowIndex ).begin( );
	}

	long long CProtocolExecuteSqlResponse::getInt64Value( unsigned int vColIndex, unsigned int vRowIndex )
	{
		if ( GetCellSize( vColIndex, vRowIndex ) < sizeof( long long ) )
			NSException( _UTF8( "数据库字段类型不匹配" ) );

		if ( GetRawType( vColIndex, vRowIndex ) != TYPE_LONGLONG )
			NSException( _UTF8( "数据库字段类型不匹配" ) );

		return *(long long*) GetRaw( vColIndex, vRowIndex ).begin( );
	}

	unsigned int CProtocolExecuteSqlResponse::getUIntValue( unsigned int vColIndex, unsigned int vRowIndex )
	{
		if ( GetCellSize( vColIndex, vRowIndex ) < sizeof( unsigned int ) )
			NSException( _UTF8( "数据库字段类型不匹配" ) );

		if ( GetRawType( vColIndex, vRowIndex ) != TYPE_LONG )
			NSException( _UTF8( "数据库字段类型不匹配" ) );

		return *(unsigned int*) GetRaw( vColIndex, vRowIndex ).begin( );
	}

	float CProtocolExecuteSqlResponse::getFloatValue( unsigned int vColIndex, unsigned int vRowIndex )
	{
		if ( GetCellSize( vColIndex, vRowIndex ) < sizeof( float ) )
			NSException( _UTF8( "数据库字段类型不匹配" ) );

		if ( GetRawType( vColIndex, vRowIndex ) != TYPE_FLOAT )
			NSException( _UTF8( "数据库字段类型不匹配" ) );

		return *(float*) GetRaw( vColIndex, vRowIndex ).begin( );
	}

	double CProtocolExecuteSqlResponse::getDoubleValue( unsigned int vColIndex, unsigned int vRowIndex )
	{
		if ( GetCellSize( vColIndex, vRowIndex ) < sizeof( double ) )
			NSException( _UTF8( "数据库字段类型不匹配" ) );

		if ( GetRawType( vColIndex, vRowIndex ) != TYPE_DOUBLE )
			NSException( _UTF8( "数据库字段类型不匹配" ) );

		return *(double*) GetRaw( vColIndex, vRowIndex ).begin( );
	}

	CNSString CProtocolExecuteSqlResponse::getStringValue( unsigned int vColIndex, unsigned int vRowIndex )
	{
		int rawType = GetRawType( vColIndex, vRowIndex );
		if ( rawType != TYPE_VAR_STRING && rawType != TYPE_STRING && rawType != TYPE_VARCHAR )
			NSException( _UTF8( "数据库字段类型不匹配" ) );

		CNSOctets rawData = GetRaw( vColIndex, vRowIndex );
		static CNSString value;
		value.clear( );
		value.pushback( (char*) rawData.begin( ), rawData.length( ) );
		return value;
	}

	CNSString CProtocolExecuteSqlResponse::getHexString( unsigned int vColIndex, unsigned int vRowIndex )
	{
		static CNSString hexText;
		hexText = "0x";
		CNSOctets tRawData = GetRaw( vColIndex, vRowIndex );
		int rawType = GetRawType( vColIndex, vRowIndex );
		if ( rawType != TYPE_VAR_STRING && rawType != TYPE_STRING && rawType != TYPE_VARCHAR )
			NSException( _UTF8( "数据库字段类型不匹配" ) );

		static CNSString byteText;
		for ( unsigned int i = 0; i < GetCellSize( vColIndex, vRowIndex ); i ++ )
		{
			const char* tpData = (char*) tRawData.begin( );
			unsigned char tValue = tpData[ i ];
			byteText.format( "%0.2x", tValue );
			hexText += byteText;
		}

		return hexText;
	}

	void CProtocolExecuteSqlResponse::pushUserData( CNSLuaStack& luaStack, CProtocolExecuteSqlResponse* ref )
	{
		static CNSVector< const luaL_Reg* > reg;
		reg.clear( );
		reg.pushback( NSMysql::NSMysqlResult );
		luaStack.pushNSWeakRef( ref, "nsMysqlResult", reg );
	}

	void CProtocolExecuteSqlResponse::popUserData( const CNSLuaStack& luaStack, CProtocolExecuteSqlResponse*& ref )
	{
		ref = (CProtocolExecuteSqlResponse*) luaStack.popNSWeakRef( "nsMysqlResult" );
	}

	class CSession
	{
	public:
		typedef void( *DataProc )( const CNSString&, TSessionID, CProtocolExecuteSqlResponse*, const CNSLuaFunction&, const CNSString& sqlCmd );

	public:
		CNSLuaFunction mLuaFunc;
		DataProc mDataProc;
		TSessionID mSessionID = -1;
		CNSString mSqlCmd;

	public:
		CSession( ) : mDataProc( NULL ), mSessionID( 0 )
		{
		}

		CSession( DataProc pProc, CNSLuaFunction func, const CNSString& sqlCmd ) : mSessionID( sSessionID ++ ), mLuaFunc( func ), mDataProc( pProc ), mSqlCmd( sqlCmd )
		{
		}

		void Fire( const CNSString& name, TSessionID sessionID, CProtocolExecuteSqlResponse* proto )
		{
			if ( mDataProc != NULL )
				( *mDataProc )( name, sessionID, proto, mLuaFunc, mSqlCmd );
		}
	};

	class CSessionDispatch
	{
	public:
		CNSMap< int, CSession >			mSessions;

	public:
		void Register( const CSession& rSession )
		{
			mSessions.insert( rSession.mSessionID, rSession );
		}

		void onProtocol( const CNSString& rName, TSessionID sessionID, CNSProtocol* proto )
		{
			switch ( proto->mProtoID )
			{
				case CProtocolExecuteSqlResponse::PID:
				{
					CProtocolExecuteSqlResponse* execSql = (CProtocolExecuteSqlResponse*) ( proto );
					HMAPINDEX findIndex = mSessions.findNode( execSql->mDBSessionID );
					if ( findIndex == NULL )
						return;

					CSession recv = mSessions.getValueEx( findIndex );
					recv.Fire( rName, sessionID, execSql );
					mSessions.eraseNode( findIndex );
					break;
				}
			}
		}
	};

	class CDBClient : public CNSNetManager
	{
	public:
		CSessionDispatch mDBDispatch;
		CNSString mName;
		bool mConnected = false;			// 是否已经连接成功，服务器启动时，数据库连接成功的脚本事件，

	public:
		CDBClient( const CNSString& name, const CNSString& address, unsigned int interval, unsigned int bufferSize, unsigned int streamSize )
			: CNSNetManager( name, address, interval, bufferSize, streamSize ), mName( name ), mConnected( false )
		{
		}

	public:
		void Register( const CSession& session )
		{
			mDBDispatch.Register( session );
		}

	public:
		virtual void onRegisterProtocol( const CNSString& name );
		virtual void onAddSession( const CNSString& name, TSessionID sessionID, const CNSSockAddr& local, const CNSSockAddr& peer );
		virtual void onAddSessionFault( const CNSString& name, int code );
		virtual void onDelSession( const CNSString& rName, TSessionID sessionID );
		virtual void onProtocol( const CNSString& name, TSessionID sessionID, CNSProtocol* proto );
	};
	CNSMap< CNSString, CDBClient* > gDBClients;

	void CDBClient::onRegisterProtocol( const CNSString& name )
	{
		static CProtocolExecuteSqlResponse executeSql;
		registerProtocol( &executeSql );
	}

	void CDBClient::onAddSession( const CNSString& name, TSessionID sessionID, const CNSSockAddr& local, const CNSSockAddr& peer )
	{
		CProtocolRegisterDataClient registerData( local.GetIPString( ) );
		NSNet::send( name, registerData.createStream( ) );

		mConnected = true;
		NSLog::log( _UTF8( "数据库[%s] 地址[%s]连接成功" ), name.getBuffer( ), mAddress.getBuffer( ) );
		NSBase::CNSLuaStack& luaStack = NSBase::CNSLuaStack::getLuaStack( );
		if ( mConnectTriggered == false )
		{
			HLISTINDEX beginIndex = gDBClients.getHead( );
			for ( ; beginIndex != NULL; gDBClients.getNext( beginIndex ) )
			{
				const CDBClient* client = gDBClients.getValue( beginIndex );
				if ( client->mConnected == false )
					return;
			}

			mConnectTriggered = true;
			NSBase::CNSLuaStack& luaStack = NSBase::CNSLuaStack::getLuaStack( );
			luaStack.preCall( "onCompleteDB" );
			luaStack.call( );
		}
	}

	void CDBClient::onAddSessionFault( const CNSString& name, int code )
	{
		registerClient( this );
		NSLog::log( _UTF8( "连接数据库[%s] 地址[%s]失败, 错误码 - %d" ), name.getBuffer( ), mAddress.getBuffer( ), code );
	}

	void CDBClient::onDelSession( const CNSString& name, TSessionID sessionID )
	{
		registerClient( this );
		mConnected = false;
		NSLog::log( _UTF8( "数据库[%s] 地址[%s]连接丢失" ), name.getBuffer( ), mAddress.getBuffer( ) );
	}

	void CDBClient::onProtocol( const CNSString& name, TSessionID sessionID, CNSProtocol* proto )
	{
		mDBDispatch.onProtocol( name, sessionID, proto );
	}

	void onScriptDBCallback( const CNSString& name, TSessionID sessionID, CProtocolExecuteSqlResponse* proto, const CNSLuaFunction& func, const CNSString& sqlCmd )
	{
		NSBase::CNSLuaStack& luaStack = NSBase::CNSLuaStack::getLuaStack( );
		if ( proto->mResultCode == 0 )
		{
			CDBClient** tpClient = gDBClients.get( name );
			if ( tpClient == NULL )
				return;

			NSLog::exception( proto->mErrorDesc.getBuffer( ) );
			return;
		}

		if ( func.isValid( ) == true )
		{
			unsigned int beginTick = CNSTimer::getCurTick( );
			luaStack.preCall( func );
			luaStack << proto;
			luaStack.call( );
			luaStack.clearFunc( func );
			proto->cleanUpRef( );
			unsigned int costTick = CNSTimer::getCurTick( ) - beginTick;
			if ( costTick > 5 )
			{
				NSLog::log( _UTF8( "SQL语句: %s" ), sqlCmd.getBuffer( ) );
				NSLog::log( _UTF8( "回调处理耗时: %dms" ), costTick );
			}
		}
	}

	void execScriptSql( const CNSString& name, const CNSLuaFunction& func, const CNSOctets& inputBuffer, const CNSString& sql, bool transfaction, bool fullInfo )
	{
		CDBClient** client = gDBClients.get( name );
		if ( client == NULL )
			return;

		CSession session( onScriptDBCallback, func, sql );
		CProtocolExecuteSqlRequest executeSql( session.mSessionID, fullInfo, transfaction, sql, &inputBuffer );
		( *client )->Register( session );
		NSNet::send( name, executeSql.createStream( ) );
	}

	void init( )
	{
		NSBase::CNSLuaStack& luaStack = NSBase::CNSLuaStack::getLuaStack( );
		luaStack.regLib( "NSMysql", NSMysql::NSMysqlLib );
	}

	void createMysql( const CNSString& name, const CNSString& address, unsigned int interval, unsigned int bufferSize, unsigned int streamSize )
	{
		if ( gDBClients.get( name ) != NULL )
			return;

		CDBClient* dbClient = new CDBClient( name, address, interval, bufferSize, streamSize );
		registerClient( dbClient );

		gDBClients.insert( name, dbClient );
		NSLog::log( _UTF8( "正在连接数据库[%s] 地址[%s]..." ), name.getBuffer( ), address.getBuffer( ) );
	}

	void killMysql( const CNSString& name, int duration )
	{
		CProtocolShutdownData shutdown( duration );
		NSNet::send( name.getBuffer( ), shutdown.createStream( ) );
	}

	void exit( )
	{
		HLISTINDEX beginIndex = gDBClients.getHead( );
		for ( ; beginIndex != NULL; gDBClients.getNext( beginIndex ) )
			delete gDBClients.getValue( beginIndex );
	}

}