#pragma once
namespace LogicServer
{
	class CBandStat
	{
	public:
		long long		mRawBytes;
		long long		mSendBytes;
		unsigned int	mRawRate;
		unsigned int	mSendRate;
		unsigned int	mRawAvgRate;
		unsigned int	mSendAvgRate;
	};

	class CDBConnInfo : public CNSLuaMarshal
	{
	public:
		CNSString		mAddress;
		unsigned int	mBufferSize;
		unsigned int	mStreamSize;
		unsigned int	mInterval;

	public:
		CDBConnInfo( const CNSString& address = "", unsigned int interval = 0, unsigned int bufferSize = 0, unsigned int streamSize = 0 )
			: mAddress( address ), mBufferSize( bufferSize ), mStreamSize( streamSize ), mInterval( interval )
		{
		}

	public:
		virtual CNSLuaStack& marshal( CNSLuaStack& luaStack ) const
		{
			luaStack.pushField( "mAddress", mAddress );
			luaStack.pushField( "mBufferSize", mBufferSize );
			luaStack.pushField( "mStreamSize", mStreamSize );
			luaStack.pushField( "mInterval", mInterval );
			return luaStack;
		}

		virtual const CNSLuaStack& unmarshal( const CNSLuaStack& luaStack )
		{
			luaStack.popField( "mAddress", mAddress );
			luaStack.popField( "mBufferSize", mBufferSize );
			luaStack.popField( "mStreamSize", mStreamSize );
			luaStack.popField( "mInterval", mInterval );
			return luaStack;
		}
	};

	class COperClient : public CNSNetManager
	{
	public:
		COperClient( const CNSString& name, const CNSString& address, unsigned int interval, unsigned int bufferSize, unsigned int streamSize )
			: CNSNetManager( name, address, interval, bufferSize, streamSize )
		{
		}

	public:
		virtual void onRegisterProtocol( const CNSString& name );
		virtual void onAddSessionFault( const CNSString& name, int code );
		virtual void onAddSession( const CNSString& name, TSessionID sessionID, const CNSSockAddr& local, const CNSSockAddr& peer );
		virtual void onDelSession( const CNSString& name, TSessionID sessionID );
		virtual void onProtocol( const CNSString& name, TSessionID sessionID, CNSProtocol* proto );
	};

	class CChargeClient : public CNSNetManager
	{
	public:
		CChargeClient( const CNSString& name, const CNSString& address, unsigned int interval, unsigned int bufferSize, unsigned int streamSize )
			: CNSNetManager( name, address, interval, bufferSize, streamSize )
		{
		}

	public:
		virtual void onRegisterProtocol( const CNSString& name );
		virtual void onAddSessionFault( const CNSString& name, int code );
		virtual void onAddSession( const CNSString& name, TSessionID sessionID, const CNSSockAddr& local, const CNSSockAddr& peer );
		virtual void onDelSession( const CNSString& name, TSessionID sessionID );
		virtual void onProtocol( const CNSString& name, TSessionID sessionID, CNSProtocol* proto );
	};

	class CHttpServer : public CNSNetManager
	{
	public:
		CHttpServer( const CNSString& name, const CNSString& address, unsigned int interval, unsigned int bufferSize, unsigned int streamSize )
			: CNSNetManager( name, address, interval, bufferSize, streamSize, true )
		{
		}

	public:
		virtual void onRegisterProtocol( const CNSString& name ) {}
		virtual void onAddSession( const CNSString& name, TSessionID sessionID, const CNSSockAddr& local, const CNSSockAddr& peer ) {}
		virtual void onDelSession( const CNSString& name, TSessionID sessionID ) {}
		virtual void onProtocol( const CNSString& name, TSessionID sessionID, const CNSMap< CNSString, CNSString >& headers, const CNSString& text, const CNSString& postData );
	};

	class CLogicServer : public CNSNetManager
	{
	public:
		CNSString mLogicName;

	public:
		CLogicServer( const CNSString& name, const CNSString& address, unsigned int interval, unsigned int bufferSize, unsigned int streamSize )
			: CNSNetManager( name + "_server", address, interval, bufferSize, streamSize, false ), mLogicName( name )
		{
		}

	public:
		virtual void onRegisterProtocol( const CNSString& name );
		virtual void onAddSession( const CNSString& name, TSessionID sessionID, const CNSSockAddr& local, const CNSSockAddr& peer );
		virtual void onDelSession( const CNSString& name, TSessionID sessionID );
		virtual void onProtocol( const CNSString& name, TSessionID sessionID, CNSProtocol* proto );
	};

	class CLogicClient : public CNSNetManager
	{
	public:
		CNSString mLogicName;

	public:
		CLogicClient( const CNSString& name, const CNSString& address, unsigned int interval, unsigned int bufferSize, unsigned int streamSize )
			: CNSNetManager( name + "_client", address, interval, bufferSize, streamSize ), mLogicName( name )
		{
		}

	public:
		virtual void onRegisterProtocol( const CNSString& name );
		virtual void onAddSession( const CNSString& name, TSessionID sessionID, const CNSSockAddr& local, const CNSSockAddr& peer );
		virtual void onAddSessionFault( const CNSString& name, int code );
		virtual void onDelSession( const CNSString& name, TSessionID sessionID );
		virtual void onProtocol( const CNSString& name, TSessionID sessionID, CNSProtocol* proto );
	};

	class CLogic : public GateClient::CInnerLogic
	{
	public:
		CNSString							mWorkPath;
		COperClient*						mpOperClient;
		CChargeClient*						mpChargeClient;

		CNSVector< CLogicClient* >			mLogicClient;
		CNSVector< CLogicServer* >			mLogicServer;
		CNSVector< CHttpServer* >			mHttpServer;
		CNSSet< CNSString >					mKeywords;
		CNSMap< CNSString, CDBConnInfo >	mDBConn;

	public:
		CLogic( );

	public:
		virtual void onRegisterClientProtocol( CNSNetManager* pManager );
		virtual void onClientProtocol( GateClient::CInnerClient* inner, TSessionID sessionID, CNSProtocol* proto );
		virtual void onClientLogin( const CNSString& name, TSessionID sessionID, long long nosnUserID );
		virtual void onClientLost( const CNSString& name, TSessionID sessionID, long long nosnUserID );
		virtual void onConnectGate( const CNSString& name, const CNSString& address );

	public:
		void onLaunchServer( );
		void send2OperServer( CNSProtocol* proto );
		void send2ChargeServer( CNSProtocol* proto );
		void send2LogicServer( const CNSString& logicName, CNSProtocol* proto );
		void send2LogicClient( const CNSString& logicName, TSessionID sessionID, CNSProtocol* proto );

		void init( NSWin32::CConsoleApp< CLogic >* app );
		void exit( );
	};
};
