#include <nsbase.h>
#include <protocol.h>
#include "gateclient.h"
#include "server.h"
#include "interface.h"

namespace OperServer
{
	void COperServer::onRegisterProtocol( const CNSString& name )
	{
		static NSOperProto::CProtocolScript scriptProto;
		registerProtocol( &scriptProto );
	}

	void COperServer::onAddSession( const CNSString& name, TSessionID sessionID, const CNSSockAddr& local, const CNSSockAddr& peer )
	{
		CNSSessionDesc* desc = NSNet::getSessionDesc( name, sessionID );
		if ( desc == NULL )
			return;

		COperLogic* logic = NSWin32::CConsoleApp< COperLogic >::getLogic( );
		logic->onOperServerAddSession( sessionID, desc );
	}

	void COperServer::onDelSession( const CNSString& name, TSessionID sessionID )
	{
		CNSSessionDesc* desc = NSNet::getSessionDesc( name, sessionID );
		if ( desc == NULL )
			return;

		COperLogic* logic = NSWin32::CConsoleApp< COperLogic >::getLogic( );
		logic->onOperServerDelSession( sessionID, desc );
	}

	void COperServer::onProtocol( const CNSString& name, TSessionID sessionID, CNSProtocol* proto )
	{
		switch ( proto->mProtoID )
		{
			case NSOperProto::CProtocolScript::PID:
			{
				NSOperProto::CProtocolScript*	script = ( NSOperProto::CProtocolScript* ) proto;
				COperLogic* logic = NSWin32::CConsoleApp< COperLogic >::getLogic( );
				logic->onOperProtocol( sessionID, script );
				break;
			}
		}
	}

	COperLogic::COperLogic( ) : mpOperServer( NULL )
	{
	}

	void COperLogic::reloadServerList( )
	{
		NSWin32::CNSBaseApp* app = NSWin32::CNSBaseApp::getApp( );
		app->loadIPName( );
		loadServerList( );
	}

	void COperLogic::loadServerList( )
	{
		mGroupIndex.clear( );
		mRegions.clear( );
		//TiXmlDocument tDoc;
		//if ( tDoc.LoadFile( "serverlist.xml" ) == false )
		//{
		//	static CNSString errorDesc;
		//	errorDesc.format( _UTF8( "serverlist.xml打开错误, 错误码: %s" ), tDoc.ErrorDesc( ) );
		//	NSException( errorDesc );
		//}

		//TiXmlElement* tpRegion = tDoc.FirstChildElement( "Region" );
		//if ( tpRegion == NULL )
		//{
		//	static CNSString errorDesc;
		//	errorDesc.format( _UTF8( "serverlist.xml文件格式错误, 节点Region没有找到" ) );
		//	NSException( errorDesc );
		//}

		//for ( ; tpRegion != NULL; tpRegion = tpRegion->NextSiblingElement( "Region" ) )
		//{
		//	CNSString regionName	= tpRegion->Attribute( "Name" );
		//	CNSString regionTag		= tpRegion->Attribute( "Tag" );
		//	CRegionInfo region;
		//	region.mName	= regionName;
		//	region.mTag		= regionTag;
		//	TiXmlElement* tpGroup = tpRegion->FirstChildElement( "Group" );
		//	if ( tpGroup == NULL )
		//	{
		//		static CNSString errorDesc;
		//		errorDesc.format( _UTF8( "serverlist.xml文件格式错误, 节点Group没有找到" ) );
		//		NSException( errorDesc );
		//		return;
		//	}
		//	for ( ; tpGroup != NULL; tpGroup = tpGroup->NextSiblingElement( "Group" ) )
		//	{
		//		CNSString groupName		= tpGroup->Attribute( "Name" );
		//		CNSString groupTag		= tpGroup->Attribute( "Tag" );
		//		CNSString groupID		= tpGroup->Attribute( "GroupID" );
		//		CNSString operAddress	= tpGroup->Attribute( "OperAddress" );
		//		CNSString oAddress		= tpGroup->Attribute( "OAddress" );
		//		CNSString channel		= tpGroup->Attribute( "Channel" );
		//		CNSString preview		= tpGroup->Attribute( "Preview" );

		//		setGroupID( groupID.toInteger( ) );
		//		CGroupInfo group;
		//		group.mName				= groupName;
		//		group.mGroupID			= groupID.toInteger( );
		//		group.mOperAddress		= Machine2IPPort( operAddress );
		//		group.mOAddress			= Machine2IPAddress( oAddress );
		//		group.mTag				= groupTag;
		//		group.mChannel			= channel;
		//		group.mPreview			= preview;
		//		region.mGroups.pushback( group );
		//	}

		//	mRegions.pushback( region );
		//}

		//HLISTINDEX beginIndex = mRegions.getHead( );
		//for ( ; beginIndex != NULL; mRegions.getNext( beginIndex ) )
		//{
		//	CRegionInfo region = mRegions.getData( beginIndex );
		//	HLISTINDEX groupIndex = region.mGroups.getHead( );
		//	for ( ; groupIndex != NULL; region.mGroups.getNext( groupIndex ) )
		//	{
		//		CGroupInfo group = region.mGroups.getData( groupIndex );
		//		mGroupIndex.insert( group.mGroupID, group );
		//	}
		//}
	}

