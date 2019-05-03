#include <fbbase.h>
#include "operprotocol.h"
#include "dataprotocol.h"
#include <httphelper.h>
#include <mysqlclient.h>
#include <luahelper.h>
#include "httpcmd.h"
#include "operdata.h"
#include "server.h"
#include "common.h"

class COperUser : public CFBMarshal
{
public:
	CFBString	mUserName;
	int			mLevel;
	CFBString	mToken;
	CFBString	mPassword;
	CFBSet< CFBString > mAuthList;

public:
	COperUser( const CFBString& userName = "", const CFBString& password = "", int level = 1, const CFBSet< CFBString >& authList = CFBSet< CFBString >( ) ) : mUserName( userName ), mPassword( password ), mLevel( level ), mAuthList( authList )
	{
	}

public:
	virtual CFBOctetsStream& Marshal( CFBOctetsStream& stream ) const
	{
		stream << mLevel;
		stream << mUserName;
		stream << mPassword;
		stream << mAuthList;
		return stream;
	}

	virtual const CFBOctetsStream& UnMarshal( const CFBOctetsStream& stream )
	{
		stream >> mLevel;
		stream >> mUserName;
		stream >> mPassword;
		stream >> mAuthList;
		return stream;
	}
};

CFBMap< CFBString, COperUser* > tokens;			// token key
CFBMap< CFBString, COperUser > operUsers;		// username key

CFBString getUserList( )
{
	CFBString userList;
	HLISTINDEX beginIndex = operUsers.GetHead( );
	for ( ; beginIndex != NULL; operUsers.GetNext( beginIndex ) )
	{
		CFBString& userName = operUsers.GetKey( beginIndex );
		CFBString item;
		item.Format( "{ label: \"%s\", value: \"%s\" }", userName.GetBuffer( ), userName.GetBuffer( ) );
		userList += item;
		if ( beginIndex != operUsers.GetTail( ) )
			userList += ", ";
	}

	return userList;
}

CFBString getUserManager( )
{
	COperCenter* oper = COperCenter::GetSingleton( );
	static CFBString userPage;
	if ( userPage.IsEmpty( ) == true )
	{
		HRSRC	hSrc = FindResource( NULL, _T("IDR_USERPAGE"), RT_HTML );
		HGLOBAL hMem = LoadResource( NULL, hSrc );
		DWORD	size = SizeofResource( NULL, hSrc );
		char*	html = (char*) LockResource( hMem );
		CFBString htmlText( html, size );
		userPage.PushBack( htmlText );
		UnlockResource( hSrc );
		FreeResource( hMem );
	}

	static CFBString addUserDialog;
	if ( addUserDialog.IsEmpty( ) == true )
	{
		HRSRC	hSrc = FindResource( NULL, _T("IDR_ADDUSERDIALOG"), RT_HTML );
		HGLOBAL hMem = LoadResource( NULL, hSrc );
		DWORD	size = SizeofResource( NULL, hSrc );
		char*	html = (char*) LockResource( hMem );
		CFBString htmlText( html, size );
		addUserDialog.PushBack( htmlText );
		UnlockResource( hSrc );
		FreeResource( hMem );
		addUserDialog.Replace( "\r\n", "" );
	}

	static CFBString modifyUserDialog;
	if ( modifyUserDialog.IsEmpty( ) == true )
	{
		HRSRC	hSrc = FindResource( NULL, _T("IDR_MODIFYUSERDIALOG"), RT_HTML );
		HGLOBAL hMem = LoadResource( NULL, hSrc );
		DWORD	size = SizeofResource( NULL, hSrc );
		char*	html = (char*) LockResource( hMem );
		CFBString htmlText( html, size );
		modifyUserDialog.PushBack( htmlText );
		UnlockResource( hSrc );
		FreeResource( hMem );
		modifyUserDialog.Replace( "\r\n", "" );
	}

	static CFBString modifyPasswordDialog;
	if ( modifyPasswordDialog.IsEmpty( ) == true )
	{
		HRSRC	hSrc = FindResource( NULL, _T("IDR_MODIFYPASSWORDDIALOG"), RT_HTML );
		HGLOBAL hMem = LoadResource( NULL, hSrc );
		DWORD	size = SizeofResource( NULL, hSrc );
		char*	html = (char*) LockResource( hMem );
		CFBString htmlText( html, size );
		modifyPasswordDialog.PushBack( htmlText );
		UnlockResource( hSrc );
		FreeResource( hMem );
		modifyPasswordDialog.Replace( "\r\n", "" );
	}

	CFBString userList = getUserList( );
	CFBString authList = oper->GetAuthSelectHtml( );
	CFBString addUser;
	addUser.Format( addUserDialog.GetBuffer( ), authList.GetBuffer( ) );
	CFBString modifyUser;
	modifyUser.Format( modifyUserDialog.GetBuffer( ), authList.GetBuffer( ) );
	CFBString retValue;
	retValue.Format( userPage.GetBuffer( ), userList.GetBuffer( ), addUser.GetBuffer( ), modifyUser.GetBuffer( ), modifyPasswordDialog.GetBuffer( ) );
	return retValue;
}

