#pragma once
typedef void( __stdcall *FHostErrorProc )( const char* text, int level );
typedef bool( __stdcall *FGoLoadProc )( const char* file, int parentID, int type );
typedef bool( __stdcall *FGoDestroyProc )( int instanceID );
typedef bool( __stdcall *FGoGetValue )( int type, int instanceID, const char* compName, const char* fieldName, void*& buffer, int& dataType );
typedef bool( __stdcall *FGoSetValue )( int type, int instanceID, const char* compName, const char* fieldName, void* buffer, int dataType );
typedef bool( __stdcall *FTouchProc )( const char* bundle, bool permanent, bool touch );
typedef const char*( __stdcall *FGoGetLastError )( );

// 初始化NSClient
extern "C" _declspec( dllexport ) bool nsClientInit( const char* authName, FHostErrorProc proc, bool enableDebug );
// 析构NSClient
extern "C" _declspec( dllexport ) void nsClientExit( );
// 轮询NSClient
extern "C" _declspec( dllexport ) void nsClientUpdate( );
// 打开调试控制台
extern "C" _declspec( dllexport ) void nsClientShowConsole( );
// 设置加载函数
extern "C" _declspec( dllexport ) void nsSetTouchProc( FTouchProc loadProc );
// 设置加载函数
extern "C" _declspec( dllexport ) void nsSetGoLoadProc( FGoLoadProc loadProc );
// 设置销毁函数
extern "C" _declspec( dllexport ) void nsSetGoDestroyProc( FGoDestroyProc destroyProc );
// 建立数据代理
extern "C" _declspec( dllexport ) void nsCreateGoProxy( int type, int instanceID, const char* objName );
// 设置组件字段读函数
extern "C" _declspec( dllexport ) void nsSetGoGetProc( FGoGetValue loadProc );
// 设置组件字段写函数
extern "C" _declspec( dllexport ) void nsSetGoSetProc( FGoSetValue loadProc );
// 设置组件字段写函数
extern "C" _declspec( dllexport ) void nsSetGoGetLastError( FGoGetLastError loadProc );
// 事件回调函数
extern "C" _declspec( dllexport ) void nsGoFireEvent( int instanceID, const char* compName );
// 加载过程回调函数
extern "C" _declspec( dllexport ) void nsTouchNotify( bool isDone, float progress );


