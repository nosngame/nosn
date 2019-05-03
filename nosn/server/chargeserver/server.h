#pragma once
namespace ChargeServer
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

	class CHttpServer : public CNSNetManager
	{
	public:
		CHttpServer( const CNSString& name, const CNSString& address, unsigned int interval, unsigned int bufferSize, unsigned int streamSize )
			: CNSNetManager( name, address, interval, bufferSize, streamSize, true )
		{
		}

	public:
		virtual void onRegisterProtocol( const CNSString& name ) { }
		virtual void onAddSession( const CNSString& name, TSessionID sessionID, const CNSSockAddr& local, const CNSSockAddr& peer ) { }
		virtual void onDelSession( const CNSString& name, TSessionID sessionID ) { }
		virtual void onProtocol( const CNSString& name, TSessionID sessionID, const CNSMap< CNSString, CNSString >& headers, const CNSString& text, const CNSString& postData );
	};

	class CChargeServer : public CNSNetManager
	{
	public:
		CChargeServer( const CNSString& name, const CNSString& address, unsigned int pollInterval, unsigned int bufferSize, unsigned int streamSize ) 
			: CNSNetManager( name, address, pollInterval, bufferSize, streamSize )
		{
		}

	public:
		virtual void onRegisterProtocol( const CNSString& name );
		virtual void onAddSession( const CNSString& name, TSessionID sessionID, const CNSSockAddr& local, const CNSSockAddr& peer );
		virtual void onDelSession( const CNSString& name, TSessionID sessionID );
		virtual void onProtocol( const CNSString& name, TSessionID sessionID, CNSProtocol* proto );
	};

	class CChargeLogic
	{
	public:
		CNSVector< CHttpServer* >	mHttpServer;
		CChargeServer*				mpChargeServer;
		CNSString					mWorkPath;

		// 区服信息
		CNSList< CRegionInfo >				mRegions;

		// 以GroupID为Key的组信息
		CNSMap< unsigned int, CGroupInfo >	mGroupIndex;

	public:
		CChargeLogic( );
		
	public:
		void luaPushServerList( CNSLuaStack& luaStack );
		void send2ChargeClient( TSessionID sessionID, CNSProtocol* proto );
		void onLaunchServer( );
		void onChargeServerAddSession( TSessionID sessionID, CNSSessionDesc* desc );
		void onChargeServerDelSession( TSessionID sessionID, CNSSessionDesc* desc );
		void onChargeProtocol( TSessionID sessionID, NSChargeProto::CProtocolScript* proto );

	public:
		void init( NSWin32::CConsoleApp< CChargeLogic >* app );
		void exit( );
		void loadServerList( );
		void reloadServerList( );
	};
}
