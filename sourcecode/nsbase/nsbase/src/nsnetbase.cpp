#include <nsbase.h>
#include <Iphlpapi.h>
#pragma comment(lib, "iphlpapi.lib")

namespace NSNet
{
	bool CNSSockAddr::findTcpConnection( const CNSString& remoteAddr, int remotePort )
	{
		DWORD curPID = GetCurrentProcessId( );
		MIB_TCPTABLE_OWNER_MODULE* tcpTables = (MIB_TCPTABLE_OWNER_MODULE*)malloc( sizeof( MIB_TCPTABLE_OWNER_MODULE ) );
		DWORD tableSize = sizeof( MIB_TCPTABLE_OWNER_MODULE );
		if (GetExtendedTcpTable( (PVOID)tcpTables, &tableSize, TRUE, AF_INET, TCP_TABLE_OWNER_MODULE_CONNECTIONS, 0 ) == ERROR_INSUFFICIENT_BUFFER)
		{
			free( tcpTables );
			tcpTables = (MIB_TCPTABLE_OWNER_MODULE*)malloc( tableSize );
		}

		char szLocalAddr[ 16 ];
		char szRemoteAddr[ 16 ];
		IN_ADDR ipAddr;
		if (GetExtendedTcpTable( (PVOID)tcpTables, &tableSize, TRUE, AF_INET, TCP_TABLE_OWNER_MODULE_CONNECTIONS, 0 ) == NO_ERROR)
		{
			for (int i = 0; i < (int)tcpTables->dwNumEntries; i ++)
			{
				ipAddr.S_un.S_addr = (u_long)tcpTables->table[ i ].dwLocalAddr;
				if (inet_ntop( AF_INET, &ipAddr, szLocalAddr, 16 ) == NULL)
				{
					static CNSString errorDesc;
					errorDesc.format( "函数[inet_ntop]调用失败, errorCode - %d", WSAGetLastError( ) );
					NSException( errorDesc );
				}

				ipAddr.S_un.S_addr = (u_long)tcpTables->table[ i ].dwRemoteAddr;
				if (inet_ntop( AF_INET, &ipAddr, szRemoteAddr, 16 ) == NULL)
				{
					static CNSString errorDesc;
					errorDesc.format( "函数[inet_ntop]调用失败, errorCode - %d", WSAGetLastError( ) );
					NSException( errorDesc );
				}

				int rPort = ntohs( (u_short)tcpTables->table[ i ].dwRemotePort );
				if (remoteAddr == szRemoteAddr && remotePort == rPort && curPID == tcpTables->table[ i ].dwOwningPid)
					return true;
			}
		}

		return false;
	}

	CNSString& CNSSockAddr::getMacAddress( )
	{
		static CNSString macAddress;
		ULONG outBufLen = sizeof( IP_ADAPTER_ADDRESSES );
		PIP_ADAPTER_ADDRESSES pAddresses = (IP_ADAPTER_ADDRESSES*)malloc( outBufLen );
		if (pAddresses == NULL)
			return macAddress;

		// Make an initial call to GetAdaptersAddresses to get the necessary size into the ulOutBufLen variable
		if (GetAdaptersAddresses( AF_UNSPEC, 0, NULL, pAddresses, &outBufLen ) == ERROR_BUFFER_OVERFLOW)
		{
			free( pAddresses );
			pAddresses = (IP_ADAPTER_ADDRESSES*)malloc( outBufLen );
			if (pAddresses == NULL)
				return macAddress;
		}

		if (GetAdaptersAddresses( AF_UNSPEC, 0, NULL, pAddresses, &outBufLen ) == NO_ERROR)
		{
			// If successful, output some information from the data we received
			for (PIP_ADAPTER_ADDRESSES pCurrAddresses = pAddresses; pCurrAddresses != NULL; pCurrAddresses = pCurrAddresses->Next)
			{
				// 确保MAC地址的长度为 00-00-00-00-00-00
				if (pCurrAddresses->PhysicalAddressLength != 6)
					continue;

				macAddress.format( "%02X-%02X-%02X-%02X-%02X-%02X",
								   int( pCurrAddresses->PhysicalAddress[ 0 ] ),
								   int( pCurrAddresses->PhysicalAddress[ 1 ] ),
								   int( pCurrAddresses->PhysicalAddress[ 2 ] ),
								   int( pCurrAddresses->PhysicalAddress[ 3 ] ),
								   int( pCurrAddresses->PhysicalAddress[ 4 ] ),
								   int( pCurrAddresses->PhysicalAddress[ 5 ] ) );
				break;
			}
		}

		free( pAddresses );
		return macAddress;
	}
}