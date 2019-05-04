#include <nsbase.h>
#include "PPExcelParser.h"
#include <fstream>
#include "FileConfig.h"

static unsigned int MAX_OUT_FILE_SIZE = 1024 * 1024 * 100;
/// ��Ч�Ա�ʶ
const static char* g_pEffective = "TPLDATA";

ExcelData::ExcelData( CNSString& rWorkSheetName ) :mWorkSheetName( rWorkSheetName )
{

}

//��xml��ȡ�ļ�
bool ExcelData::LoadDataFromXmlFile( const CNSString& filePath )
{
	mTemplateFileName = filePath;
	CNSFile xml;
	xml.openExist( filePath );
	CNSOctets buffer;
	xml.readAllBytes( buffer );

	TiXmlDocument tXmlDoc;
	if ( tXmlDoc.Parse( (char*) buffer.begin( ) ) == NULL )
	{
		NSLog::log( _UTF8( "��Ч��ģ���ļ�%s" ), mTemplateFileName.getBuffer( ) );
		return false;
	}

	TiXmlElement* tpRootElement = tXmlDoc.RootElement( );
	if ( tpRootElement == NULL )
		return false;

	//��ȡģ������
	TiXmlElement* tpWorkSheetElement = tpRootElement->FirstChildElement( "Worksheet" );
	if ( LoadTemplateName( tpWorkSheetElement ) == false )
		return false;

	TiXmlElement* tpTableElement = tpWorkSheetElement->FirstChildElement( "Table" );
	if ( tpTableElement == NULL )
	{
		NSLog::log( _UTF8( "��Ч��ģ���ļ�%s" ), mTemplateFileName.getBuffer( ) );
		return false;
	}

	//��ȡģ������Ϣ
	TiXmlElement* tpColumnRowElement = tpTableElement->FirstChildElement( "Row" );
	if ( LoadColumnInfo( tpColumnRowElement ) == false )
		return false;

	//��ȡģ������
	TiXmlElement* tpDataRowElement = tpColumnRowElement->NextSiblingElement( "Row" );
	while ( tpDataRowElement != NULL )
	{
		if ( LoadRowData( tpDataRowElement ) == false )
			return false;

		tpDataRowElement = tpDataRowElement->NextSiblingElement( "Row" );
	}

	//��ȡģ��������Ч��
	TiXmlElement* tpValidationElement = tpWorkSheetElement->FirstChildElement( "DataValidation" );
	while ( tpValidationElement != NULL )
	{
		if ( LoadColumnDataValidation( tpValidationElement ) == false )
			return false;

		tpValidationElement = tpValidationElement->NextSiblingElement( "DataValidation" );
	}

	//��֤ģ����Ч��
	if ( CheckUpIsKeyContinuous( ) == false )
		return false;

	if ( CheckUpDataCountValidity( ) == false )
		return false;

	if ( CheckUpDataValidity( ) == false )
		return false;

	return true;
}

//���
void ExcelData::Reset( )
{
	mWorkSheetName.clear( );

	for ( size_t i = 0; i < mDatas.getCount( ); i ++ )
	{
		if ( mDatas[ i ] != NULL )
			delete( mDatas[ i ] );
	}
	mDatas.clear( );

	for ( size_t i = 0; i < mColumnInfos.getCount( ); i ++ )
	{
		if ( mColumnInfos[ i ] != NULL )
			delete( mColumnInfos[ i ] );
	}
	mColumnInfos.clear( );
}

//���ģ������
CNSString& ExcelData::GetTemplateName( )
{
	return mWorkSheetName;
}

//��÷���������Ϣ
CNSVector<ColumnInfo*> ExcelData::GetServerColumnsInfo( )
{
	CNSVector<ColumnInfo*> tServerColumnList;
	for ( size_t i = 0; i < mColumnInfos.getCount( ); i ++ )
	{
		if ( mColumnInfos[ i ]->mType == COLUMN_TYPE_SERVER || mColumnInfos[ i ]->mType == COLUMN_TYPE_ALL )
		{
			tServerColumnList.pushback( mColumnInfos[ i ] );
		}
	}

	return tServerColumnList;
}

