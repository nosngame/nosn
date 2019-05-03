#include <nsbase.h>
#include <Scintilla.h> 
#include <SciLexer.h> 

namespace NSWin32
{
	HMODULE CNSVsEdit::mSciMod = NULL;
	CNSVsEdit::CNSVsEdit( const CNSString& windowID ) : CNSWindow( windowID, NS_VSEDIT )
	{
	}

	void CNSVsEdit::init( )
	{
		mSciMod = ::LoadLibrary( _T( "scilexer.dll" ) );
		if ( mSciMod == NULL )
		{
			const CNSString& curDir = CNSString::getCurDir( );
			static CNSString errorDesc;
			errorDesc.format( _UTF8( "Win32º¯Êý[LoadLibrary]µ÷ÓÃÊ§°Ü %s/scilexer.dll ¼ÓÔØÊ§°Ü, ´íÎóÂë£º%d" ), curDir.getBuffer( ), GetLastError( ) );
			NSException( errorDesc );
		}

		CNSWindow::superClass( _T( "Scintilla" ), WC_NS_VSEDIT );
	}

	void CNSVsEdit::exit( )
	{
		if ( mSciMod != NULL )
			FreeLibrary( mSciMod );

		UnregisterClass( WC_NS_VSEDIT, CNSWindow::instance );
	}

	void CNSVsEdit::enableReadonly( bool readOnly )
	{
		mReadOnly = readOnly;
		SendMessage( mHWnd, SCI_SETREADONLY, mReadOnly == true ? TRUE : FALSE, 0 );
	}

	void CNSVsEdit::redrawScrollbar( )
	{
		HDC dc = GetWindowDC( getHWnd( ) );
		CNSWindow::redrawScrollbar( dc );
		ReleaseDC( getHWnd( ), dc );
	}

	void CNSVsEdit::alloc( int bytes )
	{
		SendMessage( mHWnd, SCI_ALLOCATE, bytes, 0 );
	}

	void CNSVsEdit::append( const CNSString& text )
	{
		if ( mReadOnly == true )
			SendMessage( mHWnd, SCI_SETREADONLY, FALSE, 0 );

		SendMessage( mHWnd, SCI_APPENDTEXT, (WPARAM) text.getLength( ), (LPARAM) text.getBuffer( ) );

		if ( mReadOnly == true )
			SendMessage( mHWnd, SCI_SETREADONLY, TRUE, 0 );
	}

	void CNSVsEdit::clear( )
	{
		if ( mReadOnly == true )
			SendMessage( mHWnd, SCI_SETREADONLY, FALSE, 0 );

		SendMessage( mHWnd, SCI_CLEARALL, FALSE, 0 );

		if ( mReadOnly == true )
			SendMessage( mHWnd, SCI_SETREADONLY, TRUE, 0 );
	}

	void CNSVsEdit::setEditorText( const CNSString& text )
	{
		if ( mReadOnly == true )
			SendMessage( mHWnd, SCI_SETREADONLY, FALSE, 0 );

		SendMessage( mHWnd, SCI_SETTEXT, 0, (LPARAM) text.getBuffer( ) );
		SendMessage( mHWnd, SCI_SETMOUSEDWELLTIME, 500, 0 );

		if ( mReadOnly == true )
			SendMessage( mHWnd, SCI_SETREADONLY, TRUE, 0 );

		SendMessage( mHWnd, SCI_SETXOFFSET, 0, NULL );
	}

	void CNSVsEdit::setSavePoint( )
	{
		SendMessage( mHWnd, SCI_SETSAVEPOINT, 0, 0 );
	}

	void CNSVsEdit::clearUndo( )
	{
		SendMessage( mHWnd, SCI_EMPTYUNDOBUFFER, 0, 0 );
	}

