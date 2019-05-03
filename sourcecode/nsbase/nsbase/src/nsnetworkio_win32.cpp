#include <nsbase.h>
#include <mstcpip.h>
namespace NSNet
{
	extern CNSMap< CNSString, CNSNetworkIO* >	networkIOs;
	// ********************************************************************** //
	// CNSNetworkIO
	// ********************************************************************** //
	CNSNetworkIO::CNSNetworkIO( const CNSString& rName ) : mName( rName )
	{
	}

	// ********************************************************************** //
	// CNSPassiveIO
	// ********************************************************************** //
	CNSPassiveIO::CNSPassiveIO( CNSNetManager* pManager, const CNSString& rName, unsigned int acceptNum ) : CNSNetworkIO( rName ), mpManager( pManager ),
		mListenSocket( NULL ), mAcceptNum( acceptNum )
	{
		mAcceptBuffer = new CNSOctets[ mAcceptNum ];
		mAcceptSocket = new SOCKET[ mAcceptNum ];
		mAcceptOverlapped = new WSAOVERLAPPED[ mAcceptNum ];
		for (unsigned int i = 0; i < mAcceptNum; i ++)
		{
			::memset( &mAcceptOverlapped[ i ], 0, sizeof( WSAOVERLAPPED ) );
			mAcceptMap.insert( &mAcceptOverlapped[ i ], i );
			mAcceptBuffer[ i ] = CNSOctets( 64 );
			mAcceptSocket[ i ] = NULL;
		}

		::memset( &mRecvOverlapped, 0, sizeof( WSAOVERLAPPED ) );
		::memset( &mSendOverlapped, 0, sizeof( WSAOVERLAPPED ) );
	}

	CNSPassiveIO::~CNSPassiveIO( )
	{
		::closesocket( mListenSocket );
		for (unsigned int i = 0; i < mAcceptNum; i ++)
			::closesocket( mAcceptSocket[ i ] );

		delete[] mAcceptBuffer;
		delete[] mAcceptSocket;
		delete[] mAcceptOverlapped;
	}

	CNSSession* CNSPassiveIO::getSession( TSessionID sessionID )
	{
		CNSSession* session = NULL;
		if (mSessions.find( sessionID, session ) == true)
			return session;

		return NULL;
	}

	int CNSPassiveIO::send( TSessionID sessionID, const void* begin, const void* end )
	{
		if (sessionID == 0)
		{
			static CNSVector< CNSSession* > sessions;
			sessions.clear( );
			HLISTINDEX beginIndex = mSessions.getHead( );
			for (; beginIndex != NULL; mSessions.getNext( beginIndex ))
			{
				CNSSession* session = mSessions.getValue( beginIndex );
				sessions.pushback( session );
			}

			for (unsigned int i = 0; i < sessions.getCount( ); i ++)
			{
				CNSSession* session = sessions[ i ];
				if (session->send( begin, end ) == true)
					permitSend( session );
			}

			return NSNet::ERESULT_SUCCESS;
		}

		CNSSession* session = getSession( sessionID );
		if (session == NULL)
			return NSNet::ERESULT_SESSIONLOST;

		if (session->send( begin, end ) == true)
			return permitSend( session );

		return NSNet::ERESULT_SUCCESS;
	}

	int CNSPassiveIO::send( TSessionID sessionID, const void* begin, size_t size )
	{
		if (sessionID == 0)
		{
			static CNSVector< CNSSession* > sessions;
			sessions.clear( );
			HLISTINDEX beginIndex = mSessions.getHead( );
			for (; beginIndex != NULL; mSessions.getNext( beginIndex ))
			{
				CNSSession* session = mSessions.getValue( beginIndex );
				sessions.pushback( session );
			}

			for (unsigned int i = 0; i < sessions.getCount( ); i ++)
			{
				CNSSession* session = sessions[ i ];
				if (session->send( begin, size ) == true)
					permitSend( session );
			}

			return NSNet::ERESULT_SUCCESS;
		}

		CNSSession* session = getSession( sessionID );
		if (session == NULL)
			return NSNet::ERESULT_SESSIONLOST;

		if (session->send( begin, size ) == true)
			return permitSend( session );

		return NSNet::ERESULT_SUCCESS;
	}

