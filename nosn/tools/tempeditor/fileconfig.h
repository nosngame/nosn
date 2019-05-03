#ifndef _FILE_CONFIG_H_
#define _FILE_CONFIG_H_
#include "PPSingleton.h"

class FileConfig : public PPSingleton<FileConfig>
{
private:
	// �����ļ�·��
	CNSString					mInputFilePath;
	// ����������ļ�·��
	CNSString					mServerOutputFilePath;
	// �ͻ�������ļ�·��
	CNSString					mClientOutputFilePath;
	// �����ļ�
	CNSVector< CNSString >		mInputFiles;
	// �ļ���С
	int							mMaxOutPutFileSize;

public:
	// ��ʼ��
	bool Init( );

	// ������е��ļ�
	CNSVector< CNSString >& GetInputFiles( );

	// ���server���·��
	CNSString GetServerOutputFilePath( );

	// ���client���·��
	CNSString GetClientOutputFilePath( );

	// ����ļ���С
	int GetMaxOutPutFileSize( );

	// ��һ��Ŀ¼������е��ļ����ļ��У�������Ҫ����ļ��ŵ�VEC��
	void LoadChildFiles( const CNSString& tDirectory );
};

#endif //_FILE_CONFIG_H_

