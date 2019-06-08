#include <nsbase.h>

namespace NSBase
{
	unsigned int CNSLuaStack::sLuaMemUsed = 0;
	unsigned int CNSLuaStack::sLuaAlloc = 0;
	CNSLuaStack* gpLuaStack = NULL;
	// ************************************************************ //
	// CFBLuaModule
	// ************************************************************ //
	CNSLuaStack::CNSLuaStack( bool enableDebug ) : mEnableDebug( enableDebug )
	{
		mpLuaState = lua_newstate( luaAlloc, NULL );
		::luaL_openlibs( mpLuaState );
	}

	CNSLuaStack::~CNSLuaStack( )
	{
		if ( mFromState == true )
			return;

		if ( mpLuaState == NULL )
			return;

		lua_close( mpLuaState );
	}

	void CNSLuaStack::errorOutput( int errorCode, const CNSString& errorTitle )
	{
		static CNSString errorText;
		errorText.clear( );

		CNSString error = lua_tostring( mpLuaState, -1 );
		error.replace( "\n", "\n" );
		errorText = errorTitle + "\n===============================================\n";

		if ( errorCode == LUA_ERRSYNTAX )
		{
			CNSString fileName;
			int firstSym = error.nocaseFindFirstOf( "\"" );
			int endSym = error.nocaseFindFirstOf( "\"", firstSym + 1 );
			error.copy( fileName, endSym - firstSym - 1, firstSym + 1 );

			CNSString lineNumber;
			firstSym = error.nocaseFindFirstOf( ":", endSym );
			endSym = error.nocaseFindFirstOf( ":", firstSym + 1 );
			error.copy( lineNumber, endSym - firstSym - 1, firstSym + 1 );

			CNSString errorDesc;
			firstSym = error.nocaseFindFirstOf( ":", endSym ) + 1;
			endSym = (int) error.getLength( );
			error.copy( errorDesc, endSym - firstSym - 1, firstSym + 1 );
			errorText = errorText + _UTF8( "lua error: 编译时期错误\n" );
			errorText = errorText + _UTF8( "文件: \n\t[" ) + fileName + "]\n";
			errorText = errorText + _UTF8( "行号: \n\t[" ) + lineNumber + "]\n";
			errorText = errorText + _UTF8( "错误描述: \n\t" ) + errorDesc + "\n";
			errorText = errorText + "===============================================";
			onError( errorText );
			return;
		}

		if ( errorCode == LUA_ERRERR )
			errorText = errorText + _UTF8( "lua error: 错误处理时发生错误\n" );
		if ( errorCode == LUA_ERRMEM )
			errorText = errorText + _UTF8( "lua error: 内存分配错误\n" );
		if ( errorCode == LUA_ERRRUN )
			errorText = errorText + _UTF8( "lua error: 运行时错误\n" );

		CNSString filename;
		int firstSym = error.nocaseFindFirstOf( "\"" );
		int endSym = error.nocaseFindFirstOf( "\"", firstSym + 1 );
		if ( firstSym == -1 || endSym == -1 )
		{
			errorText = errorText + _UTF8( "错误描述:\n\t" ) + error + _UTF8( "\n\t可能是事件绑定函数没有找到\n" );
			errorText = errorText + "===============================================";
			onError( errorText );
			return;
		}
		error.copy( filename, endSym - firstSym - 1, firstSym + 1 );

		CNSString lineNumber;
		firstSym = error.nocaseFindFirstOf( ":", endSym );
		endSym = error.nocaseFindFirstOf( ":", firstSym + 1 );
		error.copy( lineNumber, endSym - firstSym - 1, firstSym + 1 );

		CNSString errorDesc;
		firstSym = endSym;
		endSym = error.nocaseFindFirstOf( "\nstack traceback:", firstSym + 1 );
		if ( endSym == -1 )
			endSym = (int) error.getLength( );
		error.copy( errorDesc, endSym - firstSym - 2, firstSym + 2 );

		errorText = errorText + _UTF8( "文件: \n\t[" ) + filename + "]\n";
		errorText = errorText + _UTF8( "行号: \n\t[" ) + lineNumber + "]\n";
		errorText = errorText + _UTF8( "错误描述: \n\t" ) + errorDesc + "\n";
		errorText = errorText + _UTF8( "调用堆栈: \n" );

		int lineS = 0;
		int lineE = endSym;
		int level = 0;
		while ( lineE != error.getLength( ) - 1 )
		{
			errorText = errorText + _UTF8( "\t层级:" ) + CNSString::number2String( level ++ ) + "\n";

			lineS = error.findFirstOf( "[", lineE );
			if ( lineS == -1 )
				break;

			lineE = error.findFirstOf( "\n", lineS );
			if ( lineE == -1 )
				lineE = (int) error.getLength( ) - 1;

			CNSString line;
			error.copy( line, lineE - lineS + 1, lineS );

			if ( line.findFirstOf( "[string" ) == 0 )
			{
				CNSString fileName;
				CNSString lineNumber;
				CNSString funcName;
				firstSym = line.nocaseFindFirstOf( "\"" );
				endSym = line.nocaseFindFirstOf( "\"", firstSym + 1 );
				line.copy( fileName, endSym - firstSym - 1, firstSym + 1 );

				firstSym = line.nocaseFindFirstOf( ":", endSym );
				endSym = line.nocaseFindFirstOf( ":", firstSym + 1 );
				line.copy( lineNumber, endSym - firstSym - 1, firstSym + 1 );

				firstSym = line.nocaseFindFirstOf( "'", endSym );
				if ( firstSym == -1 )
					firstSym = line.nocaseFindFirstOf( "<", endSym );
				if ( firstSym == -1 )
					firstSym = line.nocaseFindFirstOf( ":", endSym ) + 1;

				endSym = line.nocaseFindFirstOf( "'", firstSym + 1 );
				if ( endSym == -1 )
					endSym = line.nocaseFindFirstOf( ">", firstSym + 1 );
				if ( endSym == -1 )
					endSym = (int) line.getLength( );

				line.copy( funcName, endSym - firstSym - 1, firstSym + 1 );
				errorText = errorText + _UTF8( "\t文件:[" ) + fileName + "]\n";
				errorText = errorText + _UTF8( "\t行号:[" ) + lineNumber + "]\n";

				if ( funcName.findFirstOf( "[string" ) == 0 )
					errorText = errorText + _UTF8( "\t函数:" ) + "\n";
				else
					errorText = errorText + _UTF8( "\t函数:[" ) + funcName + "]\n";
			}

			if ( line.findFirstOf( "[C" ) == 0 )
			{
				CNSString funcName;
				firstSym = line.nocaseFindFirstOf( "'", 0 );
				endSym = line.nocaseFindFirstOf( "'", firstSym + 1 );
				line.copy( funcName, endSym - firstSym - 1, firstSym + 1 );

				errorText = errorText + _UTF8( "\tC++函数:[" ) + funcName + "]\n";
			}

			errorText = errorText + "\t-----------------------------------------------\n";
		}

		errorText = errorText + "===============================================";
		onError( errorText );
	}