	void CNSVsEdit::enableFold( )
	{
		SendMessage( mHWnd, SCI_SETPROPERTY, ( WPARAM ) "fold", ( LPARAM )"1" );

		SendMessage( mHWnd, SCI_SETMARGINTYPEN, 3, SC_MARGIN_COLOUR );//Ò³±ßÀàÐÍ 
		SendMessage( mHWnd, SCI_SETMARGINMASKN, 3, SC_MASK_FOLDERS ); //Ò³±ßÑÚÂë 
		SendMessage( mHWnd, SCI_SETMARGINWIDTHN, 3, 11 ); //Ò³±ß¿í¶È 
		SendMessage( mHWnd, SCI_SETMARGINSENSITIVEN, 3, TRUE ); //ÏìÓ¦Êó±êÏûÏ¢ 
		SendMessage( mHWnd, SCI_SETFOLDMARGINCOLOUR, true, RGB( 30, 30, 30 ) );
		SendMessage( mHWnd, SCI_SETFOLDMARGINHICOLOUR, true, RGB( 30, 30, 30 ) );

		// ÕÛµþ±êÇ©ÑùÊ½
		SendMessage( mHWnd, SCI_MARKERDEFINE, SC_MARKNUM_FOLDER, SC_MARK_RGBAIMAGE );
		SendMessage( mHWnd, SCI_MARKERDEFINE, SC_MARKNUM_FOLDEROPEN, SC_MARK_RGBAIMAGE );
		SendMessage( mHWnd, SCI_MARKERDEFINE, SC_MARKNUM_FOLDEREND, SC_MARK_RGBAIMAGE );
		SendMessage( mHWnd, SCI_MARKERDEFINE, SC_MARKNUM_FOLDEROPENMID, SC_MARK_RGBAIMAGE );
		SendMessage( mHWnd, SCI_MARKERDEFINE, SC_MARKNUM_FOLDERMIDTAIL, SC_MARK_TCORNER );
		SendMessage( mHWnd, SCI_MARKERDEFINE, SC_MARKNUM_FOLDERSUB, SC_MARK_VLINE );
		SendMessage( mHWnd, SCI_MARKERDEFINE, SC_MARKNUM_FOLDERTAIL, SC_MARK_LCORNER );

		CImage* plus = CImage::loadPngImageRGBA( "plus.png" );
		CImage* minus = CImage::loadPngImageRGBA( "minus.png" );
		SendMessage( mHWnd, SCI_RGBAIMAGESETWIDTH, 9, 0 );
		SendMessage( mHWnd, SCI_RGBAIMAGESETHEIGHT, 9, 0 );
		SendMessage( mHWnd, SCI_MARKERDEFINERGBAIMAGE, SC_MARKNUM_FOLDER, (sptr_t) plus->mImageData.begin( ) );
		SendMessage( mHWnd, SCI_MARKERDEFINERGBAIMAGE, SC_MARKNUM_FOLDEROPEN, (sptr_t) minus->mImageData.begin( ) );
		SendMessage( mHWnd, SCI_MARKERDEFINERGBAIMAGE, SC_MARKNUM_FOLDEREND, (sptr_t) plus->mImageData.begin( ) );
		SendMessage( mHWnd, SCI_MARKERDEFINERGBAIMAGE, SC_MARKNUM_FOLDEROPENMID, (sptr_t) minus->mImageData.begin( ) );

		// ÕÛµþ±êÇ©ÑÕÉ«
		SendMessage( mHWnd, SCI_MARKERSETBACK, SC_MARKNUM_FOLDEREND, RGB( 165, 165, 165 ) );
		SendMessage( mHWnd, SCI_MARKERSETBACK, SC_MARKNUM_FOLDEROPENMID, RGB( 165, 165, 165 ) );
		SendMessage( mHWnd, SCI_MARKERSETBACK, SC_MARKNUM_FOLDERMIDTAIL, RGB( 165, 165, 165 ) );
		SendMessage( mHWnd, SCI_MARKERSETBACK, SC_MARKNUM_FOLDERSUB, RGB( 165, 165, 165 ) );
		SendMessage( mHWnd, SCI_MARKERSETBACK, SC_MARKNUM_FOLDERTAIL, RGB( 165, 165, 165 ) );
	}