void returnError( const CFBString& name, TSessionID sessionID, const CFBString& error )
{
	static CFBString errorPage;
	if ( errorPage.IsEmpty( ) == true )
	{
		HRSRC	hSrc = FindResource( NULL, _T("IDR_ERRORPAGE"), RT_HTML );
		HGLOBAL hMem = LoadResource( NULL, hSrc );
		DWORD	size = SizeofResource( NULL, hSrc );
		char*	html = (char*) LockResource( hMem );
		CFBString htmlText( html, size );
		errorPage.PushBack( htmlText );
		UnlockResource( hSrc );
		FreeResource( hMem );
	}

	CFBString retValue;
	retValue.Format( errorPage.GetBuffer( ), error.GetBuffer( ) );
	HttpReturnUtf8Helper( name, sessionID, retValue );
}

void CFBGameWelcome::onHttpCommand( const CFBString& name, TSessionID sessionID, const CFBVector< CFBString >& paramList )
{
	static CFBString loginPage;
	if ( loginPage.IsEmpty( ) == true )
	{
		HRSRC	hSrc = FindResource( NULL, _T("IDR_LOGINPAGE"), RT_HTML );
		HGLOBAL hMem = LoadResource( NULL, hSrc );
		DWORD	size = SizeofResource( NULL, hSrc );
		char*	html = (char*) LockResource( hMem );
		CFBString htmlText( html, size );
		loginPage.PushBack( htmlText );
		UnlockResource( hSrc );
		FreeResource( hMem );
	}

	HttpReturnUtf8Helper( name, sessionID, loginPage );
}

