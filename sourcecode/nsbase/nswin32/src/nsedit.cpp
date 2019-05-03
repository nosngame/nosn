#include <nsbase.h>
namespace NSWin32
{
	HBRUSH CNSEdit::brushBack = NULL;
	CNSEdit::CNSEdit( const CNSString& windowID ) : CNSWindow( windowID, NS_EDIT )
	{
	}

	void CNSEdit::init( )
	{
		CNSEdit::brushBack = CreateSolidBrush( RGB( 63, 63, 70 ) );
		CNSWindow::superClass( WC_EDIT, WC_NS_EDIT );
	}

	void CNSEdit::exit( )
	{
		UnregisterClass( WC_NS_EDIT, CNSWindow::instance );
		if ( CNSEdit::brushBack != NULL )
			DeleteObject( CNSEdit::brushBack );
	}

	void CNSEdit::regLuaLib( )
	{
		NSBase::CNSLuaStack& luaStack = NSBase::CNSLuaStack::getLuaStack( );
		luaStack.newTable( );
		luaStack.pushField( "EDIT_NUMBER", CNSEdit::EDIT_NUMBER );
		luaStack.pushField( "EDIT_SINGLETEXT", CNSEdit::EDIT_SINGLETEXT );
		luaStack.pushField( "EDIT_MULTITEXT", CNSEdit::EDIT_MULTITEXT );
		luaStack.setGlobalTable( "editStyle" );
	}

	bool CNSEdit::onEditCtlColor( CNSWindow* child, WPARAM wParam, LPARAM lParam, LRESULT* result )
	{
		HDC dcEdit = (HDC) wParam;
		SetTextColor( dcEdit, RGB( 255, 255, 255 ) );
		SetBkColor( dcEdit, RGB( 63, 63, 70 ) );
		*result = (LRESULT) CNSEdit::brushBack;
		return true;
	}

	void CNSEdit::onPostCreateWindow( CNSWindow* parent )
	{
		CNSWindow::onPostCreateWindow( parent );
		registerEvent( WM_CTLCOLOREDIT, CNSEdit::onEditCtlColor );
	}

	void CNSEdit::scrollLine( int line )
	{
		int lineCount = line;
		if ( line == -1 )
			lineCount = ( int ) ::SendMessage( mHWnd, EM_GETLINECOUNT, 0, 0 );

		::SendMessage( mHWnd, EM_LINESCROLL, 0, lineCount );
	}

	void CNSEdit::setSel( int start, int end )
	{
		::SendMessage( mHWnd, EM_SETSEL, start, end );
	}

	int CNSEdit::append( const CNSString& text )
	{
		int len = ( int ) ::SendMessage( mHWnd, WM_GETTEXTLENGTH, 0, 0 );
		::SendMessage( mHWnd, EM_SETSEL, len, -1 );
		::SendMessage( mHWnd, EM_REPLACESEL, 0, (LPARAM) CNSString::toTChar( text ) );
		return 0;
	}

	void CNSEdit::pushUserData( CNSLuaStack& luaStack, CNSEdit* ref )
	{
		static CNSVector< const luaL_Reg* > reg;
		reg.clear( );
		reg.pushback( NSWin32::NSWindow );
		reg.pushback( NSWin32::NSEdit );
		luaStack.pushNSWeakRef( ref, NS_EDIT, reg );
	}

	void CNSEdit::popUserData( const CNSLuaStack& luaStack, CNSEdit*& ref )
	{
		ref = (CNSEdit*) luaStack.popNSWeakRef( NS_EDIT );
	}
}