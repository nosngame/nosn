#include <nsbase.h>
#include "protocol.h"
#include "gateclient.h"
#include "server.h"
#include "interface.h"

namespace LogicServer
{
	void COperClient::onRegisterProtocol( const CNSString& name )
	{
		static NSOperProto::CProtocolScript scriptProto;
		registerProtocol( &scriptProto );
	}

	void COperClient::onAddSession( const CNSString& name, TSessionID sessionID, const CNSSockAddr& local, const CNSSockAddr& peer )
	{
		NSBase::CNSLuaStack& luaStack = NSBase::CNSLuaStack::getLuaStack( );
		luaStack.preCall( "onOperClientAddSession" );
		luaStack << sessionID;
		luaStack.call( );
		NSLog::log( _UTF8( "oper逻辑客戶端 地址[%s]连接成功" ), mAddress.getBuffer( ) );
	}

	void COperClient::onAddSessionFault( const CNSString& name, int code )
	{
		registerClient( this );

		NSBase::CNSLuaStack& luaStack = NSBase::CNSLuaStack::getLuaStack( );
		luaStack.preCall( "onOperClientAddSessionFault" );
		luaStack << code;
		luaStack.call( );
		NSLog::log( _UTF8( "oper逻辑客戶端 地址[%s]连接失败, 错误码 - %d" ), mAddress.getBuffer( ), code );
	}

	void COperClient::onDelSession( const CNSString& name, TSessionID sessionID )
	{
		registerClient( this );
		NSBase::CNSLuaStack& luaStack = NSBase::CNSLuaStack::getLuaStack( );
		luaStack.preCall( "onOperClientDelSession" );
		luaStack.call( );
		NSLog::log( _UTF8( "oper逻辑客戶端 地址[%s]连接丢失" ), mAddress.getBuffer( ) );
	}

	void COperClient::onProtocol( const CNSString& name, TSessionID sessionID, CNSProtocol* proto )
	{
		switch ( proto->mProtoID )
		{
			// 逻辑服务器脚本协议
			case NSOperProto::CProtocolScript::PID:
			{
				NSOperProto::CProtocolScript* script = ( NSOperProto::CProtocolScript* ) proto;
				NSBase::CNSLuaStack& luaStack = NSBase::CNSLuaStack::getLuaStack( );
				luaStack.preCall( "onOperProtocol" );
				CNSOctetsStream stream( *script->mpScriptBuffer );
				for ( ; stream.isEnd( ) == false; )
					luaStack << stream;
				luaStack.call( );
				break;
			}
		}
	}
	void CChargeClient::onRegisterProtocol( const CNSString& name )
	{
		static NSChargeProto::CProtocolScript scriptProto;
		registerProtocol( &scriptProto );
	}

	void CChargeClient::onAddSession( const CNSString& name, TSessionID sessionID, const CNSSockAddr& local, const CNSSockAddr& peer )
	{
		NSBase::CNSLuaStack& luaStack = NSBase::CNSLuaStack::getLuaStack( );
		luaStack.preCall( "onChargeClientAddSession" );
		luaStack << sessionID;
		luaStack.call( );
		NSLog::log( _UTF8( "charge逻辑客戶端 地址[%s]连接成功" ), mAddress.getBuffer( ) );
	}

	void CChargeClient::onAddSessionFault( const CNSString& name, int code )
	{
		registerClient( this );

		NSBase::CNSLuaStack& luaStack = NSBase::CNSLuaStack::getLuaStack( );
		luaStack.preCall( "onChargeClientAddSessionFault" );
		luaStack << code;
		luaStack.call( );
		NSLog::log( _UTF8( "charge逻辑客戶端 地址[%s]连接失败, 错误码 - %d" ), mAddress.getBuffer( ), code );
	}

	void CChargeClient::onDelSession( const CNSString& name, TSessionID sessionID )
	{
		registerClient( this );
		NSBase::CNSLuaStack& luaStack = NSBase::CNSLuaStack::getLuaStack( );
		luaStack.preCall( "onChargeClientDelSession" );
		luaStack.call( );
		NSLog::log( _UTF8( "charge逻辑客戶端 地址[%s]连接丢失" ), mAddress.getBuffer( ) );
	}

