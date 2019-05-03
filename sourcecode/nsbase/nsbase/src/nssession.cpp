#include <nsbase.h>
#include <time.h>

unsigned int CNSSession::sSessionID = 1;
CNSSession::CNSSession( CNSNetManager* pManager, SOCKET tPeerSocket, CNSNetworkIO* pNetworkIO ) : mSessionID( NextSessionID( ) ), mpISecurity( NULL ), mpOSecurity( NULL ),
mIBuffer( pManager->getBufferSize( ) ), mOBuffer( pManager->getBufferSize( ) ), mpManager( pManager ), mIStream( CNSOctetsStream::EStreamMemory, pManager->getStreamSize( ) ),
mPeerSocket( tPeerSocket ), mpNetworkIO( pNetworkIO ), mLastProto( NULL ), mIsNotifyProgress( mpManager->isNotifyProgress( ) ), mIsHttp( mpManager->isHttp( ) ), mHttpStatus( HTTP_DATA ), mPostDataSize( 0 )
{
}

CNSSession::~CNSSession( )
{
}

bool CNSSession::sendReady( )
{
	// 如果正在发送缓冲里还有数据，那么不能发送任何数据
	if ( mOBuffer.length( ) > 0 )
		return false;

	if ( mOStream.length( ) == 0 )
		return false;

	if ( mpOSecurity != NULL )
		mpOSecurity->Update( mOStream );

	unsigned int tBufferRemain = mOBuffer.size( ) - mOBuffer.length( );
	unsigned int tBytesSend = min( tBufferRemain, mOStream.length( ) );
	mOBuffer.insert( mOBuffer.end( ), mOStream.begin( ), (char*) mOStream.begin( ) + tBytesSend );
	mOStream.erase( mOStream.begin( ), (char*) mOStream.begin( ) + tBytesSend );
	mSessionDesc.mSBBytes = mOStream.length( );
	return true;
}

bool CNSSession::send( const void* begin, const void* end )
{
	size_t len = (ptrdiff_t) end - (ptrdiff_t) begin;
	if ( len != 0 && mpManager->onCheckAccumulate( mpNetworkIO->GetName( ), mOStream.length( ) ) == true )
	{
		mOStream.insert( mOStream.end( ), begin, end );
		mSessionDesc.mSBBytes = mOStream.length( );
		return sendReady( );
	}

	return false;
}

bool CNSSession::send( const void* begin, size_t len )
{
	if ( len != 0 && mpManager->onCheckAccumulate( mpNetworkIO->GetName( ), mOStream.length( ) ) == true )
	{
		mOStream.insert( mOStream.end( ), begin, (char*) begin + len );
		mSessionDesc.mSBBytes = mOStream.length( );
		return sendReady( );
	}

	return false;
}

bool CNSSession::send( const CNSOctets& buffer )
{
	if ( buffer.length( ) != 0 && mpManager->onCheckAccumulate( mpNetworkIO->GetName( ), mOStream.length( ) ) == true )
	{
		mOStream.insert( mOStream.end( ), buffer.begin( ), buffer.end( ) );
		mSessionDesc.mSBBytes = mOStream.length( );
		return sendReady( );
	}

	return false;
}

bool CNSSession::onSessionSend( unsigned int vBytesTran )
{
	mSessionDesc.mSendBytes += vBytesTran;
	mOBuffer.erase( mOBuffer.begin( ), (char*) mOBuffer.begin( ) + vBytesTran );
	return sendReady( );
}

bool CNSSession::httpInput( char ch )
{
	if ( ch == '\r' )
	{
		if ( mHttpStatus == HTTP_DATA )
		{
			// 等待HTTP数据接收完成
			mHttpStatus = HTTP_WAITDATAEND;
			return true;
		}

		if ( mHttpStatus == HTTP_WAITVALUE )
		{
			// 等待Header数据接收完成
			mHttpStatus = HTTP_WAITRETURN;
			return true;
		}

		if ( mHttpStatus == HTTP_WAITKEY )
		{
			// 等待Header 键值接收完成
			mHttpStatus = HTTP_WAITRETURN;
			return true;
		}
		return false;
	}

	if ( ch == '\n' )
	{
		if ( mHttpStatus == HTTP_WAITDATAEND )
		{
			if ( mHttpData.findFirstOf( "GET " ) == -1 && mHttpData.findFirstOf( "POST " ) == -1 )
				return false;

			mHttpStatus = HTTP_WAITKEY;
			return true;
		}
		else if ( mHttpStatus == HTTP_WAITRETURN )
		{
			mHttpStatus = HTTP_WAITKEY;
			// 一行结束，可以处理一次Header
			if ( mKey == "Content-Length" )
				mPostDataSize = mValue.toInteger( );

			if ( mKey.isEmpty( ) == true )
			{
				if ( mPostDataSize == 0 )
					// 如果没有POSTDATA, 那么HTTP结束
					mHttpStatus = HTTP_END;
				else
					// 如果有POSTDATA, 等待HTTPPOST
					mHttpStatus = HTTP_WAITPOSTDATA;
			}

			mHeaders.insert( mKey, mValue );
			mKey.clear( );
			mValue.clear( );
			return true;
		}
		else
			return false;
	}

	if ( ch == ':' )
	{
		if ( mHttpStatus == HTTP_WAITKEY )
		{
			mHttpStatus = HTTP_WAITVALUE_SPACE;
			return true;
		}
		else if ( mHttpStatus != HTTP_DATA &&
			mHttpStatus != HTTP_WAITVALUE &&
			mHttpStatus != HTTP_WAITPOSTDATA )
		{
			return false;
		}
	}

	if ( ch == 0x20 )
	{
		if ( mHttpStatus == HTTP_WAITVALUE_SPACE )
		{
			mHttpStatus = HTTP_WAITVALUE;
			return true;
		}
		else if ( mHttpStatus != HTTP_DATA &&
			mHttpStatus != HTTP_WAITVALUE &&
			mHttpStatus != HTTP_WAITPOSTDATA )
		{
			return false;
		}
	}

	if ( mHttpStatus == HTTP_DATA )
		mHttpData.pushback( ch );
	else if ( mHttpStatus == HTTP_WAITKEY )
		mKey.pushback( ch );
	else if ( mHttpStatus == HTTP_WAITVALUE )
		mValue.pushback( ch );
	else if ( mHttpStatus == HTTP_WAITPOSTDATA )
	{
		mPostData.pushback( ch );
		if ( mPostData.getLength( ) == mPostDataSize )
			mHttpStatus = HTTP_END;
	}

	if ( mHttpData.getLength( ) > 65535 )
		return false;

	if ( mKey.getLength( ) > 1024 )
		return false;

	if ( mValue.getLength( ) > 1024 )
		return false;

	if ( mPostData.getLength( ) > 1024 * 1024 )
		return false;

	return true;
}

