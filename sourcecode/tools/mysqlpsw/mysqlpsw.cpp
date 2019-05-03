// mysqlpsw.cpp : 定义控制台应用程序的入口点。
//

#include <nsbase.h>

int main(int argc, char* argv[])
{
	if ( argc != 2 )
	{
		NSLog::log( "param is error, no password" );
		return 1;
	}

	CNSString psw = argv[ 1 ];
	CNSString pswEnc;
	for ( size_t i = 0; i < psw.getLength( ); i ++ )
	{
		for ( int t = 0; t < 8; t ++ )
		{
			unsigned char bit = ( psw[ i ] & ( 1 << t ) ) >> t;
			char text = NSBase::NSFunction::random( 34, 122 );
			if ( bit == 1 )
				text = ( ( text >> 1 ) << 1 ) | 0x01;
			if ( bit == 0 )
				text = ( ( text >> 1 ) << 1 );

			pswEnc += text;
		}
	}

	FILE* tpFile = fopen( "psw.txt", "wb" );
	if ( tpFile == NULL )
	{
		NSLog::log( "psw.txt open error" );
		return 0;
	}

	fwrite( pswEnc.getBuffer( ), sizeof( char ), pswEnc.getLength( ), tpFile );
	fclose( tpFile );
	return 0;
}

