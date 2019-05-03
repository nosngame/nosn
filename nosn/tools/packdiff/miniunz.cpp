/*
   miniunz.c
   Version 1.1, February 14h, 2010
   sample part of the MiniZip project - ( http://www.winimage.com/zLibDll/minizip.html )

		 Copyright (C) 1998-2010 Gilles Vollant (minizip) ( http://www.winimage.com/zLibDll/minizip.html )

		 Modifications of Unzip for Zip64
		 Copyright (C) 2007-2008 Even Rouault

		 Modifications for Zip64 support on both zip and unzip
		 Copyright (C) 2009-2010 Mathias Svensson ( http://result42.com )
*/

#include <nsbase.h>
#include <LzmaLib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <string>
#include <map>
#include <vector>

#ifndef _WIN32
#ifndef __USE_FILE_OFFSET64
#define __USE_FILE_OFFSET64
#endif
#ifndef __USE_LARGEFILE64
#define __USE_LARGEFILE64
#endif
#ifndef _LARGEFILE64_SOURCE
#define _LARGEFILE64_SOURCE
#endif
#ifndef _FILE_OFFSET_BIT
#define _FILE_OFFSET_BIT 64
#endif
#endif

#ifdef unix
# include <unistd.h>
# include <utime.h>
#else
# include <direct.h>
# include <io.h>
#endif

#include "unzip.h"
#include "zip.h"

#define CASESENSITIVITY (0)
#define WRITEBUFFERSIZE (8192)
#define MAXFILENAME (256)

#ifdef _WIN32
#define USEWIN32IOAPI
#include "iowin32.h"
#endif

class zipDiff
{
public:
	unz_file_info64 mFileInfo;
	ZPOS64_T		mOffset;
	unzFile			mZFHandle;
	CNSString		mFileInZip;
	CNSString		mMD5;

public:
	zipDiff( unzFile zf, unz_file_info64 fileInfo, ZPOS64_T offset, const CNSString& rFileInZip, const CNSString& rMD5 )
	{
		mZFHandle = zf;
		mFileInfo = fileInfo;
		mOffset = offset;
		mFileInZip = rFileInZip;
		mMD5 = rMD5;
	}
};

struct AssetBundleFileHead {
	struct LevelInfo {
		unsigned int PackSize;
		unsigned int UncompressedSize;
	};

	CNSString       FileID;
	unsigned int    Version;
	CNSString       MainVersion;
	CNSString       BuildVersion;
	size_t          MinimumStreamedBytes;
	size_t          HeaderSize;
	size_t          NumberOfLevelsToDownloadBeforeStreaming;
	size_t          LevelCount;
	LevelInfo*		 LevelList;
	size_t          CompleteFileSize;
	size_t          FileInfoHeaderSize;
	bool			 Compressed;

public:
	~AssetBundleFileHead( )
	{
		delete[] LevelList;
	}

	void dump( )
	{
		printf( "FileID - %s\n", FileID.getBuffer( ) );
		printf( "Version - %d\n", Version );
		printf( "MainVersion - %s\n", MainVersion.getBuffer( ) );
		printf( "BuildVersion - %s\n", BuildVersion.getBuffer( ) );
		printf( "MinimumStreamedBytes - %zd\n", MinimumStreamedBytes );
		printf( "HeaderSize - %zd\n", HeaderSize );
		printf( "NumberOfLevelsToDownloadBeforeStreaming - %zd\n", NumberOfLevelsToDownloadBeforeStreaming );
		printf( "LevelCount - %zd\n", LevelCount );
		for ( size_t i = 0; i < LevelCount; i ++ )
		{
			printf( "LevelInfo/PackSize - %d\n", LevelList[ i ].PackSize );
			printf( "LevelInfo/UncompressedSize - %d\n", LevelList[ i ].UncompressedSize );
		}
		printf( "CompleteFileSize - %zd\n", CompleteFileSize );
		printf( "FileInfoHeaderSize - %zd\n", FileInfoHeaderSize );
		printf( "Compressed - %d\n", Compressed );
	}
};

struct AssetFileHeader {
	struct AssetFileInfo {
		CNSString name;
		size_t offset;
		size_t length;
	};
	size_t FileCount;
	AssetFileInfo*  File;

public:
	~AssetFileHeader( )
	{
		delete[] File;
	}

