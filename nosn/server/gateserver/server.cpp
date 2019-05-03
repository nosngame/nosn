#include <nsbase.h>
#include <protocol.h>
#include "server.h"

namespace GateServer
{
	void CInnerServer::onAddSession( const CNSString & name, TSessionID sessionID, const CNSSockAddr & local, const CNSSockAddr & peer )
	{
		CGateLogic* logic = NSWin32::CConsoleApp< CGateLogic >::getLogic( );
		logic->onInnerAddSession( name, sessionID, local, peer );
	}

	void CInnerServer::onDelSession( const CNSString& name, TSessionID sessionID )
	{
		CGateLogic* logic = NSWin32::CConsoleApp< CGateLogic >::getLogic( );
		logic->onInnerDelSession( name, sessionID );
	}

	// 1|2		从内网收4字节 4字节		  1字节		n字节	1字节	  n字节	 4字节         
	// pid		size		( tunnelsize( tunnelpid	data	tunnelpid data ) sessionid )
	// |		|			  |
	// protoID	protoSize	  protoBuffer	
	// 1		向外网发4字节 1字节		n字节	1字节	  n字节	 
	// pid		size		( tunnelpid	data	tunnelpid data )
	// |		|			  |
	// protoID	protoSize	  protoBuffer	
	void CInnerServer::onRawProtocol( const CNSString& name, TSessionID sessionID, TProtocolID* protoID, const CNSOctetsShadow& buffer )
	{
		CGateLogic* logic = NSWin32::CConsoleApp< CGateLogic >::getLogic( );
		switch ( *protoID )
		{
		case NSGateProto::CProtocolBatchTunnel::PID:
		{
			static CNSVector< TSessionID* > sessionList;
			buffer.unmarshalToPointer( sessionList );

			CNSOctetsShadow tunnelBuffer;
			buffer.unmarshalToPointer( tunnelBuffer );

			TProtocolSize tunnelLen = tunnelBuffer.length( );
			TProtocolID pid = NSGateProto::CProtocolTunnel::PID;
			void* begin = tunnelBuffer.begin( );
			void* end = tunnelBuffer.end( );
			for ( size_t i = 0; i < sessionList.getCount( ); i ++ )
			{
				TSessionID& gateSessionID = *( sessionList[ i ] );
				send( logic->mpOuter->getNetworkIO( ), &pid, sizeof( TProtocolID ), gateSessionID );
				send( logic->mpOuter->getNetworkIO( ), &tunnelLen, 4, gateSessionID );
				send( logic->mpOuter->getNetworkIO( ), begin, end, gateSessionID );
			}
			break;
		}
		case NSGateProto::CProtocolTunnel::PID:
		{
			TSessionID* gateSessionIDPtr;
			CNSOctetsShadow tunnelBuffer;
			buffer.unmarshalToPointer( tunnelBuffer );
			buffer.unmarshalToPointer( gateSessionIDPtr );
			TSessionID& gateSessionID = *gateSessionIDPtr;

			TProtocolSize tunnelLen = 0;
			bool compressed = false;
			void* begin = NULL;
			void* end = NULL;
			if ( tunnelBuffer.length( ) >= 512 )
			{
				CNSOctets buffer = tunnelBuffer.compress( );

				tunnelLen = buffer.length( );
				compressed = true;
				begin = buffer.begin( );
				end = buffer.end( );
			}
			else
			{
				tunnelLen = tunnelBuffer.length( );
				compressed = false;
				begin = tunnelBuffer.begin( );
				end = tunnelBuffer.end( );
			}

			if ( compressed == true )
			{
				unsigned char protoIDHeader = (unsigned char) ( ( *protoID ) | 0x80 );
				send( logic->mpOuter->getNetworkIO( ), &protoIDHeader, sizeof( TProtocolID ), gateSessionID );
			}
			else
			{
				send( logic->mpOuter->getNetworkIO( ), protoID, sizeof( TProtocolID ), gateSessionID );
			}

			send( logic->mpOuter->getNetworkIO( ), &tunnelLen, 4, gateSessionID );
			send( logic->mpOuter->getNetworkIO( ), begin, end, gateSessionID );
			break;
		}
		}
	}

	void CInnerServer::onProtocol( const CNSString& name, TSessionID sessionID, CNSProtocol* proto )
	{
		CGateLogic* logic = NSWin32::CConsoleApp< CGateLogic >::getLogic( );
		logic->onInnerProtocol( name, sessionID, proto );
	}

