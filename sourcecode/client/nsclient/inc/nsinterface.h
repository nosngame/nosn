#pragma once
typedef void( __stdcall *FHostErrorProc )( const char* text, int level );
typedef bool( __stdcall *FGoLoadProc )( const char* file, int parentID, int type );
typedef bool( __stdcall *FGoDestroyProc )( int instanceID );
typedef bool( __stdcall *FGoGetValue )( int type, int instanceID, const char* compName, const char* fieldName, void*& buffer, int& dataType );
typedef bool( __stdcall *FGoSetValue )( int type, int instanceID, const char* compName, const char* fieldName, void* buffer, int dataType );
typedef bool( __stdcall *FTouchProc )( const char* bundle, bool permanent, bool touch );
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


