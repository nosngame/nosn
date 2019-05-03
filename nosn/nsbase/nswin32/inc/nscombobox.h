#pragma once
namespace NSWin32
{
	class CComboBox : public CNSWindow
	{
	public:
		enum EComboBoxEvent
		{
			EventChanged = CBN_SELENDOK,
			EventInputChanged = CBN_EDITCHANGE,
		};

		enum EComboType
		{
			COMBO_DROPDOWN,
			COMBO_DROPLIST
		};

	public:
		static HBRUSH brushBack;

	public:
		static void init( );
		static void exit( );
		static bool onComboBoxCtlColor( CNSWindow* child, WPARAM wParam, LPARAM lParam, LRESULT* result );

	public:
		CComboBox( const CNSString& windowID );

	public:
		void setCurSel( int index );
		int getCurSel( ) const;
		int addItem( const CNSString& text );
		int getItemCount( ) const;
		int insertItem( int index, const CNSString& text );
		int select( int start, int end );

	public:
		virtual void onPostCreateWindow( CNSWindow* parent );

	public:
		static void pushUserData( CNSLuaStack& luaStack, CComboBox* ref );
		static void popUserData( const CNSLuaStack& luaStack, CComboBox*& ref );
	};
}