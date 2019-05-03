#include <nsbase.h>
#include <database.h>
#include "server.h"

namespace DataServer
{
	void CDataServer::onAddSession( const CNSString& name, TSessionID sessionID, const CNSSockAddr& local, const CNSSockAddr& peer )
	{
	}

	void CDataServer::onDelSession( const CNSString& name, TSessionID sessionID )
	{
		CDataLogic* logic = NSWin32::CConsoleApp< CDataLogic >::getLogic( );
		CNSString* clientName = logic->mDBClients.get( sessionID );
		if ( clientName == NULL )
			return;

		NSLog::log( _UTF8( "服务器[%s]链接丢失" ), clientName->getBuffer( ) );
	}

	void CDataServer::onRegisterProtocol( const CNSString& name )
	{
		static NSMysql::CProtocolRegisterDataClient	sRegisterData;
		static NSMysql::CProtocolExecuteSqlRequest sExecuteSql;
		static NSMysql::CProtocolShutdownData sShutdown;
		registerProtocol( &sRegisterData );
		registerProtocol( &sExecuteSql );
		registerProtocol( &sShutdown );
	}

	void CDataServer::onProtocol( const CNSString& name, TSessionID sessionID, CNSProtocol* proto )
	{
		switch ( proto->mProtoID )
		{
			case NSMysql::CProtocolShutdownData::PID:
			{
				NSMysql::CProtocolShutdownData* tpShutdown = ( NSMysql::CProtocolShutdownData* ) proto;
				CNSTimer::createDelay( DataServer::dbTimerShutdown, tpShutdown->mDuration, NULL );
				break;
			}
			case NSMysql::CProtocolRegisterDataClient::PID:
			{
				NSMysql::CProtocolRegisterDataClient* dataClient = ( NSMysql::CProtocolRegisterDataClient* ) proto;
				CDataLogic* logic = NSWin32::CConsoleApp< CDataLogic >::getLogic( );
				logic->mDBClients.insert( sessionID, dataClient->mServerName );
				NSLog::log( _UTF8( "服务器[%s]注册成功" ), dataClient->mServerName.getBuffer( ) );
				break;
			}
			case NSMysql::CProtocolExecuteSqlRequest::PID:
			{
				NSMysql::CProtocolExecuteSqlRequest* executeSql = ( NSMysql::CProtocolExecuteSqlRequest* ) proto;
				CDataLogic* logic = NSWin32::CConsoleApp< CDataLogic >::getLogic( );

				CSqlRequest sql;
				sql.mNetSessionID = sessionID;
				sql.mDBSessionID = executeSql->mSessionID;
				sql.mSqlCommand = executeSql->mSqlCommand;
				sql.mFullInfo = executeSql->mFullInfo;
				sql.mTransaction = executeSql->mTransaction;
				sql.mInputBuffer = *executeSql->mpInputBuffer;
				EnterCriticalSection( &logic->mSqlCs );
				logic->mSqlQueues.pushback( sql );
				LeaveCriticalSection( &logic->mSqlCs );
				break;
			}
		}
	}

	bool CDataLogic::threadExec( NSTool::CMySqlDatabase* db, bool fullInfo, bool transaction, const CNSString& rSqlCommand, const CNSOctets &rInputBuffer, int& rColumn,
								 CNSVector< CNSString >& fieldInfo, CNSVector< CNSPair< int, CNSOctets > >& rResult, CNSString& errorDesc )
	{
		if ( transaction == true )
			db->Transaction( );

		if ( db->Execute( rSqlCommand.getBuffer( ), rInputBuffer, errorDesc ) == false )
		{
			if ( transaction == true )
				db->Rollback( );
			return false;
		}

		if ( fullInfo == true )
			db->GetFieldName( fieldInfo );

		for ( NSTool::CMySQLRecordSet* tpRecord = db->Fetch( ); tpRecord != NULL; tpRecord = db->Fetch( ) )
		{
			for ( unsigned int i = 0; i < tpRecord->mColumns; i ++ )
			{
				CNSOctets tBuffer( tpRecord->mpDataBind[ i ].buffer, *tpRecord->mpDataBind[ i ].length );
				rResult.pushback( CNSPair< int, CNSOctets >( tpRecord->mpDataBind[ i ].buffer_type, tBuffer ) );
			}

			rColumn = tpRecord->mColumns;
			delete tpRecord;
		}

		if ( transaction == true )
			db->Commit( );

		return true;
	}

