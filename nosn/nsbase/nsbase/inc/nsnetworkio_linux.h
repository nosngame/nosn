#pragma once
namespace FBNet
{

// ********************************************************************** //
// CNSNetworkIO
// ********************************************************************** //

class CNSNetworkIO
{
public:
	CNSString			mName;

public:
	virtual ~CNSNetworkIO( )
	{
	}

public:
	CNSNetworkIO( const CNSString& rName );
	const CNSString& GetName( ) const { return mName; }

public:
	virtual void destroy( unsigned int vSessionID ) = 0;
	virtual int send( TSessionID vSessionID, CNSProtocol* pProtocol ) = 0;
	virtual int send( TSessionID vSessionID, const CNSString& rText ) { return 0; }
	virtual	void Run( unsigned int vMillsSeconds ) = 0;
	virtual CNSSessionDesc* getDesc( TSessionID vSessionID ) = 0;
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
	int					mEPollHandle;
	int					mListenSocket;
	TSessionMap			mSessions;				// 会话列表
	CNSNetManager*		mpManager;

public:
	CNSPassiveIO( CNSNetManager* pManager, const CNSString& rName );

public:
	virtual ~CNSPassiveIO( );

public:
	void onAccept( int vBytesTran, int vIndex );

public:
	CNSSession* getSession( TSessionID vSessionID );
	bool create( const CNSString& rAddress = "", const CNSString& rPort = "" );
	bool accept( int vIndex );
	void permitRecv( CNSSession* pSession );
	int permitSend( CNSSession* pSession );

public:
	virtual int send( TSessionID vSessionID, CNSProtocol* pProtocol );
	virtual int send( TSessionID vSessionID, const CNSString& rText );
	virtual void destroy( unsigned int vSessionID );
	virtual	void Run( unsigned int vMilliSeconds );
	virtual CNSSessionDesc* getDesc( unsigned int vSessionID );
};

// ********************************************************************** //
// CNSActiveIO
// ********************************************************************** //

class CNSActiveIO : public CNSNetworkIO
{
	friend class CNSSession;

protected:
	int					mSocket;
	CNSNetManager*		mpManager;
	CNSSession*			mpSession;
	bool				mSendReady;
public:
	CNSActiveIO( CNSNetManager* pManager, const CNSString& rName );

public:
	virtual ~CNSActiveIO( );

protected:
	int sendHelper( );

public:
	void onConnect( int vCode );
	void onRecv( );
	void onSend( );

public:
	virtual void destroy( TSessionID vSessionID );
	virtual int send( TSessionID vSessionID, CNSProtocol* pProtocol );
	virtual void create( const CNSString& rAddress = "", const CNSString& rPort = "" );
	virtual void Run( unsigned int vMillsSeconds );
	virtual CNSSessionDesc* getDesc( unsigned int vSessionID );
};

};