	int CNSPassiveIO::send( TSessionID sessionID, const CNSOctets& buffer )
	{
		if (sessionID == 0)
		{
			static CNSVector< CNSSession* > sessions;
			sessions.clear( );
			HLISTINDEX beginIndex = mSessions.getHead( );
			for (; beginIndex != NULL; mSessions.getNext( beginIndex ))
			{
				CNSSession* session = mSessions.getValue( beginIndex );
				sessions.pushback( session );
			}

			for (unsigned int i = 0; i < sessions.getCount( ); i ++)
			{
				CNSSession* session = sessions[ i ];
				if (session->send( buffer ) == true)
					permitSend( session );
			}

			return NSNet::ERESULT_SUCCESS;
		}

		CNSSession* session = getSession( sessionID );
		if (session == NULL)
			return NSNet::ERESULT_SESSIONLOST;

		if (session->send( buffer ) == true)
			return permitSend( session );

		return NSNet::ERESULT_SUCCESS;
	}

	void CNSPassiveIO::destroy( unsigned int sessionID )
	{
		if (sessionID == 0)
		{
			HLISTINDEX beginIndex = mSessions.getHead( );
			for (; beginIndex != NULL; mSessions.getNext( beginIndex ))
			{
				CNSSession* session = mSessions.getValue( beginIndex );
				if (session == NULL)
					continue;

				mSessionsTomb.insert( session->mSessionID, session );
				mpManager->onDelSession( mName, session->mSessionID );
				closesocket( session->mPeerSocket );
				session->mPeerSocket = NULL;
			}

			mSessions.clear( );
		}
		else
		{
			CNSSession* session = getSession( sessionID );
			if (session == NULL)
				return;

			mpManager->onDelSession( mName, sessionID );
			mSessionsTomb.insert( session->mSessionID, session );
			closesocket( session->mPeerSocket );
			session->mPeerSocket = NULL;
			mSessions.erase( sessionID );
		}
	}

	void CNSPassiveIO::create( const CNSString& rAddress, const CNSString& rPort )
	{
		if (mIOPort == NULL)
		{
			mIOPort = ::CreateIoCompletionPort( INVALID_HANDLE_VALUE, 0, 0, 1 );
			if (mIOPort == INVALID_HANDLE_VALUE)
			{
				DWORD errorCode = WSAGetLastError( );
				CNSString errorDesc;
				errorDesc.format( _UTF8( "错误[CreateIoCompletionPort 函数调用失败，错误码 - %d]" ), errorCode );
				NSException( errorDesc );
			}
		}

		// 注册一个被动IO
		if (( mListenSocket = ::WSASocket( AF_INET, SOCK_STREAM, 0, 0, 0, WSA_FLAG_OVERLAPPED ) ) == INVALID_SOCKET)
		{
			DWORD errorCode = WSAGetLastError( );
			CNSString errorDesc;
			errorDesc.format( _UTF8( "错误[WSASocket 函数调用失败，错误码 - %d]" ), errorCode );
			NSException( errorDesc );
		}

		SOCKADDR_IN addr;
		addr.sin_family = AF_INET;
		addr.sin_port = htons( rPort.toInteger( ) );
		if (inet_pton( AF_INET, rAddress.getBuffer( ), &addr.sin_addr.s_addr ) != 1)
		{
			DWORD errorCode = WSAGetLastError( );
			CNSString errorDesc;
			errorDesc.format( _UTF8( "错误[inet_pton 函数调用失败，错误码 - %d]" ), errorCode );
			NSException( errorDesc );
		}

		if (::bind( mListenSocket, (SOCKADDR*)&addr, sizeof( SOCKADDR_IN ) ) == SOCKET_ERROR)
		{
			int errorCode = GetLastError( );
			CNSString errorDesc;
			errorDesc.format( _UTF8( "错误[bind 函数调用失败，端口打开失败, 端口地址 - %s:%s, 错误码 - %d]" ), rAddress.getBuffer( ), rPort.getBuffer( ), errorCode );
			NSException( errorDesc );
		}

		if (::listen( mListenSocket, mAcceptNum ) == SOCKET_ERROR)
		{
			int errorCode = GetLastError( );
			CNSString errorDesc;
			errorDesc.format( _UTF8( "错误[listen 函数调用失败，错误码 - %d]" ), errorCode );
			NSException( errorDesc );
		}

		if (::CreateIoCompletionPort( (HANDLE)mListenSocket, mIOPort, 0, 0 ) == 0)
		{
			int errorCode = GetLastError( );
			CNSString errorDesc;
			errorDesc.format( _UTF8( "错误[CreateIoCompletionPort 函数调用失败，错误码 - %d]" ), errorCode );
			NSException( errorDesc );
		}

		for (unsigned int i = 0; i < mAcceptNum; i ++)
			accept( i );
	}