	void dump( )
	{
		printf( "FileCount - %zd\n", FileCount );
		for ( size_t i = 0; i < FileCount; i ++ )
		{
			printf( "AssetFileHeader/name - %s\n", File[ i ].name.getBuffer( ) );
			printf( "AssetFileHeader/offset - %zd\n", File[ i ].offset );
			printf( "AssetFileHeader/length - %zd\n", File[ i ].length );
		}
	}
};

struct AssetHeader {
	size_t TypeTreeSize;
	size_t FileSize;
	unsigned int format;
	size_t dataOffset;
	size_t Unknown;
};

CNSString readString( CNSOctets oct, unsigned int& rIndex )
{
	CNSString text;
	for ( size_t i = rIndex; i < oct.length( ); i ++ )
	{
		char* tpText = (char*) oct.begin( ) + i;
		text.pushback( *tpText );
		rIndex ++;
		if ( ( *tpText ) == 0 )
			return text;
	}

	return text;
}

unsigned int readUInt( CNSOctets oct, unsigned int& rIndex )
{
	unsigned int* tpValue = (unsigned int*) ( (char*) oct.begin( ) + rIndex );
	rIndex = rIndex + 4;
	return swapByte32( *tpValue );
}

bool readBool( CNSOctets oct, unsigned int& rIndex )
{
	bool* tpValue = (bool*) ( (char*) oct.begin( ) + rIndex );
	rIndex = rIndex + 1;
	return *tpValue;
}

const char* GetZipBuffer( unzFile uf, unsigned int& rSize )
{
	unz_file_info64 file_info;
	unsigned int size_buf = WRITEBUFFERSIZE;
	unzGetCurrentFileInfo64( uf, &file_info, NULL, 0, NULL, 0, NULL, 0 );

	rSize = (unsigned int&) file_info.uncompressed_size;
	unzOpenCurrentFile( uf );

	char* tpBuffer = new char[ (int) file_info.uncompressed_size ];
	char* tpBuf = new char[ WRITEBUFFERSIZE ];
	int tReadSize = 0;
	int tPointer = 0;
	do
	{
		tReadSize = unzReadCurrentFile( uf, tpBuf, size_buf );
		if ( tReadSize > 0 )
		{
			memcpy( tpBuffer + tPointer, tpBuf, tReadSize );
			tPointer += tReadSize;
		}
	}
	while ( tReadSize > 0 );
	delete tpBuf;
	unzCloseCurrentFile( uf );
	return tpBuffer;
}

static unsigned int CRC32[ 256 ];
static char tableInited = 0;

//初始化表
static void init_table( )
{
	int   i, j;
	unsigned int   crc;
	for ( i = 0; i < 256; i++ )
	{
		crc = i;
		for ( j = 0; j < 8; j++ )
		{
			if ( crc & 1 )
			{
				crc = ( crc >> 1 ) ^ 0xEDB88320;
			}
			else
			{
				crc = crc >> 1;
			}
		}
		CRC32[ i ] = crc;
	}
}

//crc32实现函数
unsigned int crc32( unsigned char *buf, int len )
{
	unsigned int ret = 0xFFFFFFFF;
	int   i;
	if ( tableInited == 0 )
	{
		init_table( );
		tableInited = 1;
	}
	for ( i = 0; i < len; i++ )
	{
		ret = CRC32[ ( ( ret & 0xFF ) ^ buf[ i ] ) ] ^ ( ret >> 8 );
	}
	ret = ~ret;
	return ret;
}