	void COperLogic::onLaunchServer( )
	{
		NSBase::CNSLuaStack& luaStack = NSBase::CNSLuaStack::getLuaStack( );
		luaStack.preCall( "onLaunchServer" );
		luaStack.call( );
	}

	void COperLogic::onOperServerAddSession( TSessionID sessionID, CNSSessionDesc* desc )
	{
		NSBase::CNSLuaStack& luaStack = NSBase::CNSLuaStack::getLuaStack( );
		luaStack.preCall( "onOperServerAddSession" );
		luaStack << sessionID;
		luaStack.call( );
		NSLog::log( _UTF8( "oper逻辑服务器 远程地址[%s]连接成功" ), desc->mPeer.getBuffer( ) );
	}

	void COperLogic::onOperServerDelSession( TSessionID sessionID, CNSSessionDesc* desc )
	{
		NSBase::CNSLuaStack& luaStack = NSBase::CNSLuaStack::getLuaStack( );
		luaStack.preCall( "onOperServerDelSession" );
		luaStack << sessionID;
		luaStack.call( );
		NSLog::log( _UTF8( "oper逻辑服务器 远程地址[%s]连接丢失" ), desc->mPeer.getBuffer( ) );
		return;
	}

	void COperLogic::onOperProtocol( TSessionID sessionID, NSOperProto::CProtocolScript* proto )
	{
		NSBase::CNSLuaStack& luaStack = NSBase::CNSLuaStack::getLuaStack( );
		luaStack.preCall( "onOperProtocol" );
		luaStack << sessionID;
		CNSOctetsStream stream( *proto->mpScriptBuffer );
		for ( ;stream.isEnd( ) == false; )
			luaStack << stream;
		luaStack.call( );
	}

	void COperLogic::onConnectGate( const CNSString& name, const CNSString& address )
	{
		NSGateProto::CProtocolRegisterOperService operService;
		NSNet::send( name, operService.createStream( ) );
		CInnerLogic::onConnectGate( name, address );
	}

	void COperLogic::onRegisterClientProtocol( CNSNetManager* pManager )
	{
		static NSClientProto::CProtocolLogin loginProto;
		NSGateProto::CProtocolTunnel::registerProtocol( &loginProto );
	}

	void COperLogic::onClientLogin( const CNSString& name, TSessionID sessionID, long long nosnUserID )
	{
		NSBase::CNSLuaStack& luaStack = NSBase::CNSLuaStack::getLuaStack( );
		luaStack.preCall( "onClientLogin" );
		luaStack << nosnUserID;
		luaStack.call( );
	}

