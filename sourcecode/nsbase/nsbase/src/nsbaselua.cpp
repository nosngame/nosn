#include <nsbase.h>

namespace NSBase
{
	static int getTick( lua_State* lua );
	static int md5Octets( lua_State* lua );
	static int md5( lua_State* lua );
	static int encodeBase64( lua_State* lua );
	static int decodeBase64( lua_State* lua );
	static int encodeURI( lua_State* lua );
	static int decodeURI( lua_State* lua );
	static int decodeRsaByPublic( lua_State* lua );
	static int decodeRsaByPrivkey( lua_State* lua );
	static int encodeHMacSha1( lua_State* lua );
	static int trace( lua_State* lua );
	static int interrupt( lua_State* lua );
	static int createTimer( lua_State* lua );
	static int createDelay( lua_State* lua );
	static int setLang( lua_State* lua );
	static int getLang( lua_State* lua );
	static int getVersion( lua_State* lua );
	static int getLangText( lua_State* lua );
	static int getLangInfo( lua_State* lua );

	BEGIN_EXPORT( NSBaseLib )
		EXPORT_FUNC( getTick )
		EXPORT_FUNC( md5Octets )
		EXPORT_FUNC( md5 )
		EXPORT_FUNC( encodeBase64 )
		EXPORT_FUNC( decodeBase64 )
		EXPORT_FUNC( encodeURI )
		EXPORT_FUNC( decodeURI )
		EXPORT_FUNC( decodeRsaByPublic )
		EXPORT_FUNC( decodeRsaByPrivkey )
		EXPORT_FUNC( encodeHMacSha1 )
		EXPORT_FUNC( trace )
		EXPORT_FUNC( interrupt )
		EXPORT_FUNC( createTimer )
		EXPORT_FUNC( createDelay )
		EXPORT_FUNC( setLang )
		EXPORT_FUNC( getLang )
		EXPORT_FUNC( getVersion )
		EXPORT_FUNC( getLangText )
		EXPORT_FUNC( getLangInfo )
	END_EXPORT

	CNSMap< int, CNSLuaFunction > gTimerHandler;
	const CNSLuaFunction& getTimerHandler( int timerID )
	{
		CNSLuaFunction* func = gTimerHandler.get( timerID );
		if ( func == NULL )
		{
			static CNSLuaFunction func;
			return func;
		}

		return *func;
	}

	int gStackDeep = 0;
	CNSString gTraceText;
	void nsTimerLuaHanlder( int timerID, unsigned int curTick, int remain, void* userdata )
	{
		NSBase::CNSLuaStack& luaStack = NSBase::CNSLuaStack::getLuaStack( );
		const CNSLuaFunction& func = getTimerHandler( timerID );

		if ( func.isValid( ) == true )
		{
			luaStack.preCall( func );
			luaStack << timerID;
			luaStack << curTick;
			luaStack.call( );
		}

		// 如果剩余次数为0
		if ( remain == 0 )
		{
			luaStack.clearFunc( func );
			gTimerHandler.erase( timerID );
		}
	}

	void regLuaLib( )
	{
		NSBase::CNSLuaStack& luaStack = NSBase::CNSLuaStack::getLuaStack( );
		luaStack.regLib( "NSBase", NSBase::NSBaseLib );

		NSBase::regNSFileLib( );
	}