	int CNSLuaStack::traceBack( lua_State* pLua )
	{
		const char* tpError = lua_tostring( pLua, 1 );
		// 调用该函数获得调用堆栈
		luaL_traceback( pLua, pLua, tpError, 1 );
		return 1;
	}

	bool CNSLuaStack::complie( const CNSString& chunkName, const char* tpBuffer, int vSize )
	{
		onOpenScript( chunkName, tpBuffer, vSize );

		int luaError = luaL_loadbuffer( mpLuaState, tpBuffer, vSize, chunkName.getBuffer( ) );
		if ( luaError != 0 )
		{
			static CNSString errorDesc;
			errorDesc.format( _UTF8( "打开文件: %s" ), chunkName.getBuffer( ) );
			errorOutput( luaError, errorDesc );
			// stack
			// 1 错误描述
			lua_pop( mpLuaState, 1 );
			return false;
		}

		return true;
	}

	bool CNSLuaStack::openScriptFromMemory( const CNSString& chunkName, const char* buffer, int size )
	{
		onOpenScript( chunkName, buffer, size );
		lua_pushcfunction( mpLuaState, traceBack );
		int luaError = luaL_loadbuffer( mpLuaState, buffer, size, chunkName.getBuffer( ) );
		if ( luaError != 0 )
		{
			static CNSString errorDesc;
			errorDesc.format( _UTF8( "打开文件: %s" ), chunkName.getBuffer( ) );
			errorOutput( luaError, errorDesc );
			// stack
			// 1 错误处理函数
			// 2 错误描述
			lua_pop( mpLuaState, 2 );
			return false;
		}

		luaError = lua_pcall( mpLuaState, 0, 0, -2 );
		if ( luaError != 0 )
		{
			static CNSString errorDesc;
			errorDesc.format( _UTF8( "打开文件: %s" ), chunkName.getBuffer( ) );
			errorOutput( luaError, errorDesc );
			// stack
			// 1 错误处理函数
			// 2 错误描述
			lua_pop( mpLuaState, 2 );
			return false;
		}

		// stack
		// 1 错误处理函数
		lua_pop( mpLuaState, 1 );
		return true;
	}

