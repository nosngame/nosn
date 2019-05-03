#pragma once
#define BYTE_ORDER_BIG_ENDIAN
#define PLATFORM_WIN32

#include <nsdeclare.h>
#include <nslock.h>
#include <nstemplate.h>
#include <nsfunctional.h>
#include <nsoctets.h>
using namespace NSBase;

#include <nsmath.h>
using namespace NSMath;

#ifdef __cplusplus
extern "C"
{
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}
#else
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#endif

#include <tinyxml.h>
#include <nslog.h>
#include <nsluawrapper.h>
#include <nsfile.h>

#include <nsnetdeclare.h>
#include <nsnetbase.h>
#include <nssession.h>
#include <nsnetmanager.h>
using namespace NSNet;

#include <nstimer.h>
#include <nshttp.h>
#include <nsmysqlclient.h>
#include <nsbaselua.h>
#include <nslocal.h>
#include <nshttpdebugger.h>

#ifdef PLATFORM_WIN32
#include <nsnetworkio_win32.h>
#endif

#ifdef PLATFORM_ANDROID
#include <nsnetworkio_andriod.h>
#endif

#ifdef PLATFORM_LINUX
#include <nsnetworkio_linux.h>
#endif

#ifdef PLATFORM_IOS
#include "fbnetworkio_ios.h"
#endif

#ifdef PLATFORM_WIN32
#include <commctrl.h>
#include <scintilla.h>

// 如果没有使用MFC，那么需要指定common-controls的链接库
#ifndef _AFXDLL
#ifdef _UNICODE
#if defined _M_IX86
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif
#endif
#endif

#include <nsconsole.h>
#include <nsguideclare.h>
#include <nsgui.h>
#include <nsvstab.h>
#include <nsvsbtn.h>
#include <nsvslist.h>
#include <nsvsedit.h>
#include <nsvstree.h>
#include <nsframe.h>
#include <nsedit.h>
#include <nscombobox.h>
#include <nsvslistbox.h>
#include <nscustom.h>
#include <nsvsstatic.h>
#include <nsvswizard.h>
#include <nsvsfilebrowser.h>
#include <nsvsfiledialog.h>
#include <nsimage.h>
#include <nswin32lua.h>
#include <nsgdilua.h>
#include <nswindowlua.h>
#include <nsvsstaticlua.h>
#include <nsvsbtnlua.h>
#include <nseditlua.h>
#include <nsvseditlua.h>
#include <nsvstablua.h>
#include <nsvstreelua.h>
#include <nsvslistlua.h>
#include <nsvslistboxlua.h>
#include <nsvsfilebrowserlua.h>
#include <nsvswizardlua.h>
#include <nscomboboxlua.h>
#include <nsframelua.h>
#include <nsguilua.h>
#endif