#include <nsbase.h>

#ifdef PLATFORM_WIN32
#pragma comment(lib, "dbghelp.lib")
#pragma comment(lib, "msimg32.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shlwapi.lib")
#endif

#ifdef _M_X64
#ifdef _DEBUG
#pragma comment(lib, "../../nsbase/dependency/mysql/lib/x64/debug/libmysql.lib")
#else
#pragma comment(lib, "../../nsbase/dependency/mysql/lib/x64/release/libmysql.lib")
#endif
#elif _M_IX86
#ifdef _DEBUG
#pragma comment(lib, "../../nsbase/dependency/mysql/lib/win32/debug/libmysql.lib")
#else
#pragma comment(lib, "../../nsbase/dependency/mysql/lib/win32/release/libmysql.lib")
#endif
#endif