	void COuterServer::onAddSession( const CNSString& name, TSessionID sessionID, const CNSSockAddr& local, const CNSSockAddr& peer )
	{
		CGateLogic* logic = NSWin32::CConsoleApp< CGateLogic >::getLogic( );
		logic->onOuterAddSession( name, sessionID, local, peer );
	}

	void COuterServer::onDelSession( const CNSString& name, TSessionID sessionID )
	{
		NSLog::log( _UTF8( "断开连接[%d]" ), sessionID );
		CGateLogic* logic = NSWin32::CConsoleApp< CGateLogic >::getLogic( );
		CGateUser* user = logic->getUser( sessionID );
		if ( user == NULL )
			return;

		logic->onOuterDelSession( name, sessionID );
	}

	// 1		从外网收4字节	4字节		1字节		n字节 
	// pid		size	  ( tunnelsize(	tunnelpid	data ) )
	// |		|			|
	// protoID	protoSize	protoBuffer	
	// 1		向内网发4字节 4字节		1字节		n字节  4字节
	// pid		size	  ( tunnelsize( tunnelpid	data ) sessionid )
	// |		|			|
	// protoID	protoSize	protoBuffer
	void COuterServer::onRawProtocol( const CNSString& name, TSessionID sessionID, TProtocolID* protoID, const CNSOctetsShadow& buffer )
	{
		CGateLogic* logic = NSWin32::CConsoleApp< CGateLogic >::getLogic( );
		CGateUser* user = logic->getUser( sessionID );
		if ( user == NULL || user->mFreeze == true )
			return;

		void* begin = (void*) protoID;
		void* end = buffer.end( );
		// 外网接收时没有sessionid，协议长度少了4字节，协议长度需要补上4字节
		*(unsigned int*) ( ( (TProtocolID*) begin ) + 1 ) += sizeof( TSessionID );

		if ( *protoID == NSGateProto::CProtocolTunnel::PID )
		{
			// 隧道协议，转发给玩家当前内部逻辑服务器
			CService* service = logic->getService( user->mCurService );
			if ( service == NULL )
				return;

			// 转发给内部逻辑服务器
			send( logic->mpInner->getNetworkIO( ), begin, end, service->mSessionID );
			send( logic->mpInner->getNetworkIO( ), &sessionID, sizeof( TSessionID ), service->mSessionID );
			// 外网接收时没有sessionid，协议长度少了4字节，协议补上4字节, 但是这里要把内存还原，否则之后的逻辑使用了错误的长度
			*(unsigned int*) ( ( (TProtocolID*) begin ) + 1 ) -= sizeof( TSessionID );
			return;
		}
		else if ( *protoID == NSGateProto::CProtocolTunnel::PPID )
		{
			// 隧道协议，转发给oper逻辑服务器
			CService* operService = logic->getOperService( );
			if ( operService == NULL )
			{
				NSWin32::CNSBaseApp* app = NSWin32::CNSBaseApp::getApp( );
				NSGateProto::CProtocolLogout logout( 4 );
				send( logic->mpOuter->getNetworkIO( ), logout.createShortStream( ), sessionID );
				NSLog::log( _UTF8( "oper服务没有开启" ) );
				return;
			}

			// 转发给内部逻辑服务器
			send( logic->mpInner->getNetworkIO( ), begin, end, operService->mSessionID );
			send( logic->mpInner->getNetworkIO( ), &sessionID, sizeof( TSessionID ), operService->mSessionID );
			// 外网接收时没有sessionid，协议长度少了4字节，协议补上4字节, 但是这里要把内存还原，否则之后的逻辑使用了错误的长度
			*(unsigned int*) ( ( (TProtocolID*) begin ) + 1 ) -= sizeof( TSessionID );
			return;
		}
	}

	void COuterServer::onProtocol( const CNSString& name, TSessionID sessionID, CNSProtocol* proto )
	{
		CGateLogic* logic = NSWin32::CConsoleApp< CGateLogic >::getLogic( );
		logic->onOuterProtocol( name, sessionID, proto );
	}

	bool CGateLogic::checkIPOpen( CNSString ipAddress )
	{
		// 如果不使用IP白名单
		if ( mEnableIP == false )
			return true;

		return mIPOpens.find( ipAddress );
	}

	void CGateLogic::boardcast( CNSProtocol* proto )
	{
		const CNSOctets& buffer = proto->createStream( );
		send( mpInner->getNetworkIO( ), buffer, 0 );
	}

