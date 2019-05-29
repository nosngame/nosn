#include "precomplie.h"

namespace NSClient
{
	CNSVector< CNSClient* > CNSClient::sNSClient;
	CNSString CNSClient::sAuthName;
	NSClient::CNSPlugin* app = NULL;
	FTouchProc gTouchProc = NULL;
	FGoLoadProc gGoLoadProc = NULL;
	FGoDestroyProc gGoDestroyProc = NULL;
	FHostErrorProc gHostErrorProc = NULL;
	FGoGetValue gGoGetValue = NULL;
	FGoSetValue gGoSetValue = NULL;
	FGoGetLayer gGoGetLayer = NULL;
	FGoGetTag gGoGetTag = NULL;
	FGoSetLayer gGoSetLayer = NULL;
	FGoSetTag gGoSetTag = NULL;
	FGoQueryMethod gGoQueryMethod = NULL;
	FGoInvoke gGoInvoke = NULL;
	FGoGetLastError gGoGetLastError = NULL;

	void NSPluginLogHandler( const CNSString& text )
	{
#ifdef PLATFORM_WIN32
		NSConsole::consoleLogHandler( text );
#endif
		NSClient::gHostErrorProc( text.getBuffer( ), 0 );
	}

	void NSPluginExceptionHandler( const CNSString& text )
	{
#ifdef PLATFORM_WIN32
		NSConsole::consoleExceptionHandler( text );
#endif
		NSClient::gHostErrorProc( text.getBuffer( ), 1 );
	}

	void CNSNetClient::onRegisterProtocol( const CNSString& name )
	{
		static NSGateProto::CProtocolTunnel tunnel;
		static NSGateProto::CProtocolLogout logout;
		static NSGateProto::CProtocolKeepAlive keepAlive;
		registerProtocol( &tunnel );
		registerProtocol( &logout );
		registerProtocol( &keepAlive );

		static NSClientProto::CProtocolScript script;
		static NSClientProto::CProtocolLoginResult loginResult;
		NSGateProto::CProtocolTunnel::registerProtocol( &script );
		NSGateProto::CProtocolTunnel::registerProtocol( &loginResult );
	}

	void CNSNetClient::onAddSession( const CNSString& name, TSessionID sessionID, const CNSSockAddr& local, const CNSSockAddr& peer )
	{
		NSClientProto::CProtocolLogin login( mpNSClient->mAuthName, mpNSClient->mAuthUserID, mpNSClient->mToken );
		mpNSClient->send2Oper( &login );

		NSBase::CNSLuaStack& luaStack = NSBase::CNSLuaStack::getLuaStack( );
		luaStack.preCall( "onClientAddSession" );
		luaStack << mpNSClient->mClientID;
		luaStack.call( );
	}

	void CNSNetClient::onAddSessionFault( const CNSString& name, int code )
	{
		NSBase::CNSLuaStack& luaStack = NSBase::CNSLuaStack::getLuaStack( );

		luaStack.preCall( "onClientAddSessionFault" );
		luaStack << mpNSClient->mClientID;
		luaStack << code;
		luaStack.call( );
	}

	void CNSNetClient::onDelSession( const CNSString& name, TSessionID sessionID )
	{
		NSBase::CNSLuaStack& luaStack = NSBase::CNSLuaStack::getLuaStack( );
		luaStack.preCall( "onClientDelSession" );
		luaStack << mpNSClient->mClientID;
		luaStack.call( );
	}

	// 1		��������4�ֽ�	  1�ֽ�		n�ֽ�	1�ֽ�	  n�ֽ�	 
	// pid		size			( tunnelpid	data	tunnelpid data )
	// |		|				  |
	// protoID	protoSize		  protoBuffer	
	void CNSNetClient::onRawProtocol( const CNSString& name, TSessionID sessionID, TProtocolID* protoID, const CNSOctetsShadow& buffer )
	{
		TProtocolID pid = *protoID;
		if ( *protoID & 0x80 )
		{
			pid = *protoID & 0x7F;
			CNSOctets uBuffer = buffer.uncompress( );
			CNSOctetsStream stream( uBuffer );
			stream.unmarshalToPointer( buffer );
		}

		switch ( pid )
		{
			case NSGateProto::CProtocolTunnel::PID:
			{
				CNSProtocol* proto = NULL;
				try
				{
					proto = NSGateProto::CProtocolTunnel::getProto( buffer );
					// ���Э��û���ҵ�
					if ( proto == NULL )
						break;

					onLogicProtocol( proto );
				}
				catch ( CNSMarshal::CException& )
				{
					NSLog::exception( _UTF8( "���Э��������, ��������ͻ��˷���Э�����������bug, ���ؾ���" ) );
					proto = NULL;
				}
				break;
			}
		}
	}

