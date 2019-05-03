#pragma once
namespace NSWin32
{
	class CNSVsTree : public CNSWindow
	{
	public:
		enum CTreeCtrlEvent
		{
			EventSelectChanged = TVN_SELCHANGED,
			EventDeleteItem = TVN_DELETEITEM,
			EventClicked = NM_CLICK,
			EventRClicked = NM_RCLICK,
			EventCustomDraw = NM_CUSTOMDRAW,
		};
		static HBRUSH brushFocusSelect;
		static HBRUSH brushUnfocusSelect;
		static HPEN penNode;
		static HBRUSH brushExpand;

	public:
		CNSVsTree( const CNSString& windowID );
		~CNSVsTree( );

	public:
		HBRUSH mBrushBkItem = NULL;

	public:
		static bool onTreeCustomDraw( CNSWindow* child, WPARAM wParam, LPARAM lParam, LRESULT* result );
		static void init( );
		static void exit( );

	public:
		void clear( );

		HTREEITEM addItem( HTREEITEM parent, HTREEITEM after, const CNSString& text, void* data = NULL, int image = 65534, int expandImage = 65534 );
		HTREEITEM getCurSel( ) const;
		void setItemText( HTREEITEM item, const CNSString& text );
		void setItemImage( HTREEITEM item, int image, int expandImage );
		void setImageList( CNSVector< CNSString >& images );

		void setBkColor( COLORREF color );
		COLORREF getBkColor( ) const;
		void setTextColor( COLORREF color );
		COLORREF getTextColor( ) const;

		void setItemBold( HTREEITEM item, bool enable );
		void setCurSel( HTREEITEM item );
		void setItemData( HTREEITEM item, void* data );
		void* getItemData( HTREEITEM item );
		void setItemHeight( unsigned int height );
		void getItemRect( HTREEITEM item, RECT& rc ) const;
		CNSString& getItemPath( HTREEITEM item );
		CNSString& getItemText( HTREEITEM item );
		HTREEITEM findItem( const CNSString& treePath );
		void select( HTREEITEM item );
		void deleteItem( HTREEITEM item );
		void deleteAllSubItems( HTREEITEM item );
		HTREEITEM getParentItem( HTREEITEM item );
		void getSubItems( HTREEITEM item, CNSVector< HTREEITEM >& subItems, bool recursion = false );

	protected:
		HTREEITEM findItemHelper( HTREEITEM parent, CNSVector< CNSString >& path, int index );

	public:
		virtual void onPostCreateWindow( CNSWindow* parent );

	public:
		static void pushUserData( CNSLuaStack& luaStack, CNSVsTree* ref );
		static void popUserData( const CNSLuaStack& luaStack, CNSVsTree*& ref );
	};
}