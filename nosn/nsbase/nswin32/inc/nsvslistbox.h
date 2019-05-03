#pragma once
namespace NSWin32
{
	class CNSVsListBox : public CNSWindow
	{
	public:
		class CListBoxItem
		{
		public:
			int			mImage = -1;
			CNSString	mText;
			RECT		mRect = { 0 };
			RECT		mImageRect = { 0 };
			intptr_t	mUserData = 0;
			int			mItemID = 0;
			bool		mNeedRecalc = true;
			int			mTextWidth = 0;
		public:
			CListBoxItem( const CNSString& text = "", int image = -1, intptr_t data = 0 ) : mText( text ), mImage( image ), mUserData( data )
			{
				static int i = 0;
				mItemID = ++ i;
			}
		};

	public:
		enum EVsListBoxEvent
		{
			EventDeleteItem = NM_FIRST - 1,
			EventSelectChanged = NM_FIRST - 2,
			EventCalcItem = NM_FIRST - 3,
			EventDrawItem = NM_FIRST - 4,
		};

		struct VSListBoxDeleteItem
		{
			NMHDR hdr;
			int deleteItemID;
		};

		struct VSListBoxSelectChanged
		{
			NMHDR hdr;
			int newItemID;
			int oldItemID;
		};

		struct VSListBoxCalcItem
		{
			NMHDR hdr;
			CListBoxItem* item;
			// 绘制的目标DC
			HDC		dc;
			// image宽度
			int		mImageCX;
			// image高度
			int		mImageCY;
		};

		struct VSListBoxDrawItem
		{
			NMHDR hdr;
			CListBoxItem* item;
			// 绘制的目标DC
			HDC dc;
			// 观察横坐标
			int	x;
			// 观察纵坐标
			int y;
			// image宽度
			int mImageCX;
			// image高度
			int	mImageCY;
		};

	public:
		static HBRUSH	backBrush;
		static HBRUSH	selectBrush;

	protected:
		mutable HBRUSH	mBackBrush = NULL;
		COLORREF		mBackColor = RGB( 27, 27, 28 );
		HPEN			mBorderPen = NULL;
		COLORREF		mBorderColor = RGB( 51, 51, 55 );
		unsigned int	mItemHeight = 20;
		COLORREF		mTextColor = RGB( 241, 241, 241 );
		bool			mNeedRedraw = false;
		HDC				mMemDC = NULL;
		HBITMAP			mMemBitmap = NULL;
		HBITMAP			mOldMemBmp = NULL;
		HFONT			mOldFont = NULL;
		HIMAGELIST		mImageList = NULL;
		CListBoxItem*	mCurSel = NULL;
		CNSMap< int, CListBoxItem > mItems;

	public:
		static LRESULT CALLBACK windowProc( HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam );
		static void init( );
		static void exit( );

	public:
		CNSVsListBox( const CNSString& windowID );

	public:
		int addItem( const CNSString& text, int image = -1, intptr_t userdata = NULL );
		void setTextColor( COLORREF color );
		void setBkColor( COLORREF color );
		void setBorderColor( COLORREF color );
		void setItemImage( int itemID, int imageIndex );
		void setImageList( int cx, int cy, CNSVector< CNSString >& images );
		HIMAGELIST getImageList( ) const;
		HBRUSH getBackBrush( ) const;
		void deleteItem( int itemID );
		void setCurSel( int itemID );
		void clear( );
		int getCurSel( ) const;
		void setItemData( int itemID, intptr_t userdata );
		intptr_t getItemData( int itemID ) const;
		int getItemCount( ) const;
		int getItemHeight( ) const;

	protected:
		void redraw( );
		void drawItemImage( int x, int y, CListBoxItem* item );
		void drawItemText( int x, int y, CListBoxItem* item );
		void notifyDeleteItem( int itemID );
		void notifySelectChanged( int oldItemID, int newItemID );
		int notifyCalcItem( int imageCX, int imageCY, CListBoxItem* item );
		int notifyDrawItem( int x, int y, int imageCX, int imageCY, CListBoxItem* item );

	public:
		virtual void onPostCreateWindow( CNSWindow* parent );

	public:
		static void pushUserData( CNSLuaStack& luaStack, CNSVsListBox* ref );
		static void popUserData( const CNSLuaStack& luaStack, CNSVsListBox*& ref );
	};
}