	bool CNSLuaStack::openScriptFromFile( const CNSString& fileName, const CNSString& chunkName )
	{
		CNSFile file;
		file.openExist( fileName, "rb" );
		
		const CNSOctets buffer;
		file.readAllBytes( buffer );
		return openScriptFromMemory( chunkName, (const char*) buffer.begin( ), buffer.length( ) );
	}

	void* CNSLuaStack::luaAlloc( void* userdata, void* ptr, size_t oldSize, size_t newSize )
	{
#if SHOW_LUAMEM_ALLOC == 1
		sLuaMemUsed -= oldSize;
		sLuaMemUsed += newSize;

		if ( pPointer == NULL )
		{
			CNSCallCounter allocCounter( 4, SHOW_TICK, "lua memory alloc" );
			sLuaAlloc ++;
		}
#endif

		if ( newSize == 0 )
		{
#if SHOW_LUAMEM_ALLOC == 1
			CNSCallCounter allocCounter( 5, SHOW_TICK, "lua memory free" );
			sLuaAlloc --;
#endif

			free( ptr );
			return NULL;
		}
		else
		{
			void* p = realloc( ptr, newSize );
			return p;
		}

		return NULL;
	}

	lua_State* CNSLuaStack::getLuaState( ) const
	{
		return mpLuaState;
	}

	CNSString CNSLuaStack::getTraceBack( )
	{
		CNSString retValue;
		retValue += "================================================";
		retValue += "<BR>";
		retValue += "stack traceback:";
		retValue += "<BR>";

		int level = 0;
		while ( 1 )
		{
			lua_Debug ar;
			if ( lua_getstack( mpLuaState, level, &ar ) == 0 )
				break;

			if ( level > 0 )
				retValue += "&nbsp;&nbsp;&nbsp;&nbsp-----------------------------------------------------------------<BR>";

			lua_getinfo( mpLuaState, "Sln", &ar );
			retValue += CNSString( "&nbsp;&nbsp;&nbsp;&nbsp;layer:" ) + CNSString::number2String( level + 1 ) + "<BR>";
			retValue += CNSString( "&nbsp;&nbsp;&nbsp;&nbsp;file:" ) + ar.source + "<BR>";
			retValue += CNSString( "&nbsp;&nbsp;&nbsp;&nbsp;line:" ) + CNSString::number2String( ar.currentline ) + "<BR>";
			if ( ar.name != NULL )
				retValue += CNSString( "&nbsp;&nbsp;&nbsp;&nbsp;function:" ) + ar.name + "<BR>";
			else
				retValue += CNSString( "&nbsp;&nbsp;&nbsp;&nbsp;function:" ) + ar.source + "<BR>";
			level ++;
		}

		return retValue;
	}

