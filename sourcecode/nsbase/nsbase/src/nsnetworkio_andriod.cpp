#include <fbbase.h>
// ********************************************************************** //
// CFBNetworkIO
// ********************************************************************** //
CFBNetworkIO::CFBNetworkIO( const CFBString& rName ) : mName( rName )
{
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
		FBExceptionEx( errno );

	unsigned long tValue = 1;
	::ioctl( mSocket, FIONBIO, &tValue );

	sockaddr_in tAddr;
	tAddr.sin_family = AF_INET;
	tAddr.sin_addr.s_addr	= inet_addr( rAddress.GetBuffer( ) );
	tAddr.sin_port = htons( rPort.ToInteger( ) );
	if ( ::connect( mSocket, (sockaddr*) &tAddr, sizeof( sockaddr_in ) ) == -1 )
	{
		if ( errno == EINPROGRESS )
			return;

		FBExceptionEx( errno );
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
	mpSession = NEW CFBSession( mpManager, mSocket, this );
	unsigned int tBufferSize = mpManager->GetBufferSize( );
	int tRet = setsockopt( mSocket, SOL_SOCKET, SO_SNDBUF, (char*) &tBufferSize, sizeof( unsigned int ) );

	bool tTcpDelay = true;
	setsockopt( mSocket, IPPROTO_TCP, TCP_NODELAY, (char*) &tTcpDelay, sizeof( bool ) );

	tBufferSize = mpManager->GetBufferSize( );
	tRet = setsockopt( mSocket, SOL_SOCKET, SO_RCVBUF, (char*) &tBufferSize, sizeof( unsigned int ) );

	sockaddr_in tLocalAddr, tPeerAddr;
	int tLocalLen	= sizeof( sockaddr_in );
	int tPeerLen	= sizeof( sockaddr_in );
	if ( ::getsockname( mSocket, (sockaddr*) &tLocalAddr, &tLocalLen ) == -1 )
		FBExceptionEx( errno );

	if ( ::getsockname( mSocket, (sockaddr*) &tPeerAddr, &tPeerLen ) == -1 )
		FBExceptionEx( errno );

	CFBSockAddr tPeer( tPeerAddr );
	mpSession->mSessionDesc.mPeer.Format( "%s:%d", tPeer.GetIPString( ).GetBuffer( ), tPeer.GetPort( ) );

	mpManager->OnAddSession( mName.GetBuffer( ), CFBSockAddr( tLocalAddr ), tPeer, 0 );
}

void CFBActiveIO::Run( unsigned int vMillsSeconds )
{
	if ( mSocket == 0 )
		return;

	struct timeval timeout = { 0, 0 };
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

