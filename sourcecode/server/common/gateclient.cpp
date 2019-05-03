#pragma once
#include <nsbase.h>
#include <protocol.h>
#include <gateclient.h>

namespace GateClient
{
	void CUser::send( CNSProtocol* proto )
	{
		static CNSOctetsStream protoBuffer;
		protoBuffer.clear( );
		protoBuffer << proto->getProtoID( );
		protoBuffer << *proto;

		NSGateProto::CProtocolTunnel protoTunnel( &protoBuffer.mBuffer, mSessionID );
		NSNet::send( mGateName, protoTunnel.createStream( ) );
	}

	void CInnerClient::onRegisterProtocol( const CNSString& name )
	{
		static NSGateProto::CProtocolTunnel tunnel( NULL, 0, NSGateProto::CProtocolTunnel::PID );
		static NSGateProto::CProtocolTunnel operTunnel( NULL, 0, NSGateProto::CProtocolTunnel::PPID );
		static NSGateProto::CProtocolSessionIPNotice onSessionIPNotice;
		static NSGateProto::CProtocolClientLost sessionLost;
		static NSGateProto::CProtocolClientLogin clientLogin;

		registerProtocol( &tunnel );
		registerProtocol( &operTunnel );
		registerProtocol( &onSessionIPNotice );
		registerProtocol( &sessionLost );
		registerProtocol( &clientLogin );

		static NSClientProto::CProtocolScript scriptProto;
		NSGateProto::CProtocolTunnel::registerProtocol( &scriptProto );
		mpLogic->onRegisterClientProtocol( this );
	}

	void CInnerClient::onAddSession( const CNSString& name, TSessionID sessionID, const CNSSockAddr& local, const CNSSockAddr& peer )
	{
		mpLogic->onConnectGate( name, mAddress );
	}

	void CInnerClient::onAddSessionFault( const CNSString& name, int code )
	{
		mpLogic->onConnectGateFault( name, mAddress, code );
		registerClient( this );
	}

	void CInnerClient::onDelSession( const CNSString& name, TSessionID sessionID )
	{
		mpLogic->onGateLost( name, mAddress );
		registerClient( this );
	}

	// 1		内网发4字节 4字节		1字节		n字节  4字节
	// pid		size	  ( tunnelsize( tunnelpid	data ) sessionid )
	// |		|			|
	// protoID	protoSize	protoBuffer
	void CInnerClient::onRawProtocol( const CNSString& name, TSessionID sessionID, TProtocolID* protoID, const CNSOctetsShadow& buffer )
	{
		switch ( *protoID )
		{
			case NSGateProto::CProtocolTunnel::PPID:
			case NSGateProto::CProtocolTunnel::PID:
			{
				TProtocolID*	tunnelPid = NULL;
				TSessionID*		gateSessionID = NULL;
				CNSProtocol*	proto = NULL;
				try
				{
					CNSOctetsShadow tunnelBuffer;
					buffer.unmarshalToPointer( tunnelBuffer );
					buffer.unmarshalToPointer( gateSessionID );
					proto = NSGateProto::CProtocolTunnel::getProto( tunnelBuffer );
				}
				catch ( CNSMarshal::CException& )
				{
					long long nosnUserID = 0;
					if ( gateSessionID != NULL && mpLogic->getUserID( name, *gateSessionID, nosnUserID ) == true )
						NSLog::log( _UTF8( "nosn UserID - %lld, 隧道协议解码错误, 可能发生隧道协议攻击" ), nosnUserID );
					else
						NSLog::log( _UTF8( "未通过验证的客户端, 隧道协议解码错误, 可能发生隧道协议攻击" ) );
					proto = NULL;
				}

				// 如果协议没有找到，通知网关断开客户端连接
				if ( proto == NULL )
				{
					NSGateProto::CProtocolUnknownUser unknown( *gateSessionID, 3 );
					NSNet::send( name, unknown.createStream( ) );
					break;
				}

				mpLogic->onClientProtocol( this, *gateSessionID, proto );
				break;
			}
		}
	}

	void CInnerClient::onProtocol( const CNSString& name, TSessionID sessionID, CNSProtocol* proto )
	{
		switch ( proto->mProtoID )
		{
			case NSGateProto::CProtocolSessionIPNotice::PID:
			{
				NSGateProto::CProtocolSessionIPNotice* ipNotice = ( NSGateProto::CProtocolSessionIPNotice* ) proto;
				// 将网关连接信息记录对应IP
				mpLogic->mIPTables.insert( name + "_" + CNSString::number2String( sessionID ), ipNotice->mIP );
				break;
			}
			case NSGateProto::CProtocolClientLogin::PID:
			{
				NSGateProto::CProtocolClientLogin* login = ( NSGateProto::CProtocolClientLogin* ) proto;
				mpLogic->bindUser( name, login->mSessionID, login->mNosnUserID );
				mpLogic->onClientLogin( name, login->mSessionID, login->mNosnUserID );
				break;
			}
			case NSGateProto::CProtocolClientLost::PID:
			{
				NSGateProto::CProtocolClientLost* sessionLost = ( NSGateProto::CProtocolClientLost* ) proto;
				mpLogic->mIPTables.erase( name + "_" + CNSString::number2String( sessionID ) );
				long long nosnUserID = 0;
				if ( mpLogic->getUserID( name, sessionLost->mSessionID, nosnUserID ) == true )
					mpLogic->onClientLost( name, sessionLost->mSessionID, nosnUserID );

				mpLogic->unBindUser( name, sessionLost->mSessionID, nosnUserID );
				break;
			}
		}
	}