	CGateUser* CGateLogic::getUser( TSessionID sessionID )
	{
		CGateUser** tpUserRef = mUsers.get( sessionID );
		if ( tpUserRef == NULL )
			return NULL;

		return ( *tpUserRef );
	}

	void CGateLogic::getUserBySessionID( TSessionID sessionID, CNSVector< TSessionID >& userSession )
	{
		HLISTINDEX userIndex = mUsers.getHead( );
		for ( ; userIndex != NULL; mUsers.getNext( userIndex ) )
		{
			TSessionID userSessionID = mUsers.getKey( userIndex );
			CGateUser* user = mUsers.getValue( userIndex );
			if ( user != NULL )
			{
				CService* service = getService( user->mCurService );
				if ( service != NULL && service->mSessionID == sessionID )
					userSession.pushback( userSessionID );
			}
		}
	}

	CService* CGateLogic::getOperService( ) const
	{
		return mServices[ 0 ];
	}

	CService* CGateLogic::getService( unsigned int serviceID )
	{
		return mServices[ serviceID ];
	}

	void CGateLogic::addUser( TSessionID sessionID )
	{
		mUsers.insert( sessionID, new CGateUser( sessionID ) );
	}

	void CGateLogic::removeUser( TSessionID sessionID )
	{
		CGateUser** tpUserRef = mUsers.get( sessionID );
		if ( tpUserRef == NULL )
			return;

		delete ( *tpUserRef );
		mUsers.erase( sessionID );
	}

	void CGateLogic::changeService( unsigned int serviceID, TSessionID sessionID )
	{
		CGateUser** tpUserRef = mUsers.get( sessionID );
		if ( tpUserRef == NULL )
			return;

		( *tpUserRef )->mCurService = serviceID;
	}

	void CGateLogic::registerOperService( TSessionID sessionID )
	{
		if ( mServices[ 0 ] != NULL )
			delete mServices[ 0 ];

		mServices[ 0 ] = new CService( 0, sessionID );
	}

	void CGateLogic::registerLogicService( unsigned int serviceID, TSessionID sessionID )
	{
		if ( serviceID >= 128 )
		{
			NSLog::log( _UTF8( "serviceID 最大不能超过128 服务注册失败，游戏服务器不能正常提供服务" ) );
			return;
		}

		if ( mServices[ serviceID ] != NULL )
			delete mServices[ serviceID ];

		mServices[ serviceID ] = new CService( serviceID, sessionID );
	}

	void CGateLogic::removeService( unsigned int serviceID )
	{
		delete mServices[ serviceID ];
		mServices[ serviceID ] = NULL;
	}

	void CGateLogic::onInnerDelSession( const CNSString& name, TSessionID sessionID )
	{
		CGateLogic* logic = NSWin32::CConsoleApp< CGateLogic >::getLogic( );
		if ( sessionID == 0 )
		{
			NSWin32::CNSBaseApp* app = NSWin32::CNSBaseApp::getApp( );
			NSGateProto::CProtocolLogout logout( 1 );
			const CNSOctets& buffer = logout.createShortStream( );

			HLISTINDEX beginIndex = logic->mUsers.getHead( );
			for ( ; beginIndex != NULL; logic->mUsers.getNext( beginIndex ) )
			{
				TSessionID sessionID = logic->mUsers.getKey( beginIndex );
				send( logic->mpOuter->getNetworkIO( ), buffer, sessionID );
			}
		}
		else
		{
			NSWin32::CNSBaseApp* app = NSWin32::CNSBaseApp::getApp( );
			NSGateProto::CProtocolLogout logout( 1 );
			const CNSOctets& buffer = logout.createShortStream( );
			CNSVector< TSessionID > userSessions;

			logic->getUserBySessionID( sessionID, userSessions );
			for ( size_t i = 0; i < userSessions.getCount( ); i ++ )
				send( logic->mpOuter->getNetworkIO( ), buffer, userSessions[ i ] );
		}

		CNSSessionDesc* desc = NSNet::getSessionDesc( name, sessionID );
		if ( desc == NULL )
			return;

		NSLog::log( _UTF8( "逻辑服务器[%s]连接丢失" ), desc->mPeer.getBuffer( ) );
	}