	void CNSPassiveIO::accept( int index )
	{
		mAcceptSocket[ index ] = ::WSASocket( AF_INET, SOCK_STREAM, 0, 0, 0, WSA_FLAG_OVERLAPPED );
		if (mAcceptSocket[ index ] == INVALID_SOCKET)
		{
			DWORD errorCode = GetLastError( );
			CNSString errorDesc;
			errorDesc.format( _UTF8( "错误[Accept 函数调用失败，错误码 - %d]" ), errorCode );
			NSException( errorDesc );
		}

		unsigned int bufferSize = mpManager->getBufferSize( );
		if (setsockopt( mAcceptSocket[ index ], SOL_SOCKET, SO_SNDBUF, (char*)&bufferSize, sizeof( unsigned int ) ) != 0)
		{
			DWORD errorCode = GetLastError( );
			CNSString errorDesc;
			errorDesc.format( _UTF8( "错误[setsockopt SO_SNDBUF函数调用失败，错误码 - %d]" ), errorCode );
			NSException( errorDesc );
		}

		if (setsockopt( mAcceptSocket[ index ], SOL_SOCKET, SO_RCVBUF, (char*)&bufferSize, sizeof( unsigned int ) ) != 0)
		{
			DWORD errorCode = GetLastError( );
			CNSString errorDesc;
			errorDesc.format( _UTF8( "错误[setsockopt SO_RCVBUF函数调用失败，错误码 - %d]" ), errorCode );
			NSException( errorDesc );
		}

		// 默认值是2小时无数据交换，90秒后重试
		bool keepAlive = true;
		if (setsockopt( mAcceptSocket[ index ], SOL_SOCKET, SO_KEEPALIVE, (char*)&keepAlive, sizeof( bool ) ) != 0)
		{
			DWORD errorCode = GetLastError( );
			CNSString errorDesc;
			errorDesc.format( _UTF8( "错误[setsockopt SO_KEEPALIVE函数调用失败，错误码 - %d]" ), errorCode );
			NSException( errorDesc );
		}

		bool noTcpDelay = false;
		if (setsockopt( mAcceptSocket[ index ], IPPROTO_TCP, TCP_NODELAY, (char*)&noTcpDelay, sizeof( bool ) ) != 0)
		{
			DWORD errorCode = GetLastError( );
			CNSString errorDesc;
			errorDesc.format( _UTF8( "错误[setsockopt TCP_NODELAY函数调用失败，错误码 - %d]" ), errorCode );
			NSException( errorDesc );
		}

		DWORD bytes;
		if (::AcceptEx( mListenSocket, mAcceptSocket[ index ], mAcceptBuffer[ index ].begin( ), 0, sizeof( SOCKADDR_IN ) + 16,
			 sizeof( SOCKADDR_IN ) + 16, &bytes, (OVERLAPPED*)&mAcceptOverlapped[ index ] ) == 0)
		{
			int errCode = ::GetLastError( );
			if (errCode != ERROR_IO_PENDING)
			{
				static CNSString errorDesc;
				errorDesc.format( _UTF8( "错误[AcceptEx 函数调用失败，错误码 - %d]" ), errCode );
				NSException( errorDesc );
			}
		}
	}

	void CNSPassiveIO::onAccept( int vBytesTran, int vIndex )
	{
		CNSSession* session = new CNSSession( mpManager, mAcceptSocket[ vIndex ], this );
		SOCKADDR_IN* tpLocalAddr = 0;
		SOCKADDR_IN* tpPeerAddr = 0;
		int tSize1 = sizeof( SOCKADDR_IN );
		int tSize2 = sizeof( SOCKADDR_IN );
		GetAcceptExSockaddrs( mAcceptBuffer[ vIndex ].begin( ), 0, sizeof( SOCKADDR_IN ) + 16, sizeof( SOCKADDR_IN ) + 16,
			(SOCKADDR**)&tpLocalAddr, &tSize1, (SOCKADDR**)&tpPeerAddr, &tSize2 );

		mSessions.insert( session->getSessionID( ), session );
		if (CreateIoCompletionPort( (HANDLE)mAcceptSocket[ vIndex ], mIOPort, (ULONG_PTR)session->getSessionID( ), 0 ) == 0)
			return;

		CNSSockAddr tPeer( *tpPeerAddr );
		session->mSessionDesc.mPeer.format( "%s", tPeer.GetIPString( ).getBuffer( ) );

		permitRecv( session );
		accept( vIndex );
		mpManager->onAddSession( mName.getBuffer( ), session->getSessionID( ), CNSSockAddr( *tpLocalAddr ), tPeer );
	}

