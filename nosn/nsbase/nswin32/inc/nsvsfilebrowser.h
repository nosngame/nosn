#pragma once
namespace NSWin32
{
	class CVsFileBrowser : public CNSWindow
	{
	public:
		enum EVsFileBrowserEvent
		{
			EventChanged = NM_FIRST - 1,
			EventSelect = NM_FIRST - 2,
			EventDBlClkFile = NM_FIRST - 3,
		};

		enum EFilterType
		{
			FilterFile = 0,
			FilterDirectory = 1
		};

	public:
		static HBRUSH backBrush;
		static HANDLE queryThread;
		static HANDLE queryEvent;

	public:
		CNSString	mCurPath;
		CNSString	mFilter;
		EFilterType mFilterType;

	public:
		static LRESULT CALLBACK windowProc( HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam );
		static void init( );
		static void exit( );
		static void __stdcall onQueryTimer( HWND wnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime );
		static bool onListCtrlDblClk( CNSWindow* child, WPARAM wParam, LPARAM lParam, LRESULT* reuslt );
		static bool onListCtrlClicked( CNSWindow* child, WPARAM wParam, LPARAM lParam, LRESULT* reuslt );
		static bool onUpButtonClicked( CNSWindow* child, WPARAM wParam, LPARAM lParam, LRESULT* reuslt );
		static bool onDirInputEditKeydown( CNSWindow* child, WPARAM wParam, LPARAM lParam, LRESULT* reuslt );

	public:
		CVsFileBrowser( const CNSString& windowID );

	public:
		CNSString& getCurPath( ) const;
		void getSelectFile( CNSVector< CNSString >& result ) const;
		void setCurPath( const CNSString& path );
		void setFilter( EFilterType type, const CNSString& filter );
		void refresh( );

	protected:
		void display( );
		void displayDrivers( );

	public:
		virtual void onPostCreateWindow( CNSWindow* parent );

	public:
		static void pushUserData( CNSLuaStack& luaStack, CVsFileBrowser* ref );
		static void popUserData( const CNSLuaStack& luaStack, CVsFileBrowser*& ref );
	};
}
