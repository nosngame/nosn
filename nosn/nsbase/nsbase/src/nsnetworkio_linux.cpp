#include <fbbase.h>
// ********************************************************************** //
// CFBNetworkIO
// ********************************************************************** //
CFBNetworkIO::CFBNetworkIO( const CFBString& rName ) : mName( rName )
{
}

// ********************************************************************** //
// CFBPassiveIO
// ********************************************************************** //

CFBPassiveIO::CFBPassiveIO( CFBNetManager* pManager, const CFBString& rName ) : CFBNetworkIO( rName ), mpManager( pManager ), 
	mListenSocket( NULL ), mEPollHandle( NULL )
{
}

CFBPassiveIO::~CFBPassiveIO( )
{
	close( mListenSocket );
	close( mEPollHandle );
}

CFBSession* CFBPassiveIO::GetSession( TSessionID vSessionID )
{
	CFBSession* tpSession = NULL;
	if ( mSessions.Find( vSessionID, tpSession ) == true )
		return tpSession;

	return NULL;
}

int CFBPassiveIO::Send( TSessionID vSessionID, CFBProtocol* pProtocol )
{
	if ( vSessionID == 0 )
	{
		CFBVector< CFBSession* > tSessions;
		HLISTINDEX tBeginIndex = mSessions.GetHead( );
		for ( ; tBeginIndex != NULL; mSessions.GetNext( tBeginIndex ) )
		{
			CFBSession* tpSession = mSessions.GetValue( tBeginIndex );
			tSessions.PushBack( tpSession );
		}

		for ( size_t i = 0; i < tSessions.GetCount( ); i ++ )
		{
			CFBSession* tpSession = tSessions[ i ];
			if ( tpSession->Send( pProtocol->Encode( CFBOctetsStream( ), mpManager->isClientTransfer( ) ) ) == true )
				PermitSend( tpSession );
		}

		return FBNet::ERESULT_SUCCESS;
	}

	CFBSession* tpSession = GetSession( vSessionID );
	if ( tpSession == NULL )
		return FBNet::ERESULT_SESSIONLOST;

	if ( tpSession->Send( pProtocol->Encode( CFBOctetsStream( ), mpManager->isClientTransfer( ) ) ) == true )
		return PermitSend( tpSession );

	return FBNet::ERESULT_SUCCESS;
}

int CFBPassiveIO::Send( TSessionID vSessionID, const CFBString& rText )
{
	CFBOctets tBuffer( rText.GetBuffer( ), rText.GetCount( ) );
	if ( vSessionID == 0 )
	{
		CFBVector< CFBSession* > tSessions;
		HLISTINDEX tBeginIndex = mSessions.GetHead( );
		for ( ; tBeginIndex != NULL; mSessions.GetNext( tBeginIndex ) )
		{
			CFBSession* tpSession = mSessions.GetValue( tBeginIndex );
			tSessions.PushBack( tpSession );
		}

		for ( size_t i = 0; i < tSessions.GetCount( ); i ++ )
		{
			CFBSession* tpSession = tSessions[ i ];
			if ( tpSession->Send( tBuffer ) == true )
				PermitSend( tpSession );
		}

		return FBNet::ERESULT_SUCCESS;
	}

	CFBSession* tpSession = GetSession( vSessionID );
	if ( tpSession == NULL )
		return FBNet::ERESULT_SESSIONLOST;

	if ( tpSession->Send( tBuffer ) == true )
		return PermitSend( tpSession );

	return FBNet::ERESULT_SUCCESS;
}

void CFBPassiveIO::Destroy( unsigned int vSessionID )
{
	if ( vSessionID == 0 )
	{
		HLISTINDEX tBeginIndex	= mSessions.GetHead( );
		for ( ; tBeginIndex != NULL; mSessions.GetNext( tBeginIndex ) )
		{
			CFBSession* tpSession = mSessions.GetValue( tBeginIndex );
			if ( tpSession == NULL )
				continue;

			mpManager->OnDelSession( mName.GetBuffer( ), tpSession->mSessionID );
			closesocket( tpSession->mPeerSocket );
			DELETEPTR tpSession;
		}

		mSessions.Clear( );
	}
	else
	{
		CFBSession* tpSession = GetSession( vSessionID );
		if ( tpSession == NULL )
			return;

		mpManager->OnDelSession( mName.GetBuffer( ), vSessionID );
		closesocket( tpSession->mPeerSocket );
		DELETEPTR tpSession;
		mSessions.Erase( vSessionID );
	}
}