	void COperLogic::onClientLost( const CNSString& name, TSessionID sessionID, long long nosnUserID )
	{
		NSBase::CNSLuaStack& luaStack = NSBase::CNSLuaStack::getLuaStack( );
		luaStack.preCall( "onClientLost" );
		luaStack << nosnUserID;
		luaStack.call( );
	}

	void COperLogic::onClientProtocol( GateClient::CInnerClient* inner, TSessionID sessionID, CNSProtocol* proto )
	{
		// 客户端登陆协议
		if ( proto->mProtoID == NSClientProto::CProtocolLogin::PID )
		{
			NSClientProto::CProtocolLogin* login = ( NSClientProto::CProtocolLogin* ) proto;
			NSBase::CNSLuaStack& luaStack = NSBase::CNSLuaStack::getLuaStack( );
			luaStack.preCall( "onClientVerify" );
			luaStack << inner->mName;
			luaStack << sessionID;
			luaStack << login->mAuthName;
			luaStack << login->mAuthUserID;
			luaStack << login->mToken;
			luaStack.call( );
			return;
		}

		long long nosnUserID = 0;
		if ( inner->mpLogic->getUserID( inner->mName, sessionID, nosnUserID ) == false )
			return;

		switch ( proto->mProtoID )
		{
			// 客户端脚本协议
			case NSClientProto::CProtocolScript::PID:
			{
				try
				{
					NSClientProto::CProtocolScript* script = ( NSClientProto::CProtocolScript* ) proto;
					NSBase::CNSLuaStack& luaStack = NSBase::CNSLuaStack::getLuaStack( );
					luaStack.preCall( "onClientProtocol" );
					luaStack << nosnUserID;
					for ( ; script->mpShadowBuffer->isEnd( ) == false; )
						luaStack << *script->mpShadowBuffer;
					luaStack.call( );
				}
				catch ( CNSException& e )
				{
					NSWin32::CNSBaseApp* app = NSWin32::CNSBaseApp::getApp( );
					NSGateProto::CProtocolUnknownUser unknown( sessionID, 3 );
					NSNet::send( inner->mName, unknown.createStream( ) );
					NSLog::exception( _UTF8( "脚本协议外挂: \n\t%s\nC++调用堆栈:\n%s" ), e.mErrorDesc, NSBase::NSFunction::getStackInfo( ).getBuffer( ) );
				}
				break;
			}
		}
	}

	void COperLogic::luaPushServerList( CNSLuaStack& luaStack )
	{
		luaStack << mRegions;
	}

	void COperLogic::init( NSWin32::CConsoleApp< COperLogic >* app )
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

		// 加载lua脚本
		CNSString workPath = baseConfig->Attribute( "workpath" );

		// 加载服务器列表
		loadServerList( );

		static CNSString title;
#ifdef _M_IX86
		title.format( _UTF8( "oper服务器 语言 - %s 32bit" ), CNSLocal::getNSLocal( ).getLang( ).getBuffer( ) );
#elif _M_X64
		title.format( _UTF8( "oper服务器 语言 - %s 64bit" ), CNSLocal::getNSLocal( ).getLang( ).getBuffer( ) );
#endif

		app->setConsoleTitle( title );
		NSLog::log( _UTF8( "正在启动oper服务器..." ) );
		NSLog::log( _UTF8( "\t当前工作路径 - %s" ), workPath.getBuffer( ) );
		NSLog::log( _UTF8( "\t当前版本号 - %s" ), CNSLocal::getNSLocal( ).getVersion( ).getBuffer( ) );
		NSLog::log( _UTF8( "\t是否开启Lua调试 - %s" ), ( app->isEnableDebug( ) == true ? _UTF8( "是" ) : _UTF8( "否" ) ).getBuffer( ) );

		TiXmlElement* operServer = doc.FirstChildElement( "operserver" );
		if ( operServer == NULL )
		{
			static CNSString errorDesc;
			errorDesc.format( _UTF8( "serverconfig.xml文件格式错误, 节点operserver没有找到" ) );
			NSException( errorDesc );
		}