//��ÿͻ�������Ϣ
CNSVector<ColumnInfo*> ExcelData::GetClientColumnsInfo( )
{
	CNSVector<ColumnInfo*> tClientColumnList;
	for ( size_t i = 0; i < mColumnInfos.getCount( ); i ++ )
	{
		if ( mColumnInfos[ i ]->mType == COLUMN_TYPE_CLIENT || mColumnInfos[ i ]->mType == COLUMN_TYPE_ALL )
		{
			tClientColumnList.pushback( mColumnInfos[ i ] );
		}
	}

	return tClientColumnList;
}

//�����������
CNSVector<RowData*>& ExcelData::GetDatas( )
{
	return mDatas;
}

//�����Ч���ݵ�����
unsigned int ExcelData::GetUsedDataCount( )
{
	CNSVector<RowData*>& tDatas = GetDatas( );
	unsigned int tTotalCount = 0;
	for ( size_t i = 0; i < tDatas.getCount( ); i ++ )
	{
		RowData* tpData = tDatas[ i ];
		if ( tpData->mIsUsed == true )
		{
			tTotalCount = tTotalCount + 1;
		}
	}

	return tTotalCount;
}

//��ȡģ������
bool ExcelData::LoadTemplateName( TiXmlElement* pWorkSheetElement )
{
	if ( pWorkSheetElement == NULL )
	{
		return false;
	}
	mWorkSheetName = pWorkSheetElement->Attribute( "ss:Name" );
	return true;
}

