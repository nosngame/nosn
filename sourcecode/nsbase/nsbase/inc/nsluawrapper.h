#pragma once
namespace NSBase
{
#define DECLARE_BEGIN_PROTECTED					\
	NSBase::CNSLuaStack& luaStack = NSBase::CNSLuaStack::fromState( lua ); \
	try											\
	{											\

#define DECLARE_END_PROTECTED					\
	}											\
	catch( CNSException& e )					\
	{											\
		luaL_error( lua, e.mErrorDesc );		\
	}											\
	return luaStack.getArgCount( );				\

#define BEGIN_EXPORT( LibName )				static const struct luaL_Reg LibName[] = {
#define EXPORT_FUNC( FuncName )				{ #FuncName, FuncName },
#define END_EXPORT							{ NULL, NULL } };
#define DOUBLE_LIMIT						999999
	class CNSLuaMarshal
	{
	public:
		virtual CNSLuaStack& marshal( CNSLuaStack& luaStack ) const = 0;
		virtual const CNSLuaStack& unmarshal( const CNSLuaStack& luaStack ) = 0;
	};

	class CNSLuaObject
	{
	};

	class CNSLuaFunction
	{
	public:
		int mLuaRef = -1;
		// 从第几个参数读取的函数
		int mPopIndex = -1;
		// 记录下设置该函数的C++函数名
		CNSString mFuncName;

	public:
		CNSLuaFunction( const char* funcName = NULL ) : mFuncName( funcName )
		{
		}

	public:
		bool isValid( ) const
		{
			return mLuaRef != -1;
		}
	};

	// 需要被导出到lua的对象从这个类派生，lua获得的弱引用
	class CNSLuaWeakRef
	{
		CNSSet< void** > mLuaRefs;

	public:
		void setLuaRef( void** luaRef )
		{
			mLuaRefs.insert( luaRef );
		}

		void removeLuaRef( void** luaRef )
		{
			mLuaRefs.erase( luaRef );
		}

		void cleanUpRef( )
		{
			HLISTINDEX beginIndex = mLuaRefs.getHead( );
			for ( ; beginIndex != NULL; mLuaRefs.getNext( beginIndex ) )
			{
				void** objRef = mLuaRefs.getKey( beginIndex );
				*objRef = NULL;
			}
		}
	};

	template< typename T >
	inline void popStreamNumber( T& stream, double value )
	{
		if ( value >= DOUBLE_LIMIT || value <= -DOUBLE_LIMIT )
		{
			stream << (char) TYPE_DOUBLE;
			stream << value;
		}
		else
		{
			if ( NSFunction::floor( value ) == value )
			{
				if ( value <= UCHAR_MAX && value >= 0 )
				{
					stream << (char) TYPE_UCHAR;
					stream << (unsigned char) value;
				}
				else if ( value <= CHAR_MAX && value >= CHAR_MIN )
				{
					stream << (char) TYPE_CHAR;
					stream << (char) value;
				}
				else if ( value <= USHRT_MAX && value >= 0 )
				{
					stream << (char) TYPE_USHORT;
					stream << (unsigned short) value;
				}
				else if ( value <= SHRT_MAX && value >= SHRT_MIN )
				{
					stream << (char) TYPE_SHORT;
					stream << (short) value;
				}
			}
			else
			{
				stream << (char) TYPE_FLOAT;
				stream << (float) value;
			}
		}
	}

	class CNSLuaStack
	{
	protected:
		lua_State* mpLuaState = NULL;
		bool mFromState = false;
		bool mEnableDebug = false;
		int mPushIndex = 0;
		mutable int mTranPos = 0;
		mutable int mPopIndex = 0;
		CNSString mErrorEntry;

	public:
		typedef struct EnumData
		{
			int type;
			const void* value;
		} EnumData;

		typedef void( *FEnumProc )( EnumData* value, EnumData* key, EnumData* data, void* userdata );
		static unsigned int	sLuaMemUsed;
		static unsigned int	sLuaAlloc;

	public:
		CNSLuaStack( bool enableDebug );

	public:
		~CNSLuaStack( );

	private:
		CNSLuaStack( const CNSLuaStack& rhs )
		{
		}

		CNSLuaStack& operator = ( const CNSLuaStack& rhs )
		{
			return *this;
		}

	public:
		// 脚本编译
		bool complie( const CNSString& chunkName, const char* tpBuffer, int size );

		// 打开脚本
		bool openScriptFromFile( const CNSString& fileName, const CNSString& chunkName );

		// 通过Buffer打开脚本
		bool openScriptFromMemory( const CNSString& chunkName, const char* tpBuffer, int size );

		// 得到lua状态机
		lua_State* getLuaState( ) const;

		// 错误打印
		void errorOutput( int error, const CNSString& errorTitle );

		// 建立一张新表
		void newTable( );
		void setGlobalTable( const char* tableName );
		void newTableField( double index );
		void newTableField( const char* name );
		void pushTableField( const char* name, int index = -2 );
		void setFieldTable( );
		// stackIndex 等于-1表示枚举整个栈，否者枚举指定索引的栈位置
		void enumStack( int index, FEnumProc proc, void* userdata );
		void load( const CNSString& workPath );
		void load( const CNSString& workPath, const CNSVector< CNSString > modList );
		int queryVar( const CNSString& name, int& index );
		void regLib( const char* libName, const luaL_Reg* lib );

		int getArgCount( ) const
		{
			return mPushIndex;
		}