		// 开始oper内部服务
		CNSString		operAddress = app->name2IPPort( operServer->Attribute( "iaddress" ) );
		unsigned int	interval = CNSString( operServer->Attribute( "interval" ) ).toInteger( );
		unsigned int	bufferSize = CNSString( operServer->Attribute( "buffersize" ) ).toInteger( );
		unsigned int	streamSize = CNSString( operServer->Attribute( "streamsize" ) ).toInteger( );

		mpOperServer = new COperServer( "operserver", operAddress, interval, bufferSize, streamSize );
		registerServer( mpOperServer, 5 );
		NSLog::log( _UTF8( "运营内部服务器端口[%s]成功打开" ), operAddress.getBuffer( ) );
		NSLog::log( _UTF8( "\tbuffsize - %d" ), bufferSize );
		NSLog::log( _UTF8( "\tstreamsize - %d" ), streamSize );
		NSLog::log( _UTF8( "\tinterval - %d" ), interval );

		// 连接数据库
		CNSString operDBAddress;
		CNSString uniDBAddress;
		TiXmlElement* database = doc.FirstChildElement( "dataserver" );
		if ( database == NULL )
		{
			static CNSString errorDesc;
			errorDesc.format( _UTF8( "serverconfig.xml文件格式错误, 节点OperDB没有找到" ) );
			NSException( errorDesc );
		}

		for ( ; database != NULL; database = database->NextSiblingElement( "dataserver" ) )
		{
			CNSString name = database->Attribute( "name" );
			if ( name == "operdb" )
				operDBAddress = app->name2IPPort( database->Attribute( "iaddress" ) );
			else if ( name == "unidb" )
				uniDBAddress = app->name2IPPort( database->Attribute( "iaddress" ) );
		}

		if ( operDBAddress.isEmpty( ) == true )
		{
			static CNSString errorDesc;
			errorDesc.format( _UTF8( "serverconfig.xml文件格式错误, 没有为oper服务器配置数据库" ) );
			NSException( errorDesc );
		}

		if ( uniDBAddress.isEmpty( ) == true )
		{
			static CNSString errorDesc;
			errorDesc.format( _UTF8( "serverconfig.xml文件格式错误, 没有为oper服务器配置唯一命数据库" ) );
			NSException( errorDesc );
		}

		// 连接operDB
		TiXmlElement* operdb = operServer->FirstChildElement( "operdb" );
		if ( operdb == NULL )
		{
			static CNSString errorDesc;
			errorDesc.format( _UTF8( "serverconfig.xml文件格式错误, 节点operdb没有找到" ) );
			NSException( errorDesc );
		}
		interval = CNSString( operdb->Attribute( "interval" ) ).toInteger( );
		bufferSize = CNSString( operdb->Attribute( "buffersize" ) ).toInteger( );
		streamSize = CNSString( operdb->Attribute( "streamsize" ) ).toInteger( );
		mServiceID = 0;
		NSMysql::createMysql( "operdb", operDBAddress, interval, bufferSize, streamSize );
		NSLog::log( _UTF8( "\tbuffsize - %d" ), bufferSize );
		NSLog::log( _UTF8( "\tstreamsize - %d" ), streamSize );
		NSLog::log( _UTF8( "\tinterval - %d" ), interval );

		// 连接uniDB
		TiXmlElement* unidb = operServer->FirstChildElement( "unidb" );
		if ( unidb == NULL )
		{
			static CNSString errorDesc;
			errorDesc.format( _UTF8( "serverconfig.xml文件格式错误, 节点operdb没有找到" ) );
			NSException( errorDesc );
		}
		interval = CNSString( unidb->Attribute( "interval" ) ).toInteger( );
		bufferSize = CNSString( unidb->Attribute( "buffersize" ) ).toInteger( );
		streamSize = CNSString( unidb->Attribute( "streamsize" ) ).toInteger( );