	CNSString CNSVsEdit::getEditorText( )
	{
		static CNSOctets buffer;
		int len = (int) SendMessage( mHWnd, SCI_GETLENGTH, 0, 0 );
		buffer.resize( len + 1 );
		SendMessage( mHWnd, SCI_GETTEXT, len + 1, (LPARAM) buffer.begin( ) );
		return CNSString( (char*) buffer.begin( ) );
	}

	void CNSVsEdit::setCaret( unsigned int pos )
	{
		::SendMessage( mHWnd, SCI_GOTOPOS, pos, 0 );
	}

	void CNSVsEdit::gotoLine( unsigned int line )
	{
		::SendMessage( mHWnd, SCI_GOTOLINE, line - 1, 0 );
	}

	void CNSVsEdit::select( unsigned int start, unsigned int end )
	{
		::SendMessage( mHWnd, SCI_SETSEL, start, end );
	}

	void CNSVsEdit::select( unsigned int line )
	{
		int start = ( int ) ::SendMessage( mHWnd, SCI_POSITIONFROMLINE, line - 1, 0 );
		if ( start == -1 )
			return;

		int end = ( int ) ::SendMessage( mHWnd, SCI_GETLINEENDPOSITION, line - 1, 0 );
		if ( end == -1 )
			return;

		::SendMessage( mHWnd, SCI_SETSEL, start, end );
	}

	int CNSVsEdit::getCaret( )
	{
		return ( int ) ::SendMessage( mHWnd, SCI_GETCURRENTPOS, 0, 0 );
	}

	void CNSVsEdit::setIndic( int indic, unsigned int start, unsigned int end )
	{
		SendMessage( mHWnd, SCI_SETINDICATORCURRENT, 0, 0 );
		SendMessage( mHWnd, SCI_INDICATORFILLRANGE, start, end - start + 1 );
		CNSVector< CIndic >* indicRef = mIndics.get( indic );
		if ( indicRef == NULL )
			indicRef = &mIndics.insert( indic, CNSVector< CIndic >( 128 ) );

		indicRef->pushback( CIndic( start, end ) );
	}

	void CNSVsEdit::clearIndic( int indic )
	{
		CNSVector< CIndic >* indicRef = mIndics.get( indic );
		if ( indicRef == NULL )
			return;

		for ( unsigned int i = 0; i < indicRef->getCount( ); i ++ )
		{
			CIndic& indicObj = ( *indicRef )[ i ];
			SendMessage( mHWnd, SCI_INDICATORCLEARRANGE, indicObj.start, indicObj.end - indicObj.start + 1 );
		}
	}

	int CNSVsEdit::getLineCount( )
	{
		return ( int ) ::SendMessage( mHWnd, SCI_GETLINECOUNT, 0, 0 );
	}

	int CNSVsEdit::getColumn( unsigned int pos )
	{
		return ( int ) ::SendMessage( mHWnd, SCI_GETCOLUMN, pos, 0 );
	}

	void CNSVsEdit::visibleLine( unsigned int line )
	{
		int lineScreen = ( int ) ::SendMessage( mHWnd, SCI_LINESONSCREEN, 0, 0 );
		int firstLine = line - 1 - (int) ( lineScreen / 2 );
		firstLine = max( 0, firstLine );

		int lineTotal = ( int ) ::SendMessage( mHWnd, SCI_GETLINECOUNT, 0, 0 );
		firstLine = min( lineTotal - lineScreen, firstLine );
		::SendMessage( mHWnd, SCI_SETFIRSTVISIBLELINE, firstLine, 0 );
		redrawScrollbar( );
	}

	void CNSVsEdit::enableLineNumber( bool enable )
	{
		SendMessage( mHWnd, SCI_SETMARGINTYPEN, 2, SC_MARGIN_NUMBER );

		if ( enable == true )
			SendMessage( mHWnd, SCI_SETMARGINWIDTHN, 2, 40 );
		else
			SendMessage( mHWnd, SCI_SETMARGINWIDTHN, 2, 0 );
	}

