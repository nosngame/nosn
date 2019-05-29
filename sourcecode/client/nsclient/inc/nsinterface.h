#pragma once

#ifdef PLATFORM_WIN32
#define NSEXPORT NSEXPORT
#define NSCALL __stdcall
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

// ��ʼ��NSClient
NSEXPORT bool nsClientInit( const char* authName, FHostErrorProc proc, bool enableDebug );
// ����NSClient
NSEXPORT void nsClientExit( );
// ��ѯNSClient
NSEXPORT void nsClientUpdate( );
// �򿪵��Կ���̨
NSEXPORT void nsClientShowConsole( );
// ���ü��غ���
NSEXPORT void nsSetTouchProc( FTouchProc loadProc );
// ���ü��غ���
NSEXPORT void nsSetGoLoadProc( FGoLoadProc loadProc );
// �������ٺ���
NSEXPORT void nsSetGoDestroyProc( FGoDestroyProc destroyProc );
// �������ݴ���
NSEXPORT void nsCreateGoProxy( int type, int instanceID, const char* objName );
// ��������ֶζ�����
NSEXPORT void nsSetGoGetProc( FGoGetValue loadProc );
// ��������ֶ�д����
NSEXPORT void nsSetGoSetProc( FGoSetValue loadProc );
// ��������ֶ�д����
NSEXPORT void nsSetGoGetLastError( FGoGetLastError loadProc );
// �¼��ص�����
NSEXPORT void nsGoFireEvent( int instanceID, const char* compName );
// ���ع��̻ص�����
NSEXPORT void nsTouchNotify( bool isDone, float progress );
// ���ö���Layer����
NSEXPORT void nsSetGoGetLayer( FGoGetLayer getLayer );
// ���ö���Tag����
NSEXPORT void nsSetGoGetTag( FGoGetTag getTag );
// ���ö���Layer����
NSEXPORT void nsSetGoSetLayer( FGoSetLayer getLayer );
// ���ö���Tag����
NSEXPORT void nsSetGoSetTag( FGoSetTag getTag );
// ���ö����ѯ����
NSEXPORT void nsSetGoQueryMethod( FGoQueryMethod queryMethod );
// ���ö����ѯ����
NSEXPORT void nsSetGoInvoke( FGoInvoke invoke );
