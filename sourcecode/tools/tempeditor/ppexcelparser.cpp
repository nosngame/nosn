#include <nsbase.h>
#include "PPExcelParser.h"
#include <fstream>
#include "FileConfig.h"

static unsigned int MAX_OUT_FILE_SIZE = 1024 * 1024 * 100;
/// 有效性标识
const static char* g_pEffective = "TPLDATA";

ExcelData::ExcelData( CNSString& rWorkSheetName ) :mWorkSheetName( rWorkSheetName )
{

}

//从xml读取文件
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
		NSLog::log( _UTF8( "无效的模板文件%s" ), mTemplateFileName.getBuffer( ) );
		return false;
	}

	TiXmlElement* tpRootElement = tXmlDoc.RootElement( );
	if ( tpRootElement == NULL )
		return false;

	//读取模板名称
	TiXmlElement* tpWorkSheetElement = tpRootElement->FirstChildElement( "Worksheet" );
	if ( LoadTemplateName( tpWorkSheetElement ) == false )
		return false;

	TiXmlElement* tpTableElement = tpWorkSheetElement->FirstChildElement( "Table" );
	if ( tpTableElement == NULL )
	{
		NSLog::log( _UTF8( "无效的模板文件%s" ), mTemplateFileName.getBuffer( ) );
		return false;
	}

	//读取模板列信息
	TiXmlElement* tpColumnRowElement = tpTableElement->FirstChildElement( "Row" );
	if ( LoadColumnInfo( tpColumnRowElement ) == false )
		return false;

	//读取模板数据
	TiXmlElement* tpDataRowElement = tpColumnRowElement->NextSiblingElement( "Row" );
	while ( tpDataRowElement != NULL )
	{
		if ( LoadRowData( tpDataRowElement ) == false )
			return false;

		tpDataRowElement = tpDataRowElement->NextSiblingElement( "Row" );
	}

	//读取模板数据有效性
	TiXmlElement* tpValidationElement = tpWorkSheetElement->FirstChildElement( "DataValidation" );
	while ( tpValidationElement != NULL )
	{
		if ( LoadColumnDataValidation( tpValidationElement ) == false )
			return false;

		tpValidationElement = tpValidationElement->NextSiblingElement( "DataValidation" );
	}

	//验证模板有效性
	if ( CheckUpIsKeyContinuous( ) == false )
		return false;

	if ( CheckUpDataCountValidity( ) == false )
		return false;

	if ( CheckUpDataValidity( ) == false )
		return false;

	return true;
}

//清空
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

//获得模板名称
CNSString& ExcelData::GetTemplateName( )
{
	return mWorkSheetName;
}

//获得服务器列信息
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

//获得客户端列信息
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

//获得主键数据
CNSVector<RowData*>& ExcelData::GetDatas( )
{
	return mDatas;
}

//获得有效数据的数量
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

//读取模板名称
bool ExcelData::LoadTemplateName( TiXmlElement* pWorkSheetElement )
{
	if ( pWorkSheetElement == NULL )
	{
		return false;
	}
	mWorkSheetName = pWorkSheetElement->Attribute( "ss:Name" );
	return true;
}