	void CGateLogic::onInnerProtocol( const CNSString& name, TSessionID sessionID, CNSProtocol* proto )
	{
		switch ( proto->mProtoID )
		{
		case NSGateProto::CProtocolClientLogin::PID:
		{
			NSGateProto::CProtocolClientLogin* clientLogin = ( NSGateProto::CProtocolClientLogin* ) proto;
			this->boardcast( clientLogin );
			break;
		}
		case NSGateProto::CProtocolChangeService::PID:
		{
			NSGateProto::CProtocolChangeService* changeService = ( NSGateProto::CProtocolChangeService* ) proto;
			this->changeService( changeService->mServiceID, changeService->mSessionID );
			break;
		}
		case NSGateProto::CProtocolUnknownUser::PID:
		{
			NSGateProto::CProtocolUnknownUser* unknown = ( NSGateProto::CProtocolUnknownUser* ) proto;
			CGateUser* user = this->getUser( unknown->mSessionID );
			if ( user == NULL )
				break;

			// 如果有原因，通知玩家关闭原因
			if ( unknown->mReasonID != 0 )
			{
				NSGateProto::CProtocolLogout logout( unknown->mReasonID );
				NSNet::send( mpOuter->getNetworkIO( ), logout.createShortStream( ), unknown->mSessionID );
			}

			// 如果玩家连接保持，不再接受玩家任何协议
			user->mFreeze = true;
			break;
		}
		case NSGateProto::CProtocolRegisterOperService::PID:
		{
			NSGateProto::CProtocolRegisterOperService* registerService = ( NSGateProto::CProtocolRegisterOperService* ) proto;
			registerOperService( sessionID );
			NSLog::log( _UTF8( "服务ID[0] oper服务 注册成功" ) );
			break;
		}
		case NSGateProto::CProtocolRegisterLogicService::PID:
		{
			NSGateProto::CProtocolRegisterLogicService* registerService = ( NSGateProto::CProtocolRegisterLogicService* ) proto;
			registerLogicService( registerService->mServiceID, sessionID );
			NSLog::log( _UTF8( "服务ID[%d] 逻辑服务[%s] 注册成功" ), registerService->mServiceID, registerService->mName.getBuffer( ) );
			break;
		}
		case NSGateProto::CProtocolShutdownGate::PID:
		{
			NSGateProto::CProtocolShutdownGate* tpShutdown = ( NSGateProto::CProtocolShutdownGate* ) proto;
			CNSTimer::createTimer( NSBase::nsTimerShutdown, tpShutdown->mDuration, NULL );
			break;
		}
		case NSGateProto::CProtocolEnableIPOpen::PID:
		{
			NSGateProto::CProtocolEnableIPOpen* ipOpen = ( NSGateProto::CProtocolEnableIPOpen* ) proto;
			mEnableIP = ipOpen->mEnableIPOpen;
			if ( ipOpen->mEnableIPOpen == true )
				NSLog::log( _UTF8( "打开IP白名单" ) );
			else
				NSLog::log( _UTF8( "关闭IP白名单" ) );
			break;
		}
		}
	}

	void CGateLogic::onInnerAddSession( const CNSString& name, TSessionID sessionID, const CNSSockAddr& local, const CNSSockAddr& peer )
	{
	}

	void CGateLogic::onOuterAddSession( const CNSString& name, TSessionID sessionID, const CNSSockAddr& local, const CNSSockAddr& peer )
	{
		if ( this->checkIPOpen( peer.GetIPString( ) ) == false )
		{
			NSWin32::CNSBaseApp* app = NSWin32::CNSBaseApp::getApp( );
			NSGateProto::CProtocolLogout logout( 2 );
			NSNet::send( mpOuter->getNetworkIO( ), logout.createShortStream( ), sessionID );
			return;
		}

		CService* operService = this->getOperService( );
		if ( operService == NULL )
		{
			static CNSString errorDesc;
			errorDesc.format( _UTF8( "oper服务器没有注册，服务器不能提供正常服务" ) );
			NSException( errorDesc );
			return;
		}

		this->addUser( sessionID );
		CNSSessionDesc* desc = NSNet::getSessionDesc( name, sessionID );
		if ( desc != NULL )
		{
			NSGateProto::CProtocolSessionIPNotice ipNotice( desc->mPeer, sessionID );
			NSNet::send( mpInner->getNetworkIO( ), ipNotice.createStream( ), operService->mSessionID );
		}
		NSLog::log( _UTF8( "接受[%s]的连接 sessionID[%d]" ), peer.GetIPString( ).getBuffer( ), sessionID );
	}

	void CGateLogic::onOuterDelSession( const CNSString& name, TSessionID sessionID )
	{
		// 是不是只断开网关的连接
		NSGateProto::CProtocolClientLost clientLost( sessionID );
		boardcast( &clientLost );
		removeUser( sessionID );
	}

