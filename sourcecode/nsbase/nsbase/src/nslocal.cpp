#include <nsbase.h>
namespace NSBase
{
	void CNSLocal::load( const CNSString& dataPath )
	{
		TiXmlDocument doc;
		if ( doc.LoadFile( dataPath + "/nosndata/localization.xml" ) == false )
		{
			static CNSString errorDesc;
            errorDesc.format( _UTF8( "路径[%s] 文件[localization.xml]打开错误, 错误码[%s]" ), dataPath.getBuffer(), doc.ErrorDesc( ) );
			NSException( errorDesc );
		}

		TiXmlElement* versionEle = doc.FirstChildElement( "version" );
		if ( versionEle != NULL )
		{
			mExeVersion = versionEle->Attribute( "exe" );
			mExcelVersion = versionEle->Attribute( "excel" );
			mScriptVersion = versionEle->Attribute( "script" );
		}

		if ( mExeVersion.isEmpty( ) == true )
			mExeVersion = "none";

		if ( mExcelVersion.isEmpty( ) == true )
			mExcelVersion = "none";

		if ( mScriptVersion.isEmpty( ) == true )
			mScriptVersion = "none";

		TiXmlElement* lang = doc.FirstChildElement( "lang" );
		for ( ; lang != NULL; lang = lang->NextSiblingElement( "lang" ) )
		{
			CNSString name = lang->Attribute( "name" );
			CNSString label = lang->Attribute( "label" );
			mLangInfo.insert( name, label );
		}

		TiXmlElement* text = doc.FirstChildElement( "text" );
		for ( ; text != NULL; text = text->NextSiblingElement( "text" ) )
		{
			TiXmlElement* item = text->FirstChildElement( "item" );
			int textID = CNSString( text->Attribute( "id" ) ).toInteger( );
			CNSLocalEntry local;
			for ( ; item != NULL; item = item->NextSiblingElement( "item" ) )
			{
				CNSString key = item->Attribute( "name" );
				CNSString value = item->Attribute( "text" );
				local.mText.insert( key, value );
			}

			mLangSet.insert( textID, local );
		}
	}

	const CNSString& CNSLocal::getLang( ) const
	{
		return mLanguage;
	}

	const CNSMap< CNSString, CNSString >& CNSLocal::getLangInfo( ) const
	{
		return mLangInfo;
	}

	const CNSString& CNSLocal::getLangText( int textID ) const
	{
		const CNSLocalEntry* local = mLangSet.get( textID );
		if ( local == NULL )
		{
			static CNSString null;
			return null;
		}

		return local->getText( mLanguage );
	}

	const CNSString& CNSLocal::getVersion( ) const
	{
		static CNSString verText;
		if ( verText.isEmpty( ) == true )
			verText = mLanguage + "." + mExeVersion + "." + mExcelVersion + "." + mScriptVersion;

		return verText;
	}
}