//��ȡ�����ݣ����������飬��ʼ�������ƣ������ͣ������ݣ�����ֵ��
bool ExcelData::LoadColumnInfo( TiXmlElement* pColumnRowElement )
{
	if ( pColumnRowElement == NULL )
	{
		NSLog::log( _UTF8( "��Ч��ģ���ļ�%s" ), mTemplateFileName.getBuffer( ) );
		return false;
	}

	TiXmlElement* tpIDCellElement = pColumnRowElement->FirstChildElement( "Cell" );
	//�жϵ�һ��֮ǰ�Ƿ��п��У���ss:index��ֵ������û��cell�ֶ����ж�Ϊ��һ��֮ǰ�Ƿ��п���
	if ( tpIDCellElement == NULL || pColumnRowElement->Attribute( "ss:Index" ) != NULL )
	{
		NSLog::log( _UTF8( "�ļ�[%s],��һ��֮ǰ�п���" ), mTemplateFileName.getBuffer( ) );
		return false;
	}

	/// �ж������׸�Cell�Ƿ����ʡ����
	if ( tpIDCellElement->Attribute( "ss:Index" ) != NULL )
	{
		NSLog::log( _UTF8( "�ļ�[%s]����һ��Ч�в��ڵ�һ����" ), mTemplateFileName.getBuffer( ) );
		return false;
	}

	// �������Ϣ
	TiXmlElement* tpInfoCellElement = tpIDCellElement->NextSiblingElement( "Cell" );
	if ( tpInfoCellElement == NULL )
	{
		NSLog::log( _UTF8( "�ļ�[%s]��û���κ�һ����Ч��" ), mTemplateFileName.getBuffer( ) );
		return false;
	}

	size_t tColumnIndex = 0;
	while ( tpInfoCellElement )
	{
		if ( tpInfoCellElement->Attribute( "ss:Index" ) != NULL )
		{
			NSLog::log( _UTF8( "�ļ�[%s]������Ϣ�м��п���" ), mTemplateFileName.getBuffer( ) );
			return false;
		}

		TiXmlElement* tpDataElement = tpInfoCellElement->FirstChildElement( "Data" );
		if ( tpDataElement == NULL )
		{
			NSLog::log( _UTF8( "�ļ�[%s]������Ϣȱʧ" ), mTemplateFileName.getBuffer( ) );
			return false;
		}

		// �жϵ�ǰ�е������ı��Ƿ���Ч(_C,_S,_CS)
		CNSString tColumnText( tpDataElement->GetText( ) );
		EColumnType tColumnType = COLUMN_TYPE_NONE;
		if ( tColumnText.findFirstOf( "_CS" ) != -1 )
		{
			tColumnType = COLUMN_TYPE_ALL;
		}
		else if ( tColumnText.findFirstOf( "_S" ) != -1 )
		{
			tColumnType = COLUMN_TYPE_SERVER;
		}
		else if ( tColumnText.findFirstOf( "_C" ) != -1 )
		{
			tColumnType = COLUMN_TYPE_CLIENT;
		}
		if ( tColumnType == COLUMN_TYPE_NONE )
		{
			NSLog::log( _UTF8( "�ļ�[%s]������Ч��������[%s]" ), mTemplateFileName.getBuffer( ), tColumnText.getBuffer( ) );
			return false;
		}

		TiXmlElement* tpColumnNameElement = tpInfoCellElement->FirstChildElement( "NamedCell" );
		if ( tpColumnNameElement == NULL )
		{
			NSLog::log( _UTF8( "�ļ�[%s]��������[%s]��Ӧ��tagID��ȱʧ" ), mTemplateFileName.getBuffer( ), tColumnText.getBuffer( ) );
			return false;
		}

		bool tHasAnotherTag = false;
		TiXmlElement* tpColumnNameElement2 = tpColumnNameElement->NextSiblingElement( "NamedCell" );
		while ( tpColumnNameElement2 != NULL )
		{
			CNSString tColumnName2( tpColumnNameElement2->Attribute( "ss:Name" ) );
			if ( tColumnName2 != "_FilterDatabase" )
			{
				tHasAnotherTag = true;
				break;
			}
			tpColumnNameElement2 = tpColumnNameElement2->NextSiblingElement( "NamedCell" );
		}

		if ( tHasAnotherTag == true )
		{
			NSLog::log( _UTF8( "�ļ�[%s]��������[%s]��������֮��Ӧ��tagID������" ), mTemplateFileName.getBuffer( ), tColumnText.getBuffer( ) );
			return false;
		}

		CNSString tColumnName( tpColumnNameElement->Attribute( "ss:Name" ) );

		ColumnInfo* tpColumnInfo = new ColumnInfo;
		tpColumnInfo->mName = tColumnName;
		tpColumnInfo->mText = tColumnText;
		tpColumnInfo->mType = tColumnType;
		tpColumnInfo->mIndex = tColumnIndex;
		tpColumnInfo->mDataType = DATA_TYPE_INVALID;
		mColumnInfos.pushback( tpColumnInfo );

		tpInfoCellElement = tpInfoCellElement->NextSiblingElement( "Cell" );
		tColumnIndex ++;
	}
	return true;
}