	void CGateLogic::onOuterProtocol( const CNSString& name, TSessionID sessionID, CNSProtocol* proto )
	{
		if ( proto->mProtoID == NSGateProto::CProtocolKeepAlive::PID )
		{
			CGateUser* user = getUser( sessionID );
			if ( user == NULL )
				return;

			unsigned int thisTick = CNSTimer::getCurTick( );
			if ( user->mCurTick != 0 )
			{
				if ( thisTick - user->mCurTick < 4900 )
				{
					user->mCheatCounter ++;
					if ( user->mCheatCounter >= 5 )
					{
						NSWin32::CNSBaseApp* app = NSWin32::CNSBaseApp::getApp( );
						NSGateProto::CProtocolLogout logout( 3 );
						send( mpOuter->getNetworkIO( ), logout.createShortStream( ), sessionID );
						return;
					}
				}
				else
					user->mCheatCounter = 0;
			}

			user->mCurTick = thisTick;
			send( mpOuter->getNetworkIO( ), proto->createShortStream( ), sessionID );
		}
	}

	void CGateLogic::loadIPOpen( )
	{
		TiXmlDocument tDoc;
		if ( tDoc.LoadFile( "ipopen.xml" ) == false )
		{
			static CNSString errorDesc;
			errorDesc.format( _UTF8( "ipopen.xml打开错误, 错误码: %s" ), tDoc.ErrorDesc( ) );
			NSException( errorDesc );
		}

		TiXmlElement* tpIPOpen = tDoc.FirstChildElement( "IPOpen" );
		for ( ; tpIPOpen != NULL; tpIPOpen = tpIPOpen->NextSiblingElement( "IPOpen" ) )
		{
			CNSString ipOpen = tpIPOpen->Attribute( "Name" );
			mIPOpens.insert( ipOpen );
		}
	}

