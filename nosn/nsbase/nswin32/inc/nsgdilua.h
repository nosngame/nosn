#pragma once

namespace NSWin32
{
	static CNSMap< COLORREF, HBRUSH > sLuaBrush;
	static int fillRect( lua_State* lua );
	static int drawText( lua_State* lua );
	static int getPSRect( lua_State* lua );
	static const struct luaL_Reg NSDc[] =
	{
		{ "fillRect", fillRect },
		{ "drawText", drawText },
		{ "getPSRect", getPSRect },
		{ NULL, NULL },
	};

	static int fillRect( lua_State* lua )
	{
		DECLARE_BEGIN_PROTECTED
		NSWin32::CNSPaintStruct* nsPs = NULL;
		CNSRect rc;
		CNSColor clr;

		luaStack >> nsPs;
		luaStack >> rc;
		luaStack >> clr;
		HBRUSH* brRef = sLuaBrush.get( clr );
		if ( brRef == NULL )
		{
			HBRUSH br = ::CreateSolidBrush( clr );
			brRef = &sLuaBrush.insert( clr, br );
		}

		nsPs->fillRect( rc, *brRef );
		DECLARE_END_PROTECTED
	}

	static int drawText( lua_State* lua )
	{
		DECLARE_BEGIN_PROTECTED
		NSWin32::CNSPaintStruct* nsPs = NULL;
		static CNSString text;
		CNSRect rc;
		int drawStyle;
		CNSColor clr;

		luaStack >> nsPs;
		luaStack >> text;
		luaStack >> rc;
		luaStack >> drawStyle;
		luaStack >> clr;

		nsPs->drawText( text, rc, drawStyle, clr );
		DECLARE_END_PROTECTED
	}

	int getPSRect( lua_State* lua )
	{
		DECLARE_BEGIN_PROTECTED
		NSWin32::CNSPaintStruct* nsPs = NULL;
		luaStack >> nsPs;
		luaStack << nsPs->getPSRect( );
		DECLARE_END_PROTECTED
	}
}

