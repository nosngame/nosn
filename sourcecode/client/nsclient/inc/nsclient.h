#pragma once

namespace NSClient
{
	class CNSClient;
	class CNSNetClient;
	class CNSPlugin;

	class CNSNetClient : public CNSNetManager
	{
	protected:
		CNSClient* mpNSClient;

	public:
		CNSNetClient( const CNSString& name, const CNSString& address, CNSClient* nsClient )
			: CNSNetManager( name, address, 0, 8192, 8192 ), mpNSClient( nsClient )
		{
		}

	public:
		virtual void onRegisterProtocol( const CNSString& name );
		virtual void onAddSession( const CNSString& name, TSessionID sessionID, const CNSSockAddr& local, const CNSSockAddr& peer );
		virtual void onAddSessionFault( const CNSString& name, int code );
		virtual void onDelSession( const CNSString& name, TSessionID sessionID );
		virtual void onRawProtocol( const CNSString& name, TSessionID sessionID, TProtocolID* protoID, const CNSOctetsShadow& buffer );
		virtual void onProtocol( const CNSString& name, TSessionID sessionID, CNSProtocol* proto );

	public:
		void onLogicProtocol( CNSProtocol* proto );
	};

	class CNSClient
	{
		friend class CNSNetClient;

	protected:
		int mClientID = 0;
		CNSString mNetName;
		CNSString mAuthName;
		CNSString mAuthUserID;
		CNSString mToken;
		CNSNetClient* mpNSNetClient = NULL;

	public:
		// 只增不减，可以用索引做ID
		static CNSVector< CNSClient* > sNSClient;
		static CNSString sAuthName;

	public:
		static CNSClient* createNSClient( )
		{
			CNSClient* client = new CNSClient( );
			client->mClientID = sNSClient.getCount( );
			sNSClient.pushback( client );

			client->mNetName = CNSString( "client_" ) + CNSString::number2String( client->mClientID );
			return client;
		}

		static CNSClient* getNSClient( int clientID )
		{
			if ( clientID < 0 || clientID >= (int) sNSClient.getCount( ) )
				return NULL;

			return sNSClient[ clientID ];
		}

		static void cleanUp( )
		{
			for ( unsigned int i = 0; i < sNSClient.getCount( ); i ++ )
				delete sNSClient[ i ];

			sNSClient.clear( );
		}

	public:
		CNSClient( )
		{
		}

	public:
		~CNSClient( )
		{
			delete mpNSNetClient;
		}

	public:
		int getClientID( ) const;
		void login( const CNSString& address, const CNSString& authName, const CNSString& authUserID, const CNSString& token );
		void send( CNSProtocol* proto );
		void send2Oper( CNSProtocol* proto );
	};

#ifdef PLATFORM_WIN32
	class CNSPlugin : public NSWin32::CNSBaseApp
	{
	protected:
		bool mPluginLoaded = false;

	public:
		CNSPlugin( bool enableDebug ) : CNSBaseApp( "NSPlugins", enableDebug )
		{
		}

	public:
		virtual void onInitApp( );
		virtual void onExitApp( );
		virtual void onIdle( );
	};
#endif

	extern FUILoadLayout gUILoadProc;
	extern FUIDestroy gUIDestroyProc;
	extern FHostErrorProc gHostErrorProc;
	extern FUIGetValue gUIGetValue;
	extern FUISetValue gUISetValue;

}

