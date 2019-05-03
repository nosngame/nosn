#pragma once
#include <typeinfo>
namespace NSWin32
{
	static int createWindow( lua_State* lua );
	static int createAppFrame( lua_State* lua );
	static int getWindow( lua_State* lua );
	static int doModal( lua_State* lua );
	static int registerGlobalHotkey( lua_State* lua );
	static int showConsole( lua_State* lua );
	static int notification( lua_State* lua );
	static int fileDialog( lua_State* lua );

	static const struct luaL_Reg NSWin32[] =
	{
		{ "createWindow", createWindow },
		{ "createAppFrame", createAppFrame },
		{ "getWindow", getWindow },
		{ "doModal", doModal },
		{ "registerGlobalHotkey", registerGlobalHotkey },
		{ "showConsole", showConsole },
		{ "notification", notification },
		{ "fileDialog", fileDialog },
		{ NULL, NULL },
	};

	static void onGlobalHotkeyHandler( int flag, int keycode )
	{
		const CNSLuaFunction& func = NSWin32::CNSWindow::getLuaGHotkeyRef( flag, keycode );
		NSBase::CNSLuaStack& luaStack = NSBase::CNSLuaStack::getLuaStack( );
		luaStack.preCall( func );
		luaStack.call( );
		return;
	}

	static int createWindow( lua_State* lua )
	{
		DECLARE_BEGIN_PROTECTED
		static CNSString windowType;
		static CNSString windowID;
		int style;
		CNSRect rc;
		NSWin32::CNSWindow* parent = NULL;
		luaStack >> windowType;
		luaStack >> windowID;
		luaStack >> style;
		luaStack >> rc;
		int t = lua_type( lua, 5 );
		const char* name = lua_typename( lua, t );
		luaStack >> parent;

		NSWin32::CNSWindow* window = NULL;
		if ( windowType == NS_VSSTATIC )
			window = NSWin32::CNSWindow::newVsStatic( windowID, style, rc, parent );
		else if ( windowType == NS_VSTAB )
			window = NSWin32::CNSWindow::newVsTab( windowID, style, rc, parent );
		else if ( windowType == NS_EDIT )
			window = NSWin32::CNSWindow::newEdit( windowID, style, rc, parent );
		else if ( windowType == NS_VSEDIT )
			window = NSWin32::CNSWindow::newEdit( windowID, style, rc, parent );
		else if ( windowType == NS_VSBTN )
			window = NSWin32::CNSWindow::newVsBtn( windowID, style, rc, parent );
		else if ( windowType == NS_COMBOBOX )
			window = NSWin32::CNSWindow::newComboBox( windowID, style, rc, parent );
		else if ( windowType == NS_VSTREE )
			window = NSWin32::CNSWindow::newVsTree( windowID, style, rc, parent );
		else if ( windowType == NS_VSLIST )
			window = NSWin32::CNSWindow::newVsList( windowID, style, rc, parent );
		else if ( windowType == NS_VSLISTBOX )
			window = NSWin32::CNSWindow::newVsListBox( windowID, style, rc, parent );
		else if ( windowType == NS_VSFILEBROWSER )
			window = NSWin32::CNSWindow::newFileBrowser( windowID, style, rc, parent );
		else if ( windowType == NS_VSWIZARD )
			window = NSWin32::CNSWindow::newWizard( windowID, style, rc, parent );
		else if ( windowType == NS_FRAME )
		{
			bool center = false;
			if ( lua_isboolean( lua, 6 ) == 1 )
				center = lua_toboolean( lua, 6 );

			window = NSWin32::CNSWindow::newFrame( windowID, style, rc, parent, center );
		}
		else
		{
			static CNSString errorDesc;
			errorDesc.format( _UTF8( "Lua函数[createWindow]指定窗口类型type - %s不存在" ), windowType.getBuffer( ) );
			NSException( errorDesc )
		}
		luaStack << window;
		DECLARE_END_PROTECTED
	}

