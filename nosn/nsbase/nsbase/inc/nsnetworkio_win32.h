#pragma once
namespace NSNet
{
	class CNSNetworkIO
	{
	public:
		CNSString			mName;

	protected:
		unsigned int	mPollTick = 0;
		bool			mIsDeleted = false;

	public:
		CNSNetworkIO( const CNSString& rName );

	public:
		virtual ~CNSNetworkIO( )
		{
		}

	public:
		const CNSString& GetName( ) const { return mName; }
		bool isDeleted( ) const
		{
			return mIsDeleted;
		}

	public:
		virtual void destroy( unsigned int vSessionID ) = 0;
		virtual int send( TSessionID vSessionID, const CNSOctets& buffer ) = 0;
		virtual int send( TSessionID vSessionID, const void* begin, const void* end ) = 0;
		virtual int send( TSessionID vSessionID, const void* begin, size_t len ) = 0;
		virtual CNSSessionDesc* getDesc( TSessionID vSessionID ) = 0;
		virtual void pollEvent( unsigned int tickCount ) = 0;
	};

	// ********************************************************************** //
	// CNSPassiveIO
	// ********************************************************************** //

	class CNSPassiveIO : public CNSNetworkIO
	{
		friend class CNSSession;
	public:
		typedef CNSMap< TSessionID, CNSSession* > TSessionMap;

	protected:
		HANDLE			mIOPort = NULL;

	protected:
		unsigned int		mAcceptNum;
		CNSOctets*			mAcceptBuffer;
		CNSMap< WSAOVERLAPPED*, int > mAcceptMap;
		WSAOVERLAPPED*		mAcceptOverlapped;
		WSAOVERLAPPED		mRecvOverlapped;
		WSAOVERLAPPED		mSendOverlapped;
		SOCKET				mListenSocket;
		SOCKET*				mAcceptSocket;
		TSessionMap			mSessions;				// 会话列表
		CNSNetManager*		mpManager;
		TSessionMap			mSessionsTomb;
		unsigned int		mTombTick = 300;

	public:
		CNSPassiveIO( CNSNetManager* pManager, const CNSString& rName, unsigned int acceptNum );

	public:
		virtual ~CNSPassiveIO( );

	public:
		virtual void pollEvent( unsigned int tickCount );

	public:
		void onAccept( int bytesTran, int index );

	public:
		CNSSession* getSession( TSessionID sessionID );
		void create( const CNSString& address = "", const CNSString& port = "" );
		void accept( int index );
		void permitRecv( CNSSession* session );
		int permitSend( CNSSession* session );

	public:
		virtual int send( TSessionID sessionID, const CNSOctets& buffer );
		virtual int send( TSessionID sessionID, const void* begin, const void* end );
		virtual int send( TSessionID sessionID, const void* begin, size_t len );
		virtual void destroy( unsigned int sessionID );
		virtual CNSSessionDesc* getDesc( unsigned int sessionID );
	};

	// ********************************************************************** //
	// CNSActiveIO
	// ********************************************************************** //

	class CNSActiveIO : public CNSNetworkIO
	{
		friend class CNSSession;
		enum
		{
			EACTIVE_IDLE,
			EACTIVE_CONNECTING,
			EACTIVE_ESTABLISH,
			EACTIVE_CLOSING,
			EACTIVE_CONNECTFAULT,
			EACTIVE_CLOSED,
		};

	protected:
		SOCKET								mSocket;
		HANDLE								mEObject;
		CNSNetManager*						mpManager;
		CNSSession*							mpSession;
		bool								mSendReady;

	public:
		virtual void pollEvent( unsigned int tickCount );

	public:
		CNSActiveIO( CNSNetManager* pManager, const CNSString& rName );

	public:
		virtual ~CNSActiveIO( );

	protected:
		int sendHelper( );

	public:
		void onConnect( int code );
		void onRecv( );
		void onSend( );

	public:
		virtual void destroy( TSessionID sessionID );
		virtual int send( TSessionID sessionID, const CNSOctets& buffer );
		virtual int send( TSessionID sessionID, const void* begin, const void* end );
		virtual int send( TSessionID sessionID, const void* begin, size_t len );
		virtual void create( const CNSString& address = "", const CNSString& port = "" );
		virtual CNSSessionDesc* getDesc( unsigned int sessionID );
	};

};
