#pragma once
typedef HMAPINDEX TAccountID;
typedef unsigned short TGroupID;

class COperData : public CFBMarshal
{
public:
	unsigned int	mDau;
	unsigned int	mNew;
	unsigned int	mRemain1;
	unsigned int	mRemain3;
	unsigned int	mRemain7;

public:
	COperData( ) : mDau( 0 ), mNew( 0 ), mRemain1( 0 ), mRemain3( 0 ), mRemain7( 0 )
	{
	}

public:
	virtual CFBOctetsStream& Marshal( CFBOctetsStream& stream ) const
	{
		stream << mDau;
		stream << mNew;
		stream << mRemain1;
		stream << mRemain3;
		stream << mRemain7;
		return stream;
	}

	virtual const CFBOctetsStream& UnMarshal( const CFBOctetsStream& stream )
	{
		stream >> mDau;
		stream >> mNew;
		stream >> mRemain1;
		stream >> mRemain3;
		stream >> mRemain7;
		return stream;
	}
};

class CLoginInfo : public CFBMarshal
{
public:
	CFBSmallTimer		mCreateTime;
	CFBSmallTimer		mLoginTime;

public:
	CLoginInfo( )
	{
	}

	CLoginInfo( const CFBSmallTimer& curTime ) : mCreateTime( curTime ), mLoginTime( curTime )
	{
	}
	
public:
	virtual CFBOctetsStream& Marshal( CFBOctetsStream& stream ) const
	{
		stream << mCreateTime;
		stream << mLoginTime;
		return stream;
	}

	virtual const CFBOctetsStream& UnMarshal( const CFBOctetsStream& stream )
	{
		stream >> mCreateTime;
		stream >> mLoginTime;
		return stream;
	}
};

class CAccountInfo : public CFBMarshal
{
public:
	CFBMap< TGroupID, CLoginInfo >		mGroupLoginInfo;
	CFBSmallTimer						mCreateTime;
	CFBSmallTimer						mLoginTime;

public:
	CAccountInfo( )
	{
	}

	CAccountInfo( const CFBSmallTimer& curTime ) : mCreateTime( curTime ), mLoginTime( curTime )
	{
	}

public:
	virtual CFBOctetsStream& Marshal( CFBOctetsStream& stream ) const
	{
		stream << mGroupLoginInfo;
		stream << mCreateTime;
		stream << mLoginTime;
		return stream;
	}

	virtual const CFBOctetsStream& UnMarshal( const CFBOctetsStream& stream )
	{
		stream >> mGroupLoginInfo;
		stream >> mCreateTime;
		stream >> mLoginTime;
		return stream;
	}
};

class CDayOperData : public CFBMarshal
{
public:
	// 每天活跃和新增
	CFBMap< TGroupID, CFBMap< CFBString, COperData > >		mGroupData;
	COperData 												mTotalData;
	
public:
	virtual CFBOctetsStream& Marshal( CFBOctetsStream& stream ) const
	{
		stream << mGroupData;
		stream << mTotalData;
		return stream;
	}

	virtual const CFBOctetsStream& UnMarshal( const CFBOctetsStream& stream )
	{
		stream >> mGroupData;
		stream >> mTotalData;
		return stream;
	}
};

// 运营指标
class COperationData
{
public:
	class CSaveTimer : public CFBTimer::CTimerProc
	{
	public:
		virtual void onTimer( unsigned int curTick, CFBTimer::CTimerObject* timeObject );
	};

public:
	enum EQueryType
	{
		DAU,
		NEW,
		REMAIN_DAY1,
		REMAIN_DAY3,
		REMAIN_DAY7,
	};

	CFBMap< long long, CAccountInfo >								mAccounts;
	CFBMap< unsigned short, CDayOperData >							mOperData;
	CSaveTimer														mSaveTimer;
	CRITICAL_SECTION												mCs;
public:
	COperationData( );

public:
	~COperationData( );

public:
	static COperationData* GetSingleton( )
	{
		static COperationData sInst;
		return &sInst;
	}

public:
	unsigned int getValue( COperData* operData, EQueryType queryType );
	void addAccount( const CFBSmallTimer& time, TGroupID groupID, const CFBString& authName, const long long& userKey );
	unsigned int getData( const CFBSmallTimer& time, TGroupID groupID, const CFBString& authName, EQueryType queryType, const CFBSet< CFBString >& authList );
	// day = 1 次日留存
	// day = 3 三日留存
	// day = 7 七日留存
	float getRemainOfDay( const CFBSmallTimer& time, TGroupID groupID, const CFBString& authName, EQueryType queryType, const CFBSet< CFBString >& authList );
};