	void CNSNetClient::onProtocol( const CNSString& name, TSessionID sessionID, CNSProtocol* proto )
	{
		switch ( proto->mProtoID )
		{
			case NSGateProto::CProtocolLogout::PID:
			{
				NSBase::CNSLuaStack& luaStack = NSBase::CNSLuaStack::getLuaStack( );
				NSGateProto::CProtocolLogout* logout = ( NSGateProto::CProtocolLogout* ) proto;
				luaStack.preCall( "onLogout" );
				luaStack << mpNSClient->mClientID;
				luaStack << logout->mReasonID;
				luaStack.call( );
				break;
			}
			case NSGateProto::CProtocolKeepAlive::PID:
			{
				break;
			}
		}
	}

	void CNSNetClient::onLogicProtocol( CNSProtocol* proto )
	{
		switch ( proto->mProtoID )
		{
			case NSClientProto::CProtocolScript::PID:
			{
				NSClientProto::CProtocolScript* script = ( NSClientProto::CProtocolScript* ) proto;
				NSBase::CNSLuaStack& luaStack = NSBase::CNSLuaStack::getLuaStack( );
				luaStack.preCall( "onClientProtocol" );
				luaStack << mpNSClient->mClientID;
				for ( ; script->mpShadowBuffer->isEnd( ) == false; )
					luaStack << *script->mpShadowBuffer;
				luaStack.call( );
				break;
			}
			case NSClientProto::CProtocolLoginResult::PID:
			{
				NSClientProto::CProtocolLoginResult* loginResult = ( NSClientProto::CProtocolLoginResult* ) proto;
				NSBase::CNSLuaStack& luaStack = NSBase::CNSLuaStack::getLuaStack( );
				luaStack.preCall( "onLoginResult" );
				luaStack << mpNSClient->mClientID;
				luaStack << loginResult->mResult;
				luaStack.call( );
				break;
			}
		}
	}

	int CNSClient::getClientID( ) const
	{
		return mClientID;
	}

	void CNSClient::login( const CNSString& address, const CNSString& authName, const CNSString& userName, const CNSString& token )
	{
		if ( mpNSNetClient != NULL )
		{
			NSNet::close( mpNSNetClient->mName );
			delete mpNSNetClient;
		}

		mAuthName = authName;
		mAuthUserID = userName;
		mToken = token;
		mpNSNetClient = new NSClient::CNSNetClient( mNetName, address, this );
		NSNet::registerClient( mpNSNetClient );

		NSBase::CNSLuaStack& luaStack = NSBase::CNSLuaStack::getLuaStack( );
		luaStack.preCall( "onConnecting" );
		luaStack << mClientID;
		luaStack.call( );
	}

	void CNSClient::send( CNSProtocol* proto )
	{
		static CNSOctetsStream protoBuffer;
		protoBuffer.clear( );
		protoBuffer << proto->getProtoID( );
		protoBuffer << *proto;

		NSGateProto::CProtocolTunnel protoTunnel( &protoBuffer.mBuffer, 0, NSGateProto::CProtocolTunnel::PID );
		NSNet::send( mNetName, protoTunnel.createStream( ) );
	}

	void CNSClient::send2Oper( CNSProtocol* proto )
	{
		static CNSOctetsStream protoBuffer;
		protoBuffer.clear( );
		protoBuffer << proto->getProtoID( );
		protoBuffer << *proto;

		NSGateProto::CProtocolTunnel protoTunnel( &protoBuffer.mBuffer, 0, NSGateProto::CProtocolTunnel::PPID );
		NSNet::send( mNetName, protoTunnel.createStream( ) );
	}

#ifdef PLATFORM_WIN32
	void CNSPlugin::onInitApp( )
	{
		NSWin32::CNSBaseApp::onInitApp( );
		CNSLocal::getNSLocal( ).setLang( "ch" );
		// ע��ű�����
		NSClient::regLuaLib( );

		// ���ؽű�
		NSBase::CNSLuaStack& luaStack = NSBase::CNSLuaStack::getLuaStack( );
		luaStack.load( "assets/script" );

		luaStack.preCall( "onLaunchClient" );
		if ( luaStack.call( ) == false )
			NSException( _UTF8( "lua����[onLaunchClient]�����쳣" ) );

		mPluginLoaded = true;
	}

	void CNSPlugin::onExitApp( )
	{
		NSWin32::CNSBaseApp::onExitApp( );
	}

	void CNSPlugin::onIdle( )
	{
		if ( mPluginLoaded == false )
			return;

		NSWin32::CNSBaseApp::onIdle( );
	}
#endif
}

