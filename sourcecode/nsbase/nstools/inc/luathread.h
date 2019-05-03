#pragma once

class CInputData
{
public:
	CNSString			mFunction;
	CNSString			mCallback;
	CNSOctetsStream		mData;
};

class COutputData
{
public:
	CNSString			mFunction;
	CNSOctetsStream		mData;
};

class CThread
{
public:
	CNSVector< CInputData >		mInput;
	CRITICAL_SECTION			mCS;
	CNSLuaStack					mLuaStack;

public:
	CThread( ) : mLuaStack( false )
	{
		InitializeCriticalSection( &mCS );
	}

	~CThread( )
	{
		DeleteCriticalSection( &mCS );
	}

public:
	static void workThread( void* pParam );
	static void execInThread( int index, const char* pFunc, const char* pCallback, CNSOctetsStream data );
	static void exitInThread( COutputData& rData );
	static void run( int index );
};

extern CRITICAL_SECTION			sOutputCS;
extern CNSVector< COutputData >	sOutput;
typedef void ( *ThreadFuncProc )( lua_State* );
void initLuaThread( int count, const CNSString& rWorkPath, const CNSString& rParent, ThreadFuncProc proc );
void finializeLuaThread( );
void luaThreadTimer( CNSLuaStack* pLua );
void exit( const char* callback, CNSOctetsStream data );
void exec( const char* func, const char* callback, CNSOctetsStream data );
