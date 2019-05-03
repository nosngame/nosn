#pragma once

namespace NSWin32
{
	static int closeFrame( lua_State* lua );
	static int exitWhenDestroy( lua_State* lua );
	static const struct luaL_Reg NSFrame[] =
	{
		{ "closeFrame", closeFrame },
		{ "exitWhenDestroy", exitWhenDestroy },
		{ NULL, NULL },
	};

	static int closeFrame( lua_State* lua )
	{
		DECLARE_BEGIN_PROTECTED
		NSWin32::CNSFrame* frame = NULL;
		unsigned int resultCode = 0;
		luaStack >> frame;
		luaStack >> resultCode;
		frame->closeFrame( ( NSWin32::CNSFrame::EResultCode ) resultCode );
		DECLARE_END_PROTECTED
	}

	int exitWhenDestroy( lua_State * lua )
	{
		DECLARE_BEGIN_PROTECTED
		NSWin32::CNSFrame* frame = NULL;
		luaStack >> frame;
		frame->exitWhenDestroy( );
		DECLARE_END_PROTECTED
	}
}