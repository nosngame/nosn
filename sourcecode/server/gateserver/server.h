#pragma once

namespace GateServer
{
	class CService
	{
	public:
		unsigned int	mServiceID;
		TSessionID		mSessionID;

	public:
		CService( unsigned int serviceID, TSessionID sessionID ) : mServiceID( serviceID ), mSessionID( sessionID )
		{
		}
	};

	class CGateUser
	{
	public:
		TSessionID		mSessionID;
		unsigned int	mCurService;
		bool			mFreeze;
		unsigned int	mCurTick;
		unsigned int	mCheatCounter;

	public:
		CGateUser( TSessionID sessionID ) : mSessionID( sessionID ), mCurService( 0 ), mFreeze( false ), mCurTick( 0 ), mCheatCounter( 0 )
		{
		}
	};

	// 内部中转服务器
	class CInnerServer : public CNSNetManager
	{
	public:
		CInnerServer( const CNSString& name, const CNSString& address, unsigned int interval, unsigned int bufferSize, unsigned int streamSize )
			: CNSNetManager( name, address, interval, bufferSize, streamSize )
		{
		}

	public:
		virtual void onRegisterProtocol( const CNSString& name )
		{
			static NSGateProto::CProtocolRegisterLogicService	registerLogicService;
			static NSGateProto::CProtocolRegisterOperService registerOperService;
			static NSGateProto::CProtocolTunnel tunnel( NULL, 0, NSGateProto::CProtocolTunnel::PID );
			static NSGateProto::CProtocolBatchTunnel batchTunnel;
			static NSGateProto::CProtocolChangeService changeService;
			static NSGateProto::CProtocolUnknownUser unknownUser;
			static NSGateProto::CProtocolEnableIPOpen	enableIPOpen;
			static NSGateProto::CProtocolShutdownGate	shutdownGate;
			static NSGateProto::CProtocolClientLogin clientLogin;

			registerProtocol( &registerLogicService );
			registerProtocol( &registerOperService );
			registerProtocol( &tunnel );
			registerProtocol( &batchTunnel );
			registerProtocol( &changeService );
			registerProtocol( &unknownUser );
			registerProtocol( &enableIPOpen );
			registerProtocol( &shutdownGate );
			registerProtocol( &clientLogin );
		}

		virtual void onAddSession( const CNSString& name, TSessionID sessionID, const CNSSockAddr& local, const CNSSockAddr& peer );
		virtual void onDelSession( const CNSString& name, TSessionID sessionID );
		virtual void onProtocol( const CNSString& name, TSessionID sessionID, CNSProtocol* proto );
		virtual void onRawProtocol( const CNSString& name, TSessionID sessionID, TProtocolID* protoID, const CNSOctetsShadow& buffer );
	};

	// 外部服务器
	class COuterServer : public CNSNetManager
	{
	public:
		COuterServer( const CNSString& name, const CNSString& address, unsigned int interval, unsigned int bufferSize, unsigned int streamSize )
			: CNSNetManager( name, address, interval, bufferSize, streamSize )
		{
		}

	public:
		bool sizePolicy( unsigned int protoType, unsigned int protoSize )
		{
			// 发送什么协议都不可能大过4K
			if (protoSize > 4096)
				return false;

			return true;
		}

		virtual void onRegisterProtocol( const CNSString& name )
		{
			static NSGateProto::CProtocolTunnel tunnel( NULL, 0, NSGateProto::CProtocolTunnel::PID );
			static NSGateProto::CProtocolTunnel operTunnel( NULL, 0, NSGateProto::CProtocolTunnel::PPID );
			static NSGateProto::CProtocolKeepAlive keepAlive;

			registerProtocol( &tunnel );
			registerProtocol( &operTunnel );
			registerProtocol( &keepAlive );
		}

		virtual void onAddSession( const CNSString& name, TSessionID sessionID, const CNSSockAddr& rLocal, const CNSSockAddr& rPeer );
		virtual void onDelSession( const CNSString& name, TSessionID sessionID );
		virtual void onProtocol( const CNSString& name, TSessionID sessionID, CNSProtocol* proto );
		virtual void onRawProtocol( const CNSString& name, TSessionID sessionID, TProtocolID* protoID, const CNSOctetsShadow& buffer );
	};

	class CGateLogic
	{
	public:
		CNSMap< TSessionID, CGateUser* > mUsers; // 回话ID为Key
		CService* mServices[ 1024 ] = { NULL };	// 服务ID为Key
		CNSSet< CNSString >	 mIPOpens; // IP白名单
		bool mEnableIP;
		CInnerServer* mpInner = NULL;
		COuterServer* mpOuter = NULL;
		CNSString mGateName; // 本进程名称
		bool mDevelopment = true;

	public:
		CGateLogic( ) : mEnableIP( true )
		{
		}

	public:
		bool checkIPOpen( CNSString ipAddress );
		void boardcast( CNSProtocol* proto );
		CGateUser* getUser( TSessionID sessionID );
		void getUserBySessionID( TSessionID sessionID, CNSVector< TSessionID >& userSession );
		CService* getOperService( ) const;
		CService* getService( unsigned int serviceID );
		void addUser( TSessionID sessionID );
		void removeUser( TSessionID sessionID );

		void changeService( unsigned int serviceID, TSessionID sessionID );
		void registerOperService( TSessionID sessionID );
		void registerLogicService( unsigned int serviceID, TSessionID sessionID );
		void removeService( unsigned int serviceID );

		void onInnerAddSession( const CNSString& name, TSessionID sessionID, const CNSSockAddr& local, const CNSSockAddr& peer );
		void onInnerDelSession( const CNSString& name, TSessionID sessionID );
		void onInnerProtocol( const CNSString& name, TSessionID sessionID, CNSProtocol* proto );

		void onOuterAddSession( const CNSString& name, TSessionID sessionID, const CNSSockAddr& local, const CNSSockAddr& peer );
		void onOuterDelSession( const CNSString& name, TSessionID sessionID );
		void onOuterProtocol( const CNSString& name, TSessionID sessionID, CNSProtocol* proto );
		void loadIPOpen( );
		void init( NSWin32::CConsoleApp< CGateLogic >* app );
		void exit( );
	};

}