	int CNSVsEdit::getLineStartPos( unsigned int line )
	{
		return ( int ) ::SendMessage( mHWnd, SCI_POSITIONFROMLINE, line - 1, NULL );
	}

	CNSString CNSVsEdit::getLine( unsigned int line )
	{
		static CNSString text;
		text.clear( );
		int len = ( int ) ::SendMessage( mHWnd, SCI_GETLINE, line - 1, NULL );

		CNSOctets buffer;
		buffer.resize( len );
		::SendMessage( mHWnd, SCI_GETLINE, line - 1, (LPARAM) buffer.begin( ) );
		text.pushback( (char*) buffer.begin( ), len );
		return text;
	}

	CNSString CNSVsEdit::getWordByPosition( unsigned int position, unsigned int& start, unsigned int& end )
	{
		static CNSString text;
		text.clear( );
		start = ( int ) ::SendMessage( mHWnd, SCI_WORDSTARTPOSITION, position, true );
		end = ( int ) ::SendMessage( mHWnd, SCI_WORDENDPOSITION, position, true );
		if ( end <= start )
			return text;

		CNSOctets buffer;
		buffer.resize( end - start + 1 );
		Sci_TextRange sr;
		sr.chrg.cpMin = start;
		sr.chrg.cpMax = end;
		sr.lpstrText = (char*) buffer.begin( );
		::SendMessage( mHWnd, SCI_GETTEXTRANGE, 0, (LPARAM) &sr );
		text.pushback( sr.lpstrText );
		end -= 1;
		return text;
	}

	void CNSVsEdit::setMargin( int marginID, int width, int mask, COLORREF marginBack, bool enableClick )
	{
		::SendMessage( mHWnd, SCI_SETMARGINTYPEN, marginID, SC_MARGIN_COLOUR );
		::SendMessage( mHWnd, SCI_SETMARGINWIDTHN, marginID, width );
		::SendMessage( mHWnd, SCI_SETMARGINMASKN, marginID, mask );
		::SendMessage( mHWnd, SCI_SETMARGINSENSITIVEN, marginID, enableClick );
		::SendMessage( mHWnd, SCI_SETMARGINBACKN, marginID, marginBack );
	}

	void CNSVsEdit::setMarginSymbol( int symbolID, int symbolType, COLORREF fore, COLORREF back )
	{
		::SendMessage( mHWnd, SCI_MARKERDEFINE, symbolID, symbolType );
		::SendMessage( mHWnd, SCI_MARKERSETFORE, symbolID, fore );
		::SendMessage( mHWnd, SCI_MARKERSETBACK, symbolID, back );
	}

	void CNSVsEdit::setMarginImage( int symbolID, int cx, int cy, const CNSString& imageFile )
	{
		CImage* image = CImage::loadPngImageRGBA( imageFile );
		SendMessage( mHWnd, SCI_RGBAIMAGESETWIDTH, cx, 0 );
		SendMessage( mHWnd, SCI_RGBAIMAGESETHEIGHT, cy, 0 );
		SendMessage( mHWnd, SCI_MARKERDEFINERGBAIMAGE, symbolID, (sptr_t) image->mImageData.begin( ) );
	}

	void CNSVsEdit::showMargin( unsigned int line, int symbolID )
	{
		::SendMessage( mHWnd, SCI_MARKERADD, line - 1, symbolID );
	}

	void CNSVsEdit::hideMargin( unsigned int line, int symbolID )
	{
		::SendMessage( mHWnd, SCI_MARKERDELETE, line - 1, symbolID );
	}

	void CNSVsEdit::hideAllMargin( int symbolID )
	{
		::SendMessage( mHWnd, SCI_MARKERDELETEALL, symbolID, 0 );
	}

