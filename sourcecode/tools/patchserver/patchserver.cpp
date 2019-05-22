#include "nsbase.h"
const char* gpPath = NULL;
class CFATable : public CNSMarshal
{
public:
	CNSString		mFileName;
	unsigned int	mAddress;
public:
	virtual CNSOctetsStream& marshal( CNSOctetsStream& rStream ) const
	{
		rStream << mFileName;
		rStream << mAddress;
		return rStream;
	}

	virtual const CNSOctetsStream& unMarshal( const CNSOctetsStream& rStream )
	{
		rStream >> mFileName;
		rStream >> mAddress;
		return rStream;
	}
};

void CreateFats( const CNSString& rPath, CNSVector< CFATable >& rFats, CNSVector< CNSOctets >& rDatas )
{
	WIN32_FIND_DATA ffd;
	HANDLE tFindHandle = FindFirstFile( (TCHAR*) CNSString::toTChar( CNSString( gpPath ) + "/" + rPath + "*.*" ), &ffd );
	if ( tFindHandle == INVALID_HANDLE_VALUE )
		return;

	BOOL tFind = TRUE;
	for ( ; tFind == TRUE; tFind = FindNextFile( tFindHandle, &ffd ) )
	{
		CNSString tFileName = CNSString::fromTChar( (char*) ffd.cFileName );
		if ( tFileName == "." )
			continue;

		if ( tFileName == ".." )
			continue;

		if ( tFileName == ".svn" )
			continue;

		if ( tFileName == "template" )
			continue;

		if ( ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
		{
			CreateFats( rPath + tFileName + "/", rFats, rDatas );
		}
		else
		{
			CFATable tFat;
			tFat.mFileName	= rPath + tFileName;
			tFat.mAddress	= rDatas.getCount( );
			tFat.mFileName.erase( 0 );
			HANDLE tFile	= CreateFile( (TCHAR*) CNSString::toTChar( CNSString( gpPath ) + "/" + tFat.mFileName ), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
			if ( tFile == INVALID_HANDLE_VALUE )
			{
				NSLog::log( _UTF8("文件[%s]打开失败"), tFat.mFileName.getBuffer( ) );
				return;
			}
			DWORD tFileSize = GetFileSize( tFile, NULL );
			CNSOctets tBuffer( tFileSize, tFileSize );
			DWORD tBytesRead;
			ReadFile( tFile, (void*) tBuffer.begin( ), tFileSize, &tBytesRead, NULL );
			CloseHandle( tFile );
			rFats.pushback( tFat );
			rDatas.pushback( tBuffer );
		}
	}

	FindClose( tFindHandle );
}

int main(int argc, char* argv[])
{
	if ( argc != 3 )
	{
		NSException( _UTF8( "启动参数错误" ) );
		return 0;
	}

	gpPath = argv[ 2 ];
	CNSVector< CFATable > tFATs;
	CNSVector< CNSOctets > tDatas;
	CreateFats( "/", tFATs, tDatas );
	CNSOctetsStream tStream;
	tStream << tFATs;
	tStream << tDatas;

	unsigned int tUnCompressSize = tStream.length( );
	CNSOctets tCompressBuffer = tStream.mBuffer.compress( );
	
	CNSOctetsStream tCompressStream;
	tCompressStream << tUnCompressSize;
	tCompressStream << tCompressBuffer;

	HANDLE tResHandle = BeginUpdateResourceA( argv[ 1 ], FALSE );
	UpdateResource( tResHandle, MAKEINTRESOURCE(10), _T("file"), 0, tCompressStream.begin( ), tCompressStream.length( ) );
	EndUpdateResource( tResHandle, FALSE );
	NSLog::log( _UTF8("打包[%s]完成\n"), argv[ 1 ] );
	return 0;
}
