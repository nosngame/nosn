#pragma once
class CChargeData
{
public:
	long long			mMoney;
	float				mAarpu;
	float				mArpu;
	float				mPayrate;
	CFBSet< long long > mUserSets;

public:
	CChargeData( ) : mMoney( 0 ), mAarpu( 0.0f ), mArpu( 0.0f ), mPayrate( 0.0f )
	{
	}
};

class CHttpProc
{
public:
	virtual void onHttpCommand( const CFBString& name, TSessionID sessionID, const CFBVector< CFBString >& paramList ) = 0;
};

class CFBGameWelcome : public CHttpProc
{
public:
	virtual void onHttpCommand( const CFBString& name, TSessionID sessionID, const CFBVector< CFBString >& paramList );
};

class CFBGameVerify : public CHttpProc
{
public:
	virtual void onHttpCommand( const CFBString& name, TSessionID sessionID, const CFBVector< CFBString >& paramList );
};

class CFBGameLogin : public CHttpProc
{
public:
	virtual void onHttpCommand( const CFBString& name, TSessionID sessionID, const CFBVector< CFBString >& paramList );
};

class CFBGameNetworkFlow : public CHttpProc
{
public:
	virtual void onHttpCommand( const CFBString& name, TSessionID sessionID, const CFBVector< CFBString >& paramList );
};

class CFBGameOnlineCounter : public CHttpProc
{
public:
	virtual void onHttpCommand( const CFBString& name, TSessionID sessionID, const CFBVector< CFBString >& paramList );
};

class CFBGameOperation : public CHttpProc
{
public:
	virtual void onHttpCommand( const CFBString& name, TSessionID sessionID, const CFBVector< CFBString >& paramList );
};

class CFBGameRemain : public CHttpProc
{
public:
	virtual void onHttpCommand( const CFBString& name, TSessionID sessionID, const CFBVector< CFBString >& paramList );
};

class CFBGameCharge : public CHttpProc
{
public:
	virtual void onHttpCommand( const CFBString& name, TSessionID sessionID, const CFBVector< CFBString >& paramList );
};

class CFBGameUserManager : public CHttpProc
{
public:
	virtual void onHttpCommand( const CFBString& name, TSessionID sessionID, const CFBVector< CFBString >& paramList );
};

class CFBGameAddUser : public CHttpProc
{
public:
	virtual void onHttpCommand( const CFBString& name, TSessionID sessionID, const CFBVector< CFBString >& paramList );
};

class CFBGameRemoveUser : public CHttpProc
{
public:
	virtual void onHttpCommand( const CFBString& name, TSessionID sessionID, const CFBVector< CFBString >& paramList );
};

class CFBGameModifyUser : public CHttpProc
{
public:
	virtual void onHttpCommand( const CFBString& name, TSessionID sessionID, const CFBVector< CFBString >& paramList );
};

class CFBGameModifyPassword : public CHttpProc
{
public:
	virtual void onHttpCommand( const CFBString& name, TSessionID sessionID, const CFBVector< CFBString >& paramList );
};

// 给赛亚人用的
class CFBGameShutdown : public CHttpProc
{
public:
	virtual void onHttpCommand( const CFBString& name, TSessionID sessionID, const CFBVector< CFBString >& paramList );
};

// 给赛亚人用的
class CFBGameClearDB : public CHttpProc
{
public:
	virtual void onHttpCommand( const CFBString& name, TSessionID sessionID, const CFBVector< CFBString >& paramList );
};
