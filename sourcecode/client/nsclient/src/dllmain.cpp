// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "precomplie.h"

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
		OutputDebugString( _T("nsclient.dll loaded" ) );
		break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
		OutputDebugString( _T( "nsclient.dll unloaded" ) );
		break;
    }
    return TRUE;
}

