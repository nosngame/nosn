#ifndef _FILE_CONFIG_H_
#define _FILE_CONFIG_H_
#include "PPSingleton.h"

class FileConfig : public PPSingleton<FileConfig>
{
private:
	// 输入文件路径
	CNSString					mInputFilePath;
	// 服务器输出文件路径
	CNSString					mServerOutputFilePath;
	// 客户端输出文件路径
	CNSString					mClientOutputFilePath;
	// 所有文件
	CNSVector< CNSString >		mInputFiles;
	// 文件大小
	int							mMaxOutPutFileSize;

public:
	// 初始化
	bool Init( );

	// 获得所有的文件
	CNSVector< CNSString >& GetInputFiles( );

	// 获得server输出路径
	CNSString GetServerOutputFilePath( );

	// 获得client输出路径
	CNSString GetClientOutputFilePath( );

	// 获得文件大小
	int GetMaxOutPutFileSize( );

	// 从一个目录获得所有的文件和文件夹，将符合要求的文件放到VEC里
	void LoadChildFiles( const CNSString& tDirectory );
};

#endif //_FILE_CONFIG_H_