		template< typename T >
		bool call( T& ret )
		{
			int argCount = getArgCount( );
			int funcIndex = lua_gettop( mpLuaState ) - argCount;
			if ( lua_isfunction( mpLuaState, funcIndex ) == 0 )
				NSException( "call之前没有调用preCall" );

			int luaError = lua_pcall( mpLuaState, argCount, 1, funcIndex - 1 );
			if ( luaError != 0 )
			{
				static CNSString errorDesc;
				errorDesc = mErrorEntry;
				errorOutput( luaError, errorDesc );
				// stack
				// 1 错误处理函数
				// 2 错误描述
				lua_pop( mpLuaState, 2 );
				return false;
			}

			mPopIndex = funcIndex - 1;
			*this >> ret;

			// stack
			// 1 错误处理函数
			// 2 返回值
			lua_pop( mpLuaState, 2 );
			return true;
		}

		bool call( )
		{
			int argCount = getArgCount( );
			int funcIndex = lua_gettop( mpLuaState ) - argCount;
			if ( lua_isfunction( mpLuaState, funcIndex ) == 0 )
				NSException( "call之前没有调用preCall" );

			int luaError = lua_pcall( mpLuaState, argCount, 0, funcIndex - 1 );
			if ( luaError != 0 )
			{
				static CNSString errorDesc;
				errorDesc = mErrorEntry;
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

		// 调用函数前
		void preCall( const CNSLuaFunction& func )
		{
			mPushIndex = 0;
			if ( lua_gettop( mpLuaState ) > 0 )
				NSException( "lua栈发生泄露" );

			lua_pushcfunction( mpLuaState, traceBack );
			lua_rawgeti( mpLuaState, LUA_REGISTRYINDEX, func.mLuaRef );
			mErrorEntry.format( _UTF8( "点燃函数: %s 参数: %d" ), func.mFuncName.getBuffer( ), func.mPopIndex );
		}

		// 调用函数前
		void preCall( const char* funcName )
		{
			mPushIndex = 0;
			if ( lua_gettop( mpLuaState ) > 0 )
				NSException( "lua栈发生泄露" );

			lua_pushcfunction( mpLuaState, traceBack );
			lua_getglobal( mpLuaState, funcName );
			mErrorEntry.format( _UTF8( "点燃函数: %s" ), funcName );
		}

		void clearFunc( const CNSLuaFunction& func )
		{
			luaL_unref( mpLuaState, LUA_REGISTRYINDEX, func.mLuaRef );
		}

		CNSLuaStack& operator >> ( const CNSLuaFunction& func )
		{
			mPopIndex ++;
			int type = lua_type( mpLuaState, mPopIndex );
			if ( type == LUA_TFUNCTION )
			{
				// luaL_ref读取栈底数据，这里要把函数拷贝到栈底
				lua_pushvalue( mpLuaState, mPopIndex );
				// luaL_ref读取栈底数据之后，会删除栈底数据
				NSFunction::removeConst( func ).mLuaRef = luaL_ref( mpLuaState, LUA_REGISTRYINDEX );
				NSFunction::removeConst( func ).mPopIndex = mPopIndex;
			}
			return *this;
		}

		template < typename T >
		CNSLuaStack& operator << ( T* value )
		{
			mPushIndex ++;
			T::pushUserData( *this, value );
			return *this;
		}

		template < typename T >
		const CNSLuaStack& operator >> ( T*& value ) const
		{
			mPopIndex ++;
			T::popUserData( *this, value );
			return *this;
		}

		const CNSLuaStack& popOctets( CNSOctets& value ) const
		{
			mPopIndex ++;
			int type = lua_type( mpLuaState, mPopIndex );
			if ( type != LUA_TSTRING )
			{
				static CNSString errorDesc;
				errorDesc.format( _UTF8( "参数[%d]类型是[%s] 不是string类型" ), mPopIndex, lua_typename( mpLuaState, type ) );
				NSException( errorDesc );
			}

			size_t len = 0;
			const char* text = lua_tolstring( mpLuaState, mPopIndex, &len );
			value.insert( value.end( ), text, len );
			return *this;
		}

		CNSLuaStack& pushOctets( const CNSOctets& value )
		{
			mPushIndex ++;
			lua_pushlstring( mpLuaState, (const char*) value.begin( ), value.length( ) );
			return *this;
		}

		// 给lua表字段赋值
		template < typename T >
		void pushField( double numberKey, const T& data )
		{
			lua_pushnumber( mpLuaState, numberKey );
			*this << (T&) data;
			//还原mPushIndex
			mPushIndex --;
			lua_settable( mpLuaState, -3 );
		}

		// 给lua表字段赋值
		template < typename T >
		void pushField( const char* stringKey, const T& data )
		{
			lua_pushstring( mpLuaState, stringKey );
			*this << (T&) data;
			//还原mPushIndex
			mPushIndex --;
			lua_settable( mpLuaState, -3 );
		}

		// 从lua表读取字段值
		template < typename T >
		void popField( double numberKey, T& data ) const
		{
			lua_pushnumber( mpLuaState, numberKey );
			lua_gettable( mpLuaState, mPopIndex );
			// 把栈顶读出来的值放在mPopIndex的后面
			lua_insert( mpLuaState, mPopIndex + 1 );
			// 从mPopIndex + 1的位置读取数值
			*this >> data;
			//还原mPopIndex
			mPopIndex --;
			// 删除栈mPopIndex + 1位置的数值
			lua_remove( mpLuaState, mPopIndex + 1 );
		}

		template < typename T >
		void popField( const char* stringKey, T& data ) const
		{
			lua_pushstring( mpLuaState, stringKey );
			lua_gettable( mpLuaState, mPopIndex );
			// 把栈顶读出来的值放在mPopIndex的后面
			lua_insert( mpLuaState, mPopIndex + 1 );
			// 从mPopIndex + 1的位置读取数值
			*this >> data;
			//还原mPopIndex
			mPopIndex --;
			// 删除栈mPopIndex + 1位置的数值
			lua_remove( mpLuaState, mPopIndex + 1 );
		}

		bool isEnd( )
		{
			return lua_isnoneornil( mpLuaState, mPopIndex + 1 ) == 1;
		}

		const CNSLuaStack& operator >> ( ETransaction trans ) const
		{
			switch ( trans )
			{
				case NSBase::EBegin:
					mTranPos = lua_gettop( mpLuaState );
					break;
				case NSBase::ERollback:
					lua_settop( mpLuaState, mTranPos );
					break;
				case NSBase::ECommit:
					break;
			}
			return *this;
		}

		const CNSLuaStack& operator >> ( const bool& value ) const
		{
			mPopIndex ++;
			int type = lua_type( mpLuaState, mPopIndex );
			if ( type != LUA_TBOOLEAN )
			{
				static CNSString errorDesc;
				errorDesc.format( _UTF8( "参数[%d]类型是[%s] 不是boolean类型" ), mPopIndex, lua_typename( mpLuaState, type ) );
				NSException( errorDesc );
			}

			NSFunction::removeConst( value ) = lua_toboolean( mpLuaState, mPopIndex );
			return *this;
		}

		const CNSLuaStack& operator >> ( const unsigned char& value ) const
		{
			mPopIndex ++;
			int type = lua_type( mpLuaState, mPopIndex );
			if ( type != LUA_TNUMBER )
			{
				static CNSString errorDesc;
				errorDesc.format( _UTF8( "参数[%d]类型是[%s] 不是number类型" ), mPopIndex, lua_typename( mpLuaState, type ) );
				NSException( errorDesc );
			}

			NSFunction::removeConst( value ) = (unsigned char) lua_tounsigned( mpLuaState, mPopIndex );
			return *this;
		}

		const CNSLuaStack& operator >> ( const char& value ) const
		{
			mPopIndex ++;
			int type = lua_type( mpLuaState, mPopIndex );
			if ( type != LUA_TNUMBER )
			{
				static CNSString errorDesc;
				errorDesc.format( _UTF8( "参数[%d]类型是[%s] 不是number类型" ), mPopIndex, lua_typename( mpLuaState, type ) );
				NSException( errorDesc );
			}

			NSFunction::removeConst( value ) = (char) lua_tointeger( mpLuaState, mPopIndex );
			return *this;
		}

		const CNSLuaStack& operator >> ( const unsigned short& value ) const
		{
			mPopIndex ++;
			int type = lua_type( mpLuaState, mPopIndex );
			if ( type != LUA_TNUMBER )
			{
				static CNSString errorDesc;
				errorDesc.format( _UTF8( "参数[%d]类型是[%s] 不是number类型" ), mPopIndex, lua_typename( mpLuaState, type ) );
				NSException( errorDesc );
			}

			NSFunction::removeConst( value ) = (unsigned short) lua_tounsigned( mpLuaState, mPopIndex );
			return *this;
		}

		const CNSLuaStack& operator >> ( const short& value ) const
		{
			mPopIndex ++;
			int type = lua_type( mpLuaState, mPopIndex );
			if ( type != LUA_TNUMBER )
			{
				static CNSString errorDesc;
				errorDesc.format( _UTF8( "参数[%d]类型是[%s] 不是number类型" ), mPopIndex, lua_typename( mpLuaState, type ) );
				NSException( errorDesc );
			}

			NSFunction::removeConst( value ) = (short) lua_tointeger( mpLuaState, mPopIndex );
			return *this;
		}

		const CNSLuaStack& operator >> ( const unsigned int& value ) const
		{
			mPopIndex ++;
			int type = lua_type( mpLuaState, mPopIndex );
			if ( type != LUA_TNUMBER )
			{
				static CNSString errorDesc;
				errorDesc.format( _UTF8( "参数[%d]类型是[%s] 不是number类型" ), mPopIndex, lua_typename( mpLuaState, type ) );
				NSException( errorDesc );
			}

			NSFunction::removeConst( value ) = (unsigned int) lua_tounsigned( mpLuaState, mPopIndex );
			return *this;
		}

		const CNSLuaStack& operator >> ( const int& value ) const
		{
			mPopIndex ++;
			int type = lua_type( mpLuaState, mPopIndex );
			if ( type != LUA_TNUMBER )
			{
				static CNSString errorDesc;
				errorDesc.format( _UTF8( "参数[%d]类型是[%s] 不是number类型" ), mPopIndex, lua_typename( mpLuaState, type ) );
				NSException( errorDesc );
			}

			NSFunction::removeConst( value ) = (int) lua_tointeger( mpLuaState, mPopIndex );
			return *this;
		}

		const CNSLuaStack& operator >> ( const unsigned long long& value ) const
		{
			mPopIndex ++;
			int type = lua_type( mpLuaState, mPopIndex );
			if ( type != LUA_TNUMBER )
			{
				static CNSString errorDesc;
				errorDesc.format( _UTF8( "参数[%d]类型是[%s] 不是number类型" ), mPopIndex, lua_typename( mpLuaState, type ) );
				NSException( errorDesc );
			}

			NSFunction::removeConst( value ) = ( unsigned long long ) lua_tonumber( mpLuaState, mPopIndex );
			return *this;
		}

		const CNSLuaStack& operator >> ( const long long& value ) const
		{
			mPopIndex ++;
			int type = lua_type( mpLuaState, mPopIndex );
			if ( type != LUA_TNUMBER )
			{
				static CNSString errorDesc;
				errorDesc.format( _UTF8( "参数[%d]类型是[%s] 不是number类型" ), mPopIndex, lua_typename( mpLuaState, type ) );
				NSException( errorDesc );
			}

			NSFunction::removeConst( value ) = (long long) lua_tonumber( mpLuaState, mPopIndex );
			return *this;
		}

		const CNSLuaStack& operator >> ( const float& value ) const
		{
			mPopIndex ++;
			int type = lua_type( mpLuaState, mPopIndex );
			if ( type != LUA_TNUMBER )
			{
				static CNSString errorDesc;
				errorDesc.format( _UTF8( "参数[%d]类型是[%s] 不是number类型" ), mPopIndex, lua_typename( mpLuaState, type ) );
				NSException( errorDesc );
			}

			NSFunction::removeConst( value ) = (float) lua_tonumber( mpLuaState, mPopIndex );
			return *this;
		}

		const CNSLuaStack& operator >> ( const double& value ) const
		{
			mPopIndex ++;
			int type = lua_type( mpLuaState, mPopIndex );
			if ( type != LUA_TNUMBER )
			{
				static CNSString errorDesc;
				errorDesc.format( _UTF8( "参数[%d]类型是[%s] 不是number类型" ), mPopIndex, lua_typename( mpLuaState, type ) );
				NSException( errorDesc );
			}

			NSFunction::removeConst( value ) = lua_tonumber( mpLuaState, mPopIndex );
			return *this;
		}

		const CNSLuaStack& operator >> ( const CNSString& text ) const
		{
			mPopIndex ++;
			int type = lua_type( mpLuaState, mPopIndex );
			if ( type != LUA_TSTRING )
			{
				static CNSString errorDesc;
				errorDesc.format( _UTF8( "参数[%d]类型是[%s] 不是string类型" ), mPopIndex, lua_typename( mpLuaState, type ) );
				NSException( errorDesc );
			}

			const char* buf = lua_tostring( mpLuaState, mPopIndex );
			NSFunction::removeConst( text ) = buf;
			return *this;
		}

		const CNSLuaStack& operator >> ( const CNSLuaMarshal& marshal ) const
		{
			mPopIndex ++;
			NSFunction::removeConst( marshal ).unmarshal( *this );
			return *this;
		}

		template< typename T1, typename T2 > const CNSLuaStack& operator >> ( const CNSPair< T1, T2 >& pair ) const
		{
			mPopIndex ++;
			T1 value1;
			T2 value2;

			popField( "mFirst", value1 );
			popField( "mSecond", value2 );

			NSFunction::removeConst( pair ).mFirst = value1;
			NSFunction::removeConst( pair ).mSecond = value2;
			return *this;
		}

		template< typename T > const CNSLuaStack& operator >> ( const CNSVector< T >& vector ) const
		{
			mPopIndex ++;
			int index = 0;
			T value;
			NSFunction::removeConst( vector ).clear( );
			// nil 入栈作为初始 key 
			lua_pushnil( mpLuaState );
			while ( lua_next( mpLuaState, mPopIndex ) != 0 )
			{
				int oldIndex = mPopIndex;
				*this >> index;
				*this >> value;
				mPopIndex = oldIndex;

				NSFunction::removeConst( vector ).pushback( value );
				lua_pop( mpLuaState, 1 );
			}

			return *this;
		}

		template< typename T > const CNSLuaStack& operator >> ( const CNSList< T >& list ) const
		{
			mPopIndex ++;
			int index = 0;
			T value;
			NSFunction::removeConst( list ).clear( );
			// nil 入栈作为初始 key 
			lua_pushnil( mpLuaState );
			while ( lua_next( mpLuaState, mPopIndex ) != 0 )
			{
				int oldIndex = mPopIndex;
				*this >> index;
				*this >> value;
				mPopIndex = oldIndex;

				NSFunction::removeConst( list ).pushback( value );
				lua_pop( mpLuaState, 1 );
			}

			return *this;
		}

		template< typename Key, typename T > const CNSLuaStack& operator >> ( const CNSMap< Key, T >& map ) const
		{
			mPopIndex ++;
			Key key;
			T value;
			NSFunction::removeConst( map ).clear( );
			// nil 入栈作为初始 key 
			lua_pushnil( mpLuaState );
			while ( lua_next( mpLuaState, mPopIndex ) != 0 )
			{
				int oldIndex = mPopIndex;
				*this >> key;
				*this >> value;
				mPopIndex = oldIndex;

				NSFunction::removeConst( map ).insert( key, value );
				lua_pop( mpLuaState, 1 );
			}

			return *this;
		}

		template< typename Key > const CNSLuaStack& operator >> ( const CNSSet< Key >& set ) const
		{
			mPopIndex ++;
			Key key;
			int value;
			NSFunction::removeConst( set ).clear( );
			// nil 入栈作为初始 key 
			lua_pushnil( mpLuaState );
			while ( lua_next( mpLuaState, mPopIndex ) != 0 )
			{
				int oldIndex = mPopIndex;
				*this >> key;
				*this >> value;
				mPopIndex = oldIndex;

				NSFunction::removeConst( set ).insert( key );
				lua_pop( mpLuaState, 1 );
			}

			return *this;
		}

		template< typename Key, typename T > const CNSLuaStack& operator >> ( const CNSHashMap< Key, T >& hashMap ) const
		{
			mPopIndex ++;
			Key key;
			T value;
			NSFunction::removeConst( hashMap ).clear( );
			// nil 入栈作为初始 key 
			lua_pushnil( mpLuaState );
			while ( lua_next( mpLuaState, mPopIndex ) != 0 )
			{
				int oldIndex = mPopIndex;
				*this >> key;
				*this >> value;
				mPopIndex = oldIndex;

				NSFunction::removeConst( hashMap ).insert( key, value );
				lua_pop( mpLuaState, 1 );
			}

			return *this;
		}

		const CNSLuaStack& operator >> ( const CNSOctetsStream& stream ) const
		{
			mPopIndex ++;
			popStreamData( NSFunction::removeConst( stream ), mPopIndex );
			return *this;
		}

		const CNSLuaStack& operator >> ( const CNSFile& file ) const;
		CNSLuaStack& operator << ( const bool value )
		{
			mPushIndex ++;
			lua_pushboolean( mpLuaState, value );
			return *this;
		}

		CNSLuaStack& operator << ( const unsigned char value )
		{
			mPushIndex ++;
			lua_pushunsigned( mpLuaState, value );
			return *this;
		}

		CNSLuaStack& operator << ( const char value )
		{
			mPushIndex ++;
			lua_pushinteger( mpLuaState, value );
			return *this;
		}

		CNSLuaStack& operator << ( const unsigned short value )
		{
			mPushIndex ++;
			lua_pushunsigned( mpLuaState, value );
			return *this;
		}

		CNSLuaStack& operator << ( const short value )
		{
			mPushIndex ++;
			lua_pushinteger( mpLuaState, value );
			return *this;
		}

		CNSLuaStack& operator << ( const unsigned int value )
		{
			mPushIndex ++;
			lua_pushunsigned( mpLuaState, value );
			return *this;
		}

		CNSLuaStack& operator << ( const int value )
		{
			mPushIndex ++;
			lua_pushinteger( mpLuaState, value );
			return *this;
		}

		CNSLuaStack& operator << ( const unsigned long long value )
		{
			mPushIndex ++;
			// 这里会损失精度，lua5.2不支持64位整数
			lua_pushnumber( mpLuaState, (double) value );
			return *this;
		}

		CNSLuaStack& operator << ( const long long value )
		{
			mPushIndex ++;
			// 这里会损失精度，lua5.2不支持64位整数
			lua_pushnumber( mpLuaState, (double) value );
			return *this;
		}

		CNSLuaStack& operator << ( const float value )
		{
			mPushIndex ++;
			lua_pushnumber( mpLuaState, value );
			return *this;
		}

		CNSLuaStack& operator << ( const double value )
		{
			mPushIndex ++;
			lua_pushnumber( mpLuaState, value );
			return *this;
		}

		CNSLuaStack& operator << ( const CNSString& text )
		{
			mPushIndex ++;
			lua_pushstring( mpLuaState, text.getBuffer( ) );
			return *this;
		}

		CNSLuaStack& operator << ( const CNSLuaMarshal& marshal )
		{
			mPushIndex ++;
			newTable( );
			return marshal.marshal( *this );
		}

		template< typename T1, typename T2 > CNSLuaStack& operator << ( const CNSPair< T1, T2 >& pair )
		{
			mPushIndex ++;
			newTable( );
			pushField( "mFirst", pair.mFirst );
			pushField( "mSecond", pair.mSecond );
			return *this;
		}

		template< typename T > CNSLuaStack& operator << ( const CNSVector< T >& vector )
		{
			mPushIndex ++;
			newTable( );
			for ( unsigned int i = 0; i < vector.getCount( ); i ++ )
				pushField( i + 1, vector[ i ] );
			return *this;
		}

		template< typename T > CNSLuaStack& operator << ( const CNSList< T >& list )
		{
			mPushIndex ++;
			newTable( );
			HLISTINDEX beginIndex = list.getHead( );
			for ( int i = 0; beginIndex != NULL; list.getNext( beginIndex ), i ++ )
				pushField( i + 1, list.getData( beginIndex ) );

			return *this;
		}

		// 只有字符串和数值作为key才能被传入lua
		template< typename Key, typename T > CNSLuaStack& operator << ( const CNSMap< Key, T >& map )
		{
			mPushIndex ++;
			newTable( );
			HLISTINDEX beginIndex = map.getHead( );
			for ( ; beginIndex != NULL; map.getNext( beginIndex ) )
				pushField( map.getKey( beginIndex ), map.getValue( beginIndex ) );

			return *this;
		}

		// 只有字符串和数值作为key才能被传入lua
		template< typename Key > CNSLuaStack& operator << ( const CNSSet< Key >& set )
		{
			mPushIndex ++;
			newTable( );
			HLISTINDEX beginIndex = set.getHead( );
			for ( ; beginIndex != NULL; set.getNext( beginIndex ) )
				pushField( set.getKey( beginIndex ), 0 );

			return *this;
		}

		// 只有字符串和数值作为key才能被传入lua
		template< typename Key, typename T > CNSLuaStack& operator << ( const CNSHashMap< Key, T >& hashMap )
		{
			mPushIndex ++;
			newTable( );
			HLISTINDEX beginIndex = hashMap.getHead( );
			for ( ; beginIndex != NULL; hashMap.rHashMap( beginIndex ) )
				pushField( CNSPair< Key, T >( hashMap.getKey( beginIndex ), hashMap.getValue( beginIndex ) ) );

			return *this;
		}

		CNSLuaStack& operator << ( const CNSOctetsStream& stream )
		{
			mPushIndex ++;
			pushStreamData( stream );
			return *this;
		}

		CNSLuaStack& operator << ( const CNSOctetsShadow& shadow )
		{
			mPushIndex ++;
			pushStreamData( shadow );
			return *this;
		}

		CNSLuaStack& operator << ( const CNSFile& file );

	protected:
		template< typename T >
		void pushStreamData( const T& stream )
		{
			char dataType;
			stream >> dataType;
			if ( dataType == TYPE_DOUBLE )
			{
				double value = 0;
				stream >> value;

				int fpc = _fpclass( value );
				if ( fpc != _FPCLASS_NN && fpc != _FPCLASS_PN && fpc != _FPCLASS_PZ && fpc != _FPCLASS_NZ )
					NSException( _UTF8( "函数[pushStreamData]发生异常, 非法浮点数" ) );

				lua_pushnumber( mpLuaState, value );
			}
			else if ( dataType == TYPE_FLOAT )
			{
				float value = 0;
				stream >> value;

				int fpc = _fpclass( value );
				if ( fpc != _FPCLASS_NN && fpc != _FPCLASS_PN && fpc != _FPCLASS_PZ && fpc != _FPCLASS_NZ )
					NSException( _UTF8( "函数[pushStreamData]发生异常, 非法浮点数" ) );

				lua_pushnumber( mpLuaState, value );
			}
			else if ( dataType == TYPE_USHORT )
			{
				unsigned short ushortValue = 0;
				stream >> ushortValue;
				lua_pushnumber( mpLuaState, ushortValue );
			}
			else if ( dataType == TYPE_SHORT )
			{
				short shortValue = 0;
				stream >> shortValue;
				lua_pushnumber( mpLuaState, shortValue );
			}
			else if ( dataType == TYPE_UCHAR )
			{
				unsigned char ucharValue = 0;
				stream >> ucharValue;
				lua_pushnumber( mpLuaState, ucharValue );
			}
			else if ( dataType == TYPE_CHAR )
			{
				char charValue = 0;
				stream >> charValue;
				lua_pushnumber( mpLuaState, charValue );
			}
			else if ( dataType == TYPE_BOOLEAN )
			{
				bool value = 0;
				stream >> value;
				lua_pushboolean( mpLuaState, value );
			}
			else if ( dataType == TYPE_STRING )
			{
				static CNSString value;
				stream >> value;
				lua_pushstring( mpLuaState, value.getBuffer( ) );
			}
			else if ( dataType == TYPE_TABLE )
				pushStreamTable( stream );
			else
				NSException( _UTF8( "函数[pushStreamData]发生异常, 未知数据类型" ) );
		}

		template< typename T >
		void pushStreamTable( const T& stream )
		{
			newTable( );
			char keyType;
			for ( stream >> keyType; keyType != 0; stream >> keyType )
			{
				static CNSString stringKey;
				double numberKey;
				if ( keyType == (char) TYPE_DOUBLE )
				{
					stream >> numberKey;
					int fpc = _fpclass( numberKey );
					if ( fpc != _FPCLASS_NN && fpc != _FPCLASS_PN && fpc != _FPCLASS_PZ && fpc != _FPCLASS_NZ )
						NSException( _UTF8( "函数[pushStreamTable]发生异常, 非法浮点数" ) );
				}
				else if ( keyType == (char) TYPE_FLOAT )
				{
					float floatKey;
					stream >> floatKey;
					int fpc = _fpclass( floatKey );
					if ( fpc != _FPCLASS_NN && fpc != _FPCLASS_PN && fpc != _FPCLASS_PZ && fpc != _FPCLASS_NZ )
						NSException( _UTF8( "函数[pushStreamTable]发生异常, 非法浮点数" ) );
					numberKey = floatKey;
				}
				else if ( keyType == (char) TYPE_USHORT )
				{
					unsigned short ushortKey;
					stream >> ushortKey;
					numberKey = ushortKey;
				}
				else if ( keyType == (char) TYPE_SHORT )
				{
					short shortKey;
					stream >> shortKey;
					numberKey = shortKey;
				}
				else if ( keyType == (char) TYPE_UCHAR )
				{
					unsigned char ucharKey;
					stream >> ucharKey;
					numberKey = ucharKey;
				}
				else if ( keyType == (char) TYPE_CHAR )
				{
					char charKey;
					stream >> charKey;
					numberKey = charKey;
				}
				else if ( keyType == (char) TYPE_STRING )
					stream >> stringKey;
				else
					throw CNSMarshal::CException( );

				char dataType;
				stream >> dataType;
				if ( dataType == (char) TYPE_BOOLEAN )
				{
					bool value;
					stream >> value;
					if ( keyType == (char) TYPE_STRING )
						pushField( stringKey, value );
					else
						pushField( numberKey, value );
				}
				else if ( dataType == (char) TYPE_STRING )
				{
					static CNSString value;
					stream >> value;
					if ( keyType == (char) TYPE_STRING )
						pushField( stringKey, value );
					else
						pushField( numberKey, value );
				}
				else if ( dataType == (char) TYPE_TABLE )
				{
					if ( keyType == (char) TYPE_STRING )
						lua_pushstring( mpLuaState, stringKey.getBuffer( ) );
					else
						lua_pushnumber( mpLuaState, numberKey );

					pushStreamTable<T>( stream );
					lua_settable( mpLuaState, -3 );
				}
				else
				{
					double value;
					if ( dataType == (char) TYPE_DOUBLE )
					{
						stream >> value;

						int fpc = _fpclass( value );
						if ( fpc != _FPCLASS_NN && fpc != _FPCLASS_PN && fpc != _FPCLASS_PZ && fpc != _FPCLASS_NZ )
							NSException( _UTF8( "函数[pushStreamTable]发生异常, 非法浮点数" ) );
					}
					else if ( dataType == (char) TYPE_FLOAT )
					{
						stream >> (float) value;

						int fpc = _fpclass( value );
						if ( fpc != _FPCLASS_NN && fpc != _FPCLASS_PN && fpc != _FPCLASS_PZ && fpc != _FPCLASS_NZ )
							NSException( _UTF8( "函数[pushStreamTable]发生异常, 非法浮点数" ) );
					}
					else if ( dataType == (char) TYPE_USHORT )
					{
						unsigned short ushortValue;
						stream >> ushortValue;
						value = ushortValue;
					}
					else if ( dataType == (char) TYPE_SHORT )
					{
						short shortValue;
						stream >> shortValue;
						value = shortValue;
					}
					else if ( dataType == (char) TYPE_UCHAR )
					{
						unsigned char ucharValue;
						stream >> ucharValue;
						value = ucharValue;
					}
					else if ( dataType == (char) TYPE_CHAR )
					{
						char charValue;
						stream >> charValue;
						value = charValue;
					}
					else
					{
						throw CNSMarshal::CException( );
					}

					if ( keyType == (char) TYPE_STRING )
						pushField( stringKey, value );
					else
						pushField( numberKey, value );
				}
			}
		}

		template< typename T >
		void popStreamData( T& stream, int index ) const
		{
			int type = lua_type( mpLuaState, index );
			if ( type == LUA_TNUMBER )
			{
				double value = lua_tonumber( mpLuaState, index );
				popStreamNumber( stream, value );
			}
			else if ( type == LUA_TBOOLEAN )
			{
				stream << (char) TYPE_BOOLEAN;
				stream << (bool) lua_toboolean( mpLuaState, index );
			}
			else if ( type == LUA_TSTRING )
			{
				stream << (char) TYPE_STRING;
				static CNSString string;
				string = lua_tostring( mpLuaState, index );
				stream << string;
			}
			else if ( type == LUA_TTABLE )
			{
				stream << (char) TYPE_TABLE;
				popStreamTable( stream, index );
			}
			else
			{
				static CNSString errorDesc;
				errorDesc.format( _UTF8( "函数[popStreamData]发生异常, 参数[%d]不支持数据类型[%s]" ), index, luaL_typename( mpLuaState, index ) );
				NSException( errorDesc );
			}
		}

		template< typename T >
		void popStreamTable( T& stream, int index ) const
		{
			lua_pushnil( mpLuaState );  // nil 入栈作为初始 key 
			while ( lua_next( mpLuaState, index ) != 0 )
			{
				int dataType = lua_type( mpLuaState, -1 );
				int keyType = lua_type( mpLuaState, -2 );
				if ( keyType == LUA_TNUMBER )
				{
					double doubleKey = lua_tonumber( mpLuaState, -2 );
					popStreamNumber( stream, doubleKey );
				}
				else if ( keyType == LUA_TSTRING )
				{
					static CNSString stringKey;
					stringKey = lua_tostring( mpLuaState, -2 );
					stream << (char) TYPE_STRING;
					stream << stringKey;
				}
				else
				{
					static CNSString errorDesc;
					errorDesc.format( _UTF8( "函数[popStreamTable]发生异常, 参数[%d] key字段不支持数据类型[%s]" ), index, luaL_typename( mpLuaState, -2 ) );
					NSException( errorDesc );
				}

				if ( dataType == LUA_TNUMBER )
				{
					double value = lua_tonumber( mpLuaState, -1 );
					popStreamNumber( stream, value );
				}
				else if ( dataType == LUA_TBOOLEAN )
				{
					stream << (char) TYPE_BOOLEAN;
					stream << (bool) lua_toboolean( mpLuaState, -1 );
				}
				else if ( dataType == LUA_TSTRING )
				{
					static CNSString stringValue;
					stringValue = lua_tostring( mpLuaState, -1 );
					stream << (char) TYPE_STRING;
					stream << stringValue;
				}
				else if ( dataType == LUA_TTABLE )
				{
					stream << (char) TYPE_TABLE;
					popStreamTable( stream, lua_gettop( mpLuaState ) );
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
					errorDesc.format( _UTF8( "函数[popStreamTable]发生异常, 参数[%d] key[%s] value字段不支持数据类型[%s]" ), index, key.getBuffer( ), luaL_typename( mpLuaState, -1 ) );
					NSException( errorDesc );
				}

				// 现在栈顶（-1）是 value，-2 位置是对应的 key 
				// 这里可以判断 key 是什么并且对 value 进行各种处理 
				lua_pop( mpLuaState, 1 );  // 弹出 value，让 key 留在栈顶 
			}
			stream << (char) 0;
		}

		void enumStackTable( int index, FEnumProc proc, void* userdata );
		void loadLuaDir( const CNSString& filePath, const CNSString& chunkPath );
		void preload( const CNSString& workPath, CNSSet< CNSString >& dirList );
		void loadMod( const CNSString& workPath, const CNSString& modName );
		void onError( const CNSString& errorText );
		void onOpenScript( const CNSString& chunkName, const char* buffer, unsigned int size );

	public:
		CNSString getTraceBack( );

		// 这个不能在多线程中使用
		static CNSLuaStack& fromState( lua_State* lua );
		// 这个是线程安全版本
		static CNSLuaStack fromStateThread( lua_State* lua );
		static void init( bool enableDebug );
		static void exit( );
		static CNSLuaStack& getLuaStack( );

		// lua只负责清理引用本身，被引用的对象由Nosn负责清理 否则会保存一个野指针
		static int gcNSWeakRef( lua_State* lua )
		{
			CNSLuaWeakRef** objRef = (CNSLuaWeakRef**) lua_touserdata( lua, 1 );
			if ( *objRef == NULL )
				return 0;

			CNSLuaWeakRef* obj = *objRef;
			obj->removeLuaRef( (void**) objRef );
			return 0;
		}

		static int gcNSObject( lua_State* lua )
		{
			CNSLuaObject** obj = (CNSLuaObject**) lua_touserdata( lua, 1 );
			if ( obj == NULL )
				return 0;

			delete *obj;
			return 0;
		}

		// 从lua栈从得到一个NSObject对象引用
		CNSLuaWeakRef* popNSWeakRef( const char* metaName ) const
		{
			CNSLuaWeakRef** objRef = (CNSLuaWeakRef**) luaL_testudata( mpLuaState, mPopIndex, metaName );
			if ( objRef == NULL )
				return NULL;

			if ( *objRef == NULL )
			{
				static CNSString errorDesc;
				errorDesc.format( _UTF8( "参数[%d], %s引用无效" ), mPopIndex, metaName );
				NSException( errorDesc );
			}

			return *objRef;
		}

		// obj - 类型对象指针
		// meta - lua 元表名(lua类型名)
		// NSVector< luaReg > - 类型中的方法表
		// 将一个NSObject对象引用作为userdata 压入lua栈
		void pushNSWeakRef( CNSLuaWeakRef* obj, const CNSString& meta, const CNSVector< const luaL_Reg* >& reg )
		{
			CNSLuaWeakRef** objRef = (CNSLuaWeakRef**) lua_newuserdata( mpLuaState, sizeof( CNSLuaWeakRef* ) );
			*objRef = obj;
			// 需要保存引用本身，当窗口销毁之后，需要让引用指向NULL, 否则会指向野指针
			obj->setLuaRef( (void**) objRef );

			int objIndex = lua_gettop( mpLuaState );
			luaL_newmetatable( mpLuaState, meta );
			int metaIndex = lua_gettop( mpLuaState );

			for ( unsigned int i = 0; i < reg.getCount( ); i ++ )
				luaL_setfuncs( mpLuaState, reg[ i ], 0 );

			lua_pushliteral( mpLuaState, "__index" );
			lua_pushvalue( mpLuaState, metaIndex );
			lua_settable( mpLuaState, metaIndex );

			lua_pushliteral( mpLuaState, "__gc" );
			lua_pushcfunction( mpLuaState, gcNSWeakRef );
			lua_settable( mpLuaState, metaIndex );

			// 不需要__gc元方法，因为这里返回的是一个弱引用，lua只负责清理引用本身，被引用的对象由Nosn负责清理
			lua_setmetatable( mpLuaState, objIndex );
		}

		// 从lua栈从得到一个NSObject对象
		CNSLuaObject* popNSObject( const char* metaName ) const
		{
			CNSLuaObject** objRef = (CNSLuaObject**) luaL_checkudata( mpLuaState, mPopIndex, metaName );
			if ( objRef == NULL )
				return NULL;

			if ( *objRef == NULL )
			{
				static CNSString errorDesc;
				errorDesc.format( _UTF8( "参数 - %d, %s引用无效" ), mPopIndex, metaName );
				NSException( errorDesc );
			}

			return *objRef;
		}

		// 将一个NSObject对象作为userdata 压入lua栈
		CNSLuaObject* pushNSObject( CNSLuaObject* obj, const char* metaName, const luaL_Reg* reg )
		{
			CNSLuaObject** luaObj = (CNSLuaObject**) lua_newuserdata( mpLuaState, sizeof( CNSLuaObject* ) );
			*luaObj = obj;

			int objIndex = lua_gettop( mpLuaState );
			luaL_newmetatable( mpLuaState, metaName );
			int metaIndex = lua_gettop( mpLuaState );

			luaL_setfuncs( mpLuaState, reg, 0 );

			lua_pushliteral( mpLuaState, "__index" );
			lua_pushvalue( mpLuaState, metaIndex );
			lua_settable( mpLuaState, metaIndex );

			lua_pushliteral( mpLuaState, "__gc" );
			lua_pushcfunction( mpLuaState, gcNSObject );
			lua_settable( mpLuaState, metaIndex );

			// 不需要__gc元方法，因为这里返回的是一个弱引用，lua只负责清理引用本身，被引用的对象由Nosn负责清理
			lua_setmetatable( mpLuaState, objIndex );
			return obj;
		}

	public:
		static void* luaAlloc( void* ud, void* ptr, size_t osize, size_t nsize );
		static int traceBack( lua_State* lua );
	};

}