	void CChargeClient::onProtocol( const CNSString& name, TSessionID sessionID, CNSProtocol* proto )
	{
		switch ( proto->mProtoID )
		{
			// 逻辑服务器脚本协议
			case NSChargeProto::CProtocolScript::PID:
			{
				NSChargeProto::CProtocolScript* script = ( NSChargeProto::CProtocolScript* ) proto;

				NSBase::CNSLuaStack& luaStack = NSBase::CNSLuaStack::getLuaStack( );
				luaStack.preCall( "onChargeProtocol" );
				CNSOctetsStream stream( *script->mpScriptBuffer );
				for ( ; stream.isEnd( ) == false; )
					luaStack << stream;
				luaStack.call( );
				break;
			}
		}
	}

	// 给Python留的控制接口
	void CHttpServer::onProtocol( const CNSString& name, TSessionID sessionID, const CNSMap< CNSString, CNSString >& headers, const CNSString& text, const CNSString& postData )
	{
		NSBase::CNSLuaStack& luaStack = NSBase::CNSLuaStack::getLuaStack( );
		luaStack.preCall( "onHttpProtocol" );
		luaStack << name;
		luaStack << sessionID;
		luaStack << headers;
		luaStack << text;
		luaStack << postData;
		luaStack.call( );
	}

	// 逻辑服务器
	void CLogicServer::onRegisterProtocol( const CNSString& name )
	{
		static NSLogicProto::CProtocolScript scriptProto;
		registerProtocol( &scriptProto );
	}

	void CLogicServer::onAddSession( const CNSString& name, TSessionID sessionID, const CNSSockAddr& local, const CNSSockAddr& peer )
	{
		NSBase::CNSLuaStack& luaStack = NSBase::CNSLuaStack::getLuaStack( );
		luaStack.preCall( "onLogicServerAddSession" );
		luaStack << mLogicName;
		luaStack << sessionID;
		luaStack.call( );
		NSLog::log( _UTF8( "逻辑服务器[%s] 远程地址[%s]连接成功" ), mLogicName.getBuffer( ), peer.GetIPString( ).getBuffer( ) );
	}

	void CLogicServer::onDelSession( const CNSString& name, TSessionID sessionID )
	{
		CNSSessionDesc* tpDesc = NSNet::getSessionDesc( name, sessionID );
		if ( tpDesc == NULL )
			return;

		NSBase::CNSLuaStack& luaStack = NSBase::CNSLuaStack::getLuaStack( );
		luaStack.preCall( "onLogicServerDelSession" );
		luaStack << mLogicName;
		luaStack << sessionID;
		luaStack.call( );
		NSLog::log( _UTF8( "逻辑服务器[%s] 远程地址[%s]连接丢失" ), mLogicName.getBuffer( ), tpDesc->mPeer.getBuffer( ) );
	}

	void CLogicServer::onProtocol( const CNSString& name, TSessionID sessionID, CNSProtocol* proto )
	{
		switch ( proto->mProtoID )
		{
			// 逻辑服务器脚本协议
			case NSLogicProto::CProtocolScript::PID:
			{
				NSLogicProto::CProtocolScript* script = ( NSLogicProto::CProtocolScript* ) proto;
				NSBase::CNSLuaStack& luaStack = NSBase::CNSLuaStack::getLuaStack( );
				luaStack.preCall( "onLogicServerProtocol" );
				luaStack << mLogicName;
				luaStack << sessionID;
				CNSOctetsStream stream( *script->mpScriptBuffer );
				for ( ; stream.isEnd( ) == false; )
					luaStack << stream;
				luaStack.call( );
				break;
			}
		}
	}

	// 逻辑客户端
	void CLogicClient::onRegisterProtocol( const CNSString& name )
	{
		static NSLogicProto::CProtocolScript scriptProto;
		registerProtocol( &scriptProto );
	}

