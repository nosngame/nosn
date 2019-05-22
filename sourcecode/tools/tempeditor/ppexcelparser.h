#ifndef _PP_EXCEL_PARSER_H_
#define _PP_EXCEL_PARSER_H_

#include "PPSingleton.h"
///// ���ת��Ŀ���ļ� �л����С
//#define MAX_LINE_BUFFER_SIZE 7168
/// �����������
#define MAX_CLASS_NAME_SIZE 64
/// ע��������󳤶�
#define MAX_NOTE_DATA_SIZE 1024
/// ��ʽ��������󳤶�
#define MAX_VARIABLE_SIZE (2048)
/// ����ļ���󻺴��С
//#define MAX_OUT_FILE_SIZE   1024*1024*200 // (1024*1024*100)



/// ��������
enum EType
{
	DATA_TYPE_FLOAT = 0,		/// С����������ֵ���;�ΪС�����ͣ�
	DATA_TYPE_STRING,			/// �ַ�
	DATA_TYPE_INVALID,			/// ��Ч��������
};

enum EColumnType
{
	COLUMN_TYPE_NONE = 0,
	COLUMN_TYPE_SERVER,			//server��������
	COLUMN_TYPE_CLIENT,			//client��������
	COLUMN_TYPE_ALL,			//���߶���������
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
	CNSString			mName;				//������������
	CNSString			mText;				//�ļ�������
	EColumnType			mType;				//�����ͣ�server,client,all��
	EType				mDataType;			//���������ͣ�����������Ч�ԣ�
	size_t				mIndex;				//����������Ҫ����������server��client���ݣ������¼����һ��
};

struct RowData
{
	unsigned int			mPrimaryKey;				//����ֵ
	bool					mIsUsed;					// �����Ƿ�ʹ��
	CNSVector< CNSString >	mColumns;			//��ֵ
};

/// ��������
class ExcelData
{
private:
	CNSString				mWorkSheetName;		//ģ������
	CNSVector<ColumnInfo*>	mColumnInfos;		//����Ϣ
	CNSVector<RowData*>		mDatas;				//����
	CNSString				mTemplateFileName;	//ģ���Ӧxml�ļ���

public:
	ExcelData( ) { }

	ExcelData( CNSString& rWorkSheetName );

	//��xml��ȡ�ļ�
	bool LoadDataFromXmlFile( const CNSString& filePath );

	//���
	void Reset( );

	//���ģ������
	CNSString& GetTemplateName( );

	//��÷���������Ϣ
	CNSVector<ColumnInfo*> GetServerColumnsInfo( );

	//��ÿͻ�������Ϣ
	CNSVector<ColumnInfo*> GetClientColumnsInfo( );

	//�����������
	CNSVector<RowData*>& GetDatas( );

	//�����Ч���ݵ�����
	unsigned int GetUsedDataCount( );

private:
	//��ȡģ������
	bool LoadTemplateName( TiXmlElement* pWorkSheetElement );

	//��ȡ����Ϣ�����������飬��ʼ�������ƣ������ͣ������ݣ�����ֵ��
	bool LoadColumnInfo( TiXmlElement* pColumnRowElement );

	//��ȡ����
	bool LoadRowData( TiXmlElement* pDataRowElement );

	//��ȡ��������Ч��
	bool LoadColumnDataValidation( TiXmlElement* pValidateElement );

	//��������Ƿ�����
	bool CheckUpIsKeyContinuous( );

	//����¼�������Ƿ����ÿ����¼�����ֶ�����Ҫһֱ����¼�����ֶ�����������Ҫһ�£�
	bool CheckUpDataCountValidity( );

	//����Ƿ������ֶζ�������������Ч��
	bool CheckUpDataValidity( );

	// ���һ��cellԪ�ص�data����
	const char* GetCellDataValue( TiXmlElement* pCellElement );

	// ���������ͬ��֤���м���,�������û������������Ч�Ե���Ч��
	bool GetSameValidateList( const char* pValidationString, CNSVector<size_t>& rColumnList );
};

class PPExcelParser : public PPSingleton<PPExcelParser>
{
public:
	PPExcelParser( );
	~PPExcelParser( );

	/// ���ý�����
	void Reset( );

	/// �����ļ�
	bool ParseFiles( CNSVector< CNSString >& vInputFiles );

	/// ��ȡ����������������
	CNSOctetsStream GetServerBuffer( )
	{
		return mServerData;
	}

	/// ��ȡ�ͻ�������������
	CNSOctetsStream GetClientBuffer( )
	{
		return mClientData;
	}

private:

	void clear( );

	/// д������
	/// �ַ�������, �ַ�������(������'\0')

	/// ��ȡ������Ҫ������ļ�
	bool AddFiles( CNSVector< CNSString >& vInputFiles );

	/// ��ʽ�����ݣ�������Ӧ�Ļ�����
	void FormatServerData( );
	void FormatClientData( );

private:
	CNSVector<ExcelData*>	mFileDatas;			/// ��ǰ�ļ�����
	CNSOctetsStream			mServerData;		/// ����������������
	CNSOctetsStream			mClientData;		/// �ͻ�������������
};

#endif // _EXCEL_2_BINARY_STREAMS_H_