CNSString getFileMD5( unzFile uf )
{
	unz_file_info64 file_info;
	unsigned int size_buf = WRITEBUFFERSIZE;
	unzGetCurrentFileInfo64( uf, &file_info, NULL, 0, NULL, 0, NULL, 0 );
	CNSString result;
	result.format( "crc%x", file_info.crc );
	return result;
	/*	CNSString result;
		unsigned int size;
		const char* tpBuffer = GetZipBuffer( uf, rFileInZip.getBuffer( ), size );
		CNSOctets zipBuffer( tpBuffer, size );
		delete tpBuffer;
		static int t = 0;
		if ( rFileInZip.findFirstOf( "assetbundle" ) != -1 )
		{
			unsigned int index1 = 0;
			AssetBundleFileHead fileInfo;
			fileInfo.FileID					= readString( zipBuffer, index1 );
			fileInfo.Version				= readUInt( zipBuffer, index1 );
			fileInfo.MainVersion			= readString( zipBuffer, index1 );
			fileInfo.BuildVersion			= readString( zipBuffer, index1 );
			fileInfo.MinimumStreamedBytes	= readUInt( zipBuffer, index1 );
			fileInfo.HeaderSize				= readUInt( zipBuffer, index1 );
			fileInfo.NumberOfLevelsToDownloadBeforeStreaming	= readUInt( zipBuffer, index1 );
			fileInfo.LevelCount				= readUInt( zipBuffer, index1 );
			fileInfo.LevelList				= new AssetBundleFileHead::LevelInfo[ fileInfo.LevelCount ];
			for ( size_t i = 0; i < fileInfo.LevelCount; i ++ )
			{
				fileInfo.LevelList[ i ].PackSize			= readUInt( zipBuffer, index1 );
				fileInfo.LevelList[ i ].UncompressedSize	= readUInt( zipBuffer, index1 );
			}
			fileInfo.CompleteFileSize	= readUInt( zipBuffer, index1 );
			fileInfo.FileInfoHeaderSize	= readUInt( zipBuffer, index1 );
			fileInfo.Compressed			= readBool( zipBuffer, index1 );

			for ( size_t i = 0; i < fileInfo.LevelCount; i ++ )
			{
				char* tpBlobInfo = (char*) zipBuffer.begin( ) + index1;
				unsigned int size = *(unsigned int*) ( tpBlobInfo + 5 );
				CNSOctets inBuffer( (char*) zipBuffer.begin( ) + index1 + 13, zipBuffer.End( ) );

				CNSOctets outBuffer( size, size );
				size_t tDesSize = outBuffer.Length( );
				size_t tSrcSize = fileInfo.LevelList[ i ].PackSize;
				unsigned char* tpProp = (unsigned char*) tpBlobInfo;
				size_t tPropSize = 5;
				int tResult = LzmaUncompress( (unsigned char*) outBuffer.begin( ), &tDesSize, (unsigned char*) inBuffer.begin( ), &tSrcSize, tpProp, tPropSize );
				if ( tResult != SZ_OK )
				{
					static CNSString tErrorDesc;
					tErrorDesc.format( _UTF8( "解压缩时发生错误, 错误码: %d" ), tResult );
					NSException( tErrorDesc.getBuffer( ) );
				}

				CNSString fileName;
				fileName.format( "d:\\test_%d.dat", ++ t );
				HANDLE tHandle = CreateFile( fileName.getBuffer( ), GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
				DWORD bytes = 0;
				WriteFile( tHandle, outBuffer.begin( ), outBuffer.Length( ), &bytes, NULL );
				CloseHandle( tHandle );

				unsigned int index2 = 0;
				AssetFileHeader assetFile;
				assetFile.FileCount = readUInt( outBuffer, index2 );
				assetFile.File		= new AssetFileHeader::AssetFileInfo[ assetFile.FileCount ];
				CNSString subResult;
				for ( unsigned int i = 0; i < assetFile.FileCount; i ++ )
				{
					assetFile.File[ i ].name	= readString( outBuffer, index2 );
					assetFile.File[ i ].offset	= readUInt( outBuffer, index2 );
					assetFile.File[ i ].length	= readUInt( outBuffer, index2 );

					AssetHeader assetHeader;
					unsigned int index3 = assetFile.File[ i ].offset;
					assetHeader.TypeTreeSize = readUInt( outBuffer, index3 );
					assetHeader.FileSize = readUInt( outBuffer, index3 );
					assetHeader.format = readUInt( outBuffer, index3 );
					assetHeader.dataOffset = readUInt( outBuffer, index3 );
					assetHeader.Unknown = readUInt( outBuffer, index3 );
				}

				outBuffer.erase( outBuffer.begin( ), (char*) outBuffer.begin( ) + index2 );
				unsigned int crcCode = crc32( (unsigned char*) outBuffer.begin( ), outBuffer.Length( ) );
				subResult.format( "%x", crcCode );
				result = result + "/" + subResult;
			}
		}
		else
		{
			unsigned int crcCode = crc32( (unsigned char*) zipBuffer.begin( ), zipBuffer.Length( ) );
			result.format( "%x", crcCode );
		}

		return result;*/
}

