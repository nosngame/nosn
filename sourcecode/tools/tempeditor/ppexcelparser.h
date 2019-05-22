#ifndef _PP_EXCEL_PARSER_H_
#define _PP_EXCEL_PARSER_H_

#include "PPSingleton.h"
///// 最大转换目的文件 行缓存大小
//#define MAX_LINE_BUFFER_SIZE 7168
/// 最大类名长度
#define MAX_CLASS_NAME_SIZE 64
/// 注释数据最大长度
#define MAX_NOTE_DATA_SIZE 1024
/// 格式化变量最大长度
#define MAX_VARIABLE_SIZE (2048)
/// 输出文件最大缓存大小
//#define MAX_OUT_FILE_SIZE   1024*1024*200 // (1024*1024*100)



/// 数据类型
enum EType
{
	DATA_TYPE_FLOAT = 0,		/// 小数（所有数值类型均为小数类型）
	DATA_TYPE_STRING,			/// 字符
	DATA_TYPE_INVALID,			/// 无效数据类型
};

enum EColumnType
{
	COLUMN_TYPE_NONE = 0,
	COLUMN_TYPE_SERVER,			//server用数据列
	COLUMN_TYPE_CLIENT,			//client用数据列
	COLUMN_TYPE_ALL,			//两者都用数据列
};

class SceneInfo
{
public:
	CNSString			mSceneID;
	CNSString			mSceneName;
	CNSString			mRegionName;
	CNSString			mResName;
	CNSString			mReSceneID;
	CNSString			mRePosX;
	CNSString			mRePosY;
	CNSString			mObserveRadius;
	CNSString			mObserveGrid;
};

struct ColumnInfo
{
public:
	CNSString			mName;				//程序用列名称
	CNSString			mText;				//文件列名称
	EColumnType			mType;				//列类型（server,client,all）
	EType				mDataType;			//列数据类型（用于数据有效性）
	size_t				mIndex;				//列索引，主要是用于区分server和client数据，方便记录是哪一列
};

struct RowData
{
	unsigned int			mPrimaryKey;				//主键值
	bool					mIsUsed;					// 数据是否被使用
	CNSVector< CNSString >	mColumns;			//列值
};

/// 完整数据
class ExcelData
{
private:
	CNSString				mWorkSheetName;		//模板名称
	CNSVector<ColumnInfo*>	mColumnInfos;		//列信息
	CNSVector<RowData*>		mDatas;				//数据
	CNSString				mTemplateFileName;	//模板对应xml文件名

public:
	ExcelData( ) { }

	ExcelData( CNSString& rWorkSheetName );

	//从xml读取文件
	bool LoadDataFromXmlFile( const CNSString& filePath );

	//清空
	void Reset( );

	//获得模板名称
	CNSString& GetTemplateName( );

	//获得服务器列信息
	CNSVector<ColumnInfo*> GetServerColumnsInfo( );

	//获得客户端列信息
	CNSVector<ColumnInfo*> GetClientColumnsInfo( );

	//获得主键数据
	CNSVector<RowData*>& GetDatas( );

	//获得有效数据的数量
	unsigned int GetUsedDataCount( );

private:
	//读取模板名称
	bool LoadTemplateName( TiXmlElement* pWorkSheetElement );

	//读取列信息（创建列数组，初始化列名称，列类型，列数据，主键值）
	bool LoadColumnInfo( TiXmlElement* pColumnRowElement );

	//读取数据
	bool LoadRowData( TiXmlElement* pDataRowElement );

	//读取列数据有效性
	bool LoadColumnDataValidation( TiXmlElement* pValidateElement );

	//检查主键是否连续
	bool CheckUpIsKeyContinuous( );

	//检查记录的数量是否合理（每条记录的列字段数量要一直，记录的列字段数量和列数要一致）
	bool CheckUpDataCountValidity( );

	//检查是否所有字段都设置了数据有效性
	bool CheckUpDataValidity( );

	// 获得一个cell元素的data数据
	const char* GetCellDataValue( TiXmlElement* pCellElement );

	// 获得所有相同验证的列集合,如果遇到没有整体设置有效性的无效列
	bool GetSameValidateList( const char* pValidationString, CNSVector<size_t>& rColumnList );
};

class PPExcelParser : public PPSingleton<PPExcelParser>
{
public:
	PPExcelParser( );
	~PPExcelParser( );

	/// 重置解析类
	void Reset( );

	/// 解析文件
	bool ParseFiles( CNSVector< CNSString >& vInputFiles );

	/// 获取服务器数据流缓存
	CNSOctetsStream GetServerBuffer( )
	{
		return mServerData;
	}

	/// 获取客户端数据流缓存
	CNSOctetsStream GetClientBuffer( )
	{
		return mClientData;
	}

private:

	void clear( );

	/// 写入数据
	/// 字符串长度, 字符串内容(不包含'\0')

	/// 读取所有需要处理的文件
	bool AddFiles( CNSVector< CNSString >& vInputFiles );

	/// 格式化数据，填入相应的缓存内
	void FormatServerData( );
	void FormatClientData( );

private:
	CNSVector<ExcelData*>	mFileDatas;			/// 当前文件数据
	CNSOctetsStream			mServerData;		/// 服务器需求数据流
	CNSOctetsStream			mClientData;		/// 客户端需求数据流
};

#endif // _EXCEL_2_BINARY_STREAMS_H_