		NSMysql::createMysql( "unidb", uniDBAddress, interval, bufferSize, streamSize );
		NSLog::log( _UTF8( "\tbuffsize - %d" ), bufferSize );
		NSLog::log( _UTF8( "\tstreamsize - %d" ), streamSize );
		NSLog::log( _UTF8( "\tinterval - %d" ), interval );

		// 连接网关
		TiXmlElement* connGate = operServer->FirstChildElement( "conngate" );
		if ( connGate == NULL )
		{
			static CNSString errorDesc;
			errorDesc.format( _UTF8( "serverconfig.xml文件格式错误, 节点conngate没有找到" ) );
			NSException( errorDesc );
		}

		bufferSize = CNSString( connGate->Attribute( "buffersize" ) ).toInteger( );
		streamSize = CNSString( connGate->Attribute( "streamsize" ) ).toInteger( );
		interval = CNSString( connGate->Attribute( "interval" ) ).toInteger( );
		TiXmlElement* gate = doc.FirstChildElement( "gateserver" );
		for ( ; gate != NULL; gate = gate->NextSiblingElement( "gateserver" ) )
		{
			CNSString name = gate->Attribute( "name" );
			TiXmlElement* inner = gate->FirstChildElement( "inner" );
			if ( inner == NULL )
			{
				static CNSString errorDesc;
				errorDesc.format( _UTF8( "serverconfig.xml文件格式错误, gateserver[%s]没有找到inner节点" ), name.getBuffer( ) );
				NSException( errorDesc );
			}

			const CNSString& address = app->name2IPPort( inner->Attribute( "address" ) );
			GateClient::CInnerClient* innerClient = new GateClient::CInnerClient( this, name, address, interval, bufferSize, streamSize );
			mInners.insert( name, innerClient );

			registerClient( innerClient );
			NSLog::log( _UTF8( "正在连接网关服务器[%s] 地址[%s]..." ), innerClient->mName.getBuffer( ), innerClient->mAddress.getBuffer( ) );
			NSLog::log( _UTF8( "\tbuffsize - %d" ), innerClient->mBufferSize );
			NSLog::log( _UTF8( "\tstreamsize - %d" ), innerClient->mStreamSize );
			NSLog::log( _UTF8( "\tinterval - %d" ), innerClient->mPollInterval );
		}

		CNSVector< CNSString > modList;
		TiXmlElement* module = operServer->FirstChildElement( "module" );
		for ( ; module != NULL; module = module->NextSiblingElement( "module" ) )
		{
			CNSString modName = module->Attribute( "name" );
			modList.pushback( modName );
		}

		// 注册脚本函数
		OperServer::regLuaLib( );
		NSBase::CNSLuaStack::getLuaStack( ).load( workPath + "script/oper", modList );
		onLaunchServer( );
	}

	void COperLogic::exit( )
	{
		delete mpOperServer;
	}

	void COperLogic::send2OperClient( unsigned int sessionID, CNSProtocol* proto )
	{
		NSNet::send( mpOperServer->getNetworkIO( ), proto->createStream( ), sessionID );
	}

	void COperLogic::verifyResult( const CNSString& gateName, TSessionID sessionID, long long nosnUserID, NSClientProto::CProtocolLoginResult::ELoginResult loginResult )
	{
		if ( loginResult == NSClientProto::CProtocolLoginResult::LOGIN_SUCCESS )
		{
			// 通知网关玩家登录成功
			NSGateProto::CProtocolClientLogin userLogin( sessionID, nosnUserID );
			NSNet::send( gateName, userLogin.createStream( ) );
		}

		NSClientProto::CProtocolLoginResult result( loginResult );
		CInnerLogic::send2User( gateName, sessionID, &result );
	}
}

int main( int argc, char* argv[] )
{
	try
	{
		NSWin32::CConsoleApp< OperServer::COperLogic > app;
		app.useNSMysql( true );
		app.useNSHttp( true );
		app.useNSHttpDebugger( true );
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
