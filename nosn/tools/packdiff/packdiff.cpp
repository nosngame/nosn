// packdiff.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <fbbase.h>
#include <LzmaLib.h>
struct AssetBundleFileHead {
     struct LevelInfo {
          unsigned int PackSize;
          unsigned int UncompressedSize;
     };

     CFBString       FileID;
     unsigned int    Version;
     CFBString       MainVersion;
     CFBString       BuildVersion;
     size_t          MinimumStreamedBytes;
     size_t          HeaderSize;
     size_t          NumberOfLevelsToDownloadBeforeStreaming;
     size_t          LevelCount;
     LevelInfo*		 LevelList;
     size_t          CompleteFileSize;
     size_t          FileInfoHeaderSize;
     bool			 Compressed;

public:
	void dump( )
	{
		log( "FileID - %s", FileID.GetBuffer( ) );
		log( "Version - %d", Version );
		log( "MainVersion - %s", MainVersion.GetBuffer( ) );
		log( "BuildVersion - %s", BuildVersion.GetBuffer( ) );
		log( "MinimumStreamedBytes - %d", MinimumStreamedBytes );
		log( "HeaderSize - %d", HeaderSize );
		log( "NumberOfLevelsToDownloadBeforeStreaming - %d", NumberOfLevelsToDownloadBeforeStreaming );
		log( "LevelCount - %d", LevelCount );
		for ( size_t i = 0; i < LevelCount; i ++ )
		{
			log( "LevelInfo/PackSize - %d", LevelList[ i ].PackSize );
			log( "LevelInfo/UncompressedSize - %d", LevelList[ i ].UncompressedSize );
		}
		log( "CompleteFileSize - %d", CompleteFileSize );
		log( "FileInfoHeaderSize - %d", FileInfoHeaderSize );
		log( "Compressed - %d", Compressed );
	}
};

struct AssetFileHeader {
     struct AssetFileInfo {
          CFBString name;
          size_t offset;
          size_t length;
     };
     size_t FileCount;
     AssetFileInfo*  File;

public:
	void dump( )
	{
		log( "FileCount - %d", FileCount );
		for ( size_t i = 0; i < FileCount; i ++ )
		{
			log( "AssetFileHeader/name - %s", File[ i ].name.GetBuffer( ) );
			log( "AssetFileHeader/offset - %d", File[ i ].offset );
			log( "AssetFileHeader/length - %d", File[ i ].length );
		}
	}
};

CFBString readString( CFBOctets oct, unsigned int& rIndex )
{
	CFBString text;
	for ( size_t i = rIndex; i < oct.Length( ); i ++ )
	{
		char* tpText = (char*) oct.Begin( ) + i;
		text.PushBack( *tpText );
		rIndex ++;
		if ( (*tpText) == 0 )
			return text;
	}

	return text;
}

unsigned int readUInt( CFBOctets oct, unsigned int& rIndex )
{
	unsigned int* tpValue = (unsigned int*) ( (char*) oct.Begin( ) + rIndex );
	rIndex = rIndex + 4;
	return SwapByte32( *tpValue );
}

bool readBool( CFBOctets oct, unsigned int& rIndex )
{
	bool* tpValue = (bool*) ( (char*) oct.Begin( ) + rIndex );
	rIndex = rIndex + 1;
	return *tpValue;
}

int _tmain(int argc, _TCHAR* argv[])
{
	unsigned int index1 = 0;
	HANDLE fileHandle = CreateFile( "C:/syworld/trunk/client/Assets/assetbundles/script/entry.assetbundle", GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
	DWORD size = GetFileSize( fileHandle, NULL );
	CFBOctets buffer( size, size );
	DWORD byteRead;
	ReadFile( fileHandle, buffer.Begin( ), size, &byteRead, NULL );
	AssetBundleFileHead fileInfo;
	fileInfo.FileID					= readString( buffer, index1 );
	fileInfo.Version				= readUInt( buffer, index1 );
	fileInfo.MainVersion			= readString( buffer, index1 );
	fileInfo.BuildVersion			= readString( buffer, index1 );
	fileInfo.MinimumStreamedBytes	= readUInt( buffer, index1 );
	fileInfo.HeaderSize				= readUInt( buffer, index1 );
	fileInfo.NumberOfLevelsToDownloadBeforeStreaming	= readUInt( buffer, index1 );
	fileInfo.LevelCount	= readUInt( buffer, index1 );
	fileInfo.LevelList = new AssetBundleFileHead::LevelInfo[ fileInfo.LevelCount ];
	for ( size_t i = 0; i < fileInfo.LevelCount; i ++ )
	{
		fileInfo.LevelList[ i ].PackSize			= readUInt( buffer, index1 );
		fileInfo.LevelList[ i ].UncompressedSize	= readUInt( buffer, index1 );
	}
	fileInfo.CompleteFileSize	= readUInt( buffer, index1 );
	fileInfo.FileInfoHeaderSize	= readUInt( buffer, index1 );
	fileInfo.Compressed			= readBool( buffer, index1 );
	fileInfo.dump( );

	for ( size_t i = 0; i < fileInfo.LevelCount; i ++ )
	{
		char* tpBlobInfo = (char*) buffer.Begin( ) + index1;
		unsigned int size = *(unsigned int*) ( tpBlobInfo + 5 );
		CFBOctets inBuffer( (char*) buffer.Begin( ) + index1 + 13, buffer.End( ) );

		CFBOctets outBuffer( size, size );
		size_t tDesSize = outBuffer.Length( );
		size_t tSrcSize = fileInfo.LevelList[ i ].PackSize;
		unsigned char* tpProp = (unsigned char*) tpBlobInfo;
		size_t tPropSize = 5;
		int tResult = LzmaUncompress( (unsigned char*) outBuffer.Begin( ), &tDesSize, (unsigned char*) inBuffer.Begin( ), &tSrcSize, tpProp, tPropSize );
		if ( tResult != SZ_OK )
		{
			static CFBString tErrorDesc;
			tErrorDesc.Format( "解压缩时发生错误, 错误码%d", tResult );
			FBException( tErrorDesc.GetBuffer( ) );
		}

		unsigned int index2 = 0;
		AssetFileHeader assetFile;
		assetFile.FileCount = readUInt( outBuffer, index2 );
		assetFile.File		= new AssetFileHeader::AssetFileInfo[ assetFile.FileCount ];
		for ( unsigned int i = 0; i < assetFile.FileCount; i ++ )
		{
			assetFile.File[ i ].name = readString( outBuffer, index2 );
			assetFile.File[ i ].offset = readUInt( outBuffer, index2 );
			assetFile.File[ i ].length = readUInt( outBuffer, index2 );
		}

		outBuffer.Erase( outBuffer.Begin( ), (char*) outBuffer.Begin( ) + index2 );
		assetFile.dump( );
		CFBString result;
		outBuffer.Encode( 0, result );
		log( "%s", result.GetBuffer( ) );
	}

	CloseHandle( fileHandle );
	return 0;
}