	void CInnerLogic::onConnectGate( const CNSString& name, const CNSString& address )
	{
		NSLog::log( _UTF8( "网关服务器[%s] 地址[%s]连接成功" ), name.getBuffer( ), address.getBuffer( ) );
	}

	void CInnerLogic::onConnectGateFault( const CNSString& name, const CNSString& address, unsigned int code )
	{
		NSLog::log( _UTF8( "连接网关服务器[%s] 地址[%s]失败, 错误码 - %d" ), name.getBuffer( ), address.getBuffer( ), code );
	}

	void CInnerLogic::onGateLost( const CNSString& name, const CNSString& address )
	{
		mNosnUsers.clear( );
		mUsers.clear( );
		NSLog::log( _UTF8( "网关服务器[%s] 地址[%s]连接丢失" ), name.getBuffer( ), address.getBuffer( ) );
	}

	// 通过网关名和在网关的回话ID获得用户ID
	bool CInnerLogic::getUserID( const CNSString& name, TSessionID sessionID, long long& nosnUserID )
	{
		long long* userID = mNosnUsers.get( name + CNSString::number2String( sessionID ) );
		if ( userID == NULL )
			return false;

		nosnUserID = *userID;
		return true;
	}

	void CInnerLogic::bindUser( const CNSString& name, TSessionID sessionID, long long nosnUserID )
	{
		mNosnUsers.insert( name + CNSString::number2String( sessionID ), nosnUserID );
		CUser user( nosnUserID, sessionID, name );
		mUsers.insert( nosnUserID, user );
	}

	void CInnerLogic::unBindUser( const CNSString& name, TSessionID sessionID, long long nosnUserID )
	{
		mNosnUsers.erase( name + CNSString::number2String( sessionID ) );
		mUsers.erase( nosnUserID );
	}

	void CInnerLogic::exit( )
	{
		for ( HLISTINDEX beginIndex = mInners.getHead( ); beginIndex != NULL; mInners.getNext( beginIndex ) )
			delete mInners.getValue( beginIndex );
	}

	void CInnerLogic::enableIPOpen( bool enable )
	{
		HLISTINDEX beginIndex = mInners.getHead( );
		for ( ; beginIndex != NULL; mInners.getNext( beginIndex ) )
		{
			const CNSString& name = mInners.getKey( beginIndex );
			NSGateProto::CProtocolEnableIPOpen ipOpen( enable );
			NSNet::send( name, ipOpen.createStream( ) );
		}
	}

	void CInnerLogic::send2User( long long userID, CNSProtocol* proto )
	{
		GateClient::CUser* user = mUsers.get( userID );
		if ( user == NULL )
			return;

		user->send( proto );
	}

	void CInnerLogic::send2User( const CInnerClient* gate, TSessionID sessionID, CNSProtocol* proto )
	{
		static CNSOctetsStream protoBuffer;
		protoBuffer.clear( );
		protoBuffer << proto->getProtoID( );
		protoBuffer << *proto;

		NSGateProto::CProtocolTunnel protoTunnel( &protoBuffer.mBuffer, sessionID );
		NSNet::send( gate->getNetworkIO( ), protoTunnel.createStream( ) );
	}

	void CInnerLogic::send2User( const CNSString& name, TSessionID sessionID, CNSProtocol* proto )
	{
		static CNSOctetsStream protoBuffer;
		protoBuffer.clear( );
		protoBuffer << proto->getProtoID( );
		protoBuffer << *proto;

		NSGateProto::CProtocolTunnel protoTunnel( &protoBuffer.mBuffer, sessionID );
		NSNet::send( name, protoTunnel.createStream( ) );
	}

	void CInnerLogic::send2UserList( const CInnerClient** gate, CNSVector< TSessionID >* sessionList, CNSProtocol* proto )
	{
		static CNSOctetsStream streamProto;
		streamProto.clear( );
		streamProto << (TProtocolID) proto->mProtoID;
		streamProto << ( *proto );

		NSGateProto::CProtocolBatchTunnel gateDeliver( &streamProto.mBuffer );
		for ( size_t i = 0; i < 16; i ++ )
		{
			if ( gate[ i ] == NULL )
				continue;

			CNSVector< TSessionID >&	userList = sessionList[ i ];
			const CInnerClient*			gateIO = gate[ i ];
			gateDeliver.mpUserList = &userList;
			const CNSOctets& buffer = gateDeliver.createStream( );
			send( gateIO->getNetworkIO( ), buffer );
			userList.clear( );
			gate[ i ] = NULL;
		}
	}
}