unzFile IterPack( const char* pFileName, std::map< std::string, zipDiff >& rFileList )
{
	zlib_filefunc64_def ffunc;
	fill_win32_filefunc64( &ffunc );

	unzFile uf = unzOpen2_64( CNSString::toTChar( CNSString( pFileName ) ), &ffunc );
	unz_global_info64 gi;
	int err = unzGetGlobalInfo64( uf, &gi );
	for ( int i = 0; i < gi.number_entry; i++ )
	{
		char fileNameInZip[ 256 ];
		unz_file_info64 file_info;
		err = unzGetCurrentFileInfo64( uf, &file_info, fileNameInZip, sizeof( fileNameInZip ), NULL, 0, NULL, 0 );
		CNSString fileName( fileNameInZip );

		if ( fileName.findFirstOf( "Payload" ) != -1 )
		{
			// 如果找到了Payload 说明是IOS
			int startPos = fileName.findFirstOf( "/" );
			int endPos = fileName.findFirstOf( "/", startPos + 1 );
			if ( endPos != -1 )
			{
				fileName.erase( startPos + 1, endPos - 1 );
				// 统一改成client.app
				fileName.insert( startPos + 1, "client.app" );
			}
		}

		if ( ( fileName.findFirstNotOf( "asset/StreamingAssets" ) != -1 || fileName.findFirstOf( "asset/assetbundles" ) != -1 || fileName.findFirstOf( "Data/Raw" ) != -1 || fileName.findFirstOf( "Data/assetbundles" ) != -1 ) && fileName.findFirstOf( ".meta" ) == -1 && fileName.findFirstOf( ".DS_Store" ) == -1 && ( fileName.findFirstOf( ".txt" ) != -1 || fileName.findFirstOf( ".assetbundle" ) != -1 || fileName.findFirstOf( ".xml" ) != -1 || fileName.findFirstOf( ".client" ) != -1 ) )
		{
			ZPOS64_T offset = unzGetOffset64( uf );
			CNSString md5 = getFileMD5( uf );
			rFileList.insert( std::map< std::string, zipDiff >::value_type( std::string( fileName.getBuffer( ) ), zipDiff( uf, file_info, offset, fileName, md5 ) ) );
		}

		if ( ( i + 1 ) < gi.number_entry )
		{
			err = unzGoToNextFile( uf );
			if ( err != UNZ_OK )
			{
				printf( "error %d with zipfile in unzGoToNextFile\n", err );
				break;
			}
		}
	}

	return uf;
}

bool fileDiff( zipDiff srcFile, zipDiff desFile )
{
	return srcFile.mMD5 != desFile.mMD5;
}

void Compare( std::map< std::string, zipDiff >& rSrcList, std::map< std::string, zipDiff >& rDesList, std::map< std::string, zipDiff >& rResult )
{
	std::map< std::string, zipDiff >::iterator tIter = rDesList.begin( );
	for ( ; tIter != rDesList.end( ); tIter ++ )
	{
		std::map< std::string, zipDiff >::iterator tFindIter = rSrcList.find( tIter->first );
		if ( tFindIter == rSrcList.end( ) )
		{
			rResult.insert( std::map< std::string, zipDiff >::value_type( std::string( tIter->first ), tIter->second ) );
		}
		else
		{
			if ( fileDiff( tFindIter->second, tIter->second ) == true )
				rResult.insert( std::map< std::string, zipDiff >::value_type( std::string( tIter->first ), tIter->second ) );
		}
	}
}