	void CGateLogic::init( NSWin32::CConsoleApp< CGateLogic >* app )
	{
		mGateName = app->getCmdString( "name", "NULL" );
		mEnableIP = app->getCmdBool( "enableip", false );
		CNSString psw = app->getCmdString( "psw", "" );
		if ( psw == "bef089a27c454bc6b7a97b144f9067f2" )
			mDevelopment = false;

		TiXmlDocument doc;
		if ( doc.LoadFile( "serverconfig.xml" ) == false )
		{
			static CNSString errorDesc;
			errorDesc.format( _UTF8( "serverconfig.xml打开错误, 错误码: %s" ), doc.ErrorDesc( ) );
			NSException( errorDesc );
		}

		// 加载IP白名单
		if ( mEnableIP == true )
			loadIPOpen( );

		CNSString innerAddress;
		CNSString outerAddress;
		unsigned int iInterval;
		size_t iBufferSize;
		size_t iStreamSize;

		unsigned int oInterval;
		size_t oBufferSize;
		size_t oStreamSize;

		TiXmlElement* gate = doc.FirstChildElement( "gateserver" );
		for ( ; gate != NULL; gate = gate->NextSiblingElement( "gateserver" ) )
		{
			CNSString name = gate->Attribute( "name" );
			if ( mGateName == name )
			{
				static CNSString title;
				const CNSString& lang = CNSLocal::getNSLocal( ).getLang( );
#ifdef _M_IX86
				title.format( _UTF8( "网关服务器 - %s 语言 - %s 32bit" ), mGateName.getBuffer( ), lang.getBuffer( ) );
#elif _M_X64
				title.format( _UTF8( "网关服务器 - %s 语言 - %s 64bit" ), mGateName.getBuffer( ), lang.getBuffer( ) );
#endif
				app->setConsoleTitle( title );
				NSLog::log( _UTF8( "正在启动网关服务器..." ) );

				if ( mDevelopment == true )
					NSLog::log( _UTF8( "\t启动姿态: 开发状态" ) );
				else
					NSLog::log( _UTF8( "\t启动姿态: 运营状态" ) );
				NSLog::log( "\tGate - %s", mGateName.getBuffer( ) );

				if ( mEnableIP == true )
					NSLog::log( _UTF8( "\t是否开启IP白名单 - 是" ) );
				else
					NSLog::log( _UTF8( "\t是否开启IP白名单 - 否" ) );

				const CNSString& version = CNSLocal::getNSLocal( ).getVersion( );
				NSLog::log( _UTF8( "\t当前版本号:%s " ), version.getBuffer( ) );

				TiXmlElement* inner = gate->FirstChildElement( "inner" );
				TiXmlElement* outer = gate->FirstChildElement( "outer" );
				if ( inner == NULL )
				{
					static CNSString errorDesc;
					errorDesc.format( _UTF8( "serverconfig.xml文件格式错误, gateserver[%s]没有找到inner节点" ), name.getBuffer( ) );
					NSException( errorDesc );
				}

				if ( outer == NULL )
				{
					static CNSString errorDesc;
					errorDesc.format( _UTF8( "serverconfig.xml文件格式错误, gateserver[%s]没有找到outer节点" ), name.getBuffer( ) );
					NSException( errorDesc );
				}

				innerAddress = app->name2IPPort( inner->Attribute( "address" ) );
				iBufferSize = CNSString( inner->Attribute( "buffersize" ) ).toInteger( );
				iStreamSize = CNSString( inner->Attribute( "streamsize" ) ).toInteger( );
				iInterval = CNSString( inner->Attribute( "interval" ) ).toInteger( );

				outerAddress = app->name2IPPort( outer->Attribute( "address" ) );
				oBufferSize = CNSString( outer->Attribute( "buffersize" ) ).toInteger( );
				oStreamSize = CNSString( outer->Attribute( "streamsize" ) ).toInteger( );
				oInterval = CNSString( outer->Attribute( "interval" ) ).toInteger( );
				break;
			}
		}

		if ( innerAddress.isEmpty( ) == true )
		{
			static CNSString errorDesc;
			errorDesc.format( _UTF8( "有没有找到指定[%s]网关，启动失败" ), mGateName.getBuffer( ) );
			NSException( errorDesc );
		}

		int acceptNum = 1024;
		if ( mDevelopment == true )
		{
			static CNSVector< CNSString > textList;
			textList.clear( );
			outerAddress.split( ":", textList );
			if ( textList[ 0 ] != "127.0.0.1" )
			{
				static CNSString errorDesc;
				errorDesc.format( _UTF8( "当前服务器是开发版本不能绑定外网IP" ) );
				NSException( errorDesc );
			}
			acceptNum = 1;
		}

		mpOuter = new COuterServer( "oserver", outerAddress, oInterval, oBufferSize, oStreamSize );
		registerServer( mpOuter, acceptNum );
		NSLog::log( _UTF8( "外部地址[%s]打开成功" ), outerAddress.getBuffer( ) );
		NSLog::log( _UTF8( "\tbuffsize - %d" ), oBufferSize );
		NSLog::log( _UTF8( "\tstreamsize - %d" ), oStreamSize );
		NSLog::log( _UTF8( "\tinterval - %d" ), oInterval );

		mpInner = new CInnerServer( "iserver", innerAddress, iInterval, iBufferSize, iStreamSize );
		registerServer( mpInner, 5 );
		NSLog::log( _UTF8( "内部地址[%s]打开成功" ), innerAddress.getBuffer( ) );
		NSLog::log( _UTF8( "\tbuffsize - %d" ), iBufferSize );
		NSLog::log( _UTF8( "\tstreamsize - %d" ), iStreamSize );
		NSLog::log( _UTF8( "\tinterval - %d" ), iInterval );
	}

	void CGateLogic::exit( )
	{
		delete mpInner;
		delete mpOuter;

		for ( int i = 0; i < 1024; i ++ )
		{
			if ( mServices[ i ] != NULL )
				delete mServices[ i ];
		}
	}
};

int main( int argc, char* argv[] )
{
	try
	{
		NSWin32::CConsoleApp< GateServer::CGateLogic > app;
		app.useNSMysql( false );
		app.useNSHttp( false );
		app.useNSHttpDebugger( false );
		app.useIPName( true );
		app.useNSLocal( true );
		app.run( );
	}
	catch ( CNSException& e )
	{
		// 还原标准输出
		freopen( "CON", "w", stdout );

		// 这里的异常处理是不会写文件的，stdout已经还原
		static CNSString errorDesc;
		errorDesc.format( _UTF8( "NSLog exception:\n%s\nCRT main函数发生异常\n错误描述: \n\t%s\nC++调用堆栈:\n%s" ), NSLog::sExceptionText.getBuffer( ),
						  e.mErrorDesc, NSBase::NSFunction::getStackInfo( ).getBuffer( ) );
		printf( CNSString::convertUtf8ToMbcs( errorDesc ) );
		getchar( );
		return 1;
	}

	return 0;
}
