#pragma once
namespace NSDemo
{
	typedef void( __stdcall *FHostErrorProc )( const char* text, int level );
	typedef bool( *nsClientInit )( const char* authName, const char* dataPath, FHostErrorProc proc, bool enableDebug );
	typedef void( *nsClientExit )( );
	typedef void( *nsClientUpdate )( );
	static nsClientInit initProc = NULL;
	static nsClientExit exitProc = NULL;
	static nsClientUpdate updateProc = NULL;
}


