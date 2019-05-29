#pragma once
#include <new>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <locale.h>
#include <float.h>
#include <math.h>
#include <time.h>
#include <stddef.h>
#include <stdint.h>

#pragma warning ( disable : 4267 )
#pragma warning ( disable : 4819 )
#pragma warning ( disable : 4312 )
#pragma warning ( disable : 4311 )
#pragma warning ( disable : 4800 )
#pragma warning ( disable : 4146 )
#pragma warning ( disable : 4251 )
#pragma warning ( disable : 4005 )
#pragma warning ( disable : 4288 )
#pragma warning ( disable : 4100 )
#pragma warning ( disable : 4101 )

#ifdef PLATFORM_WIN32
#include <winsdkver.h>
#include <winsock2.h>
#include <mswsock.h>
#include <windows.h>
#include <windowsx.h>
#include <ws2tcpip.h>
#include <tlhelp32.h>
#include <dbghelp.h>
#include <apihook.h>
#include <direct.h>
#include <tchar.h>
#include <io.h>
#include <Iphlpapi.h>

#define SOCKET SOCKET
#define _UTF8( x ) CNSString::convertMbcsToUtf8( x )
#define NSException( desc ) \
	{	\
		CONTEXT context;	\
		context.ContextFlags = CONTEXT_ALL;	\
		\
		HANDLE thread = GetCurrentThread( );	\
		GetThreadContext( thread, &context );	\
		NSBase::NSFunction::stackTrace( &context ); \
		throw CNSException( desc );\
	}


#pragma comment( lib, "Ws2_32" )
#pragma comment( lib, "Mswsock" )
#endif

#ifdef PLATFORM_ANDROID
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/select.h> 
#include <netinet/in.h> 
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <errno.h>
#include <linux/tcp.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#define SOCKET int

#define _UTF8( x ) CNSString::convertMbcsToUtf8( x )
#define NSException( desc )	throw CNSException( desc );
#define stricmp			strcasecmp
#define max fmax
#define min fmin
#endif

#ifdef PLATFORM_LINUX
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/select.h> 
#include <netinet/in.h> 
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#define SOCKET int

#define _UTF8( x ) CNSString::convertMbcsToUtf8( x )
#define NSException( desc )	throw CNSException( desc );
#define stricmp		strcasecmp
#define max fmax
#define min fmin
#endif

#ifdef PLATFORM_IOS
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#define SOCKET int

#define _UTF8( x )	( x )
#define NSException( desc )	throw CNSException( desc );
#define stricmp		strcasecmp
#define max fmax
#define min fmin
#endif

namespace NSMath
{
	class CNSRect;
	class CNSVector2;
	class CNSSegment;
}

namespace NSBase
{

// utf8		文本头
#define	UTF8_BOM 0xBFBBEF
#define unused( x )	(void)(x) 

// 是否使用CRT memcpy
#define USE_CRT_MEMCPY 1

// 是否开启内存分配次数检测
#define SHOW_MEM_ALLOC 0

// 是否开启内存拷贝次数检测
#define SHOW_MEM_COPY  0

// 是否开启内存移动次数检测
#define SHOW_MEM_MOVE  0

// 是否开启LUA内存分配次数检测
#define SHOW_LUAMEM_ALLOC  0

// 统计数据显示间隔时间
#define SHOW_TICK	60000

#if USE_CRT_MEMCPY == 0
#include <emmintrin.h>
#endif
    
	// 数据类型
	enum EDataType
	{
		TYPE_INT = 1,		// 数值类型
		TYPE_UINT,			// 数值类型
		TYPE_INT64,			// 数值类型
		TYPE_UINT64,		// 数值类型
		TYPE_DOUBLE,		// 数值类型
		TYPE_FLOAT,			// 数值类型
		TYPE_USHORT,		// 数值类型
		TYPE_SHORT,			// 数值类型
		TYPE_UCHAR,			// 数值类型
		TYPE_CHAR,			// 数值类型
		TYPE_BOOLEAN,		// 布尔型
		TYPE_STRING,		// 字符串型
		TYPE_TABLE,			// 表类型
		TYPE_NONE,
	};

	class CNSLock;
	class CNSFile;
	class CNSString;
	class CNSException;
	class CNSMarshal;
	class CNSOctets;
	class CNSOctetsShadow;
	class CNSOctetsStream;
	class CAllocator;
	class CNSTimer;
	class CNSSmallTimer;
	class CNSLuaStack;
	class CNSLocalEntry;
	class CNSLuaMarshal;
	class CNSLuaWeakRef;
	class CNSLuaObject;
	class CNSLuaFunction;
	template < typename T > class CNSListNode;
	template < typename FIRST, typename SECOND > class CNSPair;
	template < typename KEY, typename T > class CNSMapNode;
	template < typename Key, typename T > class CNSHashMapNode;
	template < typename T, typename Allocator = CAllocator > class CNSBitPool;
	template < typename T, typename Allocator = CAllocator > class CNSVector;
	template < typename T, typename Allocator = CAllocator > class CNSBinaryVector;
	template < typename T, typename Allocator = CAllocator > class CNSList;
	template < typename KEY, typename Allocator = CAllocator > class CNSSet;
	template < typename KEY, typename T, typename Allocator = CAllocator > class CNSHashMap;
	template < typename KEY, typename T, typename Allocator = CAllocator > class CNSMapTomb;
	template < typename KEY, typename T, typename Allocator = CAllocator > class CNSMap;

	namespace NSFunction
	{
		int round( double value );
		int floor( double value );
		int ceil( double value );
		void memcpy_sse2_16( void *dst, const void *src );
		void memcpy_sse2_32( void *dst, const void *src );
		void memcpy_sse2_64( void *dst, const void *src );
		void memcpy_sse2_128( void *dst, const void *src );
		void *memcpy_tiny( void *dst, const void *src, size_t size );
		void* memcpy_fast( void *destination, const void *source, size_t size );
		// 计算大于指定大小的2的N次方
		size_t forbsize( size_t size );
		// 快速内存移动
		void* memmove_fast( void* des, const void* src, size_t size );
		CNSString getStackInfo( );
#ifdef PLATFORM_WIN32
		void stackTrace( CONTEXT* context );
#endif
	}
};