bool nsClientInit( const char* authName, FHostErrorProc proc, bool enableDebug )
{
	try
	{
		if ( NSClient::gTouchProc == NULL )
			NSException( "û������[FTouchProc]����" )
		
		if ( NSClient::gGoGetLayer == NULL )
			NSException( "û������[FGoGetLayer]����" )

		if ( NSClient::gGoGetTag == NULL )
			NSException( "û������[FGoGetTag]����" )

		if ( NSClient::gGoQueryMethod == NULL )
			NSException( "û������[FGoQueryMethod]����" )

		if ( NSClient::gGoInvoke == NULL )
			NSException( "û������[FGoInvoke]����" )

		if ( NSClient::gGoLoadProc == NULL )
			NSException( "û������[FGoLoadProc]����" )

		if ( NSClient::gGoDestroyProc == NULL )
			NSException( "û������[FGoDestroyProc]����" )

		if ( NSClient::gGoGetValue == NULL )
			NSException( "û������[FGoGetValue]����" )

		if ( NSClient::gGoSetValue == NULL )
			NSException( "û������[FGoSetValue]����" )

		if ( NSClient::gGoGetLastError == NULL )
			NSException( "û������[FGoGetLastError]����" )

		NSClient::gHostErrorProc = proc;
#ifdef PLATFORM_WIN32
		if ( NSClient::app != NULL )
			delete NSClient::app;

		NSLog::setLogHandler( NSClient::NSPluginLogHandler );
		NSLog::setExceptionHandler( NSClient::NSPluginExceptionHandler );
		NSClient::app = new NSClient::CNSPlugin( enableDebug );

		NSClient::app->useIPName( false );
		NSClient::app->useNSHttp( true );
		NSClient::app->useNSHttpDebugger( false );
		NSClient::app->useNSMysql( false );
		NSClient::app->useNSLocal( true );
		NSClient::app->onInitApp( );
		NSClient::CNSClient::sAuthName = authName;
#else
		// �������ͨ���ֻ������Ի�������
		NSLog::init( "NSPlugins" );
        NSLog::setLogHandler( NSClient::NSPluginLogHandler );
        NSLog::setExceptionHandler( NSClient::NSPluginExceptionHandler );

		// ��ʼ�������
		NSNet::init( );

        // ��ʼ��curl
        NSHttp::init( );

        // ���ر��ػ��ļ�
        CNSLocal::getNSLocal().load( );
		CNSLocal::getNSLocal( ).setLang( "ch" );

		// ��ʼ��Lua��
		NSBase::CNSLuaStack::init( enableDebug );
        NSBase::regLuaLib( );

        // ע��ű�����
        NSClient::regLuaLib( );

		// ���ؽű�
		NSBase::CNSLuaStack& luaStack = NSBase::CNSLuaStack::getLuaStack( );
		luaStack.load( "assets/script" );

		luaStack.preCall( "onLaunchClient" );
		luaStack.call( );
#endif
	}
	catch ( NSBase::CNSException& e )
	{
		NSLog::exception( _UTF8( "�������ʱ�����쳣\n��������: \n\t%s\nC++���ö�ջ: \n%s" ), e.mErrorDesc, NSBase::NSFunction::getStackInfo( ).getBuffer( ) );
		return false;
	}

	return true;
}

void nsClientExit( )
{
	try
	{
#ifdef PLATFORM_WIN32
		NSClient::app->onExitApp( );
		delete NSClient::app;
		NSClient::app = NULL;
		NSClient::CNSClient::cleanUp( );
#else
		NSLog::exit( );
		NSNet::exit( );
		NSHttp::exit( );
		CNSLuaStack::exit( );
		NSClient::CNSClient::cleanUp( );
#endif
	}
	catch ( NSBase::CNSException& e )
	{
		NSLog::exception( _UTF8( "����ر�ʱ�����쳣\n��������: \n\t%s\nC++���ö�ջ: \n%s" ), e.mErrorDesc, NSBase::NSFunction::getStackInfo( ).getBuffer( ) );
	}
}

void nsClientUpdate( )
{
	try
	{
#ifdef PLATFORM_WIN32
		NSClient::app->onIdle( );
#else
		NSNet::pollEvent( );
		CNSTimer::updateTimer( );
#endif
	}
	catch ( NSBase::CNSException& e )
	{
		NSLog::exception( _UTF8( "�����ѭ�������쳣\n��������: \n\t%s\nC++���ö�ջ: \n%s" ), e.mErrorDesc, NSBase::NSFunction::getStackInfo( ).getBuffer( ) );
	}
}

void nsClientShowConsole( )
{
#ifdef PLATFORM_WIN32
	NSConsole::showConsole( );
#endif
}

void nsSetTouchProc( FTouchProc touchProc )
{
	NSClient::gTouchProc = touchProc;
}