	void CLogicClient::onAddSession( const CNSString& name, TSessionID sessionID, const CNSSockAddr& local, const CNSSockAddr& peer )
	{
		NSBase::CNSLuaStack& luaStack = NSBase::CNSLuaStack::getLuaStack( );
		luaStack.preCall( "onLogicClientAddSession" );
		luaStack << mLogicName;
		luaStack.call( );
		NSLog::log( _UTF8( "逻辑服务器[%s] 地址[%s]连接成功" ), mLogicName.getBuffer( ), mAddress.getBuffer( ) );
	}

	void CLogicClient::onAddSessionFault( const CNSString& name, int code )
	{
		NSBase::CNSLuaStack& luaStack = NSBase::CNSLuaStack::getLuaStack( );
		luaStack.preCall( "onLogicClientAddSessionFault" );
		luaStack << mLogicName;
		luaStack << code;
		luaStack.call(  );

		NSLog::log( _UTF8( "连接逻辑服务器[%s] 地址[%s]失败, 错误码 - %d" ), mLogicName.getBuffer( ), mAddress.getBuffer( ), code );
		registerClient( this );
	}

	void CLogicClient::onDelSession( const CNSString& name, TSessionID sessionID )
	{
		NSBase::CNSLuaStack& luaStack = NSBase::CNSLuaStack::getLuaStack( );
		luaStack.preCall( "onLogicClientDelSession" );
		luaStack << mLogicName;
		luaStack.call( );
		NSLog::log( _UTF8( "逻辑服务器[%s] 地址[%s]连接丢失" ), mLogicName.getBuffer( ), mAddress.getBuffer( ) );
		registerClient( this );
	}

	void CLogicClient::onProtocol( const CNSString& name, TSessionID sessionID, CNSProtocol* proto )
	{
		switch ( proto->mProtoID )
		{
			// 逻辑服务器脚本协议
			case NSLogicProto::CProtocolScript::PID:
			{
				NSLogicProto::CProtocolScript* script = ( NSLogicProto::CProtocolScript* ) proto;
				NSBase::CNSLuaStack& luaStack = NSBase::CNSLuaStack::getLuaStack( );
				luaStack.preCall( "onLogicClientProtocol" );
				luaStack << mLogicName;
				CNSOctetsStream stream( *script->mpScriptBuffer );
				for ( ; stream.isEnd( ) == false; )
					luaStack << stream;
				luaStack.call( );
				break;
			}
		}
	}

	CLogic::CLogic( ) : mpOperClient( NULL ), mpChargeClient( NULL )
	{
	}

	void CLogic::onRegisterClientProtocol( CNSNetManager* manager )
	{
	}

	void CLogic::onClientProtocol( GateClient::CInnerClient* inner, TSessionID sessionID, CNSProtocol* proto )
	{
		long long nosnUserID = 0;
		if ( inner->mpLogic->getUserID( inner->mName, sessionID, nosnUserID ) == false )
			return;

		switch ( proto->mProtoID )
		{
			// 客户端脚本协议
			case NSClientProto::CProtocolScript::PID:
			{
				NSBase::CNSLuaStack& luaStack = NSBase::CNSLuaStack::getLuaStack( );
				try
				{
					luaStack >> NSBase::EBegin;
					NSClientProto::CProtocolScript* script = ( NSClientProto::CProtocolScript* ) proto;
					luaStack.preCall( "onClientProtocol" );

					luaStack << nosnUserID;
					for ( ; script->mpShadowBuffer->isEnd( ) == false; )
						luaStack << *script->mpShadowBuffer;

					luaStack.call( );
					luaStack >> NSBase::ECommit;
				}
				catch ( CNSException& e )
				{
					luaStack >> NSBase::ERollback;
					NSWin32::CNSBaseApp* app = NSWin32::CNSBaseApp::getApp( );
					NSGateProto::CProtocolUnknownUser unknown( sessionID, 3 );
					NSNet::send( inner->mName, unknown.createStream( ) );
					NSLog::exception( _UTF8( "脚本协议外挂: \n\t%s\nC++调用堆栈:\n%s" ), e.mErrorDesc, NSBase::NSFunction::getStackInfo( ).getBuffer( ) );
				}
				break;
			}
		}
	}

