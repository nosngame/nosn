#pragma once
namespace NSWin32
{
	class CNSVsEdit : public CNSWindow
	{
	public:
		enum CEditorCtrlEvent
		{
			EventMarginClicked = SCN_MARGINCLICK,
			EventDwellStart = SCN_DWELLSTART,
			EventDwellEnd = SCN_DWELLEND,
			EventFocusIn = SCN_FOCUSIN,
			EventFocusOut = SCN_FOCUSOUT,
			EventChanged = SCN_MODIFIED,
			EventSavePointReached = SCN_SAVEPOINTREACHED,
			EventSavePointLeft = SCN_SAVEPOINTLEFT,
		};

		class CIndic
		{
		public:
			unsigned int start;
			unsigned int end;

		public:
			CIndic( unsigned int s = 0, unsigned int e = 0 ) : start( s ), end( e )
			{
			}
		};

	protected:
		CNSMap< int, CNSVector< CIndic > >		mIndics;

	public:
		CNSVsEdit( const CNSString& windowID );

	public:
		bool							mReadOnly = false;

	public:
		static HMODULE mSciMod;
		static void init( );
		static void exit( );

	protected:
		void redrawScrollbar( );

	public:
		void enableReadonly( bool readOnly );
		// 开启光标所在行边框提示
		void enableCaretLine( bool enable );
		// 开启行号页边
		void enableLineNumber( bool enable );
		void setEditorText( const CNSString& text );
		void setSavePoint( );
		void clearUndo( );
		void enableFold( );
		CNSString getEditorText( );
		void clear( );
		void alloc( int bytes );
		void append( const CNSString& text );
		void setLuaLexer( );
		void setTextLexer( );
		void setSelBack( COLORREF clr );
		int lineFromPosition( unsigned int pos );
		int lineStart( unsigned int line );
		int lineEnd( unsigned int line );
		bool isOverType( );
		void toggleFold( unsigned int line );
		const char* getCharPointer( ) const;
		int getTextLength( ) const;

		void setCaret( unsigned int pos );
		void gotoLine( unsigned int line );
		void select( unsigned int start, unsigned int end );
		void select( unsigned int line );
		void setIndic( int indic, unsigned int start, unsigned int end );
		void clearIndic( int indic );
		int getCaret( );
		int getLineCount( );
		int getColumn( unsigned int pos );
		void visibleLine( unsigned int line );
		CNSString getWordByPosition( unsigned int position, unsigned int& start, unsigned int& end );
		CNSString getLine( unsigned int line );
		int getLineStartPos( unsigned int line );

		void setMargin( int marginID, int width, int mask, COLORREF marginBack, bool enableClick );
		void showMargin( unsigned int line, int symbolID );
		void hideMargin( unsigned int line, int symbolID );
		void hideAllMargin( int symbolID );
		void setMarginSymbol( int symbolID, int symbolType, COLORREF fore, COLORREF back );
		void setMarginImage( int symbolID, int cx, int cy, const CNSString& imageFile );
		int search( const CNSString& text, int start, int end, bool matchCase, bool matchWord );
		void replace( const CNSString& text, int start, int end );
		void* getDocPointer( );
		void setDocPointer( void* doc );
		bool isModified( ) const;

	public:
		virtual void onPostCreateWindow( CNSWindow* parent );

	public:
		static void pushUserData( CNSLuaStack& luaStack, CNSVsEdit* ref );
		static void popUserData( const CNSLuaStack& luaStack, CNSVsEdit*& ref );
	};
}
