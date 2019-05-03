#include <nsbase.h>
#include <process.h>
#include <time.h>
#include "luathread.h"

bool enableThread			= false;
CRITICAL_SECTION			sOutputCS;
CNSVector< COutputData >	sOutput;
CThread* spLuaThread		= NULL;
CNSString gWorkPath;

void CThread::workThread( void* pParam )
{
	unsigned int index = (unsigned int)( unsigned long long ) pParam;
	srand( (unsigned int) time( NULL ) + index );

	NSLog::log( "thread[%d]open success", index );
	CThread* tpThread = &spLuaThread[ index ];
	while( 1 )
	{
		EnterCriticalSection( &tpThread->mCS );
		if ( spLuaThread[ index ].mInput.getCount( ) == 0 )
		{
			LeaveCriticalSection( &tpThread->mCS );
			Sleep( 1 );
			continue;
		}
		CInputData input = spLuaThread[ index ].mInput[ 0 ];
		spLuaThread[ index ].mInput.erase( 0 );
		LeaveCriticalSection( &tpThread->mCS );

		tpThread->mLuaStack.preCall( input.mFunction );
		tpThread->mLuaStack.call( );
	}
}

void CThread::execInThread( int index, const char* pFunc, const char* pCallback, CNSOctetsStream data )
{
	CThread* tpThread = &spLuaThread[ index ];
	CInputData input;
	input.mFunction = pFunc;
	input.mCallback = pCallback;
	input.mData		= data;

	EnterCriticalSection( &tpThread->mCS );
	tpThread->mInput.pushback( input );
	LeaveCriticalSection( &tpThread->mCS );
}

void CThread::exitInThread( COutputData& rData )
{
	EnterCriticalSection( &sOutputCS );
	sOutput.pushback( rData );
	LeaveCriticalSection( &sOutputCS );
}

void CThread::run( int index )
{
	_beginthread( workThread, 0, (void*) index );
}

void exec( const char* func, const char* callback, CNSOctetsStream data )
{
	if ( enableThread == false )
		return;

	static int sThreadIndex = 0;
	CThread::execInThread( sThreadIndex ++, func, callback, data );
	if ( sThreadIndex == 4 )
		sThreadIndex = 0;
}

void exit( const char* callback, CNSOctetsStream data )
{
	if ( enableThread == false )
		return;

	COutputData output;
	output.mFunction = callback;
	output.mData = data;

	CThread::exitInThread( output );
}

void initLuaThread( int count, const CNSString& workPath, const CNSString& parent, ThreadFuncProc proc )
{
	if ( enableThread == false )
		return;

	InitializeCriticalSection( &sOutputCS );
	spLuaThread = new CThread[ count ];
	for ( int i = 0; i < count; i ++ )
	{
		CNSString fileName = workPath + "script/" + parent + "/thread.lua";
		CNSString chunkName = CNSString( "script/" ) + parent + "/thread.lua";
		if ( spLuaThread[ i ].mLuaStack.openScriptFromFile( fileName, chunkName ) == false )
		{
			NSLog::log( _UTF8( "Lua线程脚本[%s]打开失败" ), fileName.getBuffer( ) );
			return;
		}

		proc( spLuaThread[ i ].mLuaStack.getLuaState( ) );
		CThread::run( i );
	}
}

void finializeLuaThread( )
{
	if ( enableThread == false )
		return;

	DeleteCriticalSection( &sOutputCS );
}

void luaThreadTimer( CNSLuaStack* lua )
{
	if ( enableThread == false )
		return;

	COutputData data;
	while( 1 )
	{
		EnterCriticalSection( &sOutputCS );
		if ( sOutput.getCount( ) == 0 )
		{
			LeaveCriticalSection( &sOutputCS );
			break;
		}

		data = sOutput[ 0 ];
		sOutput.erase( 0 );
		LeaveCriticalSection( &sOutputCS );

		lua->preCall( data.mFunction );
		lua->call( );
	}
}