	int CNSLuaStack::queryVar( const CNSString& varName, int& stackIndex )
	{
		lua_Debug ar;
		unsigned int level = 0;

		if ( lua_getstack( mpLuaState, level ++, &ar ) == 0 )
			return 0;

		// 查找局部变量
		int i = 1;
		const char* name = NULL;
		while ( ( name = lua_getlocal( mpLuaState, &ar, i ++ ) ) != NULL )
		{
			int index = lua_gettop( mpLuaState );
			if ( CNSString( name ) == varName )
			{
				stackIndex = index;
				return 3;
			}

			lua_pop( mpLuaState, 1 );
		}

		// 查找上值
		lua_getinfo( mpLuaState, "f", &ar );
		i = 1;
		while ( ( name = lua_getupvalue( mpLuaState, -1, i ++ ) ) != NULL )
		{
			int index = lua_gettop( mpLuaState );
			if ( CNSString( name ) == varName )
			{
				stackIndex = index;
				return 2;
			}

			lua_pop( mpLuaState, 1 );
		}
		lua_pop( mpLuaState, 1 );

		// 查找全局变量
		lua_getglobal( mpLuaState, varName.getBuffer( ) );
		int index = lua_gettop( mpLuaState );
		if ( lua_isnil( mpLuaState, index ) == 0 )
		{
			stackIndex = index;
			return 1;
		}

		lua_pop( mpLuaState, 1 );
		return 0;
	}

	// 建立一个表
	void CNSLuaStack::newTable( )
	{
		::lua_newtable( mpLuaState );
	}

	// 建立一个全局表
	void CNSLuaStack::setGlobalTable( const char* tableName )
	{
		::lua_setglobal( mpLuaState, tableName );
	}

	void CNSLuaStack::newTableField( double index )
	{
		lua_pushnumber( mpLuaState, index );
		lua_newtable( mpLuaState );
	}

	void CNSLuaStack::newTableField( const char* name )
	{
		lua_pushstring( mpLuaState, name );
		lua_newtable( mpLuaState );
	}

	void CNSLuaStack::setFieldTable( )
	{
		lua_settable( mpLuaState, -3 );
	}

	void CNSLuaStack::enumStackTable( int index, FEnumProc proc, void* userdata )
	{
		EnumData data;
		data.type = LUA_TTABLE;
		data.value = (void*) 2;
		lua_pushnil( mpLuaState );  // nil 入栈作为初始 key 
		while ( lua_next( mpLuaState, index ) != 0 )
		{
			int dataType = lua_type( mpLuaState, -1 );
			int keyType = lua_type( mpLuaState, -2 );
			double doubleKey;
			const char* stringKey;
			EnumData key;
			if ( keyType == LUA_TNUMBER )
			{
				doubleKey = lua_tonumber( mpLuaState, -2 );
				key.type = keyType;
				key.value = &doubleKey;
			}
			else if ( keyType == LUA_TSTRING )
			{
				stringKey = lua_tostring( mpLuaState, -2 );
				key.type = keyType;
				key.value = stringKey;
			}
			else
			{
				static CNSString errorDesc;
				errorDesc.format( _UTF8( "参数:%d, 无效key类型[%s]" ), index, luaL_typename( mpLuaState, -2 ) );
				NSException( errorDesc );
			}

			if ( dataType == LUA_TNUMBER )
			{
				double doubleValue = lua_tonumber( mpLuaState, -1 );
				EnumData value;
				value.type = dataType;
				value.value = &doubleValue;
				proc( &data, &key, &value, userdata );
			}
			else if ( dataType == LUA_TBOOLEAN )
			{
				bool boolValue = lua_toboolean( mpLuaState, -1 );
				EnumData value;
				value.type = dataType;
				value.value = &boolValue;
				proc( &data, &key, &value, userdata );
			}
			else if ( dataType == LUA_TSTRING )
			{
				const char* stringValue = lua_tostring( mpLuaState, -1 );
				EnumData value;
				value.type = dataType;
				value.value = stringValue;
				proc( &data, &key, &value, userdata );
			}
			else if ( dataType == LUA_TNIL )
			{
				EnumData value;
				value.type = dataType;
				value.value = NULL;
				proc( &data, &key, &value, userdata );
			}
			else if ( dataType == LUA_TNONE )
			{
				EnumData value;
				value.type = dataType;
				value.value = NULL;
				proc( &data, &key, &value, userdata );
			}
			else if ( dataType == LUA_TFUNCTION )
			{
				const void* addressValue = lua_topointer( mpLuaState, -1 );
				EnumData value;
				value.type = dataType;
				value.value = addressValue;
				proc( &data, &key, &value, userdata );
			}
			else if ( dataType == LUA_TTHREAD )
			{
				const void* addressValue = lua_topointer( mpLuaState, -1 );
				EnumData value;
				value.type = dataType;
				value.value = addressValue;
				proc( &data, &key, &value, userdata );
			}
			else if ( dataType == LUA_TUSERDATA )
			{
				const void* addressValue = lua_topointer( mpLuaState, -1 );
				EnumData value;
				value.type = dataType;
				value.value = addressValue;
				proc( &data, &key, &value, userdata );
			}
			else if ( dataType == LUA_TLIGHTUSERDATA )
			{
				const void* addressValue = lua_topointer( mpLuaState, -1 );
				EnumData value;
				value.type = dataType;
				value.value = addressValue;
				proc( &data, &key, &value, userdata );
			}
			else if ( dataType == LUA_TTABLE )
			{
				// table 开始
				EnumData begin;
				begin.type = dataType;
				begin.value = (void*) 1;
				proc( &begin, &key, NULL, userdata );
				enumStackTable( lua_gettop( mpLuaState ), proc, userdata );
			}
			else
			{
				static CNSString key;
				if ( keyType == LUA_TNUMBER )
				{
					double keyValue = lua_tonumber( mpLuaState, -2 );
					key = CNSString::number2String( keyValue );
				}
				else if ( keyType == LUA_TSTRING )
				{
					const char* keyValue = lua_tostring( mpLuaState, -2 );
					key = keyValue;
				}

				static CNSString errorDesc;
				errorDesc.format( _UTF8( "参数:%d, key:%s value:不支持数据类型[%s]" ), index, key.getBuffer( ), luaL_typename( mpLuaState, -1 ) );
				NSException( errorDesc );
			}

			// 现在栈（-1）是 value，-2 位置是对应的 key 
			// 这里可以判断 key 是什么并且对 value 进行各种处理 
			lua_pop( mpLuaState, 1 );  // 弹出 value，让 key 留在栈顶 
		}

		// table 结束
		EnumData end;
		end.type = LUA_TTABLE;
		end.value = 0;
		proc( &end, NULL, NULL, userdata );
	}

