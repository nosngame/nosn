#pragma once
namespace NSWin32
{
	class CNSVsTab : public CNSWindow
	{
	public:
		enum EVsTabStyle
		{
			TAB_FILETYPE,
			TAB_VIEWTYPE_UP,
			TAB_VIEWTYPE_DOWN,
		};

		enum EVsTabCtrlEvent
		{
			EventClose = NM_FIRST - 1,
			EventSelectChanged = NM_FIRST - 2
		};

		struct VSTabClose
		{
			NMHDR hdr;
			int deleteItemID;
		};

		struct VSTabSelectChanged
		{
			NMHDR hdr;
			int newItemID;
			int oldItemID;
		};

	protected:
		class CTabItem
		{
		public:
			CNSString		mName;
			intptr_t		mUserData = 0;
			int				mItemID = 0;
			RECT			mRect = { 0, 0, 0, 0 };
			RECT			mCloseRect = { 0, 0, 0, 0 };
			int				mImageIndex = -1;

		public:
			CTabItem( int itemID = 0, const CNSString& name = "", int imageIndex = -1, intptr_t data = 0 ) 
				: mItemID( itemID ), mName( name ), mImageIndex( imageIndex ), mUserData( data )
			{
			}
		};

	protected:
		HDC						mMemDC = NULL;
		HBITMAP					mMemBitmap = NULL;
		HBITMAP					mOldMemBmp = NULL;
		HFONT					mOldFont = NULL;
		CTabItem*				curSel = NULL;
		CTabItem*				hoverClose = NULL;
		CTabItem*				clickClose = NULL;
		CTabItem*				hoverItem = NULL;
		EVsTabStyle				mTabType = TAB_FILETYPE;
		HIMAGELIST				mImageList = NULL;

		CNSMap< int, CTabItem >	mItems;
		bool					mNeedRedraw = false;

	public:
		static LRESULT CALLBACK windowProc( HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam );
		static void init( );
		static void exit( );

	public:
		CNSVsTab( const CNSString& windowID, EVsTabStyle tabType );

	public:
		int addItem( const CNSString& text, int imageIndex = -1, intptr_t userdata = NULL );
		void setCurSel( int itemID );
		int getCurSel( ) const;
		void deleteItem( int itemID );
		void getWorkRect( RECT& workRect ) const;
		void getHeaderRect( RECT& rcHeader ) const;
		void setItemData( int itemID, intptr_t userdata );
		void getItems( CNSVector< int >& itemIDs );

		intptr_t getItemData( int itemID ) const;
		const CNSString& getItemText( int itemID ) const;
		int getItemCount( ) const;
		void setItemText( int itemID, const CNSString& text );
		void setImageList( int cx, int cy, CNSVector< CNSString >& images );
		void setItemImage( int itemID, int imageIndex );

	protected:
		void redraw( );
		void notifyClose( int itemID );
		void notifySelect( int oldItemID, int newItemID );
		void drawItem( CTabItem* item, bool redraw = false );
		int calcItemRect( CNSVsTab::CTabItem* item, int start );

	public:
		virtual void onPostCreateWindow( CNSWindow* parent );

	public:
		static void pushUserData( CNSLuaStack& luaStack, CNSVsTab* ref );
		static void popUserData( const CNSLuaStack& luaStack, CNSVsTab*& ref );
	};

}
