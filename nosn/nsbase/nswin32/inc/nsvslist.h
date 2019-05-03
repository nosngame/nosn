#pragma once
namespace NSWin32
{
	class CHeaderCtrl : public CNSWindow
	{
	public:
		CHeaderCtrl( const CNSString& windowID, HWND wnd );

	public:
		virtual CNSLuaStack& marshal( CNSLuaStack& luaStack )
		{
			return luaStack;
		}
	};

	class CNSVsList : public CNSWindow
	{
	public:
		enum EListCtrlEvent
		{
			EventClicked = NM_CLICK,
			EventDblClicked = NM_DBLCLK,
			EventKeyDown = LVN_KEYDOWN,
			EventCustomDraw = NM_CUSTOMDRAW
		};

		static HBRUSH	brItemSelect;
		static HBRUSH	backTextA;
		static HBRUSH	backTextB;
		static HBRUSH	backHeader;
		static HBRUSH	backHeaderSplit;
		static HPEN		borderHeaderPen;

		static void init( );
		static void exit( );

	private:
		static bool onHeaderRender( CNSWindow* child, WPARAM wParam, LPARAM lParam, LRESULT* result );
		static bool onListCustomDraw( CNSWindow* child, WPARAM wParam, LPARAM lParam, LRESULT* result );

	protected:
		unsigned int	mColumnCounter;
		unsigned int	mItemCounter;
		CHeaderCtrl*	mHeader;

	public:
		CNSVsList( const CNSString& windowID );

	public:
		CHeaderCtrl* getHeaderCtrl( ) const
		{
			return mHeader;
		}

		void clear( );
		void setBkColor( COLORREF color )
		{
			ListView_SetBkColor( getHWnd( ), color );
		}

		void setTextColor( COLORREF color )
		{
			ListView_SetTextColor( getHWnd( ), color );
		}

		CNSVector< int >& getCurSel( );
		CNSString getItemText( int x, int y );
		int getColumnCount( ) const
		{
			return mColumnCounter;
		}
		void setCurSel( int index );
		bool isItemSelect( int index ) const;
		int getColumnWidth( int index ) const;
		void addColumn( const CNSString& text, unsigned int width );
		int addItem( const CNSString& text );
		void setItem( int x, int y, const CNSString& text );
		void setImageList( HIMAGELIST imageList );
		void setItemImage( int x, int y, int iImage );

	public:
		virtual void onPostCreateWindow( CNSWindow* parent );
		virtual void scrollVert( int pos );
		virtual void scrollHorz( int pos );

	public:
		static void pushUserData( CNSLuaStack& luaStack, CNSVsList* ref );
		static void popUserData( const CNSLuaStack& luaStack, CNSVsList*& ref );
	};
}