#pragma once

namespace NSWin32
{
	static int enable( lua_State* lua );
	static int setText( lua_State* lua );
	static int getText( lua_State* lua );
	static int setAnchor( lua_State* lua );
	static int redraw( lua_State* lua );
	static int setWMPaintMessage( lua_State* lua );

	static const struct luaL_Reg NSWindow[] =
	{
		{ "enable", enable },
		{ "setText", setText },
		{ "getText", getText },
		{ "setAnchor", setAnchor },
		{ "redraw", redraw },
		{ "setWMPaintMessage", setWMPaintMessage },
		{ NULL, NULL },
	};

	static bool onNSLuaOnPaint( NSWin32::CNSWindow* child, WPARAM wParam, LPARAM lParam, LRESULT* result )
	{
		const CNSLuaFunction& funcRef = child->getLuaMsgHandler( WM_PAINT );
		if ( funcRef.isValid( ) == false )
			return true;

		NSBase::CNSLuaStack& luaStack = NSBase::CNSLuaStack::getLuaStack( );
		CNSPaintStruct ps( child );
		try
		{
			luaStack.preCall( funcRef );
			luaStack << &ps;

			bool retValue = true;
			luaStack.call<bool>( retValue );
			return retValue;
		}
		catch ( CNSException& e )
		{
			NSException( _UTF8( "函数[setWMPaintMessage] handler没有返回boolean值" ) );
		}

		return true;
	}

	static void onLocalHotkeyHandler( int flag, int keycode, NSWin32::CNSWindow* window )
	{
		const CNSLuaFunction& funcRef = window->getLuaLHotkeyRef( flag, keycode );
		if ( funcRef.isValid( ) == false )
			return;

		NSBase::CNSLuaStack& luaStack = NSBase::CNSLuaStack::getLuaStack( );
		luaStack.preCall( funcRef );
		luaStack.call( );
		return;
	}

	static int enable( lua_State* lua )
	{
		DECLARE_BEGIN_PROTECTED
		NSWin32::CNSWindow* window = NULL;
		bool enable = false;
		luaStack >> window;
		luaStack >> enable;
		window->enable( enable );
		DECLARE_END_PROTECTED
	}

	static int setText( lua_State* lua )
	{
		DECLARE_BEGIN_PROTECTED
		NSWin32::CNSWindow* window = NULL;
		static CNSString text;
		luaStack >> window;
		luaStack >> text;
		window->setText( text );
		DECLARE_END_PROTECTED
	}

	static int getText( lua_State* lua )
	{
		DECLARE_BEGIN_PROTECTED
		NSWin32::CNSWindow* window = NULL;
		luaStack >> window;
		luaStack << window->getText( );
		DECLARE_END_PROTECTED
	}

	static int setAnchor( lua_State* lua )
	{
		DECLARE_BEGIN_PROTECTED
		NSWin32::CNSWindow* window = NULL;
		int anchorEdge = 0;
		int anchorStyle = 0;
		luaStack >> window;
		luaStack >> anchorEdge;
		luaStack >> anchorStyle;

		if ( anchorEdge == 0 )
			window->mLeftAnchor = anchorStyle;
		else if ( anchorEdge == 1 )
			window->mRightAnchor = anchorStyle;
		else if ( anchorEdge == 2 )
			window->mTopAnchor = anchorStyle;
		else if ( anchorEdge == 3 )
			window->mBottomAnchor = anchorStyle;
		DECLARE_END_PROTECTED
	}

	static int redraw( lua_State* lua )
	{
		DECLARE_BEGIN_PROTECTED
		NSWin32::CNSWindow* window = NULL;
		luaStack >> window;
		window->refresh( );
		DECLARE_END_PROTECTED
	}

	static int setWMPaintMessage( lua_State* lua )
	{
		DECLARE_BEGIN_PROTECTED
		NSWin32::CNSWindow* window = NULL;
		CNSLuaFunction func( __FUNCTION__ );
		luaStack >> window;
		luaStack >> func;

		if ( func.isValid( ) == false )
			NSException( _UTF8( "Lua函数[setWMPaintMessage]参数1 不是一个lua函数" ) );

		window->setLuaMsgHandler( WM_PAINT, func );
		window->registerMessage( WM_PAINT, onNSLuaOnPaint );
		DECLARE_END_PROTECTED
	}

	static int registerLocalHotkey( lua_State* lua )
	{
		DECLARE_BEGIN_PROTECTED
		NSWin32::CNSWindow* window = NULL;
		int flag = 0;
		int keycode = 0;
		int funcRef = 0;
		CNSLuaFunction func( __FUNCTION__ );
		luaStack >> window;
		luaStack >> flag;
		luaStack >> keycode;
		luaStack >> func;
		if ( func.isValid( ) == false )
			NSException( _UTF8( "Lua函数[doModal]参数3 不是一个lua函数" ) )

		window->setLuaLHotkeyRef( flag, keycode, func );
		window->registerLocalHotkey( flag, keycode, onLocalHotkeyHandler );
		DECLARE_END_PROTECTED
	}

}