	void CDataLogic::workThread( void* param )
	{
		CDataLogic* logic = NSWin32::CConsoleApp< CDataLogic >::getLogic( );
		DWORD threadID = GetCurrentThreadId( );
		CNSString psw = logic->mConnInfo.mPsw;
		CNSString dbAddress = logic->mConnInfo.mDBAddress;
		CNSString userName = logic->mConnInfo.mUserName;
		CNSString dbName = logic->mConnInfo.mDBName;

		EnterCriticalSection( &logic->mSqlCs );
		NSTool::CMySqlDatabase db;
		NSLog::log( _UTF8( "ThreadID[%d], 正在连接数据库[%s]..." ), threadID, dbName.getBuffer( ) );

		CNSVector< CNSString > ipSet;
		dbAddress.split( ":", ipSet );
		int ret = db.Connect( ipSet[ 0 ], dbName, userName, psw, ipSet[ 1 ].toInteger( ) );
		if ( ret == 0 )
			NSLog::log( _UTF8( "ThreadID[%d], 连接数据库成功!" ), threadID );
		else
		{
			// CNSString::MbcsUtf8 返回的是引用一块静态内存, _UTF8内部也是用了同一块内存
			// 所以这里必须拷贝一次, 否则同一块内存会被后面的数据覆盖
			// 注意：CNSString 内部使用引用的接口都有这个问题，使用时必须根据情况决定是否需要拷贝一次
			CNSString errorUtf8 = CNSString::convertMbcsToUtf8( db.GetReason( ) );
			NSLog::log( _UTF8( "ThreadID[%d], 连接数据库失败, 失败原因: %s" ), threadID, errorUtf8.getBuffer( ) );
		}

		logic->mConnInfo.mConnections ++;
		LeaveCriticalSection( &logic->mSqlCs );

		CNSVector< CNSPair< int, CNSOctets > > results;
		CNSVector< CNSString > fieldInfo;
		CNSString errorDesc;
		int column = 0;

		for ( ; 1; )
		{
			EnterCriticalSection( &logic->mSqlCs );
			HLISTINDEX headIndex = logic->mSqlQueues.getHead( );
			if ( headIndex == NULL )
			{
				LeaveCriticalSection( &logic->mSqlCs );
				Sleep( 1 );
				continue;
			}

			CSqlRequest sql = logic->mSqlQueues.getData( headIndex );
			if ( logic->mShowSql == true )
				NSLog::log( _UTF8( "执行sql - %s" ), sql.mSqlCommand.getBuffer( ) );

			logic->mSqlQueues.erase( headIndex );
			LeaveCriticalSection( &logic->mSqlCs );

			results.clear( );
			fieldInfo.clear( );
			errorDesc.clear( );
			column = 0;

			bool hasError = false;
			while ( 1 )
			{
				if ( CDataLogic::threadExec( &db, sql.mFullInfo, sql.mTransaction, sql.mSqlCommand, sql.mInputBuffer, column, fieldInfo, results, errorDesc ) == false )
				{
					if ( errorDesc.nocaseFindFirstOf( "Lost connection" ) != -1 )
					{
						db.Close( );
						db.Connect( ipSet[ 0 ], dbName, userName, psw, ipSet[ 1 ].toInteger( ) );
						Sleep( 1 );
						continue;
					}

					if ( errorDesc.nocaseFindFirstOf( "has gone away" ) != -1 )
					{
						db.Close( );
						db.Connect( ipSet[ 0 ], dbName, userName, psw, ipSet[ 1 ].toInteger( ) );
						Sleep( 1 );
						continue;
					}

					CSqlResponse response;
					response.mNetSessionID = sql.mNetSessionID;
					response.mDBSessionID = sql.mDBSessionID;
					response.mErrorDesc = errorDesc;
					response.mRecordSets = results;
					response.mFieldInfo = fieldInfo;
					response.mColumn = 0;
					response.mResultCode = 0;

					EnterCriticalSection( &logic->mSqlCs );
					NSLog::log( errorDesc.getBuffer( ) );
					logic->mSqlResponse.pushback( response );
					LeaveCriticalSection( &logic->mSqlCs );
					hasError = true;
				}

				break;
			}

			if ( hasError == true )
				continue;

			CSqlResponse response;
			response.mNetSessionID = sql.mNetSessionID;
			response.mDBSessionID = sql.mDBSessionID;
			response.mErrorDesc = errorDesc;
			response.mRecordSets = results;
			response.mFieldInfo = fieldInfo;
			response.mColumn = column;
			response.mResultCode = 1;

			EnterCriticalSection( &logic->mSqlCs );
			logic->mSqlResponse.pushback( response );
			LeaveCriticalSection( &logic->mSqlCs );
		}
	}

