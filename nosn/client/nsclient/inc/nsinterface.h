#pragma once
typedef void( __stdcall *FHostErrorProc )( const char* text, int level );

// ��ʼ��NSClient
extern "C" _declspec( dllexport ) bool nsClientInit( const char* authName, FHostErrorProc proc, bool enableDebug );
// ����NSClient
extern "C" _declspec( dllexport ) void nsClientExit( );
// ��ѯNSClient
extern "C" _declspec( dllexport ) void nsClientUpdate( );
// �򿪵��Կ���̨
extern "C" _declspec( dllexport ) void nsClientShowConsole( );