	void* CNSVsEdit::getDocPointer( )
	{
		return ( void* ) ::SendMessage( mHWnd, SCI_GETDOCPOINTER, 0, 0 );
	}

	void CNSVsEdit::setDocPointer( void* doc )
	{
		::SendMessage( mHWnd, SCI_SETDOCPOINTER, 0, (LPARAM) doc );
	}

	bool CNSVsEdit::isModified( ) const
	{
		return ( bool ) ::SendMessage( mHWnd, SCI_GETMODIFY, 0, 0 );
	}

	void CNSVsEdit::replace( const CNSString& text, int start, int end )
	{
		::SendMessage( mHWnd, SCI_SETTARGETSTART, start, 0 );
		::SendMessage( mHWnd, SCI_SETTARGETEND, end, 0 );
		::SendMessage( mHWnd, SCI_REPLACETARGET, -1, (LPARAM) text.getBuffer( ) );
	}

	int CNSVsEdit::search( const CNSString& text, int start, int end, bool matchCase, bool matchWord )
	{
		if ( start == -1 )
			start = 0;

		if ( end == -1 )
			end = ( int ) ::SendMessage( mHWnd, SCI_GETLENGTH, 0, 0 );

		::SendMessage( mHWnd, SCI_SETTARGETSTART, start, 0 );
		::SendMessage( mHWnd, SCI_SETTARGETEND, end, 0 );
		int sciFind = 0;
		if ( matchCase == true )
			sciFind = sciFind | SCFIND_MATCHCASE;

		if ( matchWord == true )
			sciFind = sciFind | SCFIND_WHOLEWORD;

		::SendMessage( mHWnd, SCI_SETSEARCHFLAGS, sciFind, 0 );
		return ( int ) ::SendMessage( mHWnd, SCI_SEARCHINTARGET, text.getLength( ), (LPARAM) text.getBuffer( ) );
	}

	int CNSVsEdit::lineFromPosition( unsigned int pos )
	{
		int line = ( int ) ::SendMessage( mHWnd, SCI_LINEFROMPOSITION, pos, 0 );
		if ( line == -1 )
			return -1;

		return line + 1;
	}

	int CNSVsEdit::lineStart( unsigned int line )
	{
		return ( int ) ::SendMessage( mHWnd, SCI_POSITIONFROMLINE, line - 1, 0 );
	}

	int CNSVsEdit::lineEnd( unsigned int line )
	{
		return ( int ) ::SendMessage( mHWnd, SCI_GETLINEENDPOSITION, line - 1, 0 );
	}

	bool CNSVsEdit::isOverType( )
	{
		return ::SendMessage( mHWnd, SCI_GETOVERTYPE, 0, 0 );
	}

	const char* CNSVsEdit::getCharPointer( ) const
	{
		return (char*) SendMessage( mHWnd, SCI_GETCHARACTERPOINTER, 0, 0 );
	}

	int CNSVsEdit::getTextLength( ) const
	{
		return (int) SendMessage( mHWnd, SCI_GETLENGTH, 0, 0 );
	}

	void CNSVsEdit::toggleFold( unsigned int line )
	{
		SendMessage( mHWnd, SCI_TOGGLEFOLD, line - 1, 0 );
	}

	void CNSVsEdit::enableCaretLine( bool enable )
	{
		SendMessage( mHWnd, SCI_SETCARETLINEVISIBLE, enable == true ? TRUE : FALSE, 0 );
	}

	void CNSVsEdit::setSelBack( COLORREF clr )
	{
		SendMessage( mHWnd, SCI_SETSELBACK, true, clr );
	}

