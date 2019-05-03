#include <nsbase.h>
#include <protocol.h>
#include "server.h"
#include "interface.h"

namespace ChargeServer
{
	void CChargeServer::onRegisterProtocol( const CNSString& name )
	{
		static NSChargeProto::CProtocolScript scriptProto;
		registerProtocol( &scriptProto );
	}

	void CChargeServer::onAddSession( const CNSString& name, TSessionID sessionID, const CNSSockAddr& local, const CNSSockAddr& peer )
	{
		CNSSessionDesc* desc = NSNet::getSessionDesc( name, sessionID );
		if ( desc == NULL )
			return;

		CChargeLogic* logic = NSWin32::CConsoleApp< CChargeLogic >::getLogic( );
		logic->onChargeServerAddSession( sessionID, desc );
	}

	void CChargeServer::onDelSession( const CNSString& name, TSessionID sessionID )
	{
		CNSSessionDesc* desc = NSNet::getSessionDesc( name, sessionID );
		if ( desc == NULL )
			return;

		CChargeLogic* logic = NSWin32::CConsoleApp< CChargeLogic >::getLogic( );
		logic->onChargeServerDelSession( sessionID, desc );
	}

	void CChargeServer::onProtocol( const CNSString& name, TSessionID sessionID, CNSProtocol* proto )
	{
		switch ( proto->mProtoID )
		{
			case NSChargeProto::CProtocolScript::PID:
			{
				NSChargeProto::CProtocolScript*	script = ( NSChargeProto::CProtocolScript* ) proto;
				CChargeLogic* logic = NSWin32::CConsoleApp< CChargeLogic >::getLogic( );
				logic->onChargeProtocol( sessionID, script );
				break;
			}
		}
	}

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

	CChargeLogic::CChargeLogic( )
	{
	}

	void CChargeLogic::onLaunchServer( )
	{
		NSBase::CNSLuaStack& luaStack = NSBase::CNSLuaStack::getLuaStack( );
		luaStack.preCall( "onLaunchServer" );
		luaStack.call( );
	}

	void CChargeLogic::onChargeServerAddSession( TSessionID sessionID, CNSSessionDesc* desc )
	{
		NSBase::CNSLuaStack& luaStack = NSBase::CNSLuaStack::getLuaStack( );
		luaStack.preCall( "onChargeServerAddSession" );
		luaStack << sessionID;
		luaStack.call( );
		NSLog::log( _UTF8( "charge逻辑服务器 远程地址[%s]连接成功" ), desc->mPeer.getBuffer( ) );
	}

	void CChargeLogic::onChargeServerDelSession( TSessionID sessionID, CNSSessionDesc* desc )
	{
		NSBase::CNSLuaStack& luaStack = NSBase::CNSLuaStack::getLuaStack( );
		luaStack.preCall( "onChargeServerDelSession" );
		luaStack << sessionID;
		luaStack.call( );
		NSLog::log( _UTF8( "charge逻辑服务器 远程地址[%s]连接丢失" ), desc->mPeer.getBuffer( ) );
	}

	void CChargeLogic::onChargeProtocol( TSessionID sessionID, NSChargeProto::CProtocolScript* proto )
	{
		NSBase::CNSLuaStack& luaStack = NSBase::CNSLuaStack::getLuaStack( );
		luaStack.preCall( "onChargeProtocol" );
		luaStack << sessionID;
		CNSOctetsStream stream( *proto->mpScriptBuffer );
		for ( ; stream.isEnd( ) == false; )
			luaStack << stream;
		luaStack.call( );
	}

	void CChargeLogic::reloadServerList( )
	{
		NSWin32::CNSBaseApp::getApp( )->loadIPName( );
		loadServerList( );
	}

	void CChargeLogic::luaPushServerList( CNSLuaStack& luaStack )
	{
		luaStack << mRegions;
	}

	void CChargeLogic::send2ChargeClient( TSessionID sessionID, CNSProtocol* proto )
	{
		NSNet::send( "charge", proto->createStream( ), sessionID );
	}