	const CNSLuaStack& CNSLuaStack::operator >> ( const CNSFile& file ) const
	{
		mPopIndex ++;
		popStreamData( NSFunction::removeConst( file ), mPopIndex );
		return *this;
	}

	CNSLuaStack& CNSLuaStack::operator << ( const CNSFile& file )
	{
		mPushIndex ++;
		pushStreamData( file );
		return *this;
	}

	void CNSLuaStack::enumStack( int stackIndex, FEnumProc proc, void* userdata )
	{
		int index = 1;
		int top = 1;
		if ( stackIndex == -1 )
		{
			index = 1;
			top = lua_gettop( mpLuaState );
		}
		else
		{
			index = stackIndex;
			top = stackIndex;
		}

		for ( ; index <= top; index ++ )
		{
			int type = lua_type( mpLuaState, index );
			if ( type == LUA_TNUMBER )
			{
				double value = lua_tonumber( mpLuaState, index );
				EnumData data;
				data.type = type;
				data.value = &value;
				proc( &data, NULL, NULL, userdata );
			}
			else if ( type == LUA_TBOOLEAN )
			{
				bool value = (bool) lua_toboolean( mpLuaState, index );
				EnumData data;
				data.type = type;
				data.value = &value;
				proc( &data, NULL, NULL, userdata );
			}
			else if ( type == LUA_TSTRING )
			{
				const char* value = lua_tostring( mpLuaState, index );
				EnumData data;
				data.type = type;
				data.value = value;
				proc( &data, NULL, NULL, userdata );
			}
			else if ( type == LUA_TNIL )
			{
				EnumData data;
				data.type = type;
				data.value = NULL;
				proc( &data, NULL, NULL, userdata );
			}
			else if ( type == LUA_TNONE )
			{
				EnumData data;
				data.type = type;
				data.value = NULL;
				proc( &data, NULL, NULL, userdata );
			}
			else if ( type == LUA_TFUNCTION )
			{
				const void* value = lua_topointer( mpLuaState, index );
				EnumData data;
				data.type = type;
				data.value = value;
				proc( &data, NULL, NULL, userdata );
			}
			else if ( type == LUA_TTHREAD )
			{
				const void* value = lua_topointer( mpLuaState, index );
				EnumData data;
				data.type = type;
				data.value = value;
				proc( &data, NULL, NULL, userdata );
			}
			else if ( type == LUA_TUSERDATA )
			{
				const void* value = lua_topointer( mpLuaState, index );
				EnumData data;
				data.type = type;
				data.value = value;
				proc( &data, NULL, NULL, userdata );
			}
			else if ( type == LUA_TLIGHTUSERDATA )
			{
				const void* value = lua_topointer( mpLuaState, index );
				EnumData data;
				data.type = type;
				data.value = value;
				proc( &data, NULL, NULL, userdata );
			}
			else if ( type == LUA_TTABLE )
			{
				// table 开始
				EnumData begin;
				begin.type = type;
				begin.value = (void*) 1;
				proc( &begin, NULL, NULL, userdata );
				enumStackTable( index, proc, userdata );
			}
			else
			{
				static CNSString errorDesc;
				errorDesc.format( _UTF8( "参数:%d - 不支持数据类型[%s]" ), index, luaL_typename( mpLuaState, index ) );
				NSException( errorDesc );
			}
		}
	}

