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