bool CNSSession::onSessionRecv( unsigned int bytesTran )
{
	mSessionDesc.mRecvBytes += bytesTran;
	//// 重新设置接收缓冲的逻辑长度
	//mIBuffer.Length( ) = bytesTran;

	// 预约接受缓冲大小, 函数内部自动保证2的N次方对齐
	mIBuffer.resize( bytesTran );

	// 将解密后的数据放入接收数据流中
	mIStream.insert( mIStream.end( ), mIBuffer.begin( ), mIBuffer.end( ) );

	// 清空接收缓冲
	mIBuffer.clear( );

	// 从mRecvStream里面解析协议
	try
	{
		if ( mIsHttp == false )
		{
			CNSProtocol* proto = NULL;
			for ( ; proto = decode( mIStream ); )
			{
				mpManager->onProtocol( mpNetworkIO->mName, mSessionID, proto );

				// 如果接收协议逻辑删除了会话，那么不需要在循环了
				if ( mPeerSocket == NULL )
					return false;
			}
		}
		else
		{
			// 数据接收状态
			mIStream >> NSBase::EBegin;
			int	length = (unsigned int) mIStream.length( ) - mIStream.mPos;
			char* buffer = (char*) mIStream.begin( ) + mIStream.mPos;
			for ( int i = 0; i < length; i ++ )
			{
				if ( httpInput( buffer[ i ] ) == false )
				{
					// 如果HTTP协议不正确，断开和客户端的连接
					mpNetworkIO->destroy( mSessionID );
					return false;
				}

				mIStream.mPos ++;
				if ( mHttpStatus == HTTP_END )
				{
					mpManager->onProtocol( mpNetworkIO->mName, mSessionID, mHeaders, mHttpData, mPostData );

					// 如果接收协议逻辑删除了会话，那么不需要在循环了
					if ( mPeerSocket == NULL )
						return false;
				}
			}

			mIStream >> NSBase::ECommit;
		}
	}
	catch ( CNSProtocol::CException& )
	{
		mpNetworkIO->destroy( mSessionID );
		return false;
	}

	return true;
}

CNSProtocol* CNSSession::decode( CNSOctetsStream& stream )
{
	TProtocolID		protoID = 0;
	unsigned int	protoSize = 0;
	CNSProtocol*	proto = NULL;
	try
	{
		if ( mLastProto == NULL )
		{
			// 暂时不用压缩
			stream >> NSBase::EBegin;
			stream >> protoID;
			stream >> protoSize;
			stream >> NSBase::ERollback;

			mLastProto = mpManager->createProtocol( protoID );
			if ( mLastProto == NULL || mpManager->sizePolicy( protoID, protoSize ) == false )
			{
				// Log
				NSLog::log( "manager[%s] protoID[%d] not found", mpManager->mName.getBuffer( ), protoID );
				throw CNSProtocol::CException( );
			}
		}

		if ( mLastProto->isRawProto( ) == true )
		{
			TProtocolID*	pid = NULL;
			CNSOctetsShadow buffer;
			// 这类协议只有网关使用，目的是减少内存拷贝次数
			try
			{
				stream >> NSBase::EBegin;
				stream.unmarshalToPointer( pid );
				stream.unmarshalToPointer( buffer );
				stream >> NSBase::ECommit;
			}
			catch ( CNSMarshal::CException& )
			{
				throw;
			}

			proto = mLastProto;
			// 处理原始协议
			mpManager->onRawProtocol( mpNetworkIO->mName, mSessionID, pid, buffer );
		}
		else
		{
			static CNSOctetsStream data( 65535 );
			data.clear( );
			try
			{
				stream >> NSBase::EBegin;
				stream >> protoID;
				stream >> data;
				stream >> NSBase::ECommit;
			}
			catch ( CNSMarshal::CException& )
			{
				throw;
			}

			// 建立一个协议
			proto = mLastProto;
			data >> *proto;
		}

		mLastProto = NULL;
	}
	catch ( CNSMarshal::CException& e )
	{
		// 如果此时协议已经克隆，那么说明协议在内部解包出错，说明发送有可能协议攻击
		if ( proto != NULL )
			throw CNSProtocol::CException( );

		if ( mLastProto != NULL && mIsNotifyProgress == true )
			mpManager->onProtocolProgress( protoID, e.mTotal, e.mProgress );

		stream >> NSBase::ERollback;
	}

	return proto;
}
