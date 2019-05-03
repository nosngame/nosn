#pragma once

namespace NSNet
{
	class CNSSockAddr
	{
	protected:
		CNSOctets	mBuffer;

	public:
		CNSSockAddr( const sockaddr_in& rAddr ) : mBuffer( &rAddr, sizeof( rAddr ) )
		{
		}

	public:
		unsigned int GetIP( ) { return (unsigned int)( (sockaddr_in*)mBuffer.begin( ) )->sin_addr.s_addr; }
		CNSString GetIPString( ) const
		{
			char ipAddress[ 16 ];
			if (::inet_ntop( AF_INET, ( void* )&( (sockaddr_in*)mBuffer.begin( ) )->sin_addr, ipAddress, 16 ) == NULL)
			{
				static CNSString errorDesc;
				errorDesc.format( "函数[inet_ntop] 调用失败, errorCode - %d", WSAGetLastError( ) );
				NSException( errorDesc );
			}

			return CNSString( ipAddress );
		}
		unsigned int GetPort( )	const
		{
			return ntohs( ( (sockaddr_in*)mBuffer.begin( ) )->sin_port );
		}
		operator const sockaddr_in* ( ) const { return (const sockaddr_in*)mBuffer.begin( ); }
		operator sockaddr_in* ( ) { return (sockaddr_in*)mBuffer.begin( ); }


		static CNSString& getMacAddress( );
		// 查找本进程是否有指定远程IP和端口的链接
		static bool findTcpConnection( const CNSString& remoteAddr, int remotePort );
	};
};