	int createAppFrame( lua_State* lua )
	{
		DECLARE_BEGIN_PROTECTED
		static CNSString windowID;
		CNSRect rc;
		luaStack >> windowID;
		luaStack >> rc;

		NSWin32::CNSFrame* frame = NSWin32::CNSWindow::newFrame( windowID, NSWin32::CNSFrame::EFrameType::STYLE_FULL, rc, NULL, true );
		luaStack << frame;
		NSConsole::setHostWnd( frame->getHWnd( ) );
		DECLARE_END_PROTECTED
	}

	static int getWindow( lua_State* lua )
	{
		DECLARE_BEGIN_PROTECTED
		const char* name = luaL_checkstring( lua, 1 );
		NSWin32::CNSWindow* window = NSWin32::CNSWindow::getWindow( name );
		if ( window == NULL )
		{
			static CNSString errorDesc;
			errorDesc.format( _UTF8( "Lua函数[getWindow]指定窗口不存在, windowID - %s" ), name );
			NSException( errorDesc )
		}
		luaStack << window;
		DECLARE_END_PROTECTED
	}

	static int doModal( lua_State* lua )
	{
		DECLARE_BEGIN_PROTECTED
		static CNSString windowID;
		static CNSString title;
		CNSRect rc;
		NSWin32::CNSWindow* parent = NULL;
		CNSLuaFunction func;
		luaStack >> windowID;
		luaStack >> rc;
		luaStack >> title;
		luaStack >> parent;
		luaStack >> func;
		if ( func.isValid( ) == false )
			NSException( _UTF8( "Lua函数[doModal]参数5 不是一个lua函数" ) )

		NSWin32::CNSFrame* frame = NSWin32::CNSWindow::newDialog< NSWin32::CNSFrame >( windowID, 0, rc, parent, true );
		frame->setText( title );

		luaStack.preCall( func );
		luaStack << frame;
		luaStack.call( );
		luaStack.clearFunc( func );

		NSWin32::CNSFrame::CDialogResult result;
		frame->bindResult( &result );

		NSWin32::CNSWindow::doModal( frame );
		luaStack << result;
		DECLARE_END_PROTECTED
	}

	static int registerGlobalHotkey( lua_State* lua )
	{
		DECLARE_BEGIN_PROTECTED
		int flag = 0;
		int keycode = 0;
		CNSLuaFunction func;
		luaStack >> flag;
		luaStack >> keycode;
		luaStack >> func;
		if ( func.isValid( ) == false )
			NSException( _UTF8( "Lua函数[doModal]参数3 不是一个lua函数" ) )

		NSWin32::CNSWindow::setLuaGHotkeyRef( flag, keycode, func );
		NSWin32::CNSWindow::registerGlobalHotkey( flag, keycode, onGlobalHotkeyHandler );
		DECLARE_END_PROTECTED
	}

	int showConsole( lua_State * lua )
	{
		DECLARE_BEGIN_PROTECTED
		NSConsole::showConsole( );
		DECLARE_END_PROTECTED
	}

	int notification( lua_State* lua )
	{
		DECLARE_BEGIN_PROTECTED
		const char* text = luaL_checkstring( lua, 1 );
		NSWin32::CNSWindow::notification( text );
		DECLARE_END_PROTECTED
	}

	int fileDialog( lua_State * lua )
	{
		DECLARE_BEGIN_PROTECTED
		static CNSString windowID;
		static CNSString title;
		CNSRect rc;
		int style = 0;
		NSWin32::CNSWindow* parent = NULL;
		CNSLuaFunction func;
		luaStack >> windowID;
		luaStack >> rc;
		luaStack >> style;
		luaStack >> title;
		luaStack >> parent;
		luaStack >> func;

		NSWin32::CVsFileDialog* fileDlg = NSWin32::CNSWindow::newDialog< NSWin32::CVsFileDialog >( windowID, style, rc, parent, true );
		fileDlg->setText( title );
		if ( func.isValid( ) == true )
		{
			luaStack.preCall( func );
			luaStack << fileDlg;
			luaStack.call( );
			luaStack.clearFunc( func );
		}

		NSWin32::CVsFileDialog::CResult result;
		fileDlg->bindResult( &result );
		NSWin32::CNSWindow::doModal( fileDlg );
		luaStack << result;
		DECLARE_END_PROTECTED
	}
}

