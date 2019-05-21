#pragma once
typedef void( __stdcall *FHostErrorProc )( const char* text, int level );
typedef bool( __stdcall *FGoLoadProc )( const char* file, int parentID, int type );
typedef bool( __stdcall *FGoDestroyProc )( int instanceID );
typedef bool( __stdcall *FGoGetValue )( int type, int instanceID, const char* compName, const char* fieldName, void*& buffer, int& dataType );
typedef bool( __stdcall *FGoSetValue )( int type, int instanceID, const char* compName, const char* fieldName, void* buffer, int dataType );
typedef bool( __stdcall *FTouchProc )( const char* bundle, bool permanent, bool touch );

typedef bool( __stdcall *FGoGetLayer )( int type, int instanceID, const char*& layer );
typedef bool( __stdcall *FGoGetTag )( int type, int instanceID, const char*& layer );
typedef bool( __stdcall *FGoSetLayer )( int type, int instanceID, const char* layer );
typedef bool( __stdcall *FGoSetTag )( int type, int instanceID, const char* layer );
typedef bool( __stdcall *FGoQueryMethod )( int type, int instanceID, const char* compName, const char* methodName, bool& isMethod );
typedef bool( __stdcall *FGoInvoke )( int type, int instanceID, const char* compName, void* stream, int len, void*& buffer, int& dataType );
typedef const char*( __stdcall *FGoGetLastError )( );

// ��ʼ��NSClient
extern "C" _declspec( dllexport ) bool nsClientInit( const char* authName, FHostErrorProc proc, bool enableDebug );
// ����NSClient
extern "C" _declspec( dllexport ) void nsClientExit( );
// ��ѯNSClient
extern "C" _declspec( dllexport ) void nsClientUpdate( );
// �򿪵��Կ���̨
extern "C" _declspec( dllexport ) void nsClientShowConsole( );
// ���ü��غ���
extern "C" _declspec( dllexport ) void nsSetTouchProc( FTouchProc loadProc );
// ���ü��غ���
extern "C" _declspec( dllexport ) void nsSetGoLoadProc( FGoLoadProc loadProc );
// �������ٺ���
extern "C" _declspec( dllexport ) void nsSetGoDestroyProc( FGoDestroyProc destroyProc );
// �������ݴ���
extern "C" _declspec( dllexport ) void nsCreateGoProxy( int type, int instanceID, const char* objName );
// ��������ֶζ�����
extern "C" _declspec( dllexport ) void nsSetGoGetProc( FGoGetValue loadProc );
// ��������ֶ�д����
extern "C" _declspec( dllexport ) void nsSetGoSetProc( FGoSetValue loadProc );
// ��������ֶ�д����
extern "C" _declspec( dllexport ) void nsSetGoGetLastError( FGoGetLastError loadProc );
// �¼��ص�����
extern "C" _declspec( dllexport ) void nsGoFireEvent( int instanceID, const char* compName );
// ���ع��̻ص�����
extern "C" _declspec( dllexport ) void nsTouchNotify( bool isDone, float progress );
// ���ö���Layer����
extern "C" _declspec( dllexport ) void nsSetGoGetLayer( FGoGetLayer getLayer );
// ���ö���Tag����
extern "C" _declspec( dllexport ) void nsSetGoGetTag( FGoGetTag getTag );
// ���ö���Layer����
extern "C" _declspec( dllexport ) void nsSetGoSetLayer( FGoSetLayer getLayer );
// ���ö���Tag����
extern "C" _declspec( dllexport ) void nsSetGoSetTag( FGoSetTag getTag );
// ���ö����ѯ����
extern "C" _declspec( dllexport ) void nsSetGoQueryMethod( FGoQueryMethod queryMethod );
// ���ö����ѯ����
extern "C" _declspec( dllexport ) void nsSetGoInvoke( FGoInvoke invoke );