	void CNSPassiveIO::pollEvent( unsigned int tickCount )
	{
		if (tickCount - mPollTick < mpManager->mPollInterval)
			return;

		mPollTick = tickCount;
		// 处理将要被删除的会话，延迟删除会话
		if (tickCount - mTombTick >= 300)
		{
			mTombTick = tickCount;
			HLISTINDEX beginIndex = mSessionsTomb.getHead( );
			for (; beginIndex != NULL; mSessionsTomb.getNext( beginIndex ))
				delete mSessionsTomb.getValue( beginIndex );
			mSessionsTomb.clear( );
		}

		static OVERLAPPED_ENTRY		entry[ 1024 ];
		ULONG entryNumber = 0;
		if (GetQueuedCompletionStatusEx( mIOPort, entry, 1024, (PULONG)&entryNumber, mpManager->mPollTimeout, FALSE ) == TRUE)
		{
			//NSLog::log( "GetQueuedCompletionStatusEx - %d count", entryNumber );
			for (ULONG i = 0; i < entryNumber; i ++)
			{
				DWORD			bytesTran = (DWORD)entry[ i ].dwNumberOfBytesTransferred;
				ULONG			sessionID = (ULONG)entry[ i ].lpCompletionKey;
				WSAOVERLAPPED*	overlapped = entry[ i ].lpOverlapped;
				if (overlapped == &mRecvOverlapped)
				{
					if (bytesTran == 0)
						destroy( sessionID );
					else
					{
						//NSLog::log( "recv complete %d bytes in %s", bytesTran, GetName( ).getBuffer( ) );
						CNSSession* session = getSession( sessionID );
						// 如果会话还在，就继续读取数据
						if (session != NULL && session->onSessionRecv( bytesTran ) == true)
							permitRecv( session );
					}
				}
				else if (overlapped == &mSendOverlapped)
				{
					//NSLog::log( "send complete %d bytes in %s", bytesTran, GetName( ).getBuffer( ) );
					CNSSession* session = getSession( sessionID );
					if (session != NULL && session->onSessionSend( bytesTran ) == true)
						permitSend( session );
				}
				else
				{
					int* index = mAcceptMap.get( overlapped );
					if (index != NULL)
						onAccept( bytesTran, *index );
				}
			}
		}
		else
		{
			int errCode = ::GetLastError( );
			if (errCode != WAIT_TIMEOUT)
			{
				for (ULONG i = 0; i < entryNumber; i ++)
				{
					DWORD			bytesTran = (DWORD)entry[ i ].dwNumberOfBytesTransferred;
					ULONG			sessionID = (ULONG)entry[ i ].lpCompletionKey;
					WSAOVERLAPPED*	overlapped = entry[ i ].lpOverlapped;
					if (overlapped != NULL && sessionID != 0)
						destroy( sessionID );
				}

				static CNSString errorDesc;
				errorDesc.format( _UTF8( "错误[GetQueuedCompletionStatus 函数调用失败，错误码: %d]" ), errCode );
				NSLog::log( errorDesc );
				return;
			}
		}
	}

	CNSSessionDesc* CNSPassiveIO::getDesc( unsigned int sessionID )
	{
		CNSSession* session = getSession( sessionID );
		if (session == NULL)
			return NULL;

		return &session->mSessionDesc;
	}

	int CNSPassiveIO::permitSend( CNSSession* pSession )
	{
		if (pSession->mPeerSocket == NULL)
			return ERESULT_SUCCESS;

		WSABUF tWSABuffer;
		tWSABuffer.buf = (char*)pSession->mOBuffer.begin( );
		tWSABuffer.len = pSession->mOBuffer.length( );
		//NSLog::log( "commit send %d bytes in %s", tWSABuffer.len, GetName( ).getBuffer( ) );

		DWORD tFlags = 0;
		if (WSASend( pSession->mPeerSocket, &tWSABuffer, 1, NULL, tFlags, &mSendOverlapped, NULL ) == SOCKET_ERROR)
		{
			int tRetError = ::GetLastError( );
			switch (tRetError)
			{
				case ERROR_IO_PENDING:
					return ERESULT_SUCCESS;
				default:
					destroy( pSession->mSessionID );
					return ERESULT_SUCCESS;
			}
		}

		return ERESULT_SUCCESS;
	}

