#include <nsbase.h>
namespace NSNet
{
	bool mNetInitialize = false;
	CNSMap< CNSString, CNSNetworkIO* >	networkIOs;
	CNSNetworkIO* getNetworkIO( const CNSString& name )
	{
		CNSNetworkIO** tpIORef = networkIOs.get( name );
		if (tpIORef == NULL)
			return NULL;

		return *tpIORef;
	}

	CNSString getHostName( )
	{
		char hostName[ 128 ] = { 0 };
		if (gethostname( hostName, 128 ) != 0)
		{
			static CNSString errorDesc;
#ifdef PLATFORM_WIN32
			errorDesc.format( _UTF8( "函数[gethostname]错误, 错误码: %d" ), WSAGetLastError( ) );
#else
            errorDesc.format( _UTF8( "函数[gethostname]错误" ) );
#endif
			NSException( errorDesc );
		}

		return hostName;
	}

	void getIPV4Address( CNSVector< CNSString >& address )
	{
#ifdef PLATFORM_WIN32
		char hostName[ 128 ] = { 0 };
		if (gethostname( hostName, 128 ) != 0)
		{
			static CNSString errorDesc;
			errorDesc.format( _UTF8( "函数[gethostname]错误, 错误码: %d" ), WSAGetLastError( ) );
            NSException( errorDesc );
		}

		struct addrinfo hints;
		struct addrinfo *res, *cur;
		char ipv4Address[ 16 ];

		memset( &hints, 0, sizeof( struct addrinfo ) );
		hints.ai_family = AF_INET; /* Allow IPv4 */
		hints.ai_flags = AI_PASSIVE; /* For wildcard IP address */
		hints.ai_protocol = 0; /* Any protocol */
		hints.ai_socktype = SOCK_STREAM;

		int ret = getaddrinfo( hostName, NULL, &hints, &res );
		if (ret == -1)
		{
			static CNSString errorDesc;
			errorDesc.format( "函数[getaddrinfo]调用失败, errorCode - %d", WSAGetLastError( ) );
			NSException( errorDesc );
		}

		for (cur = res; cur != NULL; cur = cur->ai_next)
		{
			struct sockaddr_in *addr = ( struct sockaddr_in * ) cur->ai_addr;
			if (inet_ntop( AF_INET, &addr->sin_addr, ipv4Address, 16 ) == NULL)
			{
				static CNSString errorDesc;
				errorDesc.format( "函数[inet_ntop]调用失败, errorCode - %d", WSAGetLastError( ) );
				NSException( errorDesc );
			}

			address.pushback( CNSString( ipv4Address ) );
		}

		freeaddrinfo( res );
#endif
	}

	void init( )
	{
		if (mNetInitialize == true)
			return;

		mNetInitialize = true;
#ifdef PLATFORM_WIN32
		int tResult = WSAStartup( MAKEWORD( 2, 2 ), &WSADATA( ) );
		if (tResult != NO_ERROR)
		{
			DWORD errCode = GetLastError( );
			CNSString errorDesc;
			errorDesc.format( _UTF8( "WSAStartup 函数调用失败，错误码: %d" ), errCode );
			NSException( errorDesc );
			return;
		}
#endif
	}

	void exit( )
	{
		if (mNetInitialize == false)
			return;

		HLISTINDEX beginIndex = networkIOs.getHead( );
		for (; beginIndex != NULL; networkIOs.getNext( beginIndex ))
		{
			CNSNetworkIO* io = networkIOs.getValue( beginIndex );
			delete io;
		}

		networkIOs.clear( );
#ifdef PLATFORM_WIN32
		WSACleanup( );
#endif
	}
    
#ifdef PLATFORM_WIN32
	CNSNetworkIO* registerServer( CNSNetManager* manager, unsigned int acceptNum )
	{
		if (mNetInitialize == false)
		{
			NSException( _UTF8( "WSASocket Not initialize" ) );
			return NULL;
		}

#ifdef _DEBUG
		acceptNum = 1;
#endif
		static CNSVector< CNSString > stringList;
		stringList.clear( );
		manager->mAddress.split( ":", stringList );
		CNSString address = stringList[ 0 ];
		CNSString port = stringList[ 1 ];
		CNSPassiveIO* passive = new CNSPassiveIO( manager, manager->mName, acceptNum );
		passive->create( address, port );

		manager->setNetworkIO( passive );
		manager->onRegisterProtocol( port );
		networkIOs.insert( manager->mName, passive );
		return passive;
	}
#endif
	// CNSNetManager的回调函数全都是处在网络线程锁中，
	// 因此不能在CNSNetManager的回调函数中调用线程安全函数
	CNSNetworkIO* registerClient( CNSNetManager* manager )
	{
		if (mNetInitialize == false)
		{
			NSException( _UTF8( "WSASocket 没有初始化" ) );
			return NULL;
		}

		static CNSVector< CNSString > stringList;
		stringList.clear( );
		manager->mAddress.split( ":", stringList );
		CNSString		address = stringList[ 0 ];
		CNSString		port = stringList[ 1 ];
		CNSActiveIO*	active = new CNSActiveIO( manager, manager->mName );
		active->create( address, port );

		manager->setNetworkIO( active );
		manager->onRegisterProtocol( manager->mName );
		networkIOs.insert( manager->mName, active );
		return active;
	}

	void pollEvent( )
	{
		unsigned int thisTick = CNSTimer::getCurTick( );
		static CNSVector< CNSNetworkIO* > ioTombs;
		ioTombs.clear( );

		HLISTINDEX beginIndex = networkIOs.getHead( );
		for (; beginIndex != NULL; networkIOs.getNext( beginIndex ))
		{
			CNSNetworkIO* io = networkIOs.getValue( beginIndex );
			io->pollEvent( thisTick );
			if (io->isDeleted( ) == true)
				ioTombs.pushback( io );
		}

		for (unsigned int i = 0; i < ioTombs.getCount( ); i ++)
		{
			// 将要被删除的对象
			CNSNetworkIO* io = ioTombs[ i ];

			// 是否存在同名对象，因为重连，可能会复用相同的名字
			CNSNetworkIO** ioRef = networkIOs.get( io->mName );

			// 如果NetworkIO是同一个对象，说明没有重连，那么需要删除之前的
			if (ioRef != NULL && ( *ioRef ) == io)
				networkIOs.erase( io->mName );

			delete io;
		}
	}

	void close( const CNSString& name, TSessionID sessionID )
	{
		CNSNetworkIO* io = getNetworkIO( name );
		if (io == NULL)
			return;

		io->destroy( sessionID );
	}

	void send( CNSNetworkIO* io, const CNSOctets& buffer, TSessionID sessionID, bool force )
	{
		if (io == NULL)
			return;

		io->send( sessionID, buffer );
	}

	void send( const CNSString& name, const CNSOctets& buffer, TSessionID sessionID, bool force )
	{
		CNSNetworkIO* io = getNetworkIO( name );
		if (io == NULL)
			return;

		io->send( sessionID, buffer );
	}

	void send( CNSNetworkIO* io, const void* begin, const void* end, TSessionID sessionID, bool force )
	{
		io->send( sessionID, begin, end );
	}

	void send( CNSNetworkIO* io, const void* begin, size_t len, TSessionID sessionID, bool force )
	{
		io->send( sessionID, begin, len );
	}

	CNSSessionDesc* getSessionDesc( const CNSString& name, TSessionID sessionID )
	{
		CNSNetworkIO* io = getNetworkIO( name );
		if (io == NULL)
			return NULL;

		return io->getDesc( sessionID );
	}

	// ********************************************************************** //
	// CNSProtocol
	// ********************************************************************** //
	TProtocolID CNSProtocol::getProtoID( ) const
	{
		return mProtoID;
	}

	// ********************************************************************** //
	// CNSNetManager
	// ********************************************************************** //
	CNSNetManager::CNSNetManager( const CNSString& name, const CNSString& address, unsigned int pollInterval, unsigned int bufferSize, unsigned int streamSize, bool isHttp, bool isNotifyProgress ) :
		mName( name ), mAddress( address ), mPollInterval( pollInterval ), mPollTimeout( 0 ), mBufferSize( bufferSize ), mStreamSize( streamSize ), mIsHttp( isHttp ), mIsNotifyProgress( isNotifyProgress ), mpNetworkIO( NULL )
	{
	}

	CNSProtocol* CNSNetManager::createProtocol( TProtocolID protoType )
	{
		CNSProtocol* proto = NULL;
		if (mProtocolStubs.find( protoType, proto ) == false)
			return NULL;

		return proto;
	}

	void CNSNetManager::registerProtocol( CNSProtocol* proto )
	{
		CNSProtocol* tpFindProtocol = NULL;
		if (mProtocolStubs.find( proto->getProtoID( ), tpFindProtocol ) == true)
		{
			static CNSString sErrorDesc;
			sErrorDesc.format( _UTF8( "协议重复注册，协议ID[%d]" ), proto->getProtoID( ) );
			return;
		}

		mProtocolStubs.insert( proto->getProtoID( ), proto );
	}

};