	void CLogic::onConnectGate( const CNSString& name, const CNSString& address )
	{
		const CNSString& logicName = NSWin32::CNSBaseApp::getApp( )->getName( );
		NSGateProto::CProtocolRegisterLogicService registerService( logicName, mServiceID );
		NSNet::send( name, registerService.createStream( ) );
		CInnerLogic::onConnectGate( name, address );
	}

	void CLogic::onClientLogin( const CNSString& name, TSessionID sessionID, long long nosnUserID )
	{
		NSBase::CNSLuaStack& luaStack = NSBase::CNSLuaStack::getLuaStack( );
		luaStack.preCall( "onClientLogin" );
		luaStack << nosnUserID;
		luaStack.call( );
	}

	void CLogic::onClientLost( const CNSString& name, TSessionID sessionID, long long nosnUserID )
	{
		NSBase::CNSLuaStack& luaStack = NSBase::CNSLuaStack::getLuaStack( );
		luaStack.preCall( "onClientLost" );
		luaStack << nosnUserID;
		luaStack.call( );
	}

	void CLogic::onLaunchServer( )
	{
		NSBase::CNSLuaStack& luaStack = NSBase::CNSLuaStack::getLuaStack( );
		luaStack.preCall( "onLaunchServer" );
		luaStack.call( );
	}

	void CLogic::send2OperServer( CNSProtocol* proto )
	{
		send( mpOperClient->getNetworkIO( ), proto->createStream( ) );
	}

	void CLogic::send2ChargeServer( CNSProtocol* proto )
	{
		send( mpChargeClient->getNetworkIO( ), proto->createStream( ) );
	}

	void CLogic::send2LogicServer( const CNSString& logicName, CNSProtocol* proto )
	{
		send( logicName + "_client", proto->createStream( ) );
	}

	void CLogic::send2LogicClient( const CNSString& logicName, TSessionID sessionID, CNSProtocol* proto )
	{
		send( logicName + "_server", proto->createStream( ), sessionID );
	}