void CFBGameVerify::onHttpCommand( const CFBString& name, TSessionID sessionID, const CFBVector< CFBString >& paramList )
{
	if ( paramList.GetCount( ) != 3 )
	{
		HttpReturnUtf8Helper( name, sessionID, _UTF8( "登录请求无效" ) );
		return;
	}

	CFBString	userName	= paramList[ 1 ];
	CFBString	password	= paramList[ 2 ];
	COperUser*	user		= NULL;
	CFBString tempToken;
	if ( operUsers.GetCount( ) == 0 )
	{
		HANDLE handle = CreateFile( _T("operdata/account.odata"), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
		if ( handle != INVALID_HANDLE_VALUE )
		{
			DWORD len = GetFileSize( handle, NULL );
			CFBOctets buffer( len, len );
			DWORD bytesReaded;
			
			ReadFile( handle, buffer.Begin( ), len, &bytesReaded, NULL );
			CFBOctetsStream stream( buffer );
			stream >> operUsers;
			CloseHandle( handle );
		}
	}

	if ( userName == "FBGameAdministrator" )
	{
		if ( password == "FBGame20130409xsj" )
		{
			static COperUser rootUser( "FBGameAdministrator", "FBGame20130409xsj" );
			user = &rootUser;
		}
		else
		{
			HttpReturnUtf8Helper( name, sessionID, _UTF8( "密码错误" ) );
			return;
		}
	}
	else if ( userName == "root" )
	{
		if ( password == "ddgdaniubi" )
		{
			static COperUser rootUser( "root", "", 1 );
			user = &rootUser;
		}
		else
		{
			HttpReturnUtf8Helper( name, sessionID, _UTF8( "密码错误" ) );
			return;
		}
	}
	else
	{
		COperUser* userRef = operUsers.Get( userName );
		if ( userRef == NULL )
		{
			HttpReturnUtf8Helper( name, sessionID, _UTF8( "用户不存在" ) );
			return;
		}
		else if ( userRef->mPassword != password )
		{
			HttpReturnUtf8Helper( name, sessionID, _UTF8( "密码错误" ) );
			return;
		}
		user = userRef;
	}

	for ( int i = 0; i < 32; i ++ )
		tempToken.PushBack( 'a' + Random( 0, 25 ) );

	CFBTimer curTime;
	tempToken += curTime.getTimeText( );

	static CFBOctets buffer;
	buffer.Replace( tempToken.GetBuffer( ), tempToken.GetLength( ) );

	static CFBOctets result;
	buffer.EncodeMD5( result );
	user->mToken = result.toHexString( );
	tokens.Insert( user->mToken, user );
	HttpReturnUtf8Helper( name, sessionID, user->mToken );
}
	
void CFBGameLogin::onHttpCommand( const CFBString& name, TSessionID sessionID, const CFBVector< CFBString >& paramList )
{
	if ( paramList.GetCount( ) != 2 )
	{
		HttpReturnUtf8Helper( name, sessionID, _UTF8("登录请求无效") );
		return;
	}

	CFBString token			= paramList[ 1 ];
	COperUser** userInfo	= tokens.Get( token );
	if ( userInfo == NULL )
	{
		HttpReturnUtf8Helper( name, sessionID, _UTF8("登录请求无效") );
		return;
	}

	static CFBString mainPage;
	if ( mainPage.IsEmpty( ) == true )
	{
		HRSRC	hSrc = FindResource( NULL, _T("IDR_MAINPAGE"), RT_HTML );
		HGLOBAL hMem = LoadResource( NULL, hSrc );
		DWORD	size = SizeofResource( NULL, hSrc );
		char*	html = (char*) LockResource( hMem );
		CFBString htmlText( html, size );
		mainPage.PushBack( htmlText );
		UnlockResource( hSrc );
		FreeResource( hMem );
	}

	CFBString retValue;
	retValue.Format( mainPage.GetBuffer( ), (*userInfo)->mLevel, token.GetBuffer( ) );
	HttpReturnUtf8Helper( name, sessionID, retValue );
}

void CFBGameNetworkFlow::onHttpCommand( const CFBString& name, TSessionID sessionID, const CFBVector< CFBString >& paramList )
{
	if ( paramList.GetCount( ) != 3 )
	{
		returnError( name, sessionID, _UTF8("令牌无效") );
		return;
	}

	CFBString token			= paramList[ 1 ];
	unsigned int groupID	= paramList[ 2 ].ToInteger( );
	COperUser** userInfo	= tokens.Get( token );
	if ( userInfo == NULL )
	{
		returnError( name, sessionID, _UTF8("令牌无效") );
		return;
	}

	CProtocolStatRequest statRequest( name, sessionID, groupID );
	COperCenter::GetSingleton( )->Send2Center( groupID, &statRequest );
}

void CFBGameOnlineCounter::onHttpCommand( const CFBString& name, TSessionID sessionID, const CFBVector< CFBString >& paramList )
{
	unsigned int t1 = CFBTimer::getCurTick( );
	if ( paramList.GetCount( ) != 4 )
	{
		returnError( name, sessionID, _UTF8("令牌无效") );
		return;
	}

	CFBString token			= paramList[ 1 ];
	unsigned int groupID	= paramList[ 2 ].ToInteger( );
	CFBString authName		= paramList[ 3 ];
	COperUser** userInfo	= tokens.Get( token );
	if ( userInfo == NULL )
	{
		returnError( name, sessionID, _UTF8("令牌无效") );
		return;
	}

	if ( (*userInfo)->mAuthList.GetCount( ) != 0 && (*userInfo)->mAuthList.Find( authName ) == false )
	{
		returnError( name, sessionID, _UTF8("用户权限不足") );
		return;
	}

	static CFBString onlinePage;
	if ( onlinePage.IsEmpty( ) == true )
	{
		HRSRC	hSrc = FindResource( NULL, _T("IDR_ONLINECOUNTER"), RT_HTML );
		HGLOBAL hMem = LoadResource( NULL, hSrc );
		DWORD	size = SizeofResource( NULL, hSrc );
		char*	html = (char*) LockResource( hMem );
		CFBString htmlText( html, size );
		onlinePage.PushBack( htmlText );
		UnlockResource( hSrc );
		FreeResource( hMem );
	}

	COperCenter* oper			= COperCenter::GetSingleton( );
	unsigned int totalOnline	= oper->GetTotalOnline( (*userInfo)->mAuthList );
	unsigned int acu			= oper->GetAcu( (*userInfo)->mAuthList );
	unsigned int pcu			= oper->GetPcu( (*userInfo)->mAuthList );
	CFBString serverCombo		= CFBString( _UTF8("{name: \"所有服务器\", id:\"0\"}, " ) ) + oper->GetServerComboHtml( );
	CFBString serverPie			= oper->GetServerPieHtml( totalOnline, (*userInfo)->mAuthList );
	CFBString authCombo			= CFBString( _UTF8("{name: \"所有渠道\", id:\"\"}, " ) ) + oper->GetAuthComboHtml( (*userInfo)->mAuthList );
	CFBString authPie			= oper->GetAuthPieHtml( totalOnline, (*userInfo)->mAuthList );
	CFBString onlineCounter		= "";
	unsigned int maxOnline		= 0;
	CFBVector< unsigned int > userCounter;
	oper->GetUserCounter( groupID, authName, (*userInfo)->mAuthList, userCounter );
	for ( size_t i = 0; i < userCounter.GetCount( ); i ++ )
	{
		onlineCounter += CFBString( "{x: " ) + CFBString::Number2String( i ) + ", y: " + CFBString::Number2String( userCounter[ i ] ) + "}";
		if ( maxOnline < userCounter[ i ] )
			maxOnline = userCounter[ i ];

		if ( i < userCounter.GetCount( ) - 1 )
			onlineCounter += ",";
	}

	CFBString selectGroup;
	CFBString selectAuth;
	CGroupInfo* groupRef = oper->mGroupIndex.Get( groupID );
	if ( groupRef != NULL )
		selectGroup = groupRef->mName;
	else
		selectGroup = _UTF8("所有服务器");

	CFBString* authLabelRef = oper->mAuthInfo.Get( authName );
	if ( authLabelRef != NULL )
		selectAuth = *authLabelRef;
	else
		selectAuth = _UTF8("所有渠道");

	CFBString retValue;
	retValue.Format( onlinePage.GetBuffer( ), groupID, authName.GetBuffer( ), selectGroup.GetBuffer( ), selectAuth.GetBuffer( ), serverPie.GetBuffer( ), authPie.GetBuffer( ), 
		totalOnline, acu, pcu, onlineCounter.GetBuffer( ), FBBase::forbsize( maxOnline ), serverCombo.GetBuffer( ), authCombo.GetBuffer( ) );
	HttpReturnUtf8Helper( name, sessionID, retValue );
	unsigned int t2 = CFBTimer::getCurTick( ) - t1;
	FBLog::log( "cost - %d\r\n", t2 );
}

void CFBGameOperation::onHttpCommand( const CFBString& name, TSessionID sessionID, const CFBVector< CFBString >& paramList )
{
	COperCenter* oper = COperCenter::GetSingleton( );
	COperationData* data = COperationData::GetSingleton( );
	if ( paramList.GetCount( ) != 8 )
	{
		returnError( name, sessionID, _UTF8("令牌无效") );
		return;
	}

	CFBString token			= paramList[ 1 ];
	unsigned int groupID	= paramList[ 2 ].ToInteger( );
	CFBString authName		= paramList[ 3 ];
	int year				= paramList[ 4 ].ToInteger( );
	int month				= paramList[ 5 ].ToInteger( );
	int day					= paramList[ 6 ].ToInteger( );
	int rangeType			= paramList[ 7 ].ToInteger( );
	COperUser** userInfo	= tokens.Get( token );
	if ( userInfo == NULL )
	{
		returnError( name, sessionID, _UTF8("令牌无效") );
		return;
	}

	if ( (*userInfo)->mAuthList.GetCount( ) != 0 && (*userInfo)->mAuthList.Find( authName ) == false )
	{
		returnError( name, sessionID, _UTF8("用户权限不足") );
		return;
	}

	static CFBString operPage;
	if ( operPage.IsEmpty( ) == true )
	{
		HRSRC	hSrc = FindResource( NULL, _T("IDR_OPERPAGE"), RT_HTML );
		HGLOBAL hMem = LoadResource( NULL, hSrc );
		DWORD	size = SizeofResource( NULL, hSrc );
		char*	html = (char*) LockResource( hMem );
		CFBString htmlText( html, size );
		operPage.PushBack( htmlText );
		UnlockResource( hSrc );
		FreeResource( hMem );
	}

	CFBString selectGroup;
	CFBString selectAuth;
	CFBString selectRange;
	CGroupInfo* groupRef = oper->mGroupIndex.Get( groupID );
	if ( groupRef != NULL )
		selectGroup = groupRef->mName;
	else
		selectGroup = _UTF8("所有服务器");

	CFBString* authLabelRef = oper->mAuthInfo.Get( authName );
	if ( authLabelRef != NULL )
		selectAuth = *authLabelRef;
	else
		selectAuth = _UTF8("所有渠道");

	if ( rangeType == 7 )
		selectRange = _UTF8( "显示7日" );
	else if ( rangeType == 30 )
		selectRange = _UTF8( "显示30日" );
	else if ( rangeType == 180 )
		selectRange = _UTF8( "显示180日" );

	CFBString datePool;
	CFBString dauIndex;
	CFBString newIndex;
	CFBTimer curTime( year, month, day );

	datePool	+= CFBString( "\"\", " );
	unsigned int maxDau = 0;
	unsigned int maxNew = 0;
	for ( int i = 0; i < rangeType; i ++, curTime.nextDay( 1 ) )
	{
		CFBString& date = curTime.getTimeTextOfDay( );
		datePool += CFBString( "\"") + date + "\"";

		unsigned int dauValue = data->getData( curTime, groupID, authName, COperationData::DAU, (*userInfo)->mAuthList );
		unsigned int newValue = data->getData( curTime, groupID, authName, COperationData::NEW, (*userInfo)->mAuthList );
		maxDau = max( maxDau, dauValue );
		maxNew = max( maxNew, newValue );
		dauIndex += CFBString::Number2String( dauValue );
		newIndex += CFBString::Number2String( newValue );

		if ( i < rangeType - 1 )
		{
			dauIndex += ", ";
			newIndex += ", ";
			datePool += ", ";
		}
	}

	CFBString serverCombo		= CFBString( _UTF8("{name: \"所有服务器\", id:\"0\"}, " ) ) + oper->GetServerComboHtml( );
	CFBString authCombo			= CFBString( _UTF8("{name: \"所有渠道\", id:\"\"}, " ) ) + oper->GetAuthComboHtml( (*userInfo)->mAuthList );

	CFBString retValue;
	retValue.Format( operPage.GetBuffer( ), groupID, authName.GetBuffer( ), year, month, day, rangeType, selectGroup.GetBuffer( ), selectAuth.GetBuffer( ), selectRange.GetBuffer( ), serverCombo.GetBuffer( ), authCombo.GetBuffer( ), 
		datePool.GetBuffer( ), dauIndex.GetBuffer( ), FBBase::forbsize( maxDau ), newIndex.GetBuffer( ), FBBase::forbsize( maxNew ) );
	HttpReturnUtf8Helper( name, sessionID, retValue );
}

void CFBGameRemain::onHttpCommand( const CFBString& name, TSessionID sessionID, const CFBVector< CFBString >& paramList )
{
	COperCenter* oper = COperCenter::GetSingleton( );
	COperationData* data = COperationData::GetSingleton( );
	if ( paramList.GetCount( ) != 8 )
	{
		returnError( name, sessionID, _UTF8("令牌无效") );
		return;
	}

	CFBString token			= paramList[ 1 ];
	unsigned int groupID	= paramList[ 2 ].ToInteger( );
	CFBString authName		= paramList[ 3 ];
	int year				= paramList[ 4 ].ToInteger( );
	int month				= paramList[ 5 ].ToInteger( );
	int day					= paramList[ 6 ].ToInteger( );
	int rangeType			= paramList[ 7 ].ToInteger( );
	COperUser** userInfo	= tokens.Get( token );
	if ( userInfo == NULL )
	{
		returnError( name, sessionID, _UTF8("令牌无效") );
		return;
	}

	static CFBString remainPage;
	if ( remainPage.IsEmpty( ) == true )
	{
		HRSRC	hSrc = FindResource( NULL, _T("IDR_REMAINPAGE"), RT_HTML );
		HGLOBAL hMem = LoadResource( NULL, hSrc );
		DWORD	size = SizeofResource( NULL, hSrc );
		char*	html = (char*) LockResource( hMem );
		CFBString htmlText( html, size );
		remainPage.PushBack( htmlText );
		UnlockResource( hSrc );
		FreeResource( hMem );
	}

	CFBString selectGroup;
	CFBString selectAuth;
	CFBString selectRange;
	CGroupInfo* groupRef = oper->mGroupIndex.Get( groupID );
	if ( groupRef != NULL )
		selectGroup = groupRef->mName;
	else
		selectGroup = _UTF8("所有服务器");

	CFBString* authLabelRef = oper->mAuthInfo.Get( authName );
	if ( authLabelRef != NULL )
		selectAuth = *authLabelRef;
	else
		selectAuth = _UTF8("所有渠道");

	if ( rangeType == 7 )
		selectRange = _UTF8( "显示7日" );
	else if ( rangeType == 30 )
		selectRange = _UTF8( "显示30日" );
	else if ( rangeType == 180 )
		selectRange = _UTF8( "显示180日" );

	CFBString datePool;
	CFBString remain1Index;
	CFBString remain3Index;
	CFBString remain7Index;
	CFBTimer curTime( year, month, day );

	datePool	+= CFBString( "\"\", " );
	for ( int i = 0; i < rangeType; i ++, curTime.nextDay( 1 ) )
	{
		CFBString date = curTime.getTimeTextOfDay( );
		datePool += CFBString( "\"") + date + "\"";

		float remain1Value = data->getRemainOfDay( curTime, groupID, authName, COperationData::REMAIN_DAY1, (*userInfo)->mAuthList );
		float remain3Value = data->getRemainOfDay( curTime, groupID, authName, COperationData::REMAIN_DAY3, (*userInfo)->mAuthList );
		float remain7Value = data->getRemainOfDay( curTime, groupID, authName, COperationData::REMAIN_DAY7, (*userInfo)->mAuthList );

		remain1Index += CFBString::Number2String( remain1Value );
		remain3Index += CFBString::Number2String( remain3Value );
		remain7Index += CFBString::Number2String( remain7Value );

		if ( i < rangeType - 1 )
		{
			remain1Index += ", ";
			remain3Index += ", ";
			remain7Index += ", ";
			datePool += ", ";
		}
	}

	CFBString serverCombo		= CFBString( _UTF8("{name: \"所有服务器\", id:\"0\"}, ") ) + oper->GetServerComboHtml( );
	CFBString authCombo			= CFBString( _UTF8("{name: \"所有渠道\", id:\"\"}, ") ) + oper->GetAuthComboHtml( (*userInfo)->mAuthList );

	CFBString retValue;
	retValue.Format( remainPage.GetBuffer( ), groupID, authName.GetBuffer( ), year, month, day, rangeType, selectGroup.GetBuffer( ), selectAuth.GetBuffer( ), selectRange.GetBuffer( ), serverCombo.GetBuffer( ), authCombo.GetBuffer( ), 
		datePool.GetBuffer( ), remain1Index.GetBuffer( ), remain3Index.GetBuffer( ), remain7Index.GetBuffer( ) );
	HttpReturnUtf8Helper( name, sessionID, retValue );
}

void onQueryCharge( const CFBString& dbName, TSessionID dbSessionID, CProtocolExecuteSqlResponse* proto, CFBOctetsStream& stream )
{
	COperationData* data = COperationData::GetSingleton( );
	COperCenter* oper = COperCenter::GetSingleton( );
	CFBVector< CFBString > paramList;
	CFBString name;
	TSessionID sessionID;
	COperUser* userInfo = NULL;
	stream >> name;
	stream >> sessionID;
	stream >> paramList;
	stream >> (size_t) userInfo;

	if ( proto->mResultCode == 0 )
	{
		static CFBString errorDesc;
		errorDesc.Format( _UTF8( "数据库执行错误, 错误描述: %s" ), proto->mErrorDesc.GetBuffer( ) );
		returnError( name, sessionID, errorDesc );
		return;
	}

	static CFBString chargePage;
	if ( chargePage.IsEmpty( ) == true )
	{
		HRSRC	hSrc = FindResource( NULL, _T("IDR_CHARGEPAGE"), RT_HTML );
		HGLOBAL hMem = LoadResource( NULL, hSrc );
		DWORD	size = SizeofResource( NULL, hSrc );
		char*	html = (char*) LockResource( hMem );
		CFBString htmlText( html, size );
		chargePage.PushBack( htmlText );
		UnlockResource( hSrc );
		FreeResource( hMem );
	}

	unsigned int groupID		= paramList[ 2 ].ToInteger( );
	CFBString& authName			= paramList[ 3 ];
	int year					= paramList[ 4 ].ToInteger( );
	int month					= paramList[ 5 ].ToInteger( );
	int day						= paramList[ 6 ].ToInteger( );
	int rangeType				= paramList[ 7 ].ToInteger( );

	CFBString selectGroup;
	CFBString selectAuth;
	CFBString selectRange;
	CGroupInfo* groupRef = oper->mGroupIndex.Get( groupID );
	if ( groupRef != NULL )
		selectGroup = groupRef->mName;
	else
		selectGroup = _UTF8("所有服务器");

	CFBString* authLabelRef = oper->mAuthInfo.Get( authName );
	if ( authLabelRef != NULL )
		selectAuth = *authLabelRef;
	else
		selectAuth = _UTF8("所有渠道");

	if ( rangeType == 7 )
		selectRange = _UTF8( "显示7日" );
	else if ( rangeType == 30 )
		selectRange = _UTF8( "显示30日" );
	else if ( rangeType == 180 )
		selectRange = _UTF8( "显示180日" );

	CFBString serverCombo		= CFBString( _UTF8("{name: \"所有服务器\", id:\"0\"}, ") ) + oper->GetServerComboHtml( );
	CFBString authCombo			= CFBString( _UTF8("{name: \"所有渠道\", id:\"\"}, ") ) + oper->GetAuthComboHtml( userInfo->mAuthList );
	
	CFBString datePool;
	CFBMap< time_t, CChargeData > dayMoney;
	CFBTimer curTime( year, month, day );

	datePool	+= CFBString( "\"\", " );
	for ( int i = 0; i < rangeType; i ++, curTime.nextDay( 1 ) )
	{
		CFBString date = curTime.getTimeTextOfDay( );
		dayMoney.Insert( curTime.getUnixTime( ), CChargeData( ) );
		datePool += CFBString( "\"") + date + "\"";

		if ( i < rangeType - 1 )
			datePool += ", ";
	}
	
	for ( unsigned int i = 0; i < proto->GetRowCount( ); i ++ )
	{
		CFBString		userAuthName	= proto->GetStringValue( 0, i );
		int				userGroupID		= proto->GetIntValue( 1, i );
		int				money			= proto->GetIntValue( 2, i );
		long long		userID			= proto->GetInt64Value( 3, i );
		unsigned int	time			= proto->GetUIntValue( 4, i );
		CFBTimer chargeTime( (time_t) time );
		chargeTime.setHour( 0 );
		chargeTime.setMin( 0 );
		chargeTime.setSec( 0 );
		CChargeData* moneyCounter = dayMoney.Get( chargeTime.getUnixTime( ) );
		if ( moneyCounter != NULL )
		{
			if ( groupID == 0 && authName.IsEmpty( ) == true )
			{
				if ( userInfo->mAuthList.GetCount( ) > 0 )
				{
					if ( userInfo->mAuthList.Find( authName ) == true )
					{
						moneyCounter->mUserSets.Insert( userID );
						moneyCounter->mMoney += money;
					}
				}
				else
				{
					moneyCounter->mUserSets.Insert( userID );
					moneyCounter->mMoney += money;
				}
			}
			else if ( groupID != 0 && authName.IsEmpty( ) == true )
			{
				if ( userInfo->mAuthList.GetCount( ) > 0 )
				{
					if ( userInfo->mAuthList.Find( authName ) == true )
					{
						moneyCounter->mUserSets.Insert( userID );
						moneyCounter->mMoney += money;
					}
				}
				else if ( groupID == userGroupID )
				{
					moneyCounter->mUserSets.Insert( userID );
					moneyCounter->mMoney += money;
				}
			}
			else if ( authName.IsEmpty( ) == false && groupID == 0 )
			{
				if ( authName == userAuthName && ( userInfo->mAuthList.GetCount( ) == 0 || userInfo->mAuthList.Find( authName ) == true ) )
				{
					moneyCounter->mUserSets.Insert( userID );
					moneyCounter->mMoney += money;
				}
			}
		}
	}

	long long maxMoney = 0L;
	float maxAarpu = 0.0f;
	float maxArpu = 0.0f;
	float maxPayrate = 0.0f;
	CFBString dayMoneyText;
	CFBString aarpuText;
	CFBString arpuText;
	CFBString payrateText;
	HLISTINDEX beginIndex = dayMoney.GetHead( );
	for ( ; beginIndex != NULL; dayMoney.GetNext( beginIndex ) )
	{
		CFBTimer curTime( dayMoney.GetKey( beginIndex ) );
		CChargeData& chargeData = dayMoney.GetValue( beginIndex );
		chargeData.mAarpu	= chargeData.mMoney / (float) max( 1, chargeData.mUserSets.GetCount( ) );
		chargeData.mArpu	= chargeData.mMoney / (float) max( 1, data->getData( curTime, groupID, authName, COperationData::DAU, userInfo->mAuthList ) );
		chargeData.mPayrate = chargeData.mUserSets.GetCount( ) / (float) max( 1, data->getData( curTime, groupID, authName, COperationData::DAU, userInfo->mAuthList ) );

		dayMoneyText	+= CFBString::Number2String( chargeData.mMoney );
		aarpuText		+= CFBString::Number2String( chargeData.mAarpu );
		arpuText		+= CFBString::Number2String( chargeData.mArpu );
		payrateText		+= CFBString::Number2String( chargeData.mPayrate );
		maxMoney = max( maxMoney, chargeData.mMoney );
		maxAarpu = max( maxAarpu, chargeData.mAarpu );
		maxArpu = max( maxArpu, chargeData.mArpu );
		maxPayrate = max( maxPayrate, chargeData.mPayrate );
		if ( beginIndex != dayMoney.GetTail( ) )
		{
			dayMoneyText += ",";
			aarpuText += ",";
			arpuText += ",";
			payrateText += ",";
		}
	}
	CFBString retValue;
	retValue.Format( chargePage.GetBuffer( ), groupID, authName.GetBuffer( ), year, month, day, rangeType, selectGroup.GetBuffer( ), selectAuth.GetBuffer( ), selectRange.GetBuffer( ), serverCombo.GetBuffer( ),
		authCombo.GetBuffer( ), datePool.GetBuffer( ), payrateText.GetBuffer( ), maxPayrate, arpuText.GetBuffer( ), maxArpu, aarpuText.GetBuffer( ), maxAarpu, dayMoneyText.GetBuffer( ), maxMoney );

	HttpReturnUtf8Helper( name, sessionID, retValue );
}

void CFBGameCharge::onHttpCommand( const CFBString& name, TSessionID sessionID, const CFBVector< CFBString >& paramList )
{
	if ( paramList.GetCount( ) != 8 )
	{
		returnError( name, sessionID, _UTF8( "令牌无效" ) );
		return;
	}

	CFBString token			= paramList[ 1 ];
	int year				= paramList[ 4 ].ToInteger( );
	int month				= paramList[ 5 ].ToInteger( );
	int day					= paramList[ 6 ].ToInteger( );
	int rangeType			= paramList[ 7 ].ToInteger( );
	COperUser** userInfo	= tokens.Get( token );
	if ( userInfo == NULL )
	{
		returnError( name, sessionID, _UTF8( "令牌无效" ) );
		return;
	}

	CFBTimer curTime( year, month, day );
	time_t beginTime = curTime.getUnixTime( );
	curTime.nextDay( rangeType );
	time_t endTime = curTime.getUnixTime( );

	CFBOctetsStream stream;
	stream << name;
	stream << sessionID;
	stream << paramList;
	stream << (size_t) (*userInfo);
	static CFBString sqlCommand;
	sqlCommand.Format( "select authname, money, userid, time from authrecharge where time >= %d and time < %d", (unsigned int) beginTime, (unsigned int) endTime );

	static CFBOctets buffer;
	ExecuteSql( CSession( onQueryCharge, stream ), buffer, sqlCommand );
}

void CFBGameShutdown::onHttpCommand( const CFBString& name, TSessionID sessionID, const CFBVector< CFBString >& paramList )
{
	COperCenter* oper = COperCenter::GetSingleton( );
	oper->ManagerCommand( "exit" );
	HttpReturnUtf8Helper( name, sessionID, "success" );
}

void CFBGameClearDB::onHttpCommand( const CFBString& name, TSessionID sessionID, const CFBVector< CFBString >& paramList )
{
	COperCenter* oper = COperCenter::GetSingleton( );
	oper->ManagerCommand( "clear" );
	HttpReturnUtf8Helper( name, sessionID, "success" );
}

void CFBGameUserManager::onHttpCommand( const CFBString& name, TSessionID sessionID, const CFBVector< CFBString >& paramList )
{
	COperCenter* oper = COperCenter::GetSingleton( );
	if ( paramList.GetCount( ) != 2 )
	{
		returnError( name, sessionID, _UTF8( "令牌无效" ) );
		return;
	}

	CFBString token			= paramList[ 1 ];
	COperUser** userInfo	= tokens.Get( token );
	if ( userInfo == NULL )
	{
		returnError( name, sessionID, _UTF8( "令牌无效" ) );
		return;
	}

	if ( (*userInfo)->mLevel > 1 )
	{
		returnError( name, sessionID, _UTF8( "用户权限不足" ) );
		return;
	}

	CFBString retValue = getUserManager( );
	HttpReturnUtf8Helper( name, sessionID, retValue.GetBuffer( ) );
}

void CFBGameAddUser::onHttpCommand( const CFBString& name, TSessionID sessionID, const CFBVector< CFBString >& paramList )
{
	COperCenter* oper = COperCenter::GetSingleton( );
	if ( paramList.GetCount( ) != 5 )
	{
		returnError( name, sessionID, _UTF8( "令牌无效" ) );
		return;
	}

	CFBString token			= paramList[ 1 ];
	CFBString userName		= paramList[ 2 ];
	CFBString password		= paramList[ 3 ];
	CFBString userRight		= paramList[ 4 ];

	static CFBVector< CFBString > authNameList;
	authNameList.Clear( );
	userRight.Split( ",", authNameList );

	COperUser** userInfo	= tokens.Get( token );
	if ( userInfo == NULL )
	{
		returnError( name, sessionID, _UTF8( "令牌无效" ) );
		return;
	}

	if ( (*userInfo)->mLevel > 1 )
	{
		returnError( name, sessionID, _UTF8( "用户权限不足" ) );
		return;
	}

	if ( operUsers.Get( userName ) != NULL )
	{
		returnError( name, sessionID, _UTF8( "用户已经存在" ) );
		return;
	}

	if ( operUsers.GetCount( ) > 100 )
	{
		returnError( name, sessionID, _UTF8( "用户数量达上限" ) );
		return;
	}

	operUsers.Insert( userName, COperUser( userName, password, 2, authNameList ) );
	HANDLE handle = CreateFile( _T("operdata/account.odata"), GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( handle != INVALID_HANDLE_VALUE )
	{
		static CFBOctetsStream stream;
		stream.Clear( );
		stream << operUsers;

		DWORD bytesWritten;
		WriteFile( handle, stream.Begin( ), stream.Length( ), &bytesWritten, NULL );
		CloseHandle( handle );
	}

	CFBString retValue = getUserManager( );
	HttpReturnUtf8Helper( name, sessionID, retValue.GetBuffer( ) );
}

void CFBGameRemoveUser::onHttpCommand( const CFBString& name, TSessionID sessionID, const CFBVector< CFBString >& paramList )
{
	COperCenter* oper = COperCenter::GetSingleton( );
	if ( paramList.GetCount( ) != 3 )
	{
		returnError( name, sessionID, _UTF8( "令牌无效" ) );
		return;
	}

	CFBString token			= paramList[ 1 ];
	CFBString userName		= paramList[ 2 ];

	COperUser** userInfo	= tokens.Get( token );
	if ( userInfo == NULL )
	{
		returnError( name, sessionID, _UTF8( "令牌无效" ) );
		return;
	}

	if ( (*userInfo)->mLevel > 1 )
	{
		returnError( name, sessionID, _UTF8( "用户权限不足" ) );
		return;
	}

	if ( operUsers.Get( userName ) == NULL )
	{
		returnError( name, sessionID, _UTF8( "用户不存在" ) );
		return;
	}

	operUsers.Erase( userName );
	HANDLE handle = CreateFile( _T("operdata/account.odata"), GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( handle != INVALID_HANDLE_VALUE )
	{
		static CFBOctetsStream stream;
		stream.Clear( );
		stream << operUsers;

		DWORD bytesWritten;
		WriteFile( handle, stream.Begin( ), stream.Length( ), &bytesWritten, NULL );
		CloseHandle( handle );
	}

	CFBString retValue = getUserManager( );
	HttpReturnUtf8Helper( name, sessionID, retValue.GetBuffer( ) );
}

void CFBGameModifyUser::onHttpCommand( const CFBString& name, TSessionID sessionID, const CFBVector< CFBString >& paramList )
{
	COperCenter* oper = COperCenter::GetSingleton( );
	if ( paramList.GetCount( ) != 4 )
	{
		returnError( name, sessionID, _UTF8( "令牌无效" ) );
		return;
	}

	CFBString token			= paramList[ 1 ];
	CFBString userName		= paramList[ 2 ];
	CFBString userRight		= paramList[ 3 ];
	static CFBVector< CFBString > authNameList;
	authNameList.Clear( );
	userRight.Split( ",", authNameList );

	COperUser** userInfo	= tokens.Get( token );
	if ( userInfo == NULL )
	{
		returnError( name, sessionID, _UTF8( "令牌无效" ) );
		return;
	}

	if ( (*userInfo)->mLevel > 1 )
	{
		returnError( name, sessionID, _UTF8( "用户权限不足" ) );
		return;
	}

	COperUser* modifyUser = operUsers.Get( userName );
	if ( modifyUser == NULL )
	{
		returnError( name, sessionID, _UTF8( "用户不存在" ) );
		return;
	}

	modifyUser->mAuthList = authNameList;
	HANDLE handle = CreateFile( _T("operdata/account.odata"), GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( handle != INVALID_HANDLE_VALUE )
	{
		static CFBOctetsStream stream;
		stream.Clear( );
		stream << operUsers;

		DWORD bytesWritten;
		WriteFile( handle, stream.Begin( ), stream.Length( ), &bytesWritten, NULL );
		CloseHandle( handle );
	}

	CFBString retValue = getUserManager( );
	HttpReturnUtf8Helper( name, sessionID, retValue.GetBuffer( ) );
}

void CFBGameModifyPassword::onHttpCommand( const CFBString& name, TSessionID sessionID, const CFBVector< CFBString >& paramList )
{
	COperCenter* oper = COperCenter::GetSingleton( );
	if ( paramList.GetCount( ) != 4 )
	{
		returnError( name, sessionID, _UTF8( "令牌无效" ) );
		return;
	}

	CFBString token			= paramList[ 1 ];
	CFBString userName		= paramList[ 2 ];
	CFBString password		= paramList[ 3 ];

	COperUser** userInfo	= tokens.Get( token );
	if ( userInfo == NULL )
	{
		returnError( name, sessionID, _UTF8( "令牌无效" ) );
		return;
	}

	if ( (*userInfo)->mLevel > 1 )
	{
		returnError( name, sessionID, _UTF8( "用户权限不足" ) );
		return;
	}

	COperUser* modifyUser = operUsers.Get( userName );
	if ( modifyUser == NULL )
	{
		returnError( name, sessionID, _UTF8( "用户不存在" ) );
		return;
	}

	modifyUser->mPassword = password;
	HANDLE handle = CreateFile( _T("operdata/account.odata"), GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( handle != INVALID_HANDLE_VALUE )
	{
		static CFBOctetsStream stream;
		stream.Clear( );
		stream << operUsers;

		DWORD bytesWritten;
		WriteFile( handle, stream.Begin( ), stream.Length( ), &bytesWritten, NULL );
		CloseHandle( handle );
	}

	CFBString retValue = getUserManager( );
	HttpReturnUtf8Helper( name, sessionID, retValue.GetBuffer( ) );
}