//读取列数据（创建列数组，初始化列名称，列类型，列数据，主键值）
bool ExcelData::LoadColumnInfo( TiXmlElement* pColumnRowElement )
{
	if ( pColumnRowElement == NULL )
	{
		NSLog::log( _UTF8( "无效的模板文件%s" ), mTemplateFileName.getBuffer( ) );
		return false;
	}

	TiXmlElement* tpIDCellElement = pColumnRowElement->FirstChildElement( "Cell" );
	//判断第一行之前是否有空行，以ss:index有值或者是没有cell字段来判断为第一行之前是否有空行
	if ( tpIDCellElement == NULL || pColumnRowElement->Attribute( "ss:Index" ) != NULL )
	{
		NSLog::log( _UTF8( "文件[%s],第一行之前有空行" ), mTemplateFileName.getBuffer( ) );
		return false;
	}

	/// 判断首行首个Cell是否存在省略行
	if ( tpIDCellElement->Attribute( "ss:Index" ) != NULL )
	{
		NSLog::log( _UTF8( "文件[%s]，第一有效列不在第一列上" ), mTemplateFileName.getBuffer( ) );
		return false;
	}

	// 获得列信息
	TiXmlElement* tpInfoCellElement = tpIDCellElement->NextSiblingElement( "Cell" );
	if ( tpInfoCellElement == NULL )
	{
		NSLog::log( _UTF8( "文件[%s]，没有任何一个有效列" ), mTemplateFileName.getBuffer( ) );
		return false;
	}

	size_t tColumnIndex = 0;
	while ( tpInfoCellElement )
	{
		if ( tpInfoCellElement->Attribute( "ss:Index" ) != NULL )
		{
			NSLog::log( _UTF8( "文件[%s]，列信息中间有空行" ), mTemplateFileName.getBuffer( ) );
			return false;
		}

		TiXmlElement* tpDataElement = tpInfoCellElement->FirstChildElement( "Data" );
		if ( tpDataElement == NULL )
		{
			NSLog::log( _UTF8( "文件[%s]，列信息缺失" ), mTemplateFileName.getBuffer( ) );
			return false;
		}

		// 判断当前列的名称文本是否有效(_C,_S,_CS)
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
			NSLog::log( _UTF8( "文件[%s]，有无效的列名称[%s]" ), mTemplateFileName.getBuffer( ), tColumnText.getBuffer( ) );
			return false;
		}

		TiXmlElement* tpColumnNameElement = tpInfoCellElement->FirstChildElement( "NamedCell" );
		if ( tpColumnNameElement == NULL )
		{
			NSLog::log( _UTF8( "文件[%s]，列名称[%s]对应的tagID，缺失" ), mTemplateFileName.getBuffer( ), tColumnText.getBuffer( ) );
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
			NSLog::log( _UTF8( "文件[%s]，列名称[%s]有两个与之对应的tagID，请检查" ), mTemplateFileName.getBuffer( ), tColumnText.getBuffer( ) );
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

//读取数据
bool ExcelData::LoadRowData( TiXmlElement* pDataRowElement )
{
	if ( mColumnInfos.getCount( ) == 0 )
		return false;

	size_t tCurrentRowIndex = mDatas.getCount( );
	if ( pDataRowElement == NULL )
	{
		NSLog::log( _UTF8( "无效的模板文件[%s]" ), mTemplateFileName.getBuffer( ) );
		return false;
	}

	TiXmlElement* tpPrimaryCellElement = pDataRowElement->FirstChildElement( "Cell" );
	//判断数据行与行之间是否有空行，以ss:index有值或者是没有cell字段来判断行之间是否有空行
	if ( tpPrimaryCellElement == NULL || pDataRowElement->Attribute( "ss:Index" ) != NULL )
	{
		NSLog::log( _UTF8( "文件[%s],第%u行是空行" ), mTemplateFileName.getBuffer( ), (int) tCurrentRowIndex + 2 );
		return false;
	}

	/// 判断当前行主键格是否是空
	if ( tpPrimaryCellElement->Attribute( "ss:Index" ) != NULL )
	{
		NSLog::log( _UTF8( "文件[%s]，第%u行，主键列是空值" ), mTemplateFileName.getBuffer( ), (int) tCurrentRowIndex + 2 );
		return false;
	}
	TiXmlElement* tpDataElement = tpPrimaryCellElement->FirstChildElement( "Data" );
	if ( tpDataElement == NULL )
	{
		NSLog::log( _UTF8( "文件[%s]，第%u行，数据错误" ), mTemplateFileName.getBuffer( ), (int) tCurrentRowIndex + 2 );
		return false;
	}

	CNSString tPrimaryKey( tpDataElement->GetText( ) );

	// 主键值不为空，则创建此行数据，并且初始化所有数据为"",主键若找到'!'，此行记录视为作废记录，不会输出到二进制文件
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
			NSLog::log( _UTF8( "文件[%s]，第%u行，主键列仅支持在主键数值之后加入作废标记" ), mTemplateFileName.getBuffer( ), (int) tCurrentRowIndex + 2 );
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

	// 获得列信息
	TiXmlElement* tpDataCellElement = tpPrimaryCellElement->NextSiblingElement( "Cell" );
	size_t tCurrentColumnIndex = 0;
	while ( tpDataCellElement != NULL )
	{
		// 属性ss:Index == NULL说明是当前列，否则说明index值前面的列全为空， 
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

//读取列数据有效性
bool ExcelData::LoadColumnDataValidation( TiXmlElement* pValidateElement )
{
	if ( mColumnInfos.getCount( ) == 0 )
		return false;

	if ( pValidateElement == NULL )
	{
		NSLog::log( _UTF8( "无效的模板文件[%s]" ), mTemplateFileName.getBuffer( ) );
		return false;
	}

	//读取影响列值
	TiXmlElement* tpRangeElement = pValidateElement->FirstChildElement( "Range" );
	if ( tpRangeElement == NULL )
	{
		NSLog::log( _UTF8( "无效的模板文件[%s]" ), mTemplateFileName.getBuffer( ) );
		return false;
	}

	const char* tpRangeValue = tpRangeElement->GetText( );
	CNSVector<size_t> tEffectedColumns;
	if ( GetSameValidateList( tpRangeValue, tEffectedColumns ) == false )
	{
		NSLog::log( _UTF8( "文件[%s],读取数据有效性时出错" ), mTemplateFileName.getBuffer( ) );
		return false;
	}

	//读取类型值
	TiXmlElement* tpTypeElement = pValidateElement->FirstChildElement( "Type" );
	if ( tpTypeElement == NULL )
	{
		NSLog::log( _UTF8( "无效的模板文件[%s]" ), mTemplateFileName.getBuffer( ) );
		return false;
	}

	CNSString tTypeValue = tpTypeElement->GetText( );
	for ( size_t i = 0; i < tEffectedColumns.getCount( ); i ++ )
	{
		if ( tTypeValue == "TextLength" )  /// 文本类型
		{
			mColumnInfos[ tEffectedColumns[ i ] ]->mDataType = DATA_TYPE_STRING;
		}
		else if ( tTypeValue == "Whole" || tTypeValue == "Decimal" ) /// Float类型
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
			NSLog::log( _UTF8( "文件[%s]，主键非连续或者主键没有从默认1起始，请检查主键[%u]处" ), mTemplateFileName.getBuffer( ), (int) i );
			return false;
		}
	}
	return true;
}

//检查记录的数量是否合理（每条记录的列字段数量要一直，记录的列字段数量和列数要一致）
bool ExcelData::CheckUpDataCountValidity( )
{
	if ( mDatas.getCount( ) == 0 )
	{
		NSLog::log( _UTF8( "文件[%s]，没有数据" ), mTemplateFileName.getBuffer( ) );
		return false;
	}

	if ( mColumnInfos.getCount( ) == 0 )
	{
		NSLog::log( _UTF8( "文件[%s]，没有有效列" ), mTemplateFileName.getBuffer( ) );
		return false;
	}

	size_t tFirstRowDataColumnCount = mDatas[ 0 ]->mColumns.getCount( );
	size_t tColumnCount = mColumnInfos.getCount( );

	for ( size_t i = 0; i < mDatas.getCount( ); i ++ )
	{
		if ( mDatas[ i ]->mColumns.getCount( ) != tFirstRowDataColumnCount || mDatas[ i ]->mColumns.getCount( ) != tColumnCount )
		{
			NSLog::log( _UTF8( "文件[%s]，数据无效" ), mTemplateFileName.getBuffer( ) );
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
			NSLog::log( _UTF8( "文件[%s]，数据有效性出错，请检查[%u]列，名称框为[%s]" ), mTemplateFileName.getBuffer( ), (int) i + 2, mColumnInfos[ i ]->mText.getBuffer( ) );
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
					NSLog::log( _UTF8( "警告：文件[%s],主键列设置了数据有效性，现已忽略。" ), mTemplateFileName.getBuffer( ) );
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
					NSLog::log( _UTF8( "文件[%s],第%u列数据有效性设置错误，请整列一起设置数据有效性" ), mTemplateFileName.getBuffer( ), tBeginColumnInt + 2 );
					return false;
				}

				if ( tEndRowNumberPos != -1 )
				{
					NSLog::log( _UTF8( "文件[%s],第%u列数据有效性设置错误，请整列一起设置数据有效性" ), mTemplateFileName.getBuffer( ), tEndColumnInt + 2 );
					return false;
				}

				for ( int i = tBeginColumnInt; i <= tEndColumnInt; i ++ )
				{
					if ( i == -1 )
						NSLog::log( _UTF8( "警告：文件[%s],主键列设置了数据有效性，现已忽略" ), mTemplateFileName.getBuffer( ) );
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

/// 解析文件
bool PPExcelParser::ParseFiles( CNSVector< CNSString >& vInputFiles )
{
	Reset( );
	if ( AddFiles( vInputFiles ) == false )
		return false;

	FormatServerData( );
	FormatClientData( );
	return true;
}

/// 读取所有需要处理的文件
bool PPExcelParser::AddFiles( CNSVector< CNSString >& vInputFiles )
{
	for ( size_t i = 0; i < vInputFiles.getCount( ); i ++ )
	{
		CNSString& tfileName = vInputFiles[ i ];
		ExcelData* tpExcelData = new ExcelData;
		NSLog::log( _UTF8( "开始加载文件[%s]" ), tfileName.getBuffer( ) );
		if ( tpExcelData->LoadDataFromXmlFile( tfileName ) == false )
		{
			NSLog::log( _UTF8( "读取xml文件失败[%s]" ), tfileName.getBuffer( ) );
			return false;
		}

		mFileDatas.pushback( tpExcelData );
	}

	return true;
}

void PPExcelParser::FormatServerData( )
{
	mServerData << (char) EDataType::TYPE_TABLE;

	/// 写模板数量
	mServerData << (char) EDataType::TYPE_STRING;
	mServerData << CNSString( "tempInfo" );
	mServerData << (char) EDataType::TYPE_TABLE;
	for ( size_t i = 0; i < mFileDatas.getCount( ); i ++ )
	{
		ExcelData* tpTemplateFile = mFileDatas[ i ];

		/// 写入模板名称
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
		/// 写入字段名称字段类型
		for ( size_t i = 0; i < tServerColumnList.getCount( ); i ++ )
		{
			CNSString& tFieldName = tServerColumnList[ i ]->mName;
			EType tFieldType = tServerColumnList[ i ]->mDataType;

			/// 字段名称
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
			/// 此记录不使用 则不写入
			if ( tDataList[ i ]->mIsUsed == false )
				continue;

			mServerData << (char) EDataType::TYPE_FLOAT;
			mServerData << (float) tDataList[ i ]->mPrimaryKey;
			mServerData << (char) EDataType::TYPE_TABLE;
			/// 字段值
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
	/// 写模板数量
	//mClientData << mFileDatas.getCount( );

	//for ( size_t i = 0; i < mFileDatas.getCount( ); i ++ )
	//{
	//	ExcelData* tpTemplateFile = mFileDatas[ i ];

	//	/// 写入模板名称
	//	mClientData << tpTemplateFile->GetTemplateName( );

	//	/// 写入记录数量
	//	///PushData2Client((size_t)tpTemplateFile->GetDatas( ).size( ) );
	//	mClientData << tpTemplateFile->GetUsedDataCount( );

	//	CNSVector<ColumnInfo*>& tClientColumnList = tpTemplateFile->GetClientColumnsInfo( );
	//	CNSVector<RowData*>& tDataList = tpTemplateFile->GetDatas( );

	//	/// 仅仅写入服务器需求的字段数量
	//	mClientData << tClientColumnList.getCount( );

	//	/// 写入字段名称字段类型
	//	for ( size_t i = 0; i < tClientColumnList.getCount( ); i ++ )
	//	{
	//		CNSString& tFieldName = tClientColumnList[ i ]->mName;
	//		EType tFieldType = tClientColumnList[ i ]->mDataType;

	//		/// 字段名称
	//		mClientData << tFieldName;
	//		/// 字段类型
	//		mClientData << (unsigned int) tFieldType;
	//	}

	//	for ( size_t i = 0; i < tDataList.getCount( ); i ++ )
	//	{
	//		/// 此记录不使用 则不写入
	//		if ( tDataList[ i ]->mIsUsed == false )
	//			continue;

	//		/// 模板ID
	//		mClientData << tDataList[ i ]->mPrimaryKey;

	//		/// 字段值
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



