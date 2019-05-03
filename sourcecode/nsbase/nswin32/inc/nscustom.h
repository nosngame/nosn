#pragma once
namespace NSWin32
{
	class CNSCustom : public CNSWindow
	{
	public:
		static LRESULT CALLBACK windowProc( HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam );

	public:
		static void init( );
		static void exit( );

	public:
		CNSCustom( const CNSString& windowID );

	public:
		static void pushUserData( CNSLuaStack& luaStack, CNSCustom* ref );
		static void popUserData( const CNSLuaStack& luaStack, CNSCustom*& ref );
	};
}