	void CNSPassiveIO::permitRecv( CNSSession* pSession )
	{
		if (pSession->mPeerSocket == NULL)
			return;

		WSABUF tWSABuffer;
		tWSABuffer.buf = (char*)pSession->mIBuffer.end( );
		tWSABuffer.len = pSession->mIBuffer.size( ) - pSession->mIBuffer.length( );
		//NSLog::log( "commit recv %d bytes in %s", tWSABuffer.len, GetName( ).getBuffer( ) );

		DWORD tBytesRecv = 0;
		DWORD tFlags = 0;
		if (WSARecv( pSession->mPeerSocket, &tWSABuffer, 1, NULL, &tFlags, &mRecvOverlapped, NULL ) == SOCKET_ERROR)
		{
			int tRetError = ::GetLastError( );
			switch (tRetError)
			{
				case ERROR_IO_PENDING:
					return;
				default:
					destroy( pSession->mSessionID );
					return;
			}
		}

		return;
	}

	// ********************************************************************** //
	// CNSActiveIO
	// ********************************************************************** //
	CNSActiveIO::CNSActiveIO( CNSNetManager* pManager, const CNSString& name ) : CNSNetworkIO( name ), mpSession( NULL ), mpManager( pManager ),
		mEObject( NULL ), mSocket( NULL ), mSendReady( false )
	{
	}

	CNSActiveIO::~CNSActiveIO( )
	{
		::closesocket( mSocket );
		::CloseHandle( mEObject );
		mpManager = NULL;
		mSocket = NULL;
		mEObject = NULL;
		delete mpSession;
	}

	void CNSActiveIO::destroy( TSessionID sessionID )
	{
		if (mpSession == NULL)
			return;

		mIsDeleted = true;
		mpManager->onDelSession( mName, mpSession->mSessionID );

		// 如果NetworkIO对象没有发生变化，说明没有重连，那么需要清除Manager中保存的指针
		if (mpManager->getNetworkIO( ) == this)
			mpManager->setNetworkIO( NULL );
	}

	void CNSActiveIO::create( const CNSString& rAddress, const CNSString& rPort )
	{
		// 注册一个主动IO
		if (( mSocket = ::WSASocket( AF_INET, SOCK_STREAM, 0, 0, 0, WSA_FLAG_OVERLAPPED ) ) == INVALID_SOCKET)
		{
			DWORD errorCode = GetLastError( );
			static CNSString errorDesc;
			errorDesc.format( _UTF8( "错误[WSASocket 函数调用失败，错误码: %d]" ), errorCode );
			NSException( errorDesc );
		}

		unsigned int bufferSize = mpManager->getBufferSize( );
		if (setsockopt( mSocket, SOL_SOCKET, SO_SNDBUF, (char*)&bufferSize, sizeof( unsigned int ) ) != 0)
		{
			DWORD errorCode = GetLastError( );
			static CNSString errorDesc;
			errorDesc.format( _UTF8( "错误[setsockopt SO_SNDBUF函数调用失败，错误码: %d]" ), errorCode );
			NSException( errorDesc );
		}

		if (setsockopt( mSocket, SOL_SOCKET, SO_RCVBUF, (char*)&bufferSize, sizeof( unsigned int ) ) != 0)
		{
			DWORD errorCode = GetLastError( );
			static CNSString errorDesc;
			errorDesc.format( _UTF8( "错误[setsockopt SO_RCVBUF函数调用失败，错误码: %d]" ), errorCode );
			NSException( errorDesc );
		}

		bool noTcpDelay = false;
		if (setsockopt( mSocket, IPPROTO_TCP, TCP_NODELAY, (char*)&noTcpDelay, sizeof( bool ) ) != 0)
		{
			DWORD errorCode = GetLastError( );
			static CNSString errorDesc;
			errorDesc.format( _UTF8( "错误[setsockopt TCP_NODELAY函数调用失败，错误码: %d]" ), errorCode );
			NSException( errorDesc );
		}

		if (( mEObject = ::CreateEvent( 0, false, false, 0 ) ) == INVALID_HANDLE_VALUE)
		{
			DWORD errorCode = GetLastError( );
			static CNSString errorDesc;
			errorDesc.format( _UTF8( "错误[CreateEvent 函数调用失败，错误码: %d]" ), errorCode );
			NSException( errorDesc );
		}

		if (::WSAEventSelect( mSocket, mEObject, FD_CONNECT | FD_CLOSE ) == SOCKET_ERROR)
		{
			DWORD errorCode = GetLastError( );
			static CNSString errorDesc;
			errorDesc.format( _UTF8( "错误[WSAEventSelect 函数调用失败，错误码: %d]" ), errorCode );
			NSException( errorDesc );
		}

		SOCKADDR_IN addr;
		addr.sin_family = AF_INET;
		addr.sin_port = htons( rPort.toInteger( ) );
		if (inet_pton( AF_INET, rAddress.getBuffer( ), &addr.sin_addr.s_addr ) != 1)
		{
			DWORD errorCode = WSAGetLastError( );
			CNSString errorDesc;
			errorDesc.format( _UTF8( "错误[inet_pton 函数调用失败，错误码 - %d]" ), errorCode );
			NSException( errorDesc );
		}

		//ConnectEx
		if (::WSAConnect( mSocket, (sockaddr*)&addr, sizeof( SOCKADDR_IN ), 0, 0, 0, 0 ) == SOCKET_ERROR)
		{
			int tRetCode = ::GetLastError( );
			if (tRetCode == WSAEWOULDBLOCK)
				return;

			DWORD errorCode = tRetCode;
			static CNSString errorDesc;
			errorDesc.format( _UTF8( "错误[WSAConnect 函数调用失败，错误码 - %d]" ), errorCode );
			NSException( errorDesc );
		}
	}

