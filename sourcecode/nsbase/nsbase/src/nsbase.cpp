#include <nsbase.h>

#ifdef PLATFORM_WIN32
#pragma comment(lib, "dbghelp.lib")
#pragma comment(lib, "msimg32.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "wldap32.lib")
#pragma comment(lib, "crypt32.lib")
#pragma comment(lib, "normaliz.lib")
#pragma comment(lib, "iphlpapi.lib")
#endif

#ifdef _M_X64
#ifdef _DEBUG
#pragma comment(lib, "../../nsbase/dependency/mysql/lib/x64/debug/libmysql.lib")
#pragma comment(lib, "../../nsbase/dependency/libcurl/lib/x64/debug/libcurld.lib")
#else
#pragma comment(lib, "../../nsbase/dependency/mysql/lib/x64/release/libmysql.lib")
#pragma comment(lib, "../../nsbase/dependency/libcurl/lib/x64/release/libcurl.lib")
#endif
#elif _M_IX86
#ifdef _DEBUG
#pragma comment(lib, "../../nsbase/dependency/mysql/lib/win32/debug/libmysql.lib")
#pragma comment(lib, "../../nsbase/dependency/libcurl/lib/win32/debug/libcurld.lib")
#else
#pragma comment(lib, "../../nsbase/dependency/mysql/lib/win32/release/libmysql.lib")
#pragma comment(lib, "../../nsbase/dependency/libcurl/lib/win32/release/libcurl.lib")
#endif
#endif
