#pragma once
namespace NSWin32
{
	class CNSEdit : public CNSWindow
	{
	public:
		enum EEditCtrlEvent
		{
			EventChanged = EN_CHANGE,
			EventKillFocus = EN_KILLFOCUS
		};

		enum EEditCtrlType
		{
			EDIT_NUMBER,
			EDIT_SINGLETEXT,
			EDIT_MULTITEXT
		};

	public:
		static HBRUSH brushBack;

	public:
		static void init( );
		static void exit( );
		static void regLuaLib( );
		static bool onEditCtlColor( CNSWindow* child, WPARAM wParam, LPARAM lParam, LRESULT* result );

	public:
		CNSEdit( const CNSString& windowID );

	public:
		void scrollLine( int line );
		void setSel( int start = 0, int end = -1 );
		// 如果超过最大限制，前面的文本会被删除，append的返回就是删除字节个数(utf8格式)
		int append( const CNSString& text );

	public:
		virtual void onPostCreateWindow( CNSWindow* parent );

	public:
		static void pushUserData( CNSLuaStack& luaStack, CNSEdit* ref );
		static void popUserData( const CNSLuaStack& luaStack, CNSEdit*& ref );
	};
}