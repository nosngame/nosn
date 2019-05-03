#pragma once
static int image_LoadBitmap( lua_State* lua );

BEGIN_EXPORT( Image )
	EXPORT_FUNC_EX( "image_LoadBitmap", image_LoadBitmap )
END_EXPORT

static int image_LoadBitmap( lua_State* lua )
{
	DECLARE_BEGIN_PROTECTED
	DECLARE_END_PROTECTED
	return 0;
}