	void CNSVsEdit::setTextLexer( )
	{
		SendMessage( mHWnd, SCI_SETLEXER, SCLEX_TEX, 0 );
		SendMessage( mHWnd, SCI_STYLESETFORE, STYLE_DEFAULT, RGB( 255, 255, 255 ) );
		SendMessage( mHWnd, SCI_STYLESETBACK, STYLE_DEFAULT, RGB( 30, 30, 30 ) );	//×Ö·û 
		for ( int i = 0; i <= 20; i ++ )
		{
			SendMessage( mHWnd, SCI_STYLESETFORE, i, RGB( 255, 255, 255 ) );	//×Ö·û 
			SendMessage( mHWnd, SCI_STYLESETBACK, i, RGB( 30, 30, 30 ) );	//×Ö·û 
		}
	}

	void CNSVsEdit::setLuaLexer( )
	{
		const char* szKeywords1 =
			"function return ipairs pairs local while end do if for nil else in elseif tostring tonumber type true false";
		const char* szKeywords2 =
			"NSBase.trace NSBase.MD5 NSBase.decodeBase64 NSBase.encodeBase64 NSBase.MD5Octets "\
			"Http.HttpReturnUtf8 Http.HttpsGet Http.HttpGet Http.HttpsPost Http.HttpPost "\
			"System.OperCommand "\
			"Mysql.ExecSqlNoDataNB Mysql.ExecSqlWithDataNB Mysql.execSqlNoData Mysql.execSqlWithData Mysql.getInt64Value"\
			"table os.date os.time string.len string.find string.byte string.char math.pow math.random";

		// C++Óï·¨½âÎö 
		SendMessage( mHWnd, SCI_SETLEXER, SCLEX_LUA, 0 );
		SendMessage( mHWnd, SCI_SETKEYWORDS, 0, (sptr_t) szKeywords1 );			//ÉèÖÃ¹Ø¼ü×Ö 
		SendMessage( mHWnd, SCI_SETKEYWORDS, 1, (sptr_t) szKeywords2 );			//ÉèÖÃ¹Ø¼ü×Ö 

		SendMessage( mHWnd, SCI_STYLESETFORE, STYLE_DEFAULT, RGB( 200, 200, 200 ) );
		SendMessage( mHWnd, SCI_STYLESETBACK, STYLE_DEFAULT, RGB( 30, 30, 30 ) );	//×Ö·û 

		for ( int i = SCE_LUA_DEFAULT; i <= SCE_LUA_LABEL; i ++ )
		{
			SendMessage( mHWnd, SCI_STYLESETFORE, i, RGB( 200, 200, 200 ) );	//×Ö·û 
			SendMessage( mHWnd, SCI_STYLESETBACK, i, RGB( 30, 30, 30 ) );	//×Ö·û 
		}

		// ÏÂÃæÉèÖÃ¸÷ ÖÖÓï·¨ÔªËØ·ç¸ñ 
		SendMessage( mHWnd, SCI_STYLESETFORE, SCE_LUA_WORD, RGB( 86, 156, 214 ) );	//¹Ø¼ü×Ö 
		SendMessage( mHWnd, SCI_STYLESETFORE, SCE_LUA_WORD2, RGB( 189, 99, 197 ) );	//¹Ø¼ü×Ö 

		SendMessage( mHWnd, SCI_STYLESETFORE, SCE_LUA_STRING, RGB( 214, 157, 133 ) );	//×Ö·û´® 
		SendMessage( mHWnd, SCI_STYLESETFORE, SCE_LUA_IDENTIFIER, RGB( 200, 200, 200 ) );	//×Ö·û´® 

		SendMessage( mHWnd, SCI_STYLESETFORE, SCE_LUA_CHARACTER, RGB( 214, 157, 133 ) );	//×Ö·û 
		SendMessage( mHWnd, SCI_STYLESETFORE, SCE_LUA_OPERATOR, RGB( 255, 255, 255 ) );	//×Ö·û 
		SendMessage( mHWnd, SCI_STYLESETFORE, SCE_LUA_NUMBER, RGB( 181, 206, 168 ) );	//×Ö·û 
		SendMessage( mHWnd, SCI_STYLESETFORE, SCE_LUA_PREPROCESSOR, RGB( 155, 155, 155 ) );	//Ô¤±àÒë¿ª¹Ø 
		SendMessage( mHWnd, SCI_STYLESETFORE, SCE_LUA_COMMENT, RGB( 87, 166, 74 ) );	//¿é×¢ÊÍ 
		SendMessage( mHWnd, SCI_STYLESETFORE, SCE_LUA_COMMENTLINE, RGB( 87, 166, 74 ) );	//ÐÐ×¢ÊÍ 
		SendMessage( mHWnd, SCI_STYLESETFORE, SCE_LUA_COMMENTDOC, RGB( 87, 166, 74 ) );	//ÎÄµµ×¢ÊÍ£¨/**¿ªÍ·£© 

		::SendMessage( mHWnd, SCI_STYLESETBACK, STYLE_LINENUMBER, RGB( 30, 30, 30 ) );
		::SendMessage( mHWnd, SCI_STYLESETFORE, STYLE_LINENUMBER, RGB( 43, 145, 175 ) );
	}

