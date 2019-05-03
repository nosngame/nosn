#include <nsbase.h>
namespace NSWin32
{
	HBRUSH CComboBox::brushBack = NULL;
	CComboBox::CComboBox( const CNSString& windowID ) : CNSWindow( windowID, NS_COMBOBOX )
	{
	}

	void CComboBox::init( )
	{
		CNSEdit::brushBack = CreateSolidBrush( RGB( 63, 63, 70 ) );
		CNSWindow::superClass( WC_COMBOBOX, WC_NS_COMBOBOX );
	}

	void CComboBox::exit( )
	{
		UnregisterClass( WC_NS_COMBOBOX, CNSWindow::instance );

		if ( CComboBox::brushBack != NULL )
			DeleteObject( CComboBox::brushBack );
	}

	bool CComboBox::onComboBoxCtlColor( CNSWindow* child, WPARAM wParam, LPARAM lParam, LRESULT* result )
	{
		HDC dcCombo = (HDC) wParam;
		SetTextColor( dcCombo, RGB( 255, 255, 255 ) );
		SetBkColor( dcCombo, RGB( 63, 63, 70 ) );
		*result = (LRESULT) CComboBox::brushBack;
		return true;
	}

	void CComboBox::onPostCreateWindow( CNSWindow* parent )
	{
		CNSWindow::onPostCreateWindow( parent );
		registerEvent( WM_CTLCOLOREDIT, CComboBox::onComboBoxCtlColor );
	}

	void CComboBox::setCurSel( int index )
	{
		::SendMessage( mHWnd, CB_SETCURSEL, index, 0 );
	}

	int CComboBox::getCurSel( ) const
	{
		return ( int ) ::SendMessage( mHWnd, CB_GETCURSEL, 0, 0 );
	}

	int CComboBox::addItem( const CNSString& text )
	{
		return ( int ) ::SendMessage( mHWnd, CB_ADDSTRING, 0, (WPARAM) CNSString::toTChar( text ) );
	}

	int CComboBox::getItemCount( ) const
	{
		return ( int ) ::SendMessage( mHWnd, CB_GETCOUNT, 0, 0 );
	}

	int CComboBox::insertItem( int index, const CNSString& text )
	{
		return ( int ) ::SendMessage( mHWnd, CB_INSERTSTRING, index, (WPARAM) CNSString::toTChar( text ) );
	}

	int CComboBox::select( int start, int end )
	{
		return ( int ) ::SendMessage( mHWnd, CB_SETEDITSEL, 0, MAKELPARAM( (short) start, (short) end ) );
	}

	void CComboBox::pushUserData( CNSLuaStack& luaStack, CComboBox* ref )
	{
		static CNSVector< const luaL_Reg* > reg;
		reg.clear( );
		reg.pushback( NSWin32::NSWindow );
		reg.pushback( NSWin32::NSComboBox );
		luaStack.pushNSWeakRef( ref, NS_COMBOBOX, reg );
	}

	void CComboBox::popUserData( const CNSLuaStack& luaStack, CComboBox*& ref )
	{
		ref = (CComboBox*) luaStack.popNSWeakRef( NS_COMBOBOX );
	}
}