	void CLogic::init( NSWin32::CConsoleApp< CLogic >* app )
	{
		TiXmlDocument doc;
		if ( doc.LoadFile( "serverconfig.xml" ) == false )
		{
			static CNSString errorDesc;
			errorDesc.format( _UTF8( "serverconfig.xml打开错误, 错误码: %s" ), doc.ErrorDesc( ) );
			NSException( errorDesc );
		}

		TiXmlElement* baseConfig = doc.FirstChildElement( "baseconfig" );
		if ( baseConfig == NULL )
		{
			static CNSString errorDesc;
			errorDesc.format( _UTF8( "serverconfig.xml文件格式错误, 节点baseconfig没有找到" ) );
			NSException( errorDesc );
		}

		const CNSString& logicName = NSWin32::CNSBaseApp::getApp( )->getName( );
		mWorkPath = baseConfig->Attribute( "workpath" );

		bool find = false;
		TiXmlElement* logic = doc.FirstChildElement( "logicserver" );
		for ( ; logic != NULL; logic = logic->NextSiblingElement( "logicserver" ) )
		{
			CNSString name = logic->Attribute( "name" );
			if ( name == logicName )
			{
				find = true;
				static CNSString title;
#ifdef _M_IX86
				title.format( _UTF8( "逻辑服务器 - %s 语言 - %s 32bit" ), logicName.getBuffer( ), CNSLocal::getNSLocal( ).getLang( ).getBuffer( ) );
#elif _M_X64
				title.format( _UTF8( "逻辑服务器 - %s 语言 - %s 64bit" ), logicName.getBuffer( ), CNSLocal::getNSLocal( ).getLang( ).getBuffer( ) );
#endif
				NSWin32::CConsoleApp< CLogic >::setConsoleTitle( title );

				NSLog::log( _UTF8( "正在启动%s..." ), logicName.getBuffer( ) );
				NSLog::log( _UTF8( "\t当前工作路径 - %s" ), mWorkPath.getBuffer( ) );
				NSLog::log( _UTF8( "\t当前版本号 - %s" ), CNSLocal::getNSLocal( ).getVersion( ).getBuffer( ) );
				NSLog::log( _UTF8( "\t是否开启Lua调试 - %s" ), ( app->isEnableDebug( ) == true ? _UTF8( "是" ) : _UTF8( "否" ) ).getBuffer( ) );

				TiXmlElement* listen = logic->FirstChildElement( "listen" );
				for ( ; listen != NULL; listen = listen->NextSiblingElement( "listen" ) )
				{
					CNSString		logicName = listen->Attribute( "name" );
					CNSString		address = app->name2IPPort( listen->Attribute( "iaddress" ) );
					bool			isHttp = CNSString( listen->Attribute( "ishttp" ) ).toBoolean( );
					unsigned int	bufferSize = CNSString( listen->Attribute( "buffersize" ) ).toInteger( );
					unsigned int	streamSize = CNSString( listen->Attribute( "streamsize" ) ).toInteger( );
					unsigned int	interval = CNSString( listen->Attribute( "interval" ) ).toInteger( );
					if ( isHttp == false )
					{
						CLogicServer* logic = new CLogicServer( logicName, address, interval, bufferSize, streamSize );
						NSNet::registerServer( logic, 5 );

						mLogicServer.pushback( logic );
						NSLog::log( _UTF8( "服务[%s]端口[%s]成功打开, 逻辑服务" ), logicName.getBuffer( ), address.getBuffer( ) );
						NSLog::log( _UTF8( "\tbuffsize - %d" ), bufferSize );
						NSLog::log( _UTF8( "\tstreamsize - %d" ), streamSize );
						NSLog::log( _UTF8( "\tinterval - %d" ), interval );
					}
					else
					{
						CHttpServer* http = new CHttpServer( logicName, address, interval, bufferSize, streamSize );
						NSNet::registerServer( http, 5 );

						mHttpServer.pushback( http );
						NSLog::log( _UTF8( "服务[%s]端口[%s]成功打开, http服务" ), logicName.getBuffer( ), address.getBuffer( ) );
						NSLog::log( _UTF8( "\tbuffsize - %d" ), bufferSize );
						NSLog::log( _UTF8( "\tstreamsize - %d" ), streamSize );
						NSLog::log( _UTF8( "\tinterval - %d" ), interval );
					}
				}

				// 连接数据库
				TiXmlElement* conndb = logic->FirstChildElement( "conndb" );
				for ( ; conndb != NULL; conndb = conndb->NextSiblingElement( "conndb" ) )
				{
					CNSString name = conndb->Attribute( "name" );

					// 读取gamedb连接信息
					TiXmlElement* data = doc.FirstChildElement( "dataserver" );
					for ( ; data != NULL; data = data->NextSiblingElement( "dataserver" ) )
					{
						CNSString	dbName = data->Attribute( "name" );
						if ( name == dbName )
						{
							CNSString		dbAddress = app->name2IPPort( data->Attribute( "iaddress" ) );
							unsigned int	bufferSize = CNSString( conndb->Attribute( "buffersize" ) ).toInteger( );
							unsigned int	streamSize = CNSString( conndb->Attribute( "streamsize" ) ).toInteger( );
							unsigned int	interval = CNSString( conndb->Attribute( "interval" ) ).toInteger( );
							CDBConnInfo conn( dbAddress, interval, bufferSize, streamSize );
							mDBConn.insert( dbName, conn );
							break;
						}
					}
				}

				// 连接网关
				TiXmlElement* connGate = logic->FirstChildElement( "conngate" );
				if ( connGate == NULL )
				{
					static CNSString errorDesc;
					errorDesc.format( _UTF8( "serverconfig.xml文件格式错误, 节点conngate没有找到" ) );
					NSException( errorDesc );
				}

				unsigned int bufferSize = CNSString( connGate->Attribute( "buffersize" ) ).toInteger( );
				unsigned int streamSize = CNSString( connGate->Attribute( "streamsize" ) ).toInteger( );
				unsigned int interval = CNSString( connGate->Attribute( "interval" ) ).toInteger( );
				mServiceID = CNSString( connGate->Attribute( "serviceid" ) ).toInteger( );
				TiXmlElement* gate = doc.FirstChildElement( "gateserver" );
				for ( ; gate != NULL; gate = gate->NextSiblingElement( "gateserver" ) )
				{
					CNSString name = gate->Attribute( "name" );
					TiXmlElement* inner = gate->FirstChildElement( "inner" );
					if ( inner == NULL )
					{
						static CNSString errorDesc;
						errorDesc.format( _UTF8( "serverconfig.xml文件格式错误, gateserver[%s]没有找到inner节点" ),
										  name.getBuffer( ) );
						NSException( errorDesc );
					}

					const CNSString& address = app->name2IPPort( inner->Attribute( "address" ) );
					GateClient::CInnerClient* innerClient = new GateClient::CInnerClient( this, name, address, interval, bufferSize, streamSize );
					mInners.insert( name, innerClient );
					registerClient( innerClient );

					NSLog::log( _UTF8( "正在连接网关服务器[%s] 地址[%s]..." ), innerClient->mName.getBuffer( ), innerClient->mAddress.getBuffer( ) );
					NSLog::log( _UTF8( "\tbuffsize - %d" ), bufferSize );
					NSLog::log( _UTF8( "\tstreamsize - %d" ), streamSize );
					NSLog::log( _UTF8( "\tinterval - %d" ), interval );
				}

				// 连接oper
				TiXmlElement* connOper = logic->FirstChildElement( "connoper" );
				if ( connOper != NULL )
				{
					// 连接operserver
					TiXmlElement* oper = doc.FirstChildElement( "operserver" );
					CNSString address = app->name2IPPort( oper->Attribute( "iaddress" ) );
					size_t bufferSize = CNSString( connOper->Attribute( "buffersize" ) ).toInteger( );
					size_t streamSize = CNSString( connOper->Attribute( "streamsize" ) ).toInteger( );
					size_t interval = CNSString( connOper->Attribute( "interval" ) ).toInteger( );

					mpOperClient = new COperClient( "operclient", address, interval, bufferSize, streamSize );
					registerClient( mpOperClient );

					NSLog::log( _UTF8( "正在连接oper服务器[%s]..." ), address.getBuffer( ) );
					NSLog::log( _UTF8( "\tbuffsize - %d" ), bufferSize );
					NSLog::log( _UTF8( "\tstreamsize - %d" ), streamSize );
					NSLog::log( _UTF8( "\tinterval - %d" ), interval );
				}

				// 连接charge
				TiXmlElement* connCharge = logic->FirstChildElement( "conncharge" );
				if ( connCharge != NULL )
				{
					// 连接chargeserver
					TiXmlElement* charge = doc.FirstChildElement( "chargeserver" );
					CNSString address = app->name2IPPort( charge->Attribute( "iaddress" ) );
					size_t bufferSize = CNSString( connCharge->Attribute( "buffersize" ) ).toInteger( );
					size_t streamSize = CNSString( connCharge->Attribute( "streamsize" ) ).toInteger( );
					size_t interval = CNSString( connCharge->Attribute( "interval" ) ).toInteger( );

					mpChargeClient = new CChargeClient( "chargeclient", address, interval, bufferSize, streamSize );
					registerClient( mpChargeClient );

					NSLog::log( _UTF8( "正在连接charge服务器[%s]..." ), address.getBuffer( ) );
					NSLog::log( _UTF8( "\tbuffsize - %d" ), bufferSize );
					NSLog::log( _UTF8( "\tstreamsize - %d" ), streamSize );
					NSLog::log( _UTF8( "\tinterval - %d" ), interval );
				}

				// 连接逻辑服务器
				TiXmlElement* conn = logic->FirstChildElement( "conn" );
				for ( ; conn != NULL; conn = conn->NextSiblingElement( "conn" ) )
				{
					CNSString name = conn->Attribute( "name" );
					// 读取网关信息
					CNSVector< CNSString > nameSet;
					name.split( ".", nameSet );
					if ( nameSet.getCount( ) != 2 )
					{
						static CNSString errorDesc;
						errorDesc.format( _UTF8( "serverconfig.xml文件格式错误, logicserver[%s] conn节点[%s], name属性格式不正确" ),
										  logicName.getBuffer( ), name.getBuffer( ) );
						NSException( errorDesc );
					}

					const CNSString& serverName = nameSet[ 0 ];
					const CNSString& logicName = nameSet[ 1 ];
					CNSString address;
					TiXmlElement* logic = doc.FirstChildElement( "logicserver" );
					for ( ; logic != NULL; logic = logic->NextSiblingElement( "logicserver" ) )
					{
						CNSString serverIter = logic->Attribute( "name" );
						if ( serverIter == serverName )
						{
							TiXmlElement* listen = logic->FirstChildElement( "listen" );
							for ( ; listen != NULL; listen = listen->NextSiblingElement( "listen" ) )
							{
								CNSString logicIter = listen->Attribute( "name" );
								if ( logicIter == logicName )
								{
									address = app->name2IPPort( listen->Attribute( "iaddress" ) );
									break;
								}
							}
						}
					}

					unsigned int bufferSize = CNSString( conn->Attribute( "buffersize" ) ).toInteger( );
					unsigned int streamSize = CNSString( conn->Attribute( "streamsize" ) ).toInteger( );
					unsigned int interval = CNSString( conn->Attribute( "interval" ) ).toInteger( );
					CLogicClient* logicClient = new CLogicClient( logicName, address, interval, bufferSize, streamSize );
					registerClient( logicClient );

					mLogicClient.pushback( logicClient );
					NSLog::log( _UTF8( "正在连接逻辑服务器[%s] 地址[%s]..." ), logicName.getBuffer( ), address.getBuffer( ) );
					NSLog::log( _UTF8( "\tbuffsize - %d" ), bufferSize );
					NSLog::log( _UTF8( "\tstreamsize - %d" ), streamSize );
					NSLog::log( _UTF8( "\tinterval - %d" ), interval );
				}
				break;
			}
		}

		// 连接数据库
		HLISTINDEX beginIndex = mDBConn.getHead( );
		for ( ; beginIndex != NULL; mDBConn.getNext( beginIndex ) )
		{
			const CNSString&	dbName = mDBConn.getKey( beginIndex );
			const CDBConnInfo&	dbConn = mDBConn.getValue( beginIndex );
			NSMysql::createMysql( dbName, dbConn.mAddress, dbConn.mInterval, dbConn.mBufferSize, dbConn.mStreamSize );
			NSLog::log( _UTF8( "\tbuffsize - %d" ), dbConn.mBufferSize );
			NSLog::log( _UTF8( "\tstreamsize - %d" ), dbConn.mStreamSize );
			NSLog::log( _UTF8( "\tinterval - %d" ), dbConn.mInterval );
		}

		if ( find == false )
		{
			static CNSString errorDesc;
			errorDesc.format( _UTF8( "逻辑服务器[%s]沒有找到" ), logicName.getBuffer( ) );
			NSException( errorDesc );
		}

		CNSVector< CNSString > modList;
		TiXmlElement* module = logic->FirstChildElement( "module" );
		for ( ; module != NULL; module = module->NextSiblingElement( "module" ) )
		{
			CNSString modName = module->Attribute( "name" );
			modList.pushback( modName );
		}

		// 注册脚本函数
		LogicServer::regLuaLib( );
		NSBase::CNSLuaStack::getLuaStack( ).load( mWorkPath + "script/" + logicName, modList );
		onLaunchServer( );
	}

	void CLogic::exit( )
	{
		CInnerLogic::exit( );
		for ( size_t i = 0; i < mLogicServer.getCount( ); i++ )
			delete mLogicServer[ i ];

		for ( size_t i = 0; i < mHttpServer.getCount( ); i++ )
			delete mHttpServer[ i ];

		for ( size_t i = 0; i < mLogicClient.getCount( ); i++ )
			delete mLogicClient[ i ];

		delete mpOperClient;
		delete mpChargeClient;
	}
}

int main( int argc, char* argv[] )
{
	try
	{
		NSWin32::CConsoleApp< LogicServer::CLogic > app;
		app.useNSHttp( true );
		app.useNSHttpDebugger( true );
		app.useNSMysql( true );
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