	void stackTraceProc( CNSLuaStack::EnumData* data, CNSLuaStack::EnumData* key, CNSLuaStack::EnumData* value, void* userdata )
	{
		static CNSString indent;
		static CNSString dataText;
		if ( data->type == LUA_TNUMBER )
			dataText.format( "\n%s%1.2f[number]", indent.getBuffer( ), *(double*) data->value );
		else if ( data->type == LUA_TSTRING )
			dataText.format( "\n%s%s[string]", indent.getBuffer( ), (char*) data->value );
		else if ( data->type == LUA_TBOOLEAN )
			dataText.format( "\n%s%s[boolean]", indent.getBuffer( ), ( *(bool*) data->value ) == true ? "true" : "false" );
		else if ( data->type == LUA_TNIL )
			dataText.format( "\n%s[nil]", indent.getBuffer( ) );
		else if ( data->type == LUA_TNONE )
			dataText.format( "\n%s[none]", indent.getBuffer( ) );
		else if ( data->type == LUA_TFUNCTION )
			dataText.format( "\n%s0x%016x[function]", indent.getBuffer( ), data->value );
		else if ( data->type == LUA_TTHREAD )
			dataText.format( "\n%s0x%016x[thread]", indent.getBuffer( ), data->value );
		else if ( data->type == LUA_TUSERDATA )
			dataText.format( "\n%s0x%016x[userdata]", indent.getBuffer( ), data->value );
		else if ( data->type == LUA_TLIGHTUSERDATA )
			dataText.format( "\n%s0x%016x[lightuserdata]", indent.getBuffer( ), data->value );
		else if ( data->type == LUA_TTABLE )
		{
			if ( (ptrdiff_t) data->value == 1 )
			{
				// table开始
				if ( key != NULL )
				{
					if ( key->type == LUA_TNUMBER )
						dataText.format( "\n%s%1.2f = \n%s{", indent.getBuffer( ), *(double*) key->value, indent.getBuffer( ) );
					else if ( key->type == LUA_TSTRING )
						dataText.format( "\n%s%s = \n%s{", indent.getBuffer( ), key->value, indent.getBuffer( ) );
				}
				else
					dataText.format( "\n%s{", indent.getBuffer( ) );
				gStackDeep ++;
				indent.clear( );
				for ( int i = 0; i < gStackDeep; i ++ )
					indent += "    ";
			}
			else if ( (ptrdiff_t) data->value == 0 )
			{
				// table结束
				gStackDeep --;
				indent.clear( );
				for ( int i = 0; i < gStackDeep; i ++ )
					indent += "    ";
				dataText.format( "\n%s}", indent.getBuffer( ) );
			}
			else if ( (ptrdiff_t) data->value == 2 )
			{
				// table内容
				static CNSString keyText;
				static CNSString valueText;
				if ( key->type == LUA_TNUMBER )
					keyText.format( "%s%1.2f[number]", indent.getBuffer( ), *(double*) key->value );
				else if ( key->type == LUA_TSTRING )
					keyText.format( "%s%s[string]", indent.getBuffer( ), (char*) key->value );

				if ( value->type == LUA_TNUMBER )
					valueText.format( "%1.2f[number]", *(double*) value->value );
				else if ( value->type == LUA_TSTRING )
					valueText.format( "%s[string]", (char*) value->value );
				else if ( value->type == LUA_TBOOLEAN )
					valueText.format( "%s[boolean]", ( *(bool*) value->value ) == true ? "true" : "false" );
				else if ( value->type == LUA_TNIL )
					valueText.format( "[nil]" );
				else if ( value->type == LUA_TNONE )
					valueText.format( "[none]" );
				else if ( value->type == LUA_TFUNCTION )
					valueText.format( "0x%016x[function]", value->value );
				else if ( value->type == LUA_TTHREAD )
					valueText.format( "0x%016x[thread]", value->value );
				else if ( value->type == LUA_TUSERDATA )
					valueText.format( "0x%016x[userdata]", value->value );
				else if ( value->type == LUA_TLIGHTUSERDATA )
					valueText.format( "0x%016x[lightuserdata]", value->value );
				dataText.format( "\n%s = %s", keyText.getBuffer( ), valueText.getBuffer( ) );
			}
		}

		gTraceText += dataText;
	}

	static int trace( lua_State* lua )
	{
		DECLARE_BEGIN_PROTECTED
		gStackDeep = 0;
		gTraceText.clear( );
		gTraceText = "NSBase.trace:";
		luaStack.enumStack( -1, stackTraceProc, NULL );
		NSLog::log( gTraceText.getBuffer( ) );
		DECLARE_END_PROTECTED
	}

	static int getTick( lua_State* lua )
	{
		DECLARE_BEGIN_PROTECTED
		luaStack << CNSTimer::getCurTick( );
		DECLARE_END_PROTECTED
	}

	static int decodeRsaByPrivkey( lua_State* lua )
	{
		DECLARE_BEGIN_PROTECTED
		static CNSString key;
		luaStack >> key;

		static CNSOctets content;
		luaStack.popOctets( content );
		
		CNSOctets output = content.decodeRsaByPrivkey( key );
		luaStack << output.toValidString( );
		DECLARE_END_PROTECTED
	}

	static int decodeRsaByPublic( lua_State* lua )
	{
		DECLARE_BEGIN_PROTECTED
		static CNSString key;
		luaStack >> key;

		static CNSOctets content;
		luaStack.popOctets( content );

		CNSOctets output = content.decodeRsaByPubkey( key );
		luaStack << output.toValidString( );
		DECLARE_END_PROTECTED
	}

	static int md5Octets( lua_State* lua )
	{
		DECLARE_BEGIN_PROTECTED
		static CNSOctets input;
		luaStack.popOctets( input );

		static CNSOctets result;
		result = input.encodeMD5( );
		luaStack.pushOctets( result );
		DECLARE_END_PROTECTED
	}

	static int md5( lua_State* lua )
	{
		DECLARE_BEGIN_PROTECTED
		static CNSOctets input;
		luaStack.popOctets( input );

		static CNSOctets result;
		result = input.encodeMD5( );

		static CNSString md5Text;
		md5Text = result.toHexString( );
		luaStack << md5Text;
		DECLARE_END_PROTECTED
	}

	static int encodeURI( lua_State* lua )
	{
		DECLARE_BEGIN_PROTECTED
		static CNSString text;
		luaStack >> text;

		static CNSString retText;
		retText = text.encodeURI( );
		luaStack << retText;
		DECLARE_END_PROTECTED
	}

	static int decodeURI( lua_State* lua )
	{
		DECLARE_BEGIN_PROTECTED
		static CNSString text;
		luaStack >> text;

		static CNSString retText;
		retText = text.decodeURI( );
		luaStack << retText;
		DECLARE_END_PROTECTED
	}

