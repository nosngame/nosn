#include <nsbase.h>
#include "database.h"
namespace NSTool
{

	// ********************************************************************** //
	// CMySQLRecordSet
	// ********************************************************************** //

	CMySQLRecordSet::CMySQLRecordSet( const CNSString &rSqlCommand, MYSQL_STMT* pStmt )
		: mpStmt( pStmt ), mpMySqlRes( NULL ), mColumns( 0 ), mpDataBind( NULL ), mpLength( NULL ), mHasError( false ), mIsNull( 0 ), mError( 0 )
	{
		mpMySqlRes = mysql_stmt_result_metadata( pStmt );
		// 如果没有结果集，那么就直接返回
		if ( mpMySqlRes == NULL )
			return;

		mColumns = mysql_stmt_field_count( pStmt );
		mpDataBind = new MYSQL_BIND[ mColumns ];
		mpLength = new unsigned long[ mColumns ];
		memset( mpDataBind, 0, sizeof( MYSQL_BIND ) * mColumns );
		memset( mpLength, 0, sizeof( unsigned long ) * mColumns );

		// 读取每个字段的信息
		MYSQL_FIELD *tpField = NULL;
		int tIndex = 0;
		while ( tpField = mysql_fetch_field( mpMySqlRes ) )
		{
			switch ( tpField->type )
			{
			case MYSQL_TYPE_LONGLONG:
				mpDataBind[ tIndex ].buffer = new long long;
				mpDataBind[ tIndex ].buffer_length = sizeof( long long );
				break;
			case MYSQL_TYPE_LONG:
			case MYSQL_TYPE_FLOAT:
				mpDataBind[ tIndex ].buffer = new char[ sizeof( long ) ];
				mpDataBind[ tIndex ].buffer_length = sizeof( long );
				break;
			case MYSQL_TYPE_TIMESTAMP:
				mpDataBind[ tIndex ].buffer = new char[ sizeof( MYSQL_TIME ) ];
				mpDataBind[ tIndex ].buffer_length = sizeof( MYSQL_TIME );
				break;
			case MYSQL_TYPE_DOUBLE:
				mpDataBind[ tIndex ].buffer = new char[ sizeof( double ) ];
				mpDataBind[ tIndex ].buffer_length = sizeof( double );
				break;
			case MYSQL_TYPE_VARCHAR:
			case MYSQL_TYPE_BLOB:
			case MYSQL_TYPE_MEDIUM_BLOB:
			case MYSQL_TYPE_STRING:
			case MYSQL_TYPE_VAR_STRING:
				mpDataBind[ tIndex ].buffer = new char[ tpField->length ];
				mpDataBind[ tIndex ].buffer_length = tpField->length;
				break;
			}

			mpDataBind[ tIndex ].buffer_type = tpField->type;
			mpDataBind[ tIndex ].is_null = &mIsNull;
			mpDataBind[ tIndex ].error = &mError;
			mpDataBind[ tIndex ].length = &mpLength[ tIndex ];
			tIndex ++;
		}

		if ( mysql_stmt_bind_result( mpStmt, mpDataBind ) != 0 )
		{
			CNSString tErrorDesc = ::mysql_stmt_error( mpStmt );
			NSLog::log( "Execute Mysql Error:\n\t SqlCommand: %s\n\t ErrorDesc: %s", rSqlCommand.getBuffer( ), tErrorDesc.getBuffer( ) );
			mHasError = true;
			return;
		}
	}

	CMySQLRecordSet::~CMySQLRecordSet( )
	{
		for ( unsigned int i = 0; i < mColumns; i ++ )
			delete mpDataBind[ i ].buffer;

		if ( mpMySqlRes != NULL )
			mysql_free_result( mpMySqlRes );

		mpMySqlRes = NULL;

		if ( mpDataBind != NULL )
			delete[] mpDataBind;

		if ( mpLength != NULL )
			delete[] mpLength;
	}

	bool CMySQLRecordSet::Fetch( )
	{
		if ( mpMySqlRes == NULL )
			return false;

		if ( mysql_stmt_fetch( mpStmt ) == MYSQL_NO_DATA )
			return false;

		return true;
	}

	// ********************************************************************** //
	// CMySqlDatabase
	// ********************************************************************** //

	int CMySqlDatabase::sRefCount = 0;
	CMySqlDatabase::CMySqlDatabase( ) : mpMySql( NULL ), mpStmt( NULL )
	{
		// 如果是第一次使用MySqlDatabase, 初始化MySqlDB
		if ( sRefCount == 0 )
			mysql_library_init( 0, NULL, NULL );

		sRefCount ++;
	}

	CMySqlDatabase::~CMySqlDatabase( )
	{
		sRefCount --;
		Close( );
		// 如果没有人使用了，可以释放MySqlDB
		if ( sRefCount == 0 )
			mysql_library_end( );
	}

	int CMySqlDatabase::Connect( const CNSString& rHost, const CNSString& rDatabase, const CNSString& rUserName, const CNSString& rPassword, unsigned int vPort )
	{
		mpMySql = mysql_init( NULL );
		mpStmt = mysql_stmt_init( mpMySql );
		mpMySql->free_me = 0;

		MYSQL* tpMySql = NULL;
		tpMySql = mysql_real_connect( mpMySql, rHost.getBuffer( ), rUserName.getBuffer( ), rPassword.getBuffer( ), rDatabase.getBuffer( ), vPort, NULL, 0 );
		if ( tpMySql == NULL )
			return mysql_errno( mpMySql );

		mysql_set_character_set( mpMySql, "utf8" );

		char reconnect = 1;
		::mysql_options( mpMySql, MYSQL_OPT_RECONNECT, (char*) &reconnect );

		return mysql_errno( mpMySql );
	}

