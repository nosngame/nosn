#pragma once
namespace NSWin32
{
	class CVsWizard : public CNSWindow
	{
	protected:
		static HBRUSH	backBrush;
		static HBRUSH	wizardMainBrush;
		static HBRUSH	selectBrush;
		static HFONT	mTitleFont;
		static HFONT	mSubTitleFont;

	public:
		enum EVsWizardEvent
		{
			EventConfirm,
			EventCancel
		};

		enum ESignType
		{
			SignIncomplete,
			SignError,
			SignComplete
		};

	public:
		static LRESULT CALLBACK windowProc( HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam );
		static void init( );
		static void exit( );

	public:
		class CWizardItem
		{
		public:
			CNSString	mDesc;
			CNSFrame*		mFrame = NULL;

		public:
			CWizardItem( const CNSString& desc, CNSFrame* frame ) : mDesc( desc ), mFrame( frame )
			{
			}
		};

	public:
		CNSVector< CWizardItem >			mItems;
		CNSString							mTitle;
		CNSString							mSubTitle;
		int									mCurSel = 0;
		HDC									mMemDC = NULL;
		HBITMAP								mMemBitmap = NULL;
		HBITMAP								mOldMemBmp = NULL;
		HFONT								mOldFont = NULL;
		bool								mEnableHint = false;

	public:
		CVsWizard( const CNSString& windowID );

	protected:
		void redraw( );
		void select( int index );
		void getItemRect( int index, RECT& rc );
		void getSubTitleRect( RECT& rc );
		void getTitleRect( RECT& rc );
		void getWorkRect( RECT& rc );

	public:
		int addItem( const CNSString& desc );
		void setItemFrame( int index, CNSFrame* frame );
		void setWizardTitle( const CNSString& title );
		void next( );
		void prev( );
		void setCurPage( int index );

	protected:
		static bool onWizardCancelClicked( NSWin32::CNSWindow* child, WPARAM wParam, LPARAM lParam, LRESULT* result );
		static bool onWizardConfirmClicked( NSWin32::CNSWindow* child, WPARAM wParam, LPARAM lParam, LRESULT* result );
		static bool onWizardNextClicked( NSWin32::CNSWindow* child, WPARAM wParam, LPARAM lParam, LRESULT* result );
		static bool onWizardPrevClicked( NSWin32::CNSWindow* child, WPARAM wParam, LPARAM lParam, LRESULT* result );

	public:
		virtual void onPostCreateWindow( CNSWindow* parent );

	public:
		static void pushUserData( CNSLuaStack& luaStack, CVsWizard* ref );
		static void popUserData( const CNSLuaStack& luaStack, CVsWizard*& ref );
	};
}