	static int encodeBase64( lua_State* lua )
	{
		DECLARE_BEGIN_PROTECTED
		static CNSOctets buffer;
		luaStack.popOctets( buffer );

		static CNSOctets result;
		result = buffer.encodeBase64( );
		luaStack.pushOctets( result );
		DECLARE_END_PROTECTED
	}

	static int decodeBase64( lua_State* lua )
	{
		DECLARE_BEGIN_PROTECTED
		static CNSOctets buffer;
		luaStack.popOctets( buffer );

		static CNSOctets result;
		result = buffer.decodeBase64( );
		luaStack.pushOctets( result );
		DECLARE_END_PROTECTED
	}

	static int encodeHMacSha1( lua_State* lua )
	{
		DECLARE_BEGIN_PROTECTED
		static CNSString key;
		luaStack >> key;

		static CNSOctets content;
		luaStack.popOctets( content );

		static CNSOctets result;
		//		buffer.EncodeHMacSha1( key, result );
		luaStack.pushOctets( result );
		DECLARE_END_PROTECTED
		return 1;
	}

	static int interrupt( lua_State* lua )
	{
		DECLARE_BEGIN_PROTECTED
		NSConsole::interrupt( );
		DECLARE_END_PROTECTED
		return 0;
	}

	int createTimer( lua_State* lua )
	{
		DECLARE_BEGIN_PROTECTED
		unsigned int duration = 0;
		CNSLuaFunction func( __FUNCTION__ );
		luaStack >> duration;
		luaStack >> func;
		if ( func.isValid( ) == false )
			NSException( _UTF8( "Lua函数[createTimer]参数2 不是一个lua函数" ) );

		int timerID = CNSTimer::createTimer( nsTimerLuaHanlder, duration, NULL );
		gTimerHandler.insert( timerID, func );
		lua_pushinteger( lua, timerID );
		DECLARE_END_PROTECTED
		return 1;
	}

	int createDelay( lua_State* lua )
	{
		DECLARE_BEGIN_PROTECTED
		unsigned int duration = 0;
		CNSLuaFunction func( __FUNCTION__ );
		luaStack >> duration;
		luaStack >> func;
		if ( func.isValid( ) == false )
			NSException( _UTF8( "Lua函数[createTimer]参数2 不是一个lua函数" ) );

		int timerID = CNSTimer::createDelay( nsTimerLuaHanlder, duration, NULL );
		gTimerHandler.insert( timerID, func );
		lua_pushinteger( lua, timerID );
		DECLARE_END_PROTECTED
		return 1;
	}

	int removeTimer( lua_State* lua )
	{
		DECLARE_BEGIN_PROTECTED
		unsigned int timerID = luaL_checkint( lua, 1 );
		luaStack >> timerID;
		CNSTimer::removeTimer( timerID );
		NSBase::HMAPINDEX findIndex = gTimerHandler.findNode( timerID );
		if ( findIndex != NULL )
		{
			const CNSLuaFunction& func = gTimerHandler.getValueEx( findIndex );
			luaStack.clearFunc( func );
			gTimerHandler.eraseNode( findIndex );
		}
		DECLARE_END_PROTECTED
		return 0;
	}

	static int setLang( lua_State* lua )
	{
		DECLARE_BEGIN_PROTECTED
		static CNSString lang;
		luaStack >> lang;
		CNSLocal::getNSLocal( ).setLang( lang );
		DECLARE_END_PROTECTED
		return 0;
	}

	static int getLang( lua_State* lua )
	{
		DECLARE_BEGIN_PROTECTED
		const CNSString& lang = CNSLocal::getNSLocal( ).getLang( );
		luaStack << lang;
		DECLARE_END_PROTECTED
		return 1;
	}

	static int getVersion( lua_State* lua )
	{
		DECLARE_BEGIN_PROTECTED
		const CNSString& version = CNSLocal::getNSLocal( ).getVersion( );
		luaStack << version;
		DECLARE_END_PROTECTED
		return 1;
	}

	static int getLangText( lua_State* lua )
	{
		DECLARE_BEGIN_PROTECTED
		int textID = 0;
		luaStack >> textID;
		const CNSString& text = CNSLocal::getNSLocal( ).getLangText( textID );
		luaStack << text;
		DECLARE_END_PROTECTED
		return 1;
	}

	static int getLangInfo( lua_State* lua )
	{
		DECLARE_BEGIN_PROTECTED
		const CNSMap< CNSString, CNSString >& langInfo = CNSLocal::getNSLocal( ).getLangInfo( );
		luaStack << langInfo;
		DECLARE_END_PROTECTED
		return 1;
	}

	void exit( )
	{
		NSBase::CNSLuaStack& luaStack = NSBase::CNSLuaStack::getLuaStack( );
		for ( HLISTINDEX beginIndex = gTimerHandler.getHead( ); beginIndex != NULL; gTimerHandler.getNext( beginIndex ) )
		{
			const CNSLuaFunction& func = gTimerHandler.getValue( beginIndex );
			luaStack.clearFunc( func );
		}
	}
}