	void CMySqlDatabase::Close( )
	{
		if ( mpMySql != NULL )
			mysql_close( mpMySql );

		if ( mpStmt != NULL )
			mysql_stmt_close( mpStmt );
	}

	int CMySqlDatabase::Transaction( )
	{
		return mysql_real_query( mpMySql, "START TRANSACTION", ( unsigned int ) ::strlen( "START TRANSACTION" ) );
	}

	int CMySqlDatabase::Commit( )
	{
		return mysql_real_query( mpMySql, "COMMIT", ( unsigned int ) ::strlen( "COMMIT" ) );
	}

	int CMySqlDatabase::Rollback( )
	{
		return mysql_real_query( mpMySql, "ROLLBACK", ( unsigned int ) ::strlen( "ROLLBACK" ) );
	}

	CNSString CMySqlDatabase::EscapeString( const CNSString& rSrc )
	{
		char sqlCmd[ 2048 ] = { 0 };
		mysql_real_escape_string( mpMySql, (char*) sqlCmd, (char*) rSrc.getBuffer( ), (unsigned long) rSrc.getLength( ) );
		CNSString retValue = CNSString( sqlCmd );
		return retValue;
	}

	CMySQLRecordSet* CMySqlDatabase::Fetch( )
	{
		CMySQLRecordSet* tpRecord = new CMySQLRecordSet( mSqlCommand, mpStmt );
		if ( tpRecord->mHasError == false && tpRecord->Fetch( ) == true )
			return tpRecord;

		delete tpRecord;
		return NULL;
	}

	void CMySqlDatabase::ping( )
	{
		mysql_ping( mpMySql );
	}

	bool CMySqlDatabase::GetFieldName( CNSVector< CNSString >& fieldInfo )
	{
		MYSQL_RES* tpMySqlRes = mysql_stmt_result_metadata( mpStmt );
		// 如果没有结果集，那么就直接返回
		if ( tpMySqlRes == NULL )
			return false;

		int columns = mysql_stmt_field_count( mpStmt );
		// 读取每个字段的信息
		MYSQL_FIELD* tpField = NULL;
		int tIndex = 0;
		while ( tpField = mysql_fetch_field( tpMySqlRes ) )
		{
			CNSString name( tpField->name, tpField->name_length );
			fieldInfo.pushback( name );
		}

		return true;
	}

	bool CMySqlDatabase::Execute( const CNSString& rSqlCommand, const CNSOctets& rInputBuffer, CNSString& errorDesc )
	{
		mSqlCommand = rSqlCommand;
		if ( mysql_stmt_prepare( mpStmt, mSqlCommand.getBuffer( ), mSqlCommand.getLength( ) ) != 0 )
		{
			CNSString tErrorDesc = ::mysql_stmt_error( mpStmt );
			errorDesc.format( "Execute Mysql Error:\n\t SqlCommand: %s\n\t ErrorDesc: %s", mSqlCommand.getBuffer( ), tErrorDesc.getBuffer( ) );
			return false;
		}

		// 如果有blob数据
		MYSQL_BIND*		tpMySqlInBind = NULL;
		unsigned long*	tpLength = NULL;
		tpMySqlInBind = new MYSQL_BIND;
		tpLength = new unsigned long;
		memset( tpMySqlInBind, 0, sizeof( MYSQL_BIND ) );

		*tpLength = rInputBuffer.length( );
		tpMySqlInBind->buffer_type = MYSQL_TYPE_MEDIUM_BLOB;
		tpMySqlInBind->buffer = (char*) rInputBuffer.begin( );
		tpMySqlInBind->buffer_length = rInputBuffer.length( );
		tpMySqlInBind->length = tpLength;

		if ( mysql_stmt_bind_param( mpStmt, tpMySqlInBind ) != 0 )
		{
			CNSString tErrorDesc = ::mysql_stmt_error( mpStmt );
			errorDesc.format( "Execute Mysql Error:\n\t SqlCommand: %s\n\t ErrorDesc: %s", mSqlCommand.getBuffer( ), tErrorDesc.getBuffer( ) );
			return false;
		}

		if ( mysql_stmt_execute( mpStmt ) != 0 )
		{
			CNSString tErrorDesc = ::mysql_stmt_error( mpStmt );
			errorDesc.format( "Execute Mysql Error:\n\t SqlCommand: %s\n\t ErrorDesc: %s", mSqlCommand.getBuffer( ), tErrorDesc.getBuffer( ) );
			return false;
		}

		if ( mysql_stmt_store_result( mpStmt ) != 0 )
		{
			CNSString tErrorDesc = ::mysql_stmt_error( mpStmt );
			errorDesc.format( "Execute Mysql Error:\n\t SqlCommand: %s\n\t ErrorDesc: %s", mSqlCommand.getBuffer( ), tErrorDesc.getBuffer( ) );
			return false;
		}

		if ( tpMySqlInBind != NULL )
			delete tpMySqlInBind;

		if ( tpLength != NULL )
			delete tpLength;

		return true;
	}
}