	CNSLuaStack& CNSLuaStack::fromState( lua_State* lua )
	{
		CNSLuaStack& luaStack = CNSLuaStack::getLuaStack( );
		static CNSLuaStack newStack( luaStack.mEnableDebug );
		newStack.mpLuaState = lua;
		newStack.mFromState = true;
		newStack.mPopIndex = 0;
		newStack.mPushIndex = 0;
		newStack.mErrorEntry = "";
		newStack.mTranPos = 0;
		return newStack;
	}

	CNSLuaStack CNSLuaStack::fromStateThread( lua_State* lua )
	{
		CNSLuaStack& luaStack = CNSLuaStack::getLuaStack( );
		CNSLuaStack newStack( luaStack.mEnableDebug );
		newStack.mpLuaState = lua;
		newStack.mFromState = true;
		newStack.mPopIndex = 0;
		newStack.mPushIndex = 0;
		newStack.mErrorEntry = "";
		newStack.mTranPos = 0;
		return newStack;
	}

	void CNSLuaStack::init( bool enableDebug )
	{
		gpLuaStack = new CNSLuaStack( enableDebug );
		NSBase::regLuaLib( );
	}

	void CNSLuaStack::exit( )
	{
		if ( gpLuaStack != NULL )
			delete gpLuaStack;
	}

	CNSLuaStack& CNSLuaStack::getLuaStack( )
	{
		if ( gpLuaStack == NULL )
		{
			static CNSString errorDesc;
			errorDesc.format(_UTF8("CNSLuaStack::init 未调用"));
			NSException( errorDesc );
		}

		return *gpLuaStack;
	}

#ifdef PLATFORM_IOS
    void CNSLuaStack::loadLuaDir( const CNSString& filePath, const CNSString& chunkPath )
    {
        DIR* fd = opendir( filePath );
        if ( fd != NULL )
        {
            struct dirent* ent = readdir( fd );
            for ( ; ent != NULL; ent = readdir( fd ) )
            {
                static CNSString luaFile;
                luaFile = ent->d_name;
                if ( luaFile == ".." )
                    continue;
                
                if ( luaFile == "." )
                    continue;
                
                if ( luaFile == ".svn" )
                    continue;
                
                if ( luaFile.nocaseFindFirstOf( ".meta" ) != -1 )
                    continue;
                
                if ( ent->d_type & DT_DIR )
                {
                    CNSString subFilePath = filePath + luaFile + "/";
                    CNSString subChunkPath = chunkPath + luaFile + "/";
                    loadLuaDir( subFilePath, subChunkPath );
                }
                else if ( ent->d_type & DT_REG )
                {
                    if ( luaFile.nocaseFindFirstOf( "main" ) == -1 )
                    {
                        CNSString fileName = filePath + luaFile;
                        CNSString chunkName = chunkPath + luaFile;
                        if ( openScriptFromFile( fileName, chunkName ) == false )
                        {
                            static CNSString errorDesc;
                            errorDesc.format( _UTF8( "lua文件[%s]加载错误" ), fileName.getBuffer( ) );
                            NSException( errorDesc );
                        }
                    }
                }
            }
            closedir( fd );
        }
    }
    
