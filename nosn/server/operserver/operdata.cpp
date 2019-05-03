#include <fbbase.h>
#include <dataprotocol.h>
#include "common.h"
using namespace Common;

#include "httphelper.h"
#include "httpcmd.h"
#include "operdata.h"
#include <luahelper.h>
#include <mysqlclient.h>
#include "server.h"

static int daySecCount = 24 * 60 * 60;
static int day3SecCount = 24 * 60 * 60 * 3;
static int day7SecCount = 24 * 60 * 60 * 7;

BOOL WINAPI ConsoleHandler( DWORD CEvent )
{
	switch ( CEvent )
	{
        case CTRL_C_EVENT:
        case CTRL_BREAK_EVENT:
        case CTRL_CLOSE_EVENT:
        case CTRL_LOGOFF_EVENT:
        case CTRL_SHUTDOWN_EVENT:
		{
			COperationData* operData = COperationData::GetSingleton( );
			CFBOctetsStream tempBuffer;
			EnterCriticalSection( &operData->mCs );
			tempBuffer << operData->mAccounts;
			tempBuffer << operData->mOperData;

			HANDLE fileHandle = CreateFile( _T("operdata/data.odata"), GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
			DWORD bytesWritten;
			::WriteFile( fileHandle, tempBuffer.Begin( ), tempBuffer.Length( ), &bytesWritten, NULL );
			::FlushFileBuffers( fileHandle );
			::CloseHandle( fileHandle );
			LeaveCriticalSection( &operData->mCs );
			FBLog::log( "safty exit" );
			return FALSE;
		}
	} 

	return TRUE;
}

void COperationData::CSaveTimer::onTimer( unsigned int curTick, CFBTimer::CTimerObject* timeObject )
{
	COperationData* operData = COperationData::GetSingleton( );
	EnterCriticalSection( &operData->mCs );
	static CFBOctetsStream tempBuffer;
	tempBuffer.Clear( );
	tempBuffer << COperationData::GetSingleton( )->mAccounts;
	tempBuffer << COperationData::GetSingleton( )->mOperData;

	HANDLE fileHandle = CreateFile( _T("operdata/data.odata"), GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
	DWORD bytesWritten;
	::WriteFile( fileHandle, tempBuffer.Begin( ), tempBuffer.Length( ), &bytesWritten, NULL );
	::FlushFileBuffers( fileHandle );
	::CloseHandle( fileHandle );
	LeaveCriticalSection( &operData->mCs );
}

COperationData::COperationData( )
{
	InitializeCriticalSection( &mCs );
	EnterCriticalSection( &mCs );
	::CreateDirectory( _T("operdata"), NULL );
	HANDLE dataHandle = CreateFile( _T("operdata/data.odata"), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( dataHandle != INVALID_HANDLE_VALUE )
	{
		DWORD size = ::GetFileSize( dataHandle, NULL );
		CFBOctets buffer( size, size );
		DWORD bytesReaded;
		::ReadFile( dataHandle, buffer.Begin( ), size, &bytesReaded, NULL );
		try
		{
			CFBOctetsStream stream( buffer );
			stream >> mAccounts;
			stream >> mOperData;
		}
		catch( CFBMarshal::CException& )
		{
			mAccounts.Clear( );
			mOperData.Clear( );
		}
		::CloseHandle( dataHandle );
	}

	LeaveCriticalSection( &mCs );
	SetConsoleCtrlHandler( (PHANDLER_ROUTINE) ConsoleHandler, TRUE );
	CFBTimer::createTimer( &mSaveTimer, 1000 * 60 * 5, NULL );
}

COperationData::~COperationData( )
{
	DeleteCriticalSection( &mCs );
}

void COperationData::addAccount( const CFBSmallTimer& curTime, TGroupID groupID, const CFBString& authName, const long long& userKey )
{
	EnterCriticalSection( &mCs );
	bool isNewAccount = false;
	bool isNewGroupAccount = false;
	CAccountInfo* accountInfo = mAccounts.Get( userKey );
	if ( accountInfo == NULL )
	{
		isNewAccount = true;
		accountInfo = &mAccounts.Insert( userKey, CAccountInfo( curTime ) );
	}

	CLoginInfo* loginInfo = accountInfo->mGroupLoginInfo.Get( groupID );
	if ( loginInfo == NULL )
	{
		isNewGroupAccount = true;
		loginInfo = &accountInfo->mGroupLoginInfo.Insert( groupID, CLoginInfo( curTime ) );
	}
	
	CDayOperData* operData = mOperData.Get( curTime.getTime( ) );
	if ( operData == NULL )
		operData = &mOperData.Insert( curTime.getTime( ), CDayOperData( ) );

	CFBMap< TGroupID, CFBMap< CFBString, COperData > >* groupDayData = &operData->mGroupData;
	if ( groupDayData != NULL )
	{
		CFBMap< CFBString, COperData >* groupData = groupDayData->Get( groupID );
		if ( groupData == NULL )
			groupData = &groupDayData->Insert( groupID, CFBMap< CFBString, COperData >( ) );
	
		COperData* authData = groupData->Get( authName );
		if ( authData == NULL )
			authData = &groupData->Insert( authName, COperData( ) );

		if ( isNewAccount == true )
		{
			operData->mTotalData.mNew ++;
			operData->mTotalData.mDau ++;
			authData->mNew ++;
			authData->mDau ++;
		}
	
		if ( isNewGroupAccount == true )
		{
			authData->mNew ++;
			authData->mDau ++;
		}

		if ( loginInfo->mLoginTime != curTime )
		{
			loginInfo->mLoginTime = curTime;
			authData->mDau ++;
			time_t escapeSec = curTime - loginInfo->mCreateTime;
			if ( escapeSec == daySecCount )
				authData->mRemain1 ++;
			if ( escapeSec == day3SecCount )
				authData->mRemain3 ++;
			if ( escapeSec == day7SecCount )
				authData->mRemain7 ++;
		}
	}

	if ( accountInfo->mLoginTime != curTime )
	{
		accountInfo->mLoginTime = curTime;
		operData->mTotalData.mDau ++;
		time_t escapeSec = curTime - accountInfo->mCreateTime;
		if ( escapeSec == daySecCount )
			operData->mTotalData.mRemain1 ++;
		if ( escapeSec == day3SecCount )
			operData->mTotalData.mRemain3 ++;
		if ( escapeSec == day7SecCount )
			operData->mTotalData.mRemain7 ++;
	}
	LeaveCriticalSection( &mCs );
}

unsigned int COperationData::getValue( COperData* operData, EQueryType queryType )
{
	unsigned int value = 0;
	if ( operData == NULL )
		return value;

	if ( queryType == REMAIN_DAY1 )
		value = operData->mRemain1;
	else if ( queryType == REMAIN_DAY3 )
		value = operData->mRemain3;
	else if ( queryType == REMAIN_DAY7 )
		value = operData->mRemain7;
	else if ( queryType == DAU )
		value = operData->mDau;
	else if ( queryType == NEW )
		value = operData->mDau;

	return value;
}

unsigned int COperationData::getData( const CFBSmallTimer& time, TGroupID groupID, const CFBString& authName, EQueryType queryType, const CFBSet< CFBString >& authList )
{
	CDayOperData* data = mOperData.Get( time.getTime( ) );
	if ( data == NULL )
		return 0;

	unsigned int value = 0;
	if ( groupID == 0 && authName.IsEmpty( ) == true )
	{
		if ( authList.GetCount( ) > 0 )
		{
			HLISTINDEX beginIndex = authList.GetHead( );
			for (; beginIndex != NULL; authList.GetNext( beginIndex ) )
			{
				const CFBString& name = authList.GetKey( beginIndex );
				HLISTINDEX groupIndex = data->mGroupData.GetHead( );
				for (; groupIndex != NULL; data->mGroupData.GetNext( groupIndex ) )
				{
					CFBMap< CFBString, COperData >& groupData = data->mGroupData.GetValue( groupIndex );
					COperData* authData = groupData.Get( name );
					value += getValue( authData, queryType );
				}
			}

			return value;
		}

		return getValue( &data->mTotalData, queryType );
	}
	else if ( groupID != 0 && authName.IsEmpty( ) == true )
	{
		CFBMap< CFBString, COperData >* groupData = data->mGroupData.Get( groupID );
		if ( groupData == NULL )
			return 0;

		if ( authList.GetCount( ) > 0 )
		{
			HLISTINDEX beginIndex = authList.GetHead( );
			for (; beginIndex != NULL; authList.GetNext( beginIndex ) )
			{
				const CFBString& name = authList.GetKey( beginIndex );
				COperData* authData = groupData->Get( name );
				value += getValue( authData, queryType );
			}

			return value;
		}

		HLISTINDEX beginIndex = groupData->GetHead( );
		for (; beginIndex != NULL; groupData->GetNext( beginIndex ) )
		{
			COperData& authData = groupData->GetValue( beginIndex );
			value += getValue( &authData, queryType );
		}

		return value;
	}
	else if ( authName.IsEmpty( ) == false && groupID == 0 )
	{
		if ( authList.GetCount( ) == 0 || authList.Find( authName ) == true )
		{
			HLISTINDEX beginIndex = data->mGroupData.GetHead( );
			for (; beginIndex != NULL; data->mGroupData.GetNext( beginIndex ) )
			{
				CFBMap< CFBString, COperData >& groupData = data->mGroupData.GetValue( beginIndex );
				COperData* authData = groupData.Get( authName );
				value += getValue( authData, queryType );
			}
			
			return value;
		}
	}
	else if ( authName.IsEmpty( ) == false && groupID != 0 )
	{
		if ( authList.GetCount( ) == 0 || authList.Find( authName ) == true )
		{
			CFBMap< CFBString, COperData >* groupData = data->mGroupData.Get( groupID );
			if ( groupData != NULL )
			{
				COperData* authData = groupData->Get( authName );
				value += getValue( authData, queryType );
			}
		}

		return value;
	}

	return 0;
}
	
// day = 1 次日留存
// day = 3 三日留存
// day = 7 七日留存
float COperationData::getRemainOfDay( const CFBSmallTimer& time, TGroupID groupID, const CFBString& authName, EQueryType remainType, const CFBSet< CFBString >& authList )
{
	CFBTimer curTime( time.getYear( ), time.getMonth( ), time.getDay( ) );
	CFBString baseDay		= curTime.getTimeTextOfDay( );
	if ( remainType == REMAIN_DAY1 )
		curTime.nextDay( 1 );
	else if ( remainType == REMAIN_DAY3 )
		curTime.nextDay( 3 );
	else if ( remainType == REMAIN_DAY7 )
		curTime.nextDay( 7 );

	CFBString nextDay			= curTime.getTimeTextOfDay( );
	unsigned int newCount		= getData( time, groupID, authName, COperationData::NEW, authList );
	unsigned int remainCount	= getData( time, groupID, authName, remainType, authList );
	return ( (float) remainCount / max( 1.0f, newCount ) ) * 100;
}