#include <nsbase.h>
#include "FileConfig.h"

// ��ʼ��
bool FileConfig::Init( )
{
	mClientOutputFilePath = "";
	mServerOutputFilePath = "";
	mInputFilePath = "";
	return true;
}

// ������е��ļ�
CNSVector< CNSString >& FileConfig::GetInputFiles( )
{
	return mInputFiles;
}

// ���server���·��
CNSString FileConfig::GetServerOutputFilePath( )
{
	return mServerOutputFilePath;
}

// ���client���·��
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
		NSException( _UTF8( "����ʧ��!����Ŀ¼���ļ�" ) );

	while ( ::FindNextFile( hError, &FindData ) )
	{
		// ����.��..
		if ( FindData.cFileName[ 0 ] == '.' )
			continue;

		// ��������·��
		CNSString tFilePathName = tDirectory + "\\" + CNSString::fromTChar( (char*) FindData.cFileName );
		/// ��XML�ļ�����ж�ȡ
		if ( tFilePathName.findFirstOf( ".xml" ) != -1 )
			mInputFiles.pushback( tFilePathName );

		if ( FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
			LoadChildFiles( tFilePathName );
	}
}