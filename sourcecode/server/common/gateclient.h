#pragma once
namespace GateClient
{
	class CInnerLogic;
	class CInnerClient;
	class CUser
	{
	public:
		long long mNosnUserID;
		unsigned int mSessionID;
		CNSString mGateName;

	public:
		CUser( long long nosnUserID = 0, unsigned int sessionID = 0, const CNSString& gateName = "" ) :
			mNosnUserID( nosnUserID ), mSessionID( sessionID ), mGateName( gateName )
		{
		}

	public:
		virtual ~CUser( )
		{
		}

	public:
		void send( CNSProtocol* proto );
	};

	// 内部中转服务器
	class CInnerClient : public CNSNetManager
	{
	public:
		CInnerLogic* mpLogic;

	public:
		CInnerClient( CInnerLogic* pLogic, const CNSString& name, const CNSString& address, unsigned int interval, unsigned int bufferSize, unsigned int streamSize )
			: CNSNetManager( name, address, interval, bufferSize, streamSize ), mpLogic( pLogic )
		{
		}

	public:
		virtual void onRegisterProtocol( const CNSString& name );
		virtual void onAddSession( const CNSString& name, TSessionID sessionID, const CNSSockAddr& local, const CNSSockAddr& peer );
		virtual void onAddSessionFault( const CNSString& name, int code );
		virtual void onDelSession( const CNSString& name, TSessionID sessionID );
		// 1		内网发4字节 4字节		1字节		n字节  4字节
		// pid		size	  ( tunnelsize( tunnelpid	data ) sessionid )
		// |		|			|
		// protoID	protoSize	protoBuffer
		virtual void onRawProtocol( const CNSString& name, TSessionID sessionID, TProtocolID* protoID, const CNSOctetsShadow& buffer );
		virtual void onProtocol( const CNSString& name, TSessionID sessionID, CNSProtocol* proto );
	};

	class CInnerLogic
	{
		friend class CInnerClient;

	protected:
		unsigned int mServiceID;
		CNSMap< CNSString, CInnerClient* > mInners;
		CNSMap< CNSString, CNSString > mIPTables;		// 网关玩家连接IP
		CNSMap< long long, CUser > mUsers;
		CNSMap< CNSString, long long > mNosnUsers;		// 玩家在网关的连接ID为Key, UserID为Value

	public:
		CInnerLogic( )
		{
		}

	public:
		virtual void onRegisterClientProtocol( CNSNetManager* pManager ) = 0;
		virtual void onClientLogin( const CNSString& name, TSessionID sessionID, long long nosnUserID ) = 0;
		virtual void onClientLost( const CNSString& name, TSessionID sessionID, long long nosnUserID ) = 0;
		virtual void onClientProtocol( CInnerClient* inner, TSessionID sessionID, CNSProtocol* proto ) = 0;
		virtual void onConnectGate( const CNSString& name, const CNSString& address );
		virtual void onConnectGateFault( const CNSString& name, const CNSString& address, unsigned int code );
		virtual void onGateLost( const CNSString& name, const CNSString& address );

	public:
		void send2UserList( const CInnerClient** gate, CNSVector< TSessionID >* sessionList, CNSProtocol* proto );
		void send2User( const CInnerClient* gate, TSessionID sessionID, CNSProtocol* proto );
		void send2User( const CNSString& name, TSessionID sessionID, CNSProtocol* proto );
		void send2User( long long userID, CNSProtocol* proto );
		void enableIPOpen( bool enable );
		// 通过网关名和在网关的回话ID获得用户ID
		bool getUserID( const CNSString& name, TSessionID sessionID, long long& nosnUserID );
		void bindUser( const CNSString& name, TSessionID sessionID, long long nosnUserID );
		void unBindUser( const CNSString& name, TSessionID sessionID, long long nosnUserID );
		void exit( );
	};
}