	void CNSActiveIO::onConnect( int vCode )
	{
		if (vCode != 0)
		{
			mIsDeleted = true;
			mpManager->onAddSessionFault( mName, vCode );

			// 如果NetworkIO对象没有发生变化，说明没有重连，那么需要清除Manager中保存的指针
			if (mpManager->getNetworkIO( ) == this)
				mpManager->setNetworkIO( NULL );
			return;
		}

		// 如果连接成功
		::WSAEventSelect( mSocket, mEObject, FD_WRITE | FD_READ | FD_CLOSE );
		mpSession = new CNSSession( mpManager, mSocket, this );

		SOCKADDR_IN tLocalAddr, peerAddr;
		int tLocalLen = sizeof( SOCKADDR_IN );
		int peerLen = sizeof( SOCKADDR_IN );
		if (::getsockname( mSocket, (sockaddr*)&tLocalAddr, &tLocalLen ) == SOCKET_ERROR)
		{
			DWORD errorCode = ::GetLastError( );
			static CNSString errorDesc;
			errorDesc.format( _UTF8( "错误[getsockname] 函数调用失败，错误码: %d" ), errorCode );
			NSException( errorDesc );
		}

		if (::getsockname( mSocket, (sockaddr*)&peerAddr, &peerLen ) == SOCKET_ERROR)
		{
			DWORD errorCode = ::GetLastError( );
			static CNSString errorDesc;
			errorDesc.format( _UTF8( "错误[getsockname] 函数调用失败，错误码: %d" ), errorCode );
			NSException( errorDesc );
			return;
		}

		CNSSockAddr peer( peerAddr );
		mpSession->mSessionDesc.mPeer.format( "%s:%d", peer.GetIPString( ), peer.GetPort( ) );
		mpManager->onAddSession( mName.getBuffer( ), 0, CNSSockAddr( tLocalAddr ), peer );
	}

