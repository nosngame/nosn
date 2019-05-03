#pragma once
namespace NSWin32
{
	class CNSVsStatic : public CNSWindow
	{
	public:
		static HBRUSH backBrush;

	public:
		static LRESULT CALLBACK windowProc( HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam );
		static void init( );
		static void exit( );

	protected:
		HBRUSH		mBackBrush = NULL;
		COLORREF	mTextColor = RGB( 255, 255, 255 );

	public:
		CNSVsStatic( const CNSString& windowID );

	public:
		void setBkColor( COLORREF color );
		void setTextColor( COLORREF color );

	public:
		virtual void onPostCreateWindow( CNSWindow* parent );

	public:
		static void pushUserData( CNSLuaStack& luaStack, CNSVsStatic* ref );
		static void popUserData( const CNSLuaStack& luaStack, CNSVsStatic*& ref );
	};
}