//��ȡ����
bool ExcelData::LoadRowData( TiXmlElement* pDataRowElement )
{
	if ( mColumnInfos.getCount( ) == 0 )
		return false;

	size_t tCurrentRowIndex = mDatas.getCount( );
	if ( pDataRowElement == NULL )
	{
		NSLog::log( _UTF8( "��Ч��ģ���ļ�[%s]" ), mTemplateFileName.getBuffer( ) );
		return false;
	}

	TiXmlElement* tpPrimaryCellElement = pDataRowElement->FirstChildElement( "Cell" );
	//�ж�����������֮���Ƿ��п��У���ss:index��ֵ������û��cell�ֶ����ж���֮���Ƿ��п���
	if ( tpPrimaryCellElement == NULL || pDataRowElement->Attribute( "ss:Index" ) != NULL )
	{
		NSLog::log( _UTF8( "�ļ�[%s],��%u���ǿ���" ), mTemplateFileName.getBuffer( ), (int) tCurrentRowIndex + 2 );
		return false;
	}

	/// �жϵ�ǰ���������Ƿ��ǿ�
	if ( tpPrimaryCellElement->Attribute( "ss:Index" ) != NULL )
	{
		NSLog::log( _UTF8( "�ļ�[%s]����%u�У��������ǿ�ֵ" ), mTemplateFileName.getBuffer( ), (int) tCurrentRowIndex + 2 );
		return false;
	}
	TiXmlElement* tpDataElement = tpPrimaryCellElement->FirstChildElement( "Data" );
	if ( tpDataElement == NULL )
	{
		NSLog::log( _UTF8( "�ļ�[%s]����%u�У����ݴ���" ), mTemplateFileName.getBuffer( ), (int) tCurrentRowIndex + 2 );
		return false;
	}

	CNSString tPrimaryKey( tpDataElement->GetText( ) );

	// ����ֵ��Ϊ�գ��򴴽��������ݣ����ҳ�ʼ����������Ϊ"",�������ҵ�'!'�����м�¼��Ϊ���ϼ�¼������������������ļ�
	RowData* tpRowData = new RowData;

	size_t tStartPos = 0;
	size_t tFindPos = tPrimaryKey.findFirstOf( '!' );
	if ( tFindPos != -1 )
	{
		if ( tFindPos > 0 )
		{
			CNSString tKeyStr;
			tPrimaryKey.copy( tKeyStr, tFindPos - tStartPos, tStartPos );
			tpRowData->mPrimaryKey = atoi( tKeyStr.getBuffer( ) );
			tpRowData->mIsUsed = false;
		}
		else
		{
			NSLog::log( _UTF8( "�ļ�[%s]����%u�У������н�֧����������ֵ֮��������ϱ��" ), mTemplateFileName.getBuffer( ), (int) tCurrentRowIndex + 2 );
			return false;
		}
	}
	else
	{
		tpRowData->mPrimaryKey = atoi( tPrimaryKey.getBuffer( ) );
		tpRowData->mIsUsed = true;
	}

	for ( size_t i = 0; i < mColumnInfos.getCount( ); i ++ )
	{
		tpRowData->mColumns.pushback( "" );
	}

	// �������Ϣ
	TiXmlElement* tpDataCellElement = tpPrimaryCellElement->NextSiblingElement( "Cell" );
	size_t tCurrentColumnIndex = 0;
	while ( tpDataCellElement != NULL )
	{
		// ����ss:Index == NULL˵���ǵ�ǰ�У�����˵��indexֵǰ�����ȫΪ�գ� 
		const char* tpSSIndex = tpDataCellElement->Attribute( "ss:Index" );
		if ( tpSSIndex != NULL )
		{
			tCurrentColumnIndex = atoi( tpSSIndex ) - 2;
		}

		const char* tpDataStr = GetCellDataValue( tpDataCellElement );
		if ( tpDataStr != NULL )
		{
			CNSString tData( tpDataStr );
			if ( tCurrentColumnIndex >= tpRowData->mColumns.getCount( ) )
				break;

			tpRowData->mColumns[ tCurrentColumnIndex ] = tData;
		}

		tpDataCellElement = tpDataCellElement->NextSiblingElement( "Cell" );
		tCurrentColumnIndex ++;
	}
	mDatas.pushback( tpRowData );
	return true;
}