	void CChargeLogic::loadServerList( )
	{
		mRegions.clear( );
		//TiXmlDocument tDoc;
		//if ( tDoc.LoadFile( "serverlist.xml" ) == false )
		//{
		//	NSLog::log( _UTF8( "serverlist.xml打开错误, 错误码: %s" ), tDoc.ErrorDesc( ) );
		//	return;
		//}

		//TiXmlElement* region = tDoc.FirstChildElement( "Region" );
		//if (region == NULL )
		//{
		//	NSLog::log( _UTF8( "serverlist.xml文件格式错误, 节点Region没有找到" ) );
		//	return;
		//}

		//for ( ; region != NULL; region = region->NextSiblingElement( "Region" ) )
		//{
		//	CNSString regionName	= region->Attribute( "Name" );
		//	CNSString regionTag		= region->Attribute( "Tag" );
		//	CRegionInfo regionInfo;
		//	regionInfo.mName	= regionName;
		//	regionInfo.mTag		= regionTag;
		//	TiXmlElement* group = region->FirstChildElement( "Group" );
		//	if ( group == NULL )
		//	{
		//		NSLog::log( _UTF8( "serverlist.xml文件格式错误, 节点Group没有找到" ) );
		//		return;
		//	}
		//	for ( ; group != NULL; group = group->NextSiblingElement( "Group" ) )
		//	{
		//		CNSString groupName		= group->Attribute( "Name" );
		//		CNSString groupTag		= group->Attribute( "Tag" );
		//		CNSString groupID		= group->Attribute( "GroupID" );
		//		setGroupID( groupID );
		//		CNSString operAddress	= Machine2IPPort( group->Attribute( "OperAddress" ) );
		//		CNSString oAddress		= Machine2IPAddress( group->Attribute( "OAddress" ) );
		//		CGroupInfo groupInfo;
		//		groupInfo.mName			= groupName;
		//		groupInfo.mGroupID		= groupID.toInteger( );
		//		groupInfo.mOperAddress	= operAddress;
		//		groupInfo.mOAddress		= oAddress;
		//		groupInfo.mTag			= groupTag;
		//		regionInfo.mGroups.pushback( groupInfo );
		//	}

		//	gRegions.pushback( regionInfo );
		//}
	}

	void CChargeLogic::init( NSWin32::CConsoleApp< CChargeLogic >* app )
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
			NSException( _UTF8( "serverconfig.xml文件格式错误, 节点baseconfig没有找到" ) );

		// 加载lua脚本
		mWorkPath = baseConfig->Attribute( "workpath" );

		// 加载服务器列表
		loadServerList( );

		static CNSString title;
#ifdef _M_IX86
		title.format( _UTF8( "charge服务器 语言 - %s 32bit" ), CNSLocal::getNSLocal( ).getLang( ).getBuffer( ) );
#elif _M_X64
		title.format( _UTF8( "charge服务器 语言 - %s 64bit" ), CNSLocal::getNSLocal( ).getLang( ).getBuffer( ) );
#endif

		app->setConsoleTitle( title );
		NSLog::log( _UTF8( "正在启动charge服务器..." ) );
		NSLog::log( _UTF8( "\t当前工作路径 - %s" ), mWorkPath.getBuffer( ) );
		NSLog::log( _UTF8( "\t当前版本号 - %s" ), CNSLocal::getNSLocal( ).getVersion( ).getBuffer( ) );
		NSLog::log( _UTF8( "\t是否开启Lua调试 - %s" ), ( app->isEnableDebug( ) == true ? _UTF8( "是" ) : _UTF8( "否" ) ).getBuffer( ) );

		// 开启charge内部服务器
		TiXmlElement* chargeServer = doc.FirstChildElement( "chargeserver" );
		if ( chargeServer == NULL )
			NSException( _UTF8( "serverconfig.xml文件格式错误, 节点chargeserver没有找到" ) );

