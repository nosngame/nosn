#pragma once			
#include <mysql.h>
#include <mysql_time.h>
namespace NSTool
{
	// ********************************************************************** //
	// CNSDataType
	// ********************************************************************** //

	class CDataDesc
	{
	public:
		CNSString			mName;
		enum_field_types	mTypeID;
		int					mLength;
		char*				mpValue;

	public:
		CDataDesc( ) : mTypeID( MYSQL_TYPE_DECIMAL ), mLength( 0 ), mpValue( NULL )
		{
		}

	public:
		~CDataDesc( )
		{
			if ( mpValue != NULL )
				delete mpValue;
		}
	};

	// ********************************************************************** //
	// CMySQLRecordSet
	// ********************************************************************** //

	class CMySQLRecordSet
	{
	public:
		MYSQL_RES*		mpMySqlRes;
		MYSQL_STMT*		mpStmt;
		MYSQL_BIND*		mpDataBind;
		unsigned long*	mpLength;
		unsigned int	mColumns;
		bool			mHasError;
		my_bool			mIsNull;
		my_bool			mError;

	public:
		CMySQLRecordSet( const CNSString &rSqlCommand, MYSQL_STMT* pStmt );

	public:
		~CMySQLRecordSet( );

	public:
		bool Fetch( );
	};

	// ********************************************************************** //
	// CMySqlDatabase
	// ********************************************************************** //

	class CMySqlDatabase
	{
	public:
		static int		sRefCount;
		MYSQL*			mpMySql;			// Êý¾Ý¿â¾ä±ú
		MYSQL_STMT*		mpStmt;
		CNSString		mSqlCommand;

	public:
		CMySqlDatabase( );

	public:
		~CMySqlDatabase( );

	public:
		int Connect( const CNSString& rHost, const CNSString& rDatabase, const CNSString& rUserName, const CNSString& rPassword, unsigned int vPort = 3306 );
		void Close( );
		int Transaction( );
		int Rollback( );
		int Commit( );
		CNSString GetReason( )
		{
			return CNSString( mysql_error( mpMySql ) );
		}

		CMySQLRecordSet* Fetch( );
		void ping( );
		bool GetFieldName( CNSVector< CNSString >& fieldName );
		bool Execute( const CNSString& rSqlCommand, const CNSOctets &rInputBuffer, CNSString& errorDesc );
		CNSString EscapeString( const CNSString& rSrc );
	};
}