bool CFBPassiveIO::Create( const CFBString& rAddress, const CFBString& rPort )
{
	mIOPort = ::CreateIoCompletionPort( INVALID_HANDLE_VALUE, 0, 0, 1 );

	// 注册一个被动IO
	if ( ( mListenSocket = ::WSASocket( AF_INET, SOCK_STREAM, 0, 0, 0, WSA_FLAG_OVERLAPPED ) ) == INVALID_SOCKET )
	{
		DWORD errorCode = GetLastError( );
		CFBString errorDesc;
		errorDesc.Format( "服务器错误[WSASocket 函数调用失败，错误码 - %d]", errorCode );
		log( errorDesc );
		return false;
	}

	SOCKADDR_IN tAddr;
	tAddr.sin_family		= AF_INET;
	tAddr.sin_addr.s_addr	= inet_addr( rAddress.GetBuffer( ) );
	tAddr.sin_port			= htons( rPort.ToInteger( ) );

	if ( ::bind( mListenSocket, (SOCKADDR*) &tAddr, sizeof( SOCKADDR_IN ) ) == SOCKET_ERROR )
	{
		DWORD errorCode = GetLastError( );
		CFBString errorDesc;
		errorDesc.Format( "服务器错误[端口打开失败, 端口地址 - %s:%s]", rAddress.GetBuffer( ), rPort.GetBuffer( ) );
		log( errorDesc );
		return false;
	}
	
	if ( ::listen( mListenSocket, ACCEPTNUM ) == SOCKET_ERROR )
	{
		DWORD errorCode = GetLastError( );
		CFBString errorDesc;
		errorDesc.Format( "服务器错误[listen 函数调用失败，错误码 - %d]", errorCode );
		log( errorDesc );
		return false;
	}

	if ( ::CreateIoCompletionPort( (HANDLE) mListenSocket, mIOPort, 0, 0 ) == 0 )
	{
		DWORD errorCode = GetLastError( );
		CFBString errorDesc;
		errorDesc.Format( "服务器错误[CreateIoCompletionPort 函数调用失败，错误码 - %d]", errorCode );
		log( errorDesc );
		return false;
	}

	for ( int i = 0; i < ACCEPTNUM; i ++ )
	{
		if ( Accept( i ) == false )
			return false;
	}

	return true;
}

bool CFBPassiveIO::Accept( int vIndex )
{
	mAcceptSocket[ vIndex ] = ::WSASocket( AF_INET, SOCK_STREAM, 0, 0, 0, WSA_FLAG_OVERLAPPED );
	if ( mAcceptSocket[ vIndex ] == INVALID_SOCKET )
	{
		DWORD errorCode = GetLastError( );
		CFBString errorDesc;
		errorDesc.Format( "服务器错误[Accept 函数调用失败，错误码 - %d]", errorCode );
		log( errorDesc );
		return false;
	}

	unsigned int tBufferSize = mpManager->GetBufferSize( );
	int tRet = 0;
	tRet = setsockopt( mAcceptSocket[ vIndex ], SOL_SOCKET, SO_SNDBUF, (char*) &tBufferSize, sizeof( unsigned int ) );
	tRet = setsockopt( mAcceptSocket[ vIndex ], SOL_SOCKET, SO_RCVBUF, (char*) &tBufferSize, sizeof( unsigned int ) );

	bool tKeepAlive = true;
	setsockopt( mAcceptSocket[ vIndex ], SOL_SOCKET, SO_KEEPALIVE, (char*) tKeepAlive, sizeof( bool ) );

	bool tTcpDelay = false;
	setsockopt( mAcceptSocket[ vIndex ], IPPROTO_TCP, TCP_NODELAY, (char*) &tTcpDelay, sizeof( bool ) );
	DWORD tBytes;
	if ( ::AcceptEx( mListenSocket, mAcceptSocket[ vIndex ], mAcceptBuffer[ vIndex ].Begin( ), 0, sizeof( SOCKADDR_IN ) + 16,
		sizeof( SOCKADDR_IN ) + 16, &tBytes, (OVERLAPPED*) &mAcceptOverlapped[ vIndex ] ) == 0 )
	{
		int tRetError = ::GetLastError( );
		if ( tRetError != ERROR_IO_PENDING )
		{
			DWORD errorCode = GetLastError( );
			CFBString errorDesc;
			errorDesc.Format( "服务器错误[AcceptEx 函数调用失败，错误码 - %d]", errorCode );
			log( errorDesc );
			return false;
		}
	}

	return true;
}