    void CNSLuaStack::preload( const CNSString& workPath, CNSSet< CNSString >& dirList )
    {
        CNSVector< CNSString > preList;
        DIR* fd = opendir( workPath );
        if ( fd != NULL )
        {
            struct dirent* ent = readdir( fd );
            for ( ; ent != NULL; ent = readdir( fd ) )
            {
                static CNSString luaFile;
                luaFile = ent->d_name;
                if ( luaFile == ".." )
                    continue;
                
                if ( luaFile == "." )
                    continue;
                
                if ( luaFile == ".svn" )
                    continue;
                
                if ( luaFile.nocaseFindFirstOf( ".meta" ) != -1 )
                    continue;
                
                if ( ent->d_type & DT_DIR )
                {
                    CNSString luaPath = workPath + "/" + luaFile + "/";
                    dirList.insert( luaFile );
                }
                else if ( ent->d_type & DT_REG )
                    preList.pushback( luaFile );
            }
            closedir( fd );
        }
        
        for ( unsigned int i = 0; i < preList.getCount( ); i ++ )
        {
            CNSString fileName = workPath + "/" + preList[ i ];
            const CNSString& chunkName = preList[ i ];
            if ( openScriptFromFile( fileName, chunkName ) == false )
            {
                static CNSString errorDesc;
                errorDesc.format( _UTF8( "预加载lua文件[%s]错误" ), fileName.getBuffer( ) );
                NSException( errorDesc );
            }
        }
    }
#endif
    
#ifdef PLATFORM_WIN32
	void CNSLuaStack::loadLuaDir( const CNSString& filePath, const CNSString& chunkPath )
	{
		_tfinddata_t fd;
		intptr_t findPtr = ::_tfindfirst( (wchar_t*) CNSString::toTChar( filePath + "*.*" ), &fd );
		if ( findPtr != -1 )
		{
			int tFindNext = 0;
			for ( ; tFindNext == 0; tFindNext = ::_tfindnext( findPtr, &fd ) )
			{
				static CNSString luaFile;
				luaFile = CNSString::fromTChar((char*) fd.name );
				if ( luaFile == ".." )
					continue;

				if ( luaFile == "." )
					continue;

				if ( luaFile == ".svn" )
					continue;

				if ( luaFile.nocaseFindFirstOf( ".meta" ) != -1 )
					continue;

				if ( fd.attrib & _A_SUBDIR )
				{
					CNSString subFilePath = filePath + luaFile + "/";
					CNSString subChunkPath = chunkPath + luaFile + "/";
					loadLuaDir( subFilePath, subChunkPath );
				}
				else
				{
					if ( CNSString::fromTChar( (char*) fd.name ).nocaseFindFirstOf( "main" ) == -1 )
					{
						CNSString fileName = filePath + luaFile;
						CNSString chunkName = chunkPath + luaFile;
						if ( openScriptFromFile( fileName, chunkName ) == false )
						{
							static CNSString errorDesc;
							errorDesc.format( _UTF8( "lua文件[%s]加载错误" ), fileName.getBuffer( ) );
							NSException( errorDesc );
						}
					}
				}
			}
		}
		::_findclose( findPtr );
	}