//��ȡ��������Ч��
bool ExcelData::LoadColumnDataValidation( TiXmlElement* pValidateElement )
{
	if ( mColumnInfos.getCount( ) == 0 )
		return false;

	if ( pValidateElement == NULL )
	{
		NSLog::log( _UTF8( "��Ч��ģ���ļ�[%s]" ), mTemplateFileName.getBuffer( ) );
		return false;
	}

	//��ȡӰ����ֵ
	TiXmlElement* tpRangeElement = pValidateElement->FirstChildElement( "Range" );
	if ( tpRangeElement == NULL )
	{
		NSLog::log( _UTF8( "��Ч��ģ���ļ�[%s]" ), mTemplateFileName.getBuffer( ) );
		return false;
	}

	const char* tpRangeValue = tpRangeElement->GetText( );
	CNSVector<size_t> tEffectedColumns;
	if ( GetSameValidateList( tpRangeValue, tEffectedColumns ) == false )
	{
		NSLog::log( _UTF8( "�ļ�[%s],��ȡ������Ч��ʱ����" ), mTemplateFileName.getBuffer( ) );
		return false;
	}

	//��ȡ����ֵ
	TiXmlElement* tpTypeElement = pValidateElement->FirstChildElement( "Type" );
	if ( tpTypeElement == NULL )
	{
		NSLog::log( _UTF8( "��Ч��ģ���ļ�[%s]" ), mTemplateFileName.getBuffer( ) );
		return false;
	}

	CNSString tTypeValue = tpTypeElement->GetText( );
	for ( size_t i = 0; i < tEffectedColumns.getCount( ); i ++ )
	{
		if ( tTypeValue == "TextLength" )  /// �ı�����
		{
			mColumnInfos[ tEffectedColumns[ i ] ]->mDataType = DATA_TYPE_STRING;
		}
		else if ( tTypeValue == "Whole" || tTypeValue == "Decimal" ) /// Float����
		{
			mColumnInfos[ tEffectedColumns[ i ] ]->mDataType = DATA_TYPE_FLOAT;
		}
	}
	return true;
}

bool ExcelData::CheckUpIsKeyContinuous( )
{
	for ( size_t i = 0; i < mDatas.getCount( ); i ++ )
	{
		if ( ( i + 1 ) != mDatas[ i ]->mPrimaryKey )
		{
			NSLog::log( _UTF8( "�ļ�[%s]��������������������û�д�Ĭ��1��ʼ����������[%u]��" ), mTemplateFileName.getBuffer( ), (int) i );
			return false;
		}
	}
	return true;
}

//����¼�������Ƿ����ÿ����¼�����ֶ�����Ҫһֱ����¼�����ֶ�����������Ҫһ�£�
bool ExcelData::CheckUpDataCountValidity( )
{
	if ( mDatas.getCount( ) == 0 )
	{
		NSLog::log( _UTF8( "�ļ�[%s]��û������" ), mTemplateFileName.getBuffer( ) );
		return false;
	}

	if ( mColumnInfos.getCount( ) == 0 )
	{
		NSLog::log( _UTF8( "�ļ�[%s]��û����Ч��" ), mTemplateFileName.getBuffer( ) );
		return false;
	}

	size_t tFirstRowDataColumnCount = mDatas[ 0 ]->mColumns.getCount( );
	size_t tColumnCount = mColumnInfos.getCount( );

	for ( size_t i = 0; i < mDatas.getCount( ); i ++ )
	{
		if ( mDatas[ i ]->mColumns.getCount( ) != tFirstRowDataColumnCount || mDatas[ i ]->mColumns.getCount( ) != tColumnCount )
		{
			NSLog::log( _UTF8( "�ļ�[%s]��������Ч" ), mTemplateFileName.getBuffer( ) );
			return false;
		}
	}

	return true;
}

bool ExcelData::CheckUpDataValidity( )
{
	for ( size_t i = 0; i < mColumnInfos.getCount( ); i ++ )
	{
		if ( mColumnInfos[ i ]->mDataType == DATA_TYPE_INVALID )
		{
			NSLog::log( _UTF8( "�ļ�[%s]��������Ч�Գ�������[%u]�У����ƿ�Ϊ[%s]" ), mTemplateFileName.getBuffer( ), (int) i + 2, mColumnInfos[ i ]->mText.getBuffer( ) );
			return false;
		}
	}
	return true;
}

const char* ExcelData::GetCellDataValue( TiXmlElement* pCellElement )
{
	if ( pCellElement == NULL )
		return NULL;

	TiXmlElement* tpDataElement = pCellElement->FirstChildElement( "Data" );
	if ( tpDataElement != NULL )
		return tpDataElement->GetText( );

	tpDataElement = pCellElement->FirstChildElement( "ss:Data" );
	if ( tpDataElement == NULL )
		return NULL;

	const char* tpText = tpDataElement->GetText( );
	if ( tpText != NULL )
		return tpText;

	TiXmlElement* tpFontElement = tpDataElement->FirstChildElement( "Font" );
	if ( tpFontElement == NULL )
		return NULL;

	return tpFontElement->GetText( );
}

