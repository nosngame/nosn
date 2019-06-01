#pragma once
namespace NSBase
{
	class CNSLocalEntry
	{
	public:
		CNSMap< CNSString, CNSString >	mText;

	public:
		const CNSString& getText( const CNSString& lang ) const
		{
			const CNSString* value = mText.get( lang );
			if ( value == NULL )
			{
				static CNSString null;
				return null;
			}

			return *value;
		}
	};

	class CNSLocal
	{
	protected:
		CNSMap< int, CNSLocalEntry > mLangSet;
		CNSMap< CNSString, CNSString > mLangInfo;
		CNSString mLanguage;
		CNSString mExeVersion;
		CNSString mExcelVersion;
		CNSString mScriptVersion;

	public:
		CNSLocal( )
		{
		}

		~CNSLocal( )
		{
		}

	public:
		static CNSLocal& getNSLocal( )
		{
			static CNSLocal local;
			return local;
		}

	public:
		void load( const CNSString& dataPath );
		const CNSString& getVersion( ) const;
		const CNSString& getLangText( int textID ) const;
		const CNSString& getLang( ) const;
		void setLang( const CNSString& lang )
		{
			mLanguage = lang;
		}

		const CNSMap< CNSString, CNSString >& getLangInfo( ) const;
	};
}