void CFBPassiveIO::OnAccept( int vBytesTran, int vIndex )
{
	CFBSession* tpSession = new CFBSession( mpManager, mAcceptSocket[ vIndex ], this );
	SOCKADDR_IN* tpLocalAddr	= 0;
	SOCKADDR_IN* tpPeerAddr		= 0;
	int tSize1 = sizeof( SOCKADDR_IN );
	int tSize2 = sizeof( SOCKADDR_IN );
	GetAcceptExSockaddrs( mAcceptBuffer[ vIndex ].Begin( ), 0, sizeof( SOCKADDR_IN ) + 16, sizeof( SOCKADDR_IN ) + 16, 
		(SOCKADDR**) &tpLocalAddr, &tSize1, (SOCKADDR**) &tpPeerAddr, &tSize2 );

	mSessions.Insert( tpSession->GetSessionID( ), tpSession );
	if ( CreateIoCompletionPort( (HANDLE) mAcceptSocket[ vIndex ], mIOPort, (ULONG_PTR) tpSession->GetSessionID( ), 0 ) == 0 )
		return;

	CFBSockAddr tPeer( *tpPeerAddr );
	tpSession->mSessionDesc.mPeer.Format( "%s:%d", tPeer.GetIPString( ), tPeer.GetPort( ) );

	PermitRecv( tpSession );
	Accept( vIndex );
	mpManager->OnAddSession( mName.GetBuffer( ), CFBSockAddr( *tpLocalAddr ), tPeer, tpSession->GetSessionID( ) );
}

void CFBPassiveIO::Run( unsigned int vMilliSeconds )
{
	DWORD	tBytesTran = 0;
	int		tSessionID = 0;
	OVERLAPPED* tpOverlapped = NULL;
	if ( GetQueuedCompletionStatus( mIOPort, &tBytesTran, (PULONG_PTR) &tSessionID, &tpOverlapped, vMilliSeconds ) == TRUE )
	{
		try
		{
			CFBSession* tpSession = GetSession( tSessionID );
			if ( tpSession == NULL )
			{
				int* tpIndex = mAcceptMap.Get( tpOverlapped );
				if ( tpIndex != NULL )
					OnAccept( tBytesTran, *tpIndex );

				return;
			}
			
			if ( tpOverlapped == &mRecvOverlapped )
			{
				if ( tBytesTran == 0 )
					Destroy( tSessionID );
				else
				{
					// 如果会话还在，就继续读取数据
					if ( tpSession->OnSessionRecv( tBytesTran ) == true && GetSession( tSessionID ) != NULL )
						PermitRecv( tpSession );
				}
			}

			if ( tpOverlapped == &mSendOverlapped )
			{
				if ( tpSession->OnSessionSend( tBytesTran ) == true )
					PermitSend( tpSession );
			}
		}
		catch( CFBExceptionEx& rException )
		{
			if ( rException.mReason == WSAECONNRESET || rException.mReason == WSAECONNABORTED )
			{
				Destroy( tSessionID );
				return;
			}
			else
				throw rException;
		}
	}
	else 
	{
		int tRetError = ::GetLastError( );
		if ( tRetError == ERROR_NETNAME_DELETED || tRetError == ERROR_SEM_TIMEOUT || tRetError == ERROR_CONNECTION_ABORTED )
		{
			if ( tSessionID == 0 )
			{
				int* tpIndex = mAcceptMap.Get( tpOverlapped );
				if ( tpIndex != NULL )
					OnAccept( tBytesTran, *tpIndex );

				return;
			}
			Destroy( tSessionID );
			return;
		}

		if ( tRetError != WAIT_TIMEOUT && tRetError != ERROR_OPERATION_ABORTED )
		{
			DWORD errorCode = tRetError;
			CFBString errorDesc;
			errorDesc.Format( "服务器错误[GetQueuedCompletionStatus 函数调用失败，错误码 - %d]", errorCode );
			log( errorDesc );
			return;
		}
	}

	return;
}

CFBSessionDesc* CFBPassiveIO::GetDesc( unsigned int vSessionID )
{
	CFBSession* tpSession = GetSession( vSessionID );
	if ( tpSession == NULL )
		return NULL;

	return &tpSession->mSessionDesc;
}