bool ExcelData::GetSameValidateList( const char* pValidationString, CNSVector<size_t>& rColumnList )
{
	rColumnList.clear( );
	CNSString tValidationString( pValidationString );
	tValidationString += ',';
	size_t tStartPos = 0;
	size_t tFindPos = tValidationString.findFirstOf( ',' );
	while ( tFindPos != -1 )
	{
		CNSString tColumns;
		tValidationString.copy( tColumns, tFindPos - tStartPos, tStartPos );
		size_t tSplitPos = tColumns.findFirstOf( ':' );
		if ( tSplitPos == -1 )
		{
			size_t tColumnNumberPos = tColumns.findFirstOf( 'C' );
			if ( tColumnNumberPos != -1 )
			{
				CNSString tSingleColumnString;
				// todo
				tColumns.copy( tSingleColumnString, -1, tColumnNumberPos + 1 );
				int tSingleColumnInt = atoi( tSingleColumnString.getBuffer( ) ) - 2;
				if ( tSingleColumnInt == -1 )
					NSLog::log( _UTF8( "���棺�ļ�[%s],������������������Ч�ԣ����Ѻ��ԡ�" ), mTemplateFileName.getBuffer( ) );
				else
					rColumnList.pushback( (size_t) tSingleColumnInt );
			}
		}
		else
		{
			CNSString tBeginColumnStringAll;
			tColumns.copy( tBeginColumnStringAll, tSplitPos, 0 );
			size_t tBeginColumnNumberPos = tBeginColumnStringAll.findFirstOf( 'C' );
			size_t tBeginRowNumberPos = tBeginColumnStringAll.findFirstOf( 'R' );

			CNSString tEndColumnStringAll;
			tColumns.copy( tEndColumnStringAll, -1, tSplitPos + 1 );
			size_t tEndColumnNumberPos = tEndColumnStringAll.findFirstOf( 'C' );
			size_t tEndRowNumberPos = tEndColumnStringAll.findFirstOf( 'R' );

			if ( tBeginColumnNumberPos != -1 && tEndColumnNumberPos != -1 )
			{
				CNSString tBeginColumnString;
				tBeginColumnStringAll.copy( tBeginColumnString, -1, tBeginColumnNumberPos + 1 );
				int tBeginColumnInt = atoi( tBeginColumnString.getBuffer( ) ) - 2;

				CNSString tEndColumnString;
				tEndColumnStringAll.copy( tEndColumnString, -1, tEndColumnNumberPos + 1 );
				int tEndColumnInt = atoi( tEndColumnString.getBuffer( ) ) - 2;

				if ( tBeginRowNumberPos != -1 )
				{
					NSLog::log( _UTF8( "�ļ�[%s],��%u��������Ч�����ô���������һ������������Ч��" ), mTemplateFileName.getBuffer( ), tBeginColumnInt + 2 );
					return false;
				}

				if ( tEndRowNumberPos != -1 )
				{
					NSLog::log( _UTF8( "�ļ�[%s],��%u��������Ч�����ô���������һ������������Ч��" ), mTemplateFileName.getBuffer( ), tEndColumnInt + 2 );
					return false;
				}

				for ( int i = tBeginColumnInt; i <= tEndColumnInt; i ++ )
				{
					if ( i == -1 )
						NSLog::log( _UTF8( "���棺�ļ�[%s],������������������Ч�ԣ����Ѻ���" ), mTemplateFileName.getBuffer( ) );
					else
						rColumnList.pushback( (size_t) i );
				}
			}
		}

		tStartPos = tFindPos + 1;
		tFindPos = tValidationString.findFirstOf( ',', tStartPos );
	}
	return true;
}

PPExcelParser::PPExcelParser( )
{

}