	void CNSVsEdit::onPostCreateWindow( CNSWindow* parent )
	{
		CNSWindow::onPostCreateWindow( parent );
		// ÉèÖÃÈ«¾Ö·ç¸ñ 
		SendMessage( mHWnd, SCI_STYLESETFONT, STYLE_DEFAULT, ( sptr_t ) "ÐÂËÎÌå" );
		SendMessage( mHWnd, SCI_STYLESETSIZE, STYLE_DEFAULT, 10 );
		SendMessage( mHWnd, SCI_STYLECLEARALL, 0, 0 );

		SendMessage( mHWnd, SCI_SETCARETLINEBACK, RGB( 128, 128, 128 ), 0 );
		SendMessage( mHWnd, SCI_SETSELBACK, true, RGB( 38, 79, 120 ) );

		SendMessage( mHWnd, SCI_SETTABWIDTH, 4, 0 );
		SendMessage( mHWnd, SCI_SETCODEPAGE, SC_CP_UTF8, 0 );
		SendMessage( mHWnd, SCI_SETCARETLINEFRAME, 1, 0 );
		SendMessage( mHWnd, SCI_SETCARETFORE, RGB( 255, 255, 255 ), 0 );

		SendMessage( mHWnd, SCI_CLEARCMDKEY, (WPARAM) ( 'S' + ( SCMOD_CTRL << 16 ) ), 0 );
		SendMessage( mHWnd, SCI_CLEARCMDKEY, (WPARAM) ( 'G' + ( SCMOD_CTRL << 16 ) ), 0 );
		SendMessage( mHWnd, SCI_CLEARCMDKEY, (WPARAM) ( 'F' + ( SCMOD_CTRL << 16 ) ), 0 );
		SendMessage( mHWnd, SCI_SETMODEVENTMASK, SC_MOD_INSERTTEXT | SC_MOD_DELETETEXT, 0 );
		SendMessage( mHWnd, SCI_INDICSETSTYLE, 0, INDIC_FULLBOX );
		SendMessage( mHWnd, SCI_INDICSETFORE, 0, 0x000080 );
		SendMessage( mHWnd, SCI_INDICSETALPHA, 0, 128 );
		SendMessage( mHWnd, SCI_INDICSETOUTLINEALPHA, 0, 255 );
		SendMessage( mHWnd, SCI_SETMARGINWIDTHN, 1, 0 );
	}

	void CNSVsEdit::pushUserData( CNSLuaStack& luaStack, CNSVsEdit* ref )
	{
		static CNSVector< const luaL_Reg* > reg;
		reg.clear( );
		reg.pushback( NSWin32::NSWindow );
		reg.pushback( NSWin32::NSVsEdit );
		luaStack.pushNSWeakRef( ref, NS_VSEDIT, reg );
	}

	void CNSVsEdit::popUserData( const CNSLuaStack& luaStack, CNSVsEdit*& ref )
	{
		ref = (CNSVsEdit*) luaStack.popNSWeakRef( NS_VSEDIT );
	}
}