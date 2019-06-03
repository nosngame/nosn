#pragma once

#ifdef PLATFORM_WIN32
    #define NSEXPORT extern "C" _declspec( dllexport )
    #define NSCALL __stdcall
#elif PLATFORM_OSX
    #define NSEXPORT extern "C"
    #define NSCALL
#elif PLATFORM_IOS
    #define NSEXPORT extern "C"
    #define NSCALL
#endif

typedef void( NSCALL *FHostErrorProc )( const char* text, int level );
typedef bool( NSCALL *FGoLoadProc )( const char* file, int parentID, int type );
typedef bool( NSCALL *FGoDestroyProc )( int instanceID );
typedef bool( NSCALL *FGoGetValue )( int type, int instanceID, const char* compName, const char* fieldName, void*& buffer, int& dataType );
typedef bool( NSCALL *FGoSetValue )( int type, int instanceID, const char* compName, const char* fieldName, void* buffer, int dataType );
typedef bool( NSCALL *FTouchProc )( const char* bundle, bool permanent, bool touch );

typedef bool( NSCALL *FGoGetLayer )( int type, int instanceID, const char*& layer );
typedef bool( NSCALL *FGoGetTag )( int type, int instanceID, const char*& layer );
typedef bool( NSCALL *FGoSetLayer )( int type, int instanceID, const char* layer );
typedef bool( NSCALL *FGoSetTag )( int type, int instanceID, const char* layer );
typedef bool( NSCALL *FGoQueryMethod )( int type, int instanceID, const char* compName, const char* methodName, bool& isMethod );
typedef bool( NSCALL *FGoInvoke )( int type, int instanceID, const char* compName, void* stream, int len, void*& buffer, int& dataType );
typedef const char*( NSCALL *FGoGetLastError )( );

// 初始化NSClient
NSEXPORT bool nsClientInit( const char* authName, const char* dataPath, FHostErrorProc proc, bool enableDebug );
// 析构NSClient
NSEXPORT void nsClientExit( );
// 轮询NSClient
NSEXPORT void nsClientUpdate( );
// 打开调试控制台
NSEXPORT void nsClientShowConsole( );
// 设置加载函数
NSEXPORT void nsSetTouchProc( FTouchProc loadProc );
// 设置加载函数
NSEXPORT void nsSetGoLoadProc( FGoLoadProc loadProc );
// 设置销毁函数
NSEXPORT void nsSetGoDestroyProc( FGoDestroyProc destroyProc );
// 建立数据代理
NSEXPORT void nsCreateGoProxy( int type, int instanceID, const char* objName );
// 设置组件字段读函数
NSEXPORT void nsSetGoGetProc( FGoGetValue loadProc );
// 设置组件字段写函数
NSEXPORT void nsSetGoSetProc( FGoSetValue loadProc );
// 设置组件字段写函数
NSEXPORT void nsSetGoGetLastError( FGoGetLastError loadProc );
// 事件回调函数
NSEXPORT void nsGoFireEvent( int instanceID, const char* compName );
// 加载过程回调函数
NSEXPORT void nsTouchNotify( bool isDone, float progress );
// 设置对象Layer函数
NSEXPORT void nsSetGoGetLayer( FGoGetLayer getLayer );
// 设置对象Tag函数
NSEXPORT void nsSetGoGetTag( FGoGetTag getTag );
// 设置对象Layer函数
NSEXPORT void nsSetGoSetLayer( FGoSetLayer getLayer );
// 设置对象Tag函数
NSEXPORT void nsSetGoSetTag( FGoSetTag getTag );
// 设置对象查询函数
NSEXPORT void nsSetGoQueryMethod( FGoQueryMethod queryMethod );
// 设置对象查询函数
NSEXPORT void nsSetGoInvoke( FGoInvoke invoke );