int ZipFile( bool vIsFirst, const char* pFileName, unz_file_info64& rFileInfo, const char* pBuffer, unsigned int vSize, const char* pInnerFile )
{
	void* buf = NULL;
	int size_buf = WRITEBUFFERSIZE;
	void* zf;

	zlib_filefunc64_def ffunc;
	fill_win32_filefunc64( &ffunc );
	if ( vIsFirst == true )
		zf = zipOpen2_64( CNSString::toTChar( CNSString( pFileName ) ), 0, NULL, &ffunc );
	else
		zf = zipOpen2_64( CNSString::toTChar( CNSString( pFileName ) ), 2, NULL, &ffunc );

	FILE* fin = NULL;
	int size_read;
	zip_fileinfo zi;
	unsigned long crcFile = 0;
	memcpy( &zi.tmz_date, &rFileInfo.tmu_date, sizeof( rFileInfo.tmu_date ) );
	zi.dosDate = rFileInfo.dosDate;
	zi.internal_fa = rFileInfo.internal_fa;
	zi.external_fa = rFileInfo.external_fa;

	zipOpenNewFileInZip3_64( zf, pInnerFile, &zi,
		NULL, 0, NULL, 0, NULL /* comment*/,
		Z_DEFLATED, Z_DEFAULT_COMPRESSION, 0,
		-MAX_WBITS, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY,
		NULL, 0, 0 );

	int tPointer = 0;
	do
	{
		size_read = min( (int) vSize - tPointer, size_buf );

		if ( size_read > 0 )
		{
			zipWriteInFileInZip( zf, pBuffer + tPointer, size_read );
			tPointer += size_read;
		}
	}
	while ( size_read > 0 );

	zipCloseFileInZip( zf );
	zipClose( zf, NULL );
	return ZIP_OK;
}

class PackParam
{
public:
	std::string mSrcPack;
	std::string mDesPack;
	std::string mPackName;
	unzFile		mDesZip;
	std::map< std::string, zipDiff > mPackList;
};

CRITICAL_SECTION cs;
DWORD PackDiffProc( LPVOID* pParam )
{
	PackParam* tpParam = (PackParam*) pParam;
	const char* pSrcPack = tpParam->mSrcPack.c_str( );
	const char* pDesPack = tpParam->mDesPack.c_str( );
	const char* pPackName = tpParam->mPackName.c_str( );
	unzFile	desUnz = tpParam->mDesZip;
	std::map< std::string, zipDiff > desList = tpParam->mPackList;
	printf( "正在比对差异:\n版本[%s] - 版本[%s]\n", pSrcPack, pDesPack );

	EnterCriticalSection( &cs );
	std::map< std::string, zipDiff > srcList;
	unzFile srcUnz = IterPack( pSrcPack, srcList );

	std::map< std::string, zipDiff > result;
	Compare( srcList, desList, result );
	bool mFirst = true;

	std::map< std::string, zipDiff >::iterator tIter = result.begin( );
	int counter = 0;
	for ( ; tIter != result.end( ); tIter ++ )
	{
		unsigned int size;
		unzSetOffset64( desUnz, tIter->second.mOffset );
		const char* tpBuffer = GetZipBuffer( desUnz, size );
		ZipFile( mFirst, pPackName, tIter->second.mFileInfo, tpBuffer, size, tIter->first.c_str( ) );
		mFirst = false;
		delete tpBuffer;
		counter = counter + 1;
		printf( "写入差异文件: %s\n", tIter->first.c_str( ) );
	}
	LeaveCriticalSection( &cs );
	unzClose( srcUnz );
	printf( "差异:发现%d出不同\n版本[%s] - 版本[%s], 比对完成\n", counter, pSrcPack, pDesPack );
	delete tpParam;
	return 0;
}

class CThreadData
{
public:
	HANDLE	mHandle;
	bool	mComplete;
};

std::map< std::string, zipDiff > desList;
std::vector< CThreadData > gThread;
std::vector< PackParam* > gCache;
unzFile desUnz = NULL;
void PackDiff( const char* pSrcPack, const char* pDesPack, const char* pPackName )
{
	PackParam* tpParam = new PackParam;
	tpParam->mSrcPack = pSrcPack;
	tpParam->mDesPack = pDesPack;
	tpParam->mPackList = desList;
	tpParam->mPackName = pPackName;
	tpParam->mDesZip = desUnz;
	if ( gThread.size( ) >= 4 )
	{
		gCache.push_back( tpParam );
		return;
	}

	DWORD tThreadID;
	HANDLE tThread = CreateThread( NULL, 0, (LPTHREAD_START_ROUTINE) PackDiffProc, (LPVOID) tpParam, 0, &tThreadID );

	CThreadData tData;
	tData.mHandle = tThread;
	tData.mComplete = false;
	gThread.push_back( tData );
}