	void CNSActiveIO::pollEvent( unsigned int tickCount )
	{
		if (tickCount - mPollTick < mpManager->mPollInterval)
			return;

		mPollTick = tickCount;
		DWORD waitResult = ::WaitForSingleObject( mEObject, mpManager->mPollTimeout );
		if (waitResult == WAIT_OBJECT_0)
		{
			WSANETWORKEVENTS tEvents;
			if (::WSAEnumNetworkEvents( mSocket, mEObject, &tEvents ) == SOCKET_ERROR)
			{
				DWORD errorCode = ::GetLastError( );
				static CNSString errorDesc;
				errorDesc.format( _UTF8( "错误[WSAEnumNetworkEvents] 函数调用失败，错误码: %d" ), errorCode );
				NSException( errorDesc );
			}

			if (tEvents.lNetworkEvents & FD_CONNECT)
				onConnect( tEvents.iErrorCode[ FD_CONNECT_BIT ] );

			if (tEvents.lNetworkEvents & FD_READ)
				onRecv( );

			if (tEvents.lNetworkEvents & FD_WRITE)
				onSend( );

			if (tEvents.lNetworkEvents & FD_CLOSE)
				destroy( 0 );
		}
		else if (waitResult == WAIT_FAILED)
		{
			DWORD errorCode = ::GetLastError( );
			static CNSString errorDesc;
			errorDesc.format( _UTF8( "错误[WaitForSingleObjects] 函数调用失败，错误码: %d" ), errorCode );
			NSException( errorDesc );
		}
	}

	int CNSActiveIO::sendHelper( )
	{
		DWORD bytesSend = 0;
		DWORD flags = 0;
		do
		{
			WSABUF tWSABuffer;
			tWSABuffer.buf = (char*)mpSession->mOBuffer.begin( );
			tWSABuffer.len = mpSession->mOBuffer.length( );

			bytesSend = 0;
			if (::WSASend( mpSession->mPeerSocket, &tWSABuffer, 1, &bytesSend, flags, NULL, NULL ) == SOCKET_ERROR)
			{
				int tErrCode = ::GetLastError( );
				switch (tErrCode)
				{
					case WSAEWOULDBLOCK:
						break;
					case WSAECONNABORTED:
					case WSAECONNRESET:
						destroy( 0 );
						return ERESULT_CONNECTLOST;
					default:
						DWORD errorCode = tErrCode;
						static CNSString errorDesc;
						errorDesc.format( _UTF8( "错误[WSAEnumNetworkEvents] 函数调用失败，错误码: %d" ), errorCode );
						NSException( errorDesc );
						break;
				}
			}
		}
		while (mpSession->onSessionSend( bytesSend ) == true);

		return ERESULT_SUCCESS;
	}

	CNSSessionDesc* CNSActiveIO::getDesc( unsigned int sessionID )
	{
		if (mpSession == NULL)
			return NULL;

		return &mpSession->mSessionDesc;
	}

	int CNSActiveIO::send( TSessionID sessionID, const CNSOctets& buffer )
	{
		if (mpSession == NULL)
			return NSNet::ERESULT_SESSIONLOST;

		if (mpSession->send( buffer ) == true)
			return sendHelper( );

		return NSNet::ERESULT_SUCCESS;
	}

	int CNSActiveIO::send( TSessionID sessionID, const void* begin, const void* end )
	{
		if (mpSession == NULL)
			return NSNet::ERESULT_SESSIONLOST;

		if (mpSession->send( begin, end ) == true)
			return sendHelper( );

		return NSNet::ERESULT_SUCCESS;
	}

	int CNSActiveIO::send( TSessionID sessionID, const void* begin, size_t size )
	{
		if (mpSession == NULL)
			return NSNet::ERESULT_SESSIONLOST;

		if (mpSession->send( begin, size ) == true)
			return sendHelper( );

		return NSNet::ERESULT_SUCCESS;
	}

	void CNSActiveIO::onSend( )
	{
		if (mpSession == NULL)
			return;

		sendHelper( );
	}

	void CNSActiveIO::onRecv( )
	{
		if (mpSession == NULL)
			return;

		WSABUF tWSABuffer;
		tWSABuffer.buf = (char*)mpSession->mIBuffer.end( );
		tWSABuffer.len = mpSession->mIBuffer.size( ) - mpSession->mIBuffer.length( );

		DWORD tBytesRecv = 0;
		DWORD tFlags = 0;
		if (WSARecv( mpSession->mPeerSocket, &tWSABuffer, 1, &tBytesRecv, &tFlags, NULL, NULL ) == SOCKET_ERROR)
		{
			int errCode = ::GetLastError( );
			switch (errCode)
			{
				case WSAEWOULDBLOCK:
					break;
				case WSAECONNABORTED:
				case WSAECONNRESET:
					destroy( 0 );
					return;
				default:
					static CNSString errorDesc;
					errorDesc.format( _UTF8( "错误[WSARecv] 函数调用失败，错误码: %d" ), errCode );
					NSException( errorDesc );
					break;
			}
		}

		mpSession->onSessionRecv( tBytesRecv );
	}
};