PPExcelParser::~PPExcelParser( )
{
	clear( );
}

void PPExcelParser::Reset( )
{
	clear( );
}

void PPExcelParser::clear( )
{
	for ( size_t i = 0; i < mFileDatas.getCount( ); i ++ )
	{
		if ( mFileDatas[ i ] != NULL )
			delete mFileDatas[ i ];
	}
	mFileDatas.clear( );
}

/// �����ļ�
bool PPExcelParser::ParseFiles( CNSVector< CNSString >& vInputFiles )
{
	Reset( );
	if ( AddFiles( vInputFiles ) == false )
		return false;

	FormatServerData( );
	FormatClientData( );
	return true;
}

/// ��ȡ������Ҫ������ļ�
bool PPExcelParser::AddFiles( CNSVector< CNSString >& vInputFiles )
{
	for ( size_t i = 0; i < vInputFiles.getCount( ); i ++ )
	{
		CNSString& tfileName = vInputFiles[ i ];
		ExcelData* tpExcelData = new ExcelData;
		NSLog::log( _UTF8( "��ʼ�����ļ�[%s]" ), tfileName.getBuffer( ) );
		if ( tpExcelData->LoadDataFromXmlFile( tfileName ) == false )
		{
			NSLog::log( _UTF8( "��ȡxml�ļ�ʧ��[%s]" ), tfileName.getBuffer( ) );
			return false;
		}

		mFileDatas.pushback( tpExcelData );
	}

	return true;
}

void PPExcelParser::FormatServerData( )
{
	mServerData << (char) EDataType::TYPE_TABLE;

	/// дģ������
	mServerData << (char) EDataType::TYPE_STRING;
	mServerData << CNSString( "tempInfo" );
	mServerData << (char) EDataType::TYPE_TABLE;
	for ( size_t i = 0; i < mFileDatas.getCount( ); i ++ )
	{
		ExcelData* tpTemplateFile = mFileDatas[ i ];

		/// д��ģ������
		mServerData << (char) EDataType::TYPE_STRING;
		mServerData << tpTemplateFile->GetTemplateName( );
		mServerData << (char) EDataType::TYPE_FLOAT;
		mServerData << (float) i + 1;
	}
	mServerData << (char) 0;

	mServerData << (char) EDataType::TYPE_STRING;
	mServerData << CNSString( "fieldInfo" );
	mServerData << (char) EDataType::TYPE_TABLE;
	for ( size_t i = 0; i < mFileDatas.getCount( ); i ++ )
	{
		ExcelData* tpTemplateFile = mFileDatas[ i ];

		CNSVector<ColumnInfo*>& tServerColumnList = tpTemplateFile->GetServerColumnsInfo( );
		mServerData << (char) EDataType::TYPE_FLOAT;
		mServerData << (float) i + 1;
		mServerData << (char) EDataType::TYPE_TABLE;
		/// д���ֶ������ֶ�����
		for ( size_t i = 0; i < tServerColumnList.getCount( ); i ++ )
		{
			CNSString& tFieldName = tServerColumnList[ i ]->mName;
			EType tFieldType = tServerColumnList[ i ]->mDataType;

			/// �ֶ�����
			mServerData << (char) EDataType::TYPE_STRING;
			mServerData << tFieldName;
			mServerData << (char) EDataType::TYPE_FLOAT;
			mServerData << (float) i + 1;
		}
		mServerData << (char) 0;
	}
	mServerData << (char) 0;

	mServerData << (char) EDataType::TYPE_STRING;
	mServerData << CNSString( "data" );
	mServerData << (char) EDataType::TYPE_TABLE;
	for ( size_t i = 0; i < mFileDatas.getCount( ); i ++ )
	{
		ExcelData* tpTemplateFile = mFileDatas[ i ];

		CNSVector<ColumnInfo*>& tServerColumnList = tpTemplateFile->GetServerColumnsInfo( );
		CNSVector<RowData*>& tDataList = tpTemplateFile->GetDatas( );
		mServerData << (char) EDataType::TYPE_FLOAT;
		mServerData << (float) i + 1;
		mServerData << (char) EDataType::TYPE_TABLE;
		for ( size_t i = 0; i < tDataList.getCount( ); i ++ )
		{
			/// �˼�¼��ʹ�� ��д��
			if ( tDataList[ i ]->mIsUsed == false )
				continue;

			mServerData << (char) EDataType::TYPE_FLOAT;
			mServerData << (float) tDataList[ i ]->mPrimaryKey;
			mServerData << (char) EDataType::TYPE_TABLE;
			/// �ֶ�ֵ
			for ( size_t j = 0; j < tServerColumnList.getCount( ); j ++ )
			{
				mServerData << (char) EDataType::TYPE_FLOAT;
				mServerData << (float) j + 1;
				ColumnInfo* tpColumnInfo = tServerColumnList[ j ];
				CNSString& tColumnData = tDataList[ i ]->mColumns[ tpColumnInfo->mIndex ];

				if ( tpColumnInfo->mDataType == DATA_TYPE_FLOAT )	/// float
				{
					float tFloatValue = (float) atof( tColumnData.getBuffer( ) );
					mServerData << (char) EDataType::TYPE_FLOAT;
					mServerData << tFloatValue;
				}
				else   /// string
				{
					mServerData << (char) EDataType::TYPE_STRING;
					mServerData << tColumnData;
				}
			}
			mServerData << (char) 0;
		}
		mServerData << (char) 0;
	}
	mServerData << (char) 0;
	mServerData << (char) 0;
}