CNSString GetExecVersion( const char* pVersion )
{
	CNSString tVersion( pVersion );
	int tSIndex = tVersion.findFirstOf( "." );
	int tEIndex = tVersion.findFirstOf( ".", tSIndex + 1 );
	CNSString tBuffer;
	tVersion.copy( tBuffer, tEIndex - tSIndex - 1, tSIndex + 1 );
	return tBuffer;
}

CNSString GetResVersion( const char* pVersion )
{
	CNSString tVersion( pVersion );
	int tSIndex = tVersion.findFirstOf( "." );
	tSIndex = tVersion.findFirstOf( ".", tSIndex + 1 );
	int tEIndex = tVersion.findFirstOf( ".", tSIndex + 1 );
	CNSString tBuffer;
	tVersion.copy( tBuffer, tEIndex - tSIndex - 1, tSIndex + 1 );
	return tBuffer;
}

CNSString GetBigVersion( const char* pVersion )
{
	CNSString tVersion( pVersion );
	int tEIndex = tVersion.findFirstOf( "." );
	CNSString tBuffer;
	tVersion.copy( tBuffer, tEIndex, 0 );
	return tBuffer;
}

void ScanPack( const char* pPath, const char* pNewVersion, const char* pType )
{
	CNSString execVersion = GetExecVersion( pNewVersion );
	CNSString desPack = pPath;
	desPack = desPack + "/" + pNewVersion + pType;
	desUnz = IterPack( desPack.getBuffer( ), desList );
	InitializeCriticalSection( &cs );

	CNSString findPath( ( std::string( pPath ) + "/*.*" ).c_str( ) );
	WIN32_FIND_DATA ffd;
	HANDLE tFindHandle = FindFirstFile( CNSString::toTChar( findPath ), &ffd );
	if ( tFindHandle == INVALID_HANDLE_VALUE )
		return;

	for ( BOOL tFind = TRUE; tFind == TRUE; tFind = FindNextFile( tFindHandle, &ffd ) )
	{
		CNSString fileName = CNSString::fromTChar( ffd.cFileName );
		if ( fileName == "." )
			continue;

		if ( fileName == ".." )
			continue;

		if ( fileName == ".svn" )
			continue;

		if ( fileName.findFirstOf( pType ) != -1 )
		{
			CNSString srcPack = pPath;
			srcPack = srcPack + "/" + fileName;

			CNSString bigVersion = GetBigVersion( fileName.getBuffer( ) );
			CNSString resVersion = GetResVersion( fileName.getBuffer( ) );

			CNSString diffPack = pPath;
			diffPack = diffPack + "/" + bigVersion + "." + execVersion + "." + resVersion + "-" + pNewVersion + ".zip";

			if ( srcPack != desPack )
				PackDiff( srcPack.getBuffer( ), desPack.getBuffer( ), diffPack.getBuffer( ) );
		}
	}

	FindClose( tFindHandle );

	while ( 1 )
	{
		bool tComplete = true;
		for ( std::vector< CThreadData >::iterator tBegin = gThread.begin( ); tBegin != gThread.end( ); tBegin ++ )
		{
			HANDLE tThread = tBegin->mHandle;
			if ( tBegin->mComplete == false )
			{
				tComplete = false;
				if ( WaitForSingleObject( tThread, 0 ) == WAIT_OBJECT_0 )
					tBegin->mComplete = true;
			}
		}

		if ( tComplete == true )
		{
			std::vector< PackParam* > tCache = gCache;
			gThread.clear( );
			gCache.clear( );
			for ( std::vector< PackParam* >::iterator tBegin = tCache.begin( ); tBegin != tCache.end( ); tBegin ++ )
			{
				PackDiff( ( *tBegin )->mSrcPack.c_str( ), ( *tBegin )->mDesPack.c_str( ), ( *tBegin )->mPackName.c_str( ) );
				tComplete = false;
			}

			if ( tComplete == true )
				break;
		}
	}
	unzClose( desUnz );
}

int main( int argc, char* argv[] )
{
	init_table( );
	const char* pPath = argv[ 1 ];
	const char* pVersion = argv[ 2 ];
	const char* pType = argv[ 3 ];
	if ( CNSString( pType ) == "android" )
		ScanPack( pPath, pVersion, ".apk" );

	if ( CNSString( pType ) == "ios" )
		ScanPack( pPath, pVersion, ".ipa" );
}
