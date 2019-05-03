#pragma once
namespace DataServer
{
	class CSqlRequest
	{
	public:
		unsigned int	mNetSessionID;
		unsigned int	mDBSessionID;
		bool			mFullInfo;
		bool			mTransaction;
		CNSString		mSqlCommand;
		CNSOctets		mInputBuffer;
	};

	class CSqlResponse
	{
	public:
		unsigned int									mNetSessionID;
		unsigned int									mDBSessionID;		// 会话ID
		unsigned int									mColumn;
		unsigned int									mResultCode;
		CNSVector< CNSString >							mFieldInfo;
		CNSVector< CNSPair< int, CNSOctets > >			mRecordSets;
		CNSMap< int, CNSMap< CNSString, CNSOctets > >	mBlobData;
		CNSString										mErrorDesc;
	};

	class CDBConnInfo
	{
	public:
		CNSString mPsw;
		CNSString mDBAddress;
		CNSString mIAddress;
		CNSString mUserName;
		CNSString mDBName;
		unsigned int mBufferSize;
		unsigned int mStreamSize;
		unsigned int mInterval;
		unsigned int mConnections = 0;
	};

	class CDataServer : public CNSNetManager
	{
	public:
		NSTool::CMySqlDatabase mDB;

	public:
		CDataServer( const CNSString& name, const CNSString& address )
			: CNSNetManager( name, address, 10, 512 * 1024, 512 * 1024 )
		{
		}

	public:
		virtual void onAddSession( const CNSString& name, TSessionID sessionID, const CNSSockAddr& local, const CNSSockAddr& peer );
		virtual void onDelSession( const CNSString& name, TSessionID sessionID );
		virtual void onRegisterProtocol( const CNSString& name );
		virtual void onProtocol( const CNSString& name, TSessionID sessionID, CNSProtocol* proto );
	};

	class CDataLogic
	{
	public:
		CNSMap< TSessionID, CNSString >	mDBClients;
		bool							mInShutdown = false;
		CNSList< CSqlRequest >			mSqlQueues;
		CNSList< CSqlResponse >			mSqlResponse;
		CDBConnInfo						mConnInfo;
		CRITICAL_SECTION				mSqlCs;
		CDataServer*					mpDataServer = NULL;
		LONG							mUsedTick = 0;
		LONG							mUsedCount = 0;
		LONG							mQueryCount = 0;
		bool							mShowSql = false;
		unsigned int					mThreadNumber = 0;

	public:
		static bool threadExec( NSTool::CMySqlDatabase* db, bool fullInfo, bool transaction, const CNSString& rSqlCommand, const CNSOctets &rInputBuffer, int& rColumn,
								CNSVector< CNSString >& fieldInfo, CNSVector< CNSPair< int, CNSOctets > >& rResult, CNSString& errorDesc );

		static void workThread( void* param );

	protected:
		void loadMysqlPsw( CNSString& psw );

	public:
		void init( NSWin32::CConsoleApp< CDataLogic >* app );
		void exit( );
	};

	void dbTimerSQLLog( int timerID, unsigned int curTick, int remain, void* userdata )
	{
		CDataLogic* logic = NSWin32::CConsoleApp< CDataLogic >::getLogic( );
		int sqlCount = logic->mSqlQueues.getCount( );
		NSLog::log( "sql queues count - %d", sqlCount );

		int resCount = logic->mSqlResponse.getCount( );
		NSLog::log( "sql response count - %d", resCount );
	}

	static void dbTimerShutdown( int timerID, unsigned int curTick, int remain, void* userdata )
	{
		CDataLogic* logic = NSWin32::CConsoleApp< CDataLogic >::getLogic( );
		logic->mInShutdown = true;
		NSNet::close( "data" );
	}

	static void dbTimerLogic( int timerID, unsigned int curTick, int remain, void* userdata )
	{
		CDataLogic* logic = NSWin32::CConsoleApp< CDataLogic >::getLogic( );
		if ( logic->mConnInfo.mConnections == logic->mThreadNumber && logic->mpDataServer == NULL )
		{
			logic->mpDataServer = new CDataServer( "data", logic->mConnInfo.mIAddress );
			registerServer( logic->mpDataServer, 5 );
			NSLog::log( _UTF8( "数据服务器[%s]端口打开成功" ), logic->mConnInfo.mIAddress.getBuffer( ) );
			NSLog::log( _UTF8( "\tbuffsize - %d" ), logic->mConnInfo.mBufferSize );
			NSLog::log( _UTF8( "\tstreamsize - %d" ), logic->mConnInfo.mStreamSize );
			NSLog::log( _UTF8( "\tinterval - %d" ), logic->mConnInfo.mInterval );
		}

		if ( logic->mInShutdown == true )
		{
			if ( logic->mSqlQueues.getCount( ) == 0 )
				exit( 0 );
		}
	}

	static void dbTimerMysqlResponse( int timerID, unsigned int curTick, int remain, void* userdata )
	{
		CDataLogic* logic = NSWin32::CConsoleApp< CDataLogic >::getLogic( );
		EnterCriticalSection( &logic->mSqlCs );
		HLISTINDEX beginIndex = logic->mSqlResponse.getHead( );
		for ( ; beginIndex != NULL; logic->mSqlResponse.getNext( beginIndex ) )
		{
			CSqlResponse& res = logic->mSqlResponse.getData( beginIndex );
			NSMysql::CProtocolExecuteSqlResponse response( res.mDBSessionID, res.mColumn, res.mResultCode, res.mFieldInfo, res.mRecordSets, res.mErrorDesc );
			NSNet::send( logic->mpDataServer->getNetworkIO( ), response.createStream( ), res.mNetSessionID );
		}
		logic->mSqlResponse.clear( );
		LeaveCriticalSection( &logic->mSqlCs );
	}
}