int CFBPassiveIO::PermitSend( CFBSession* pSession )
{
	WSABUF tWSABuffer;
	tWSABuffer.buf = (char*) pSession->mOBuffer.Begin( );
	tWSABuffer.len = pSession->mOBuffer.Length( );
		
	DWORD tBytesSend = 0;
	DWORD tFlags = 0;
	if ( WSASend( pSession->mPeerSocket, &tWSABuffer, 1, &tBytesSend, tFlags, &mSendOverlapped, NULL ) == SOCKET_ERROR )
	{
		int tRetError = ::GetLastError( );
		switch ( tRetError )
		{
		case ERROR_IO_PENDING:
			return ERESULT_SUCCESS;
		default:
			Destroy( pSession->mSessionID );
			return ERESULT_SUCCESS;
		}
	}

	return ERESULT_SUCCESS;
}

void CFBPassiveIO::PermitRecv( CFBSession* pSession )
{
	WSABUF tWSABuffer;
	tWSABuffer.buf = (char*) pSession->mIBuffer.End( );
	tWSABuffer.len = pSession->mIBuffer.Size( ) - pSession->mIBuffer.Length( );

	DWORD tBytesRecv = 0;
	DWORD tFlags = 0;
	if ( WSARecv( pSession->mPeerSocket, &tWSABuffer, 1, &tBytesRecv, &tFlags, &mRecvOverlapped, NULL ) == SOCKET_ERROR )
	{
		int tRetError = ::GetLastError( );
		switch ( tRetError )
		{
		case ERROR_IO_PENDING:
			return;
		default:
			Destroy( pSession->mSessionID );
			return;
		}
	}

	return;
}

// ********************************************************************** //
// CFBActiveIO
// ********************************************************************** //
CFBActiveIO::CFBActiveIO( CFBNetManager* pManager, const CFBString& rName ) : CFBNetworkIO( rName ), mpSession( NULL ), mpManager( pManager ), mSocket( 0 ), mSendReady( false )
{
}

CFBActiveIO::~CFBActiveIO( )
{
	// 因为断开连接和链接失败都需要关闭socket
	// 所以关闭socket要放在这里
	close( mSocket );
	mSocket = 0;
	if ( mpSession != NULL )
		DELETEPTR mpSession;
}

void CFBActiveIO::Destroy( TSessionID vSessionID )
{
	// 这里没有关闭socket,是因为不仅断开连接需要关闭socket
	// 连接失败也需要关闭socket
	spNetworkTombs->PushBack( this );
	if ( mpSession == NULL )
		return;
	
	mpManager->OnDelSession( mName, vSessionID );
	mName.Clear( );
}

void CFBActiveIO::Create( const CFBString& rAddress, const CFBString& rPort )
{
	// 注册一个主动IO
	if ( ( mSocket = ::socket( AF_INET, SOCK_STREAM, 0 ) ) == -1 )
	{
		CFBString errorDesc;
		errorDesc.Format( "服务器错误[WSAStartup 函数调用失败，错误码 - %d]", errno );
		log( errorDesc );
		return;
	}

	unsigned long tValue = 1;
	::ioctl( mSocket, FIONBIO, &tValue );

	sockaddr_in tAddr;
	tAddr.sin_family		= AF_INET;
	tAddr.sin_addr.s_addr	= inet_addr( rAddress.GetBuffer( ) );
	tAddr.sin_port			= htons( rPort.ToInteger( ) );
	if ( ::connect( mSocket, (sockaddr*) &tAddr, sizeof( sockaddr_in ) ) == -1 )
	{
		if ( errno == EINPROGRESS )
			return;

		CFBString errorDesc;
		errorDesc.Format( "服务器错误[WSAConnect 函数调用失败，错误码 - %d]", errno );
		log( errorDesc );
		return;
	}
}