	void CDataLogic::loadMysqlPsw( CNSString& psw )
	{
		FILE* tpFile = fopen( "psw.txt", "rb" );
		if ( tpFile == NULL )
		{
			NSLog::log( _UTF8( "文件psw.txt打开失败, 将使用默认数据库密码" ) );
			return;
		}

		psw.clear( );
		fseek( tpFile, 0, SEEK_END );
		int size = ftell( tpFile );
		if ( size % 8 != 0 )
		{
			fclose( tpFile );
			static CNSString errorDesc;
			errorDesc.format( "文件不是数据库密码文件" );
			NSException( errorDesc );
		}

		fseek( tpFile, 0, SEEK_SET );
		char* tpText = new char[ size + 1 ];
		fread( tpText, sizeof( char ), size, tpFile );
		tpText[ size ] = 0;
		int index = 0;
		unsigned char ch = 0;
		for ( int i = 0; i < size; i ++ )
		{
			unsigned char bit = tpText[ i ] & 0x01;
			ch = ch | ( bit << index );
			index ++;
			if ( index == 8 )
			{
				psw += ch;
				ch = 0;
				index = 0;
			}
		}

		fclose( tpFile );
		delete[] tpText;
	}

	void CDataLogic::init( NSWin32::CConsoleApp< CDataLogic >* app )
	{
		mShowSql = app->getCmdBool( "showsql", false );

		TiXmlDocument doc;
		if ( doc.LoadFile( "serverconfig.xml" ) == false )
		{
			static CNSString errorDesc;
			errorDesc.format( _UTF8( "serverconfig.xml打开错误, 错误码: %s" ), doc.ErrorDesc( ) );
			NSException( errorDesc );
		}

		// 读取数据库密码
		CNSString psw = "NOSN20130409";
		loadMysqlPsw( psw );

		bool find = false;
		TiXmlElement* data = doc.FirstChildElement( "dataserver" );
		for ( ; data != NULL; data = data->NextSiblingElement( "dataserver" ) )
		{
			CNSString name = data->Attribute( "name" );
			if ( app->getName( ) == name )
			{
				find = true;
				static CNSString title;
#ifdef _M_IX86
				title.format( _UTF8( "数据服务器 - %s 32bit" ), app->getName( ).getBuffer( ) );
#elif _M_X64
				title.format( _UTF8( "数据服务器 - %s 64bit" ), app->getName( ).getBuffer( ) );
#endif

				app->setConsoleTitle( title );
				CNSString		address = app->name2IPPort( data->Attribute( "iaddress" ) );
				CNSString		dbAddress = app->name2IPPort( data->Attribute( "dbaddress" ) );
				CNSString		userName = data->Attribute( "username" );
				CNSString		dbName = data->Attribute( "dbname" );
				unsigned int	bufferSize = CNSString( data->Attribute( "buffersize" ) ).toInteger( );
				unsigned int	streamSize = CNSString( data->Attribute( "streamsize" ) ).toInteger( );
				unsigned int	interval = CNSString( data->Attribute( "interval" ) ).toInteger( );
				mThreadNumber = CNSString( data->Attribute( "thread" ) ).toInteger( );

				::InitializeCriticalSection( &mSqlCs );
				CNSTimer::createTimer( DataServer::dbTimerSQLLog, 5000, NULL );
				CNSTimer::createTimer( DataServer::dbTimerMysqlResponse, 10, NULL );

				mConnInfo.mDBAddress = dbAddress;
				mConnInfo.mIAddress = address;
				mConnInfo.mUserName = userName;
				mConnInfo.mPsw = psw;
				mConnInfo.mDBName = dbName;
				mConnInfo.mBufferSize = bufferSize;
				mConnInfo.mStreamSize = streamSize;
				mConnInfo.mInterval = interval;
				for ( unsigned int i = 0; i < mThreadNumber; i ++ )
				{
					DWORD threadID;
					CreateThread( NULL, 0, (LPTHREAD_START_ROUTINE) workThread, NULL, 0, &threadID );
				}

				CNSTimer::createTimer( dbTimerLogic, 10, NULL );
				break;
			}
		}

		if ( find == false )
		{
			static CNSString errorDesc;
			errorDesc.format( _UTF8( "数据库名称%s没有找到，该服务器不能提供数据库服务" ), errorDesc.getBuffer( ) );
			NSException( errorDesc );
		}
	}

	void CDataLogic::exit( )
	{
		delete mpDataServer;
	}
}

int main( int argc, char* argv[] )
{
	try
	{
		NSWin32::CConsoleApp< DataServer::CDataLogic > app;
		app.useNSMysql( false );
		app.useNSHttp( false );
		app.useNSHttpDebugger( false );
		app.useIPName( true );
		app.useNSLocal( true );
		app.run( );
	}
	catch ( CNSException& e )
	{
		// 还原标准输出
		freopen( "CON", "w", stdout );

		// 这里的异常处理是不会写文件的，stdout已经还原
		static CNSString errorDesc;
		errorDesc.format( _UTF8( "NSLog exception:\n%s\nCRT main函数发生异常\n错误描述: \n\t%s\nC++调用堆栈:\n%s" ), NSLog::sExceptionText.getBuffer( ),
						  e.mErrorDesc, NSBase::NSFunction::getStackInfo( ).getBuffer( ) );
		printf( CNSString::convertUtf8ToMbcs( errorDesc ) );
		getchar( );
		return 1;
	}

	return 0;
}