#include <nsbase.h>
#include "FileConfig.h"

// 初始化
bool FileConfig::Init( )
{
	mClientOutputFilePath = "";
	mServerOutputFilePath = "";
	mInputFilePath = "";
	return true;
}

// 获得所有的文件
CNSVector< CNSString >& FileConfig::GetInputFiles( )
{
	return mInputFiles;
}

// 获得server输出路径
CNSString FileConfig::GetServerOutputFilePath( )
{
	return mServerOutputFilePath;
}

// 获得client输出路径
CNSString FileConfig::GetClientOutputFilePath( )
{
	return mClientOutputFilePath;
}

int FileConfig::GetMaxOutPutFileSize( )
{
	return mMaxOutPutFileSize;
}

void FileConfig::LoadChildFiles( const CNSString& tDirectory )
{
	WIN32_FIND_DATA FindData;
	//	int tFileCount = 0;
	CNSString tAllPath = tDirectory + "\\*.*";
	HANDLE hError = FindFirstFile( (TCHAR*) CNSString::toTChar( tAllPath ), &FindData );
	if ( hError == INVALID_HANDLE_VALUE )
		NSException( _UTF8( "搜索失败!输入目录无文件" ) );

	while ( ::FindNextFile( hError, &FindData ) )
	{
		// 过虑.和..
		if ( FindData.cFileName[ 0 ] == '.' )
			continue;

		// 构造完整路径
		CNSString tFilePathName = tDirectory + "\\" + CNSString::fromTChar( (char*) FindData.cFileName );
		/// 是XML文件则进行读取
		if ( tFilePathName.findFirstOf( ".xml" ) != -1 )
			mInputFiles.pushback( tFilePathName );

		if ( FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
			LoadChildFiles( tFilePathName );
	}
}