void PPExcelParser::FormatClientData( )
{
	/// дģ������
	//mClientData << mFileDatas.getCount( );

	//for ( size_t i = 0; i < mFileDatas.getCount( ); i ++ )
	//{
	//	ExcelData* tpTemplateFile = mFileDatas[ i ];

	//	/// д��ģ������
	//	mClientData << tpTemplateFile->GetTemplateName( );

	//	/// д���¼����
	//	///PushData2Client((size_t)tpTemplateFile->GetDatas( ).size( ) );
	//	mClientData << tpTemplateFile->GetUsedDataCount( );

	//	CNSVector<ColumnInfo*>& tClientColumnList = tpTemplateFile->GetClientColumnsInfo( );
	//	CNSVector<RowData*>& tDataList = tpTemplateFile->GetDatas( );

	//	/// ����д�������������ֶ�����
	//	mClientData << tClientColumnList.getCount( );

	//	/// д���ֶ������ֶ�����
	//	for ( size_t i = 0; i < tClientColumnList.getCount( ); i ++ )
	//	{
	//		CNSString& tFieldName = tClientColumnList[ i ]->mName;
	//		EType tFieldType = tClientColumnList[ i ]->mDataType;

	//		/// �ֶ�����
	//		mClientData << tFieldName;
	//		/// �ֶ�����
	//		mClientData << (unsigned int) tFieldType;
	//	}

	//	for ( size_t i = 0; i < tDataList.getCount( ); i ++ )
	//	{
	//		/// �˼�¼��ʹ�� ��д��
	//		if ( tDataList[ i ]->mIsUsed == false )
	//			continue;

	//		/// ģ��ID
	//		mClientData << tDataList[ i ]->mPrimaryKey;

	//		/// �ֶ�ֵ
	//		for ( size_t j = 0; j < tClientColumnList.getCount( ); j ++ )
	//		{
	//			ColumnInfo* tpColumnInfo = tClientColumnList[ j ];
	//			CNSString& tColumnData = tDataList[ i ]->mColumns[ tpColumnInfo->mIndex ];

	//			if ( tpColumnInfo->mDataType == DATA_TYPE_FLOAT )	/// float
	//			{
	//				float tFloatValue = (float) atof( tColumnData.getBuffer( ) );
	//				mClientData << tFloatValue;
	//			}
	//			else   /// string
	//			{
	//				mClientData << tColumnData;
	//			}
	//		}
	//	}
	//}
}



