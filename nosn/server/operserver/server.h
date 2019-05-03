#pragma once

namespace OperServer
{
	class CGroupInfo : public CNSLuaMarshal
	{
	public:
		CNSString mName;
		CNSString mTag;
		CNSString mOperAddress;		// 运营端口
		CNSString mOAddress;			// 客户端端口
		CNSString mChannel;
		CNSString mPreview;
		unsigned int mGroupID;

	public:
		virtual CNSLuaStack& marshal( CNSLuaStack& luaStack ) const
		{
			luaStack.pushField( "mName", mName );
			luaStack.pushField( "mTag", mTag );
			luaStack.pushField( "mOperAddress", mOperAddress );
			luaStack.pushField( "mOAddress", mOAddress );
			luaStack.pushField( "mChannel", mChannel );
			luaStack.pushField( "mPreview", mPreview );
			luaStack.pushField( "mGroupID", mGroupID );
			return luaStack;
		}

		virtual const CNSLuaStack& unmarshal( const CNSLuaStack& luaStack )
		{
			luaStack.popField( "mName", mName );
			luaStack.popField( "mTag", mTag );
			luaStack.popField( "mOperAddress", mOperAddress );
			luaStack.popField( "mOAddress", mOAddress );
			luaStack.popField( "mChannel", mChannel );
			luaStack.popField( "mPreview", mPreview );
			luaStack.popField( "mGroupID", mGroupID );
			return luaStack;
		}
	};

	class CRegionInfo : public CNSLuaMarshal
	{
	public:
		CNSString mName;
		CNSString mTag;
		CNSList< CGroupInfo > mGroups;

	public:
		virtual CNSLuaStack& marshal( CNSLuaStack& luaStack ) const
		{
			luaStack.pushField( "mName", mName );
			luaStack.pushField( "mTag", mTag );
			luaStack.pushField( "mGroups", mGroups );
			return luaStack;
		}

		virtual const CNSLuaStack& unmarshal( const CNSLuaStack& luaStack )
		{
			luaStack.popField( "mName", mName );
			luaStack.popField( "mTag", mTag );
			luaStack.popField( "mGroups", mGroups );
			return luaStack;
		}
	};

	class COperServer : public CNSNetManager
	{
	public:
		COperServer( const CNSString& name, const CNSString& address, unsigned int pollInterval, unsigned int bufferSize, unsigned int streamSize )
			: CNSNetManager( name, address, pollInterval, bufferSize, streamSize )
		{
		}

	public:
		virtual void onRegisterProtocol( const CNSString& name );
		virtual void onAddSession( const CNSString& name, TSessionID sessionID, const CNSSockAddr& local, const CNSSockAddr& peer );
		virtual void onDelSession( const CNSString& name, TSessionID sessionID );
		virtual void onProtocol( const CNSString& name, TSessionID sessionID, CNSProtocol* proto );
	};

	class COperLogic : public GateClient::CInnerLogic
	{
	public:
		COperServer* mpOperServer;

		// 区服信息
		CNSList< CRegionInfo > mRegions;

		// 以GroupID为Key的组信息
		CNSMap< unsigned int, CGroupInfo > mGroupIndex;

	public:
		COperLogic( );

	public:
		void luaPushServerList( CNSLuaStack& luaStack );

		void onLaunchServer( );
		void onOperServerAddSession( TSessionID sessionID, CNSSessionDesc* desc );
		void onOperServerDelSession( TSessionID sessionID, CNSSessionDesc* desc );
		void onOperProtocol( TSessionID sessionID, NSOperProto::CProtocolScript* proto );

	public:
		virtual void onConnectGate( const CNSString& name, const CNSString& address );
		virtual void onRegisterClientProtocol( CNSNetManager* manager );
		virtual void onClientLost( const CNSString& name, TSessionID sessionID, long long nosnUserID );
		virtual void onClientLogin( const CNSString& name, TSessionID sessionID, long long nosnUserID );
		virtual void onClientProtocol( GateClient::CInnerClient* inner, TSessionID sessionID, CNSProtocol* proto );

	public:
		void init( NSWin32::CConsoleApp< COperLogic >* app );
		void exit( );
		void loadServerList( );
		void reloadServerList( );
		void send2OperClient( unsigned int sessionID, CNSProtocol* proto );
		void verifyResult( const CNSString& gateName, TSessionID sessionID, long long nosnUserID, NSClientProto::CProtocolLoginResult::ELoginResult loginResult );
	};
}