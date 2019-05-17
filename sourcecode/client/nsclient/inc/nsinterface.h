#pragma once
typedef void( __stdcall *FHostErrorProc )( const char* text, int level );
typedef void( __stdcall *FUILoadLayout )( const char* uiFile );
typedef void( __stdcall *FUIDestroy )( int instanceID );
typedef bool( __stdcall *FUIGetValue )( int instanceID, const char* compName, const char* fieldName, void*& buffer, int& dataType );
typedef bool( __stdcall *FUISetValue )( int instanceID, const char* compName, const char* fieldName, void* buffer, int dataType );
typedef const char*( __stdcall *FUIGetLastError )( );

// ��ʼ��NSClient
extern "C" _declspec( dllexport ) bool nsClientInit( const char* authName, FHostErrorProc proc, bool enableDebug );
// ����NSClient
extern "C" _declspec( dllexport ) void nsClientExit( );
// ��ѯNSClient
extern "C" _declspec( dllexport ) void nsClientUpdate( );
// �򿪵��Կ���̨
extern "C" _declspec( dllexport ) void nsClientShowConsole( );
// ����UI���غ���
extern "C" _declspec( dllexport ) void nsSetUILoadProc( FUILoadLayout loadProc );
// ����UI���ٺ���
extern "C" _declspec( dllexport ) void nsSetUIDestroyProc( FUIDestroy loadProc );
// ����UI���ݴ���
extern "C" _declspec( dllexport ) void nsCreateUIProxy( int instanceID, const char* objName );
// ����ui����ֶζ�����
extern "C" _declspec( dllexport ) void nsSetUIGetProc( FUIGetValue loadProc );
// ����ui����ֶ�д����
extern "C" _declspec( dllexport ) void nsSetUISetProc( FUISetValue loadProc );
// ����ui����ֶ�д����
extern "C" _declspec( dllexport ) void nsSetUIGetLastError( FUIGetLastError loadProc );
// UI�¼��ص�����
extern "C" _declspec( dllexport ) void nsUIFireEvent( int instanceID, const char* compName );


