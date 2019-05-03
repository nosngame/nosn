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
	virtual void Destroy( unsigned int vSessionID ) = 0;
	virtual int Send( TSessionID vSessionID, CNSProtocol* pProtocol ) = 0;
	virtual int Send( TSessionID vSessionID, const CNSString& rText ) { return 0; }
	virtual	void Run( unsigned int vMillsSeconds ) = 0;
	virtual CFBSessionDesc* GetDesc( TSessionID vSessionID ) = 0;
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
	int SendHelper( );

public:
	void OnConnect( int vCode );
	void OnRecv( );
	void OnSend( );

public:
	virtual void Destroy( TSessionID vSessionID );
	virtual int Send( TSessionID vSessionID, CNSProtocol* pProtocol );
	virtual void Create( const CNSString& rAddress = "", const CNSString& rPort = "" );
	virtual void Run( unsigned int vMillsSeconds );
	virtual CFBSessionDesc* GetDesc( unsigned int vSessionID );
};

};