	void CNSLuaStack::preload( const CNSString& workPath, CNSSet< CNSString >& dirList )
	{
		CNSVector< CNSString > preList;
		_finddata_t fd;
		intptr_t findPtr = ::_findfirst( workPath + "/*.*", &fd );
		if ( findPtr != -1 )
		{
			int findNext = 0;
			for ( ; findNext == 0; findNext = ::_findnext( findPtr, &fd ) )
			{
				static CNSString luaFile;
				luaFile = fd.name;
				if ( luaFile == ".." )
					continue;

				if ( luaFile == "." )
					continue;

				if ( luaFile == ".svn" )
					continue;

				if ( luaFile.nocaseFindFirstOf( ".meta" ) != -1 )
					continue;

				if ( fd.attrib & _A_SUBDIR )
				{
					CNSString luaPath = workPath + "/" + fd.name + "/";
					dirList.insert( fd.name );
				}
				else
					preList.pushback( fd.name );
			}
		}
		::_findclose( findPtr );

		for ( unsigned int i = 0; i < preList.getCount( ); i ++ )
		{
			CNSString fileName = workPath + "/" + preList[ i ];
			const CNSString& chunkName = preList[ i ];
			if ( openScriptFromFile( fileName, chunkName ) == false )
			{
				static CNSString errorDesc;
				errorDesc.format( _UTF8( "预加载lua文件[%s]错误" ), fileName.getBuffer( ) );
				NSException( errorDesc );
			}
		}
	}
#endif
    
	void CNSLuaStack::loadMod( const CNSString& workPath, const CNSString& modName )
	{
		CNSString luaPath = workPath + "/" + modName + "/";
		CNSString chunkPath = modName + "/";
		CNSString fileName = luaPath + modName + "main.lua";
		CNSString chunkName = chunkPath + modName + "main.lua";
		if ( openScriptFromFile( fileName, chunkName ) == false )
		{
			static CNSString errorDesc;
			errorDesc.format( _UTF8( "模块[%s]入口加载错误" ), modName.getBuffer( ) );
			NSException( errorDesc );
		}

		loadLuaDir( luaPath, chunkPath );
		NSLog::log( _UTF8( "模块[%s]加载成功" ), modName.getBuffer( ) );
	}

	void CNSLuaStack::load( const CNSString& workPath )
	{
		CNSSet< CNSString > dirList;
		preload( workPath, dirList );
		HLISTINDEX beginIndex = dirList.getHead( );
		for ( ; beginIndex != NULL; dirList.getNext( beginIndex ) )
		{
			const CNSString& modName = dirList.getKey( beginIndex );
			loadMod( workPath, modName );
		}
	}

	void CNSLuaStack::load( const CNSString& workPath, const CNSVector< CNSString > modList )
	{
		CNSSet< CNSString > dirList;
		preload( workPath, dirList );
		for ( unsigned int i = 0; i < modList.getCount( ); i ++ )
		{
			const CNSString& modName = modList[ i ];
			if ( dirList.find( modName ) == false )
			{
				NSLog::log( _UTF8( "模块[%s]没有定义, 该模块未加载" ), modName.getBuffer( ) );
				continue;
			}

			loadMod( workPath, modName );
			dirList.erase( modName );
		}

		if ( dirList.getCount( ) > 0 )
		{
			NSLog::log( _UTF8( "下面的模块加载顺序未定义" ) );
			HLISTINDEX beginIndex = dirList.getHead( );
			for ( ; beginIndex != NULL; dirList.getNext( beginIndex ) )
			{
				const CNSString& modName = dirList.getKey( beginIndex );
				loadMod( workPath, modName );
			}
		}
	}

	void CNSLuaStack::regLib( const char* libName, const luaL_Reg* lib )
	{
		lua_createtable( mpLuaState, 0, (int) ( sizeof( lib ) / sizeof( ( lib )[ 0 ] ) - 1 ) );
		luaL_setfuncs( mpLuaState, lib, 0 );
		lua_setglobal( mpLuaState, libName );
	}

	void CNSLuaStack::onError( const CNSString& errorText )
	{
		NSLog::exception( errorText.getBuffer( ) );
	}

	void CNSLuaStack::onOpenScript( const CNSString& chunkName, const char* buffer, unsigned int size )
	{
#ifdef PLATFORM_WIN32
		if ( mEnableDebug == true )
		{
			static CNSString scriptBuffer;
			scriptBuffer.clear( );
			scriptBuffer.pushback( buffer, size );
			NSConsole::addLuaFile( chunkName, scriptBuffer );
		}
#endif
	}
}