void CFBActiveIO::OnConnect( int vCode )
{
	if ( vCode != 0 )
	{
		mpManager->OnAddSessionFault( mName, vCode );
		spNetworkTombs->PushBack( this );
		return;
	}
		
	// 如果连接成功
	mpSession = new CFBSession( mpManager, mSocket, this );
	unsigned int tBufferSize = mpManager->GetBufferSize( );
	int tRet = setsockopt( mSocket, SOL_SOCKET, SO_SNDBUF, (char*) &tBufferSize, sizeof( unsigned int ) );

	tBufferSize = mpManager->GetBufferSize( );
	tRet = setsockopt( mSocket, SOL_SOCKET, SO_RCVBUF, (char*) &tBufferSize, sizeof( unsigned int ) );

	bool tTcpDelay = true;
	setsockopt( mSocket, IPPROTO_TCP, TCP_NODELAY, (char*) &tTcpDelay, sizeof( bool ) );

	sockaddr_in tLocalAddr, tPeerAddr;
	int tLocalLen	= sizeof( sockaddr_in );
	int tPeerLen	= sizeof( sockaddr_in );
	if ( ::getsockname( mSocket, (sockaddr*) &tLocalAddr, &tLocalLen ) == -1 )
	{
		CFBString errorDesc;
		errorDesc.Format( "服务器错误[getsockname 函数调用失败，错误码 - %d]", errno );
		log( errorDesc );
		return;
	}

	if ( ::getsockname( mSocket, (sockaddr*) &tPeerAddr, &tPeerLen ) == -1 )
	{
		CFBString errorDesc;
		errorDesc.Format( "服务器错误[getsockname 函数调用失败，错误码 - %d]", errno );
		log( errorDesc );
		return;
	}

	CFBSockAddr tPeer( tPeerAddr );
	mpSession->mSessionDesc.mPeer.Format( "%s:%d", tPeer.GetIPString( ).GetBuffer( ), tPeer.GetPort( ) );

	mpManager->OnAddSession( mName.GetBuffer( ), CFBSockAddr( tLocalAddr ), tPeer, 0 );
}

void CFBActiveIO::Run( unsigned int vMillsSeconds )
{
	if ( mSocket == 0 )
		return;

	struct timeval timeout = { 0, vMillsSeconds * 1000 };
	fd_set	mReadSet;
	fd_set	mWriteSet;
	FD_ZERO( &mReadSet );
	FD_ZERO( &mWriteSet );
	FD_SET( mSocket, &mReadSet );
	FD_SET( mSocket, &mWriteSet );
	int tRet = select( mSocket + 1, &mReadSet, &mWriteSet, NULL, &timeout );
	if ( tRet > 0 )
	{
		if ( FD_ISSET( mSocket, &mWriteSet ) != 0 && FD_ISSET( mSocket, &mReadSet ) != 0 )
		{
			if ( mpSession == NULL )
			{
				OnConnect( -1 );
				return;
			}
		}
		
		if ( FD_ISSET( mSocket, &mWriteSet ) != 0 )
		{
			if ( mpSession == NULL )
				OnConnect( 0 );
			else
				OnSend( );
		}
		if ( FD_ISSET( mSocket, &mReadSet ) != 0 )
		{
			OnRecv( );
		}
	}

	return;
}

int CFBActiveIO::SendHelper( )
{
	int tBytesSend = 0;
	do
	{
		char*	tpBuffer	= (char*) mpSession->mOBuffer.Begin( );
		int	tLength			= mpSession->mOBuffer.Length( );
		if ( tLength == 0 )
			return ERESULT_SUCCESS;
		
		tBytesSend = ::send( mpSession->mPeerSocket, tpBuffer, tLength, 0 );
		if ( tBytesSend == -1 )
		{
			switch( errno )
			{
			case EAGAIN:
				return ERESULT_SUCCESS;
			default:
				Destroy( 0 );
				return ERESULT_CONNECTLOST;		
			}
		}
	}while( mpSession->OnSessionSend( tBytesSend ) == true );

	return ERESULT_SUCCESS;
}

CFBSessionDesc* CFBActiveIO::GetDesc( unsigned int vSessionID )
{
	if ( mpSession == NULL )
		return NULL;

	return &mpSession->mSessionDesc;
}

int CFBActiveIO::Send( TSessionID vSessionID, CFBProtocol* pProtocol )
{
	if ( mpSession == NULL )
		return FBNet::ERESULT_SESSIONLOST;

	mpSession->Send( pProtocol->Encode( CFBOctetsStream( ) ) );
	return FBNet::ERESULT_SUCCESS;
}

void CFBActiveIO::OnSend( )
{
	if ( mpSession == NULL )
		return;
	
	SendHelper( );
}

void CFBActiveIO::OnRecv( )
{
	if ( mpSession == NULL )
		return;

	char*	tpBuffer	= (char*) mpSession->mIBuffer.End( );
	int		tLength		= mpSession->mIBuffer.Size( ) - mpSession->mIBuffer.Length( );

	int tBytesRecv = recv( mpSession->mPeerSocket, tpBuffer, tLength, 0 );
	if ( tBytesRecv == 0 || tBytesRecv == -1 )
	{
		// 如果收到0字节，如果接受失败, 都要断开连接
		Destroy( 0 );
		return;
	}

	// 如果解包失败，也要断开连接
	if ( mpSession->OnSessionRecv( tBytesRecv ) == false )
		Destroy( 0 );
}