		CNSString		address = app->name2IPPort( chargeServer->Attribute( "iaddress" ) );
		unsigned int	interval = CNSString( chargeServer->Attribute( "interval" ) ).toInteger( );
		unsigned int	bufferSize = CNSString( chargeServer->Attribute( "buffersize" ) ).toInteger( );
		unsigned int	streamSize = CNSString( chargeServer->Attribute( "streamsize" ) ).toInteger( );
		CChargeServer*	charge = new CChargeServer( "charge", address, interval, bufferSize, streamSize );
		registerServer( charge, 1 );
		NSLog::log( _UTF8( "充值逻辑服务器端口[%s]成功打开" ), address.getBuffer( ) );
		NSLog::log( _UTF8( "\tbuffsize - %d" ), bufferSize );
		NSLog::log( _UTF8( "\tstreamsize - %d" ), streamSize );
		NSLog::log( _UTF8( "\tinterval - %d" ), interval );

		// 连接数据库
		CNSString operDBAddress;
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
			{
				operDBAddress = app->name2IPPort( database->Attribute( "iaddress" ) );
				break;
			}
		}

		if ( operDBAddress.isEmpty( ) == true )
		{
			static CNSString errorDesc;
			errorDesc.format( _UTF8( "serverconfig.xml文件格式错误, 没有为oper服务器配置数据库" ) );
			NSException( errorDesc );
		}

		TiXmlElement* operdb = chargeServer->FirstChildElement( "operdb" );
		if ( operdb == NULL )
		{
			static CNSString errorDesc;
			errorDesc.format( _UTF8( "serverconfig.xml文件格式错误, 节点operdb没有找到" ) );
			NSException( errorDesc );
		}

		interval = CNSString( operdb->Attribute( "interval" ) ).toInteger( );
		bufferSize = CNSString( operdb->Attribute( "buffersize" ) ).toInteger( );
		streamSize = CNSString( operdb->Attribute( "streamsize" ) ).toInteger( );

		NSMysql::createMysql( "operdb", operDBAddress, interval, bufferSize, streamSize );
		NSLog::log( _UTF8( "\tbuffsize - %d" ), bufferSize );
		NSLog::log( _UTF8( "\tstreamsize - %d" ), streamSize );
		NSLog::log( _UTF8( "\tinterval - %d" ), interval );

		// 开启充值监听端口
		TiXmlElement* listen = chargeServer->FirstChildElement( "listen" );
		for ( ; listen != NULL; listen = listen->NextSiblingElement( "listen" ) )
		{
			CNSString address = app->name2IPPort( listen->Attribute( "oaddress" ) );
			CNSString name = listen->Attribute( "name" );
			interval = CNSString( listen->Attribute( "interval" ) ).toInteger( );
			bufferSize = CNSString( listen->Attribute( "buffersize" ) ).toInteger( );
			streamSize = CNSString( listen->Attribute( "streamsize" ) ).toInteger( );

			CHttpServer* chargeHttp = new CHttpServer( name, address, interval, bufferSize, streamSize );
			registerServer( chargeHttp, 128 );
			mHttpServer.pushback( chargeHttp );
			NSLog::log( _UTF8( "付费[%s] 端口[%s]成功打开" ), name.getBuffer( ), address.getBuffer( ) );
			NSLog::log( _UTF8( "\tbuffsize - %d" ), bufferSize );
			NSLog::log( _UTF8( "\tstreamsize - %d" ), streamSize );
			NSLog::log( _UTF8( "\tinterval - %d" ), interval );
		}

		CNSVector< CNSString > modList;
		TiXmlElement* module = chargeServer->FirstChildElement( "module" );
		for ( ; module != NULL; module = module->NextSiblingElement( "module" ) )
		{
			CNSString modName = module->Attribute( "name" );
			modList.pushback( modName );
		}

		// 注册脚本函数
		ChargeServer::regLuaLib( );
		NSBase::CNSLuaStack::getLuaStack( ).load( mWorkPath + "script/charge", modList );
		onLaunchServer( );
	}

	void CChargeLogic::exit( )
	{
		for ( size_t i = 0; i < mHttpServer.getCount( ); i ++ )
			delete mHttpServer[ i ];

		delete mpChargeServer;
	}
}

int main( int argc, char* argv[] )
{
	try
	{
		NSWin32::CConsoleApp< ChargeServer::CChargeLogic > app;
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
