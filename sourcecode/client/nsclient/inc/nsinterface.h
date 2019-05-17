#pragma once
typedef void( __stdcall *FHostErrorProc )( const char* text, int level );
typedef void( __stdcall *FUILoadLayout )( const char* uiFile );
typedef void( __stdcall *FUIDestroy )( int instanceID );
typedef bool( __stdcall *FUIGetValue )( int instanceID, const char* compName, const char* fieldName, void*& buffer, int& dataType );
typedef bool( __stdcall *FUISetValue )( int instanceID, const char* compName, const char* fieldName, void* buffer, int dataType );
typedef const char*( __stdcall *FUIGetLastError )( );

// 初始化NSClient
extern "C" _declspec( dllexport ) bool nsClientInit( const char* authName, FHostErrorProc proc, bool enableDebug );
// 析构NSClient
extern "C" _declspec( dllexport ) void nsClientExit( );
// 轮询NSClient
extern "C" _declspec( dllexport ) void nsClientUpdate( );
// 打开调试控制台
extern "C" _declspec( dllexport ) void nsClientShowConsole( );
// 设置UI加载函数
extern "C" _declspec( dllexport ) void nsSetUILoadProc( FUILoadLayout loadProc );
// 设置UI销毁函数
extern "C" _declspec( dllexport ) void nsSetUIDestroyProc( FUIDestroy loadProc );
// 建立UI数据代理
extern "C" _declspec( dllexport ) void nsCreateUIProxy( int instanceID, const char* objName );
// 设置ui组件字段读函数
extern "C" _declspec( dllexport ) void nsSetUIGetProc( FUIGetValue loadProc );
// 设置ui组件字段写函数
extern "C" _declspec( dllexport ) void nsSetUISetProc( FUISetValue loadProc );
// 设置ui组件字段写函数
extern "C" _declspec( dllexport ) void nsSetUIGetLastError( FUIGetLastError loadProc );
// UI事件回调函数
extern "C" _declspec( dllexport ) void nsUIFireEvent( int instanceID, const char* compName );


