#pragma once
typedef void( __stdcall *FHostErrorProc )( const char* text, int level );

// 初始化NSClient
extern "C" _declspec( dllexport ) bool nsClientInit( const char* authName, FHostErrorProc proc, bool enableDebug );
// 析构NSClient
extern "C" _declspec( dllexport ) void nsClientExit( );
// 轮询NSClient
extern "C" _declspec( dllexport ) void nsClientUpdate( );
// 打开调试控制台
extern "C" _declspec( dllexport ) void nsClientShowConsole( );
