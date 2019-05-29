#include <nsbase.h>
#include <LzmaLib.h>
#include <zlib.h>
#include <mbedtls/pk.h>
#include <mbedtls/base64.h>
#include <mbedtls/md5.h>

namespace NSBase
{
	// gzip 函数
	int gzDecompress( Byte* zdata, uLong nzdata, Byte* data, uLong* ndata )
	{
		int err = 0;
		z_stream d_stream = { 0 }; /* decompression stream */
		static char dummy_head[ 2 ] = {
			0x8 + 0x7 * 0x10,
			( ( ( 0x8 + 0x7 * 0x10 ) * 0x100 + 30 ) / 31 * 31 ) & 0xFF,
		};
		d_stream.zalloc = NULL;
		d_stream.zfree = NULL;
		d_stream.opaque = NULL;
		d_stream.next_in = zdata;
		d_stream.avail_in = 0;
		d_stream.next_out = data;
		//只有设置为MAX_WBITS + 16才能在解压带header和trailer的文本
		if ( inflateInit2( &d_stream, MAX_WBITS + 16 ) != Z_OK ) return -1;
		//if(inflateInit2(&d_stream, 47) != Z_OK) return -1;
		while ( d_stream.total_out < *ndata && d_stream.total_in < nzdata )
		{
			d_stream.avail_in = d_stream.avail_out = 1; /* force small buffers */
			if ( ( err = inflate( &d_stream, Z_NO_FLUSH ) ) == Z_STREAM_END ) break;
			if ( err != Z_OK )
			{
				if ( err == Z_DATA_ERROR )
				{
					d_stream.next_in = (Bytef*) dummy_head;
					d_stream.avail_in = sizeof( dummy_head );
					if ( ( err = inflate( &d_stream, Z_NO_FLUSH ) ) != Z_OK )
					{
						return -1;
					}
				}
				else return -1;
			}
		}
		if ( inflateEnd( &d_stream ) != Z_OK ) return -1;
		*ndata = d_stream.total_out;
		return 0;
	}

	int gzCompress( Bytef *data, uLong ndata, Bytef *zdata, uLong *nzdata )
	{
		z_stream c_stream;

		if ( data && ndata > 0 )
		{
			int err = 0;
			c_stream.zalloc = NULL;
			c_stream.zfree = NULL;
			c_stream.opaque = NULL;
			//只有设置为MAX_WBITS + 16才能在在压缩文本中带header和trailer
			if ( deflateInit2( &c_stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED,
				MAX_WBITS + 16, 8, Z_DEFAULT_STRATEGY ) != Z_OK ) return -1;
			c_stream.next_in = data;
			c_stream.avail_in = (unsigned int) ndata;
			c_stream.next_out = zdata;
			c_stream.avail_out = (unsigned int) *nzdata;
			while ( c_stream.avail_in != 0 && c_stream.total_out < *nzdata )
			{
				if ( deflate( &c_stream, Z_NO_FLUSH ) != Z_OK ) return -1;
			}
			if ( c_stream.avail_in != 0 ) return c_stream.avail_in;
			for ( ;;)
			{
				if ( ( err = deflate( &c_stream, Z_FINISH ) ) == Z_STREAM_END ) break;
				if ( err != Z_OK ) return -1;
			}
			if ( deflateEnd( &c_stream ) != Z_OK ) return -1;
			*nzdata = c_stream.total_out;
			return 0;
		}
		return -1;
	}

	// ********************************************************************** //
	// CFBOctetsBuffer
	// ********************************************************************** //
	class CFBOctetsBuffer
	{
	public:
		unsigned int	mSize;			// 数据块容量
		unsigned int	mLength;		// 逻辑长度
		unsigned int	mReference;		// 引用计数

	public:
		void addRef( );
		void release( );
		// 得到数据
		void* getData( );
		// 克隆一个数据块
		void* clone( );
		// 归一化引用计数
		void* unique( );
		// 预约指定大小的空间
		void* reserve( unsigned int vSize );

	public:
		static CFBOctetsBuffer* create( unsigned int size );

	public:
		static CFBOctetsBuffer sOctNull;
	};

	// 建立一个OctetsBuffer
	CFBOctetsBuffer* CFBOctetsBuffer::create( unsigned int size )
	{
		CFBOctetsBuffer* oct = reinterpret_cast<CFBOctetsBuffer*>( CAllocator::alloc( __FILE__, __LINE__, size + sizeof( CFBOctetsBuffer ) ) );
		oct->mSize = size;
		oct->mLength = 0;
		oct->mReference = 1;
		return oct;
	}

	// 得到数据块
	CFBOctetsBuffer* OctetsBuffer( void* base )
	{
		return reinterpret_cast<CFBOctetsBuffer*>( base ) - 1;
	}

	CFBOctetsBuffer CFBOctetsBuffer::sOctNull = { 0, 0, 1 };

	// ********************************************************************** //
	// CFBOctetsBuffer
	// ********************************************************************** //
	void CFBOctetsBuffer::addRef( )
	{
		mReference ++;
	}

	void CFBOctetsBuffer::release( )
	{
		if ( -- mReference == 0 )
			CAllocator::free( this );
	}

	// 得到数据
	void* CFBOctetsBuffer::getData( )
	{
		return reinterpret_cast<CFBOctetsBuffer*>( this + 1 );
	}

	// 克隆一个数据块
	void* CFBOctetsBuffer::clone( )
	{
		CFBOctetsBuffer* oct = create( mSize );
		NSFunction::memcpy_fast( oct->getData( ), getData( ), oct->mLength = mLength );
		return oct->getData( );
	}

	// 归一化引用计数
	void* CFBOctetsBuffer::unique( )
	{
		if ( mReference > 1 )
		{
			// 首先克隆一个出来
			void* data = clone( );

			// 将自己释放，因为引用计数大于1，所有还有其他指针指向本数据块
			release( );
			return data;
		}

		return getData( );
	}

	// 预约指定大小的空间
	void* CFBOctetsBuffer::reserve( unsigned int size )
	{
		size = (unsigned int) NSFunction::forbsize( size );
		if ( size > mSize )
		{
			CFBOctetsBuffer* oct = create( size );
			NSFunction::memcpy_fast( oct->getData( ), getData( ), oct->mLength = mLength );

			release( );
			return oct->getData( );
		}
		return unique( );
	}

	// ********************************************************************** //
	// CNSOctets
	// ********************************************************************** //

	CNSOctets::CNSOctets( ) : mpBase( &CFBOctetsBuffer::sOctNull + 1 )
	{
		OctetsBuffer( mpBase )->addRef( );
	}

	CNSOctets::CNSOctets( unsigned int size, unsigned int len ) : mpBase( CFBOctetsBuffer::create( size )->getData( ) )
	{
		length( ) = len;
	}

	CNSOctets::CNSOctets( const void* data, unsigned int len ) : mpBase( CFBOctetsBuffer::create( len )->getData( ) )
	{
		NSFunction::memcpy_fast( mpBase, data, len );
		length( ) = len;
	}

	CNSOctets::CNSOctets( const void* begin, const void* end ) : mpBase( CFBOctetsBuffer::create( (unsigned int) ( (char*) end - (char*) begin ) )->getData( ) )
	{
		ptrdiff_t len = (char*) end - (char*) begin;
		NSFunction::memcpy_fast( mpBase, begin, len );
		length( ) = (unsigned int) len;
	}

	CNSOctets::CNSOctets( const CNSOctets& octets ) : mpBase( octets.mpBase )
	{
		OctetsBuffer( mpBase )->addRef( );
	}

	CNSOctets::CNSOctets( const CNSString& text ) : mpBase( CFBOctetsBuffer::create( (unsigned int) text.getLength( ) )->getData( ) )
	{
		NSFunction::memcpy_fast( mpBase, text.getBuffer( ), text.getLength( ) );
		length( ) = (unsigned int) text.getLength( );
	}

	CNSOctets::~CNSOctets( )
	{
		OctetsBuffer( mpBase )->release( );
	}

	// 归一化引用技术
	void CNSOctets::unique( )
	{
		mpBase = OctetsBuffer( mpBase )->unique( );
	}

	// 预约指定大小的空间
	CNSOctets& CNSOctets::reserve( unsigned int vSize )
	{
		mpBase = OctetsBuffer( mpBase )->reserve( vSize );
		return *this;
	}

	// 替换数据
	CNSOctets& CNSOctets::replace( const void* data, unsigned int len )
	{
		reserve( len );
		NSFunction::memcpy_fast( mpBase, data, len );
		length( ) = len;
		return *this;
	}

	CNSOctets& CNSOctets::operator = ( const CNSOctets& octets )
	{
		if ( &octets != this )
		{
			OctetsBuffer( mpBase )->release( );
			mpBase = octets.mpBase;
			OctetsBuffer( mpBase )->addRef( );
		}
		return *this;
	}

	CNSOctets& CNSOctets::operator = ( const CNSString& text )
	{
		replace( text.getBuffer( ), (unsigned int) text.getLength( ) );
		return *this;
	}

	bool CNSOctets::operator == ( const CNSOctets& octets ) const
	{
		return size( ) == octets.size( ) && !memcmp( mpBase, octets.mpBase, size( ) );
	}

	bool CNSOctets::operator != ( const CNSOctets& octets ) const
	{
		return ! operator == ( octets );
	}

	CNSOctets& CNSOctets::swap( CNSOctets& octets )
	{
		void* tpTemp = mpBase;
		mpBase = octets.mpBase;
		octets.mpBase = tpTemp;
		return *this;
	}

	void* CNSOctets::begin( )
	{
		unique( );
		return mpBase;
	}

	void* CNSOctets::end( )
	{
		unique( );
		return (char*) mpBase + length( );
	}

	const void* CNSOctets::begin( ) const
	{
		return mpBase;
	}

	const void* CNSOctets::end( ) const
	{
		return (char*) mpBase + length( );
	}

	unsigned int CNSOctets::length( ) const
	{
		return OctetsBuffer( mpBase )->mLength;
	}

	unsigned int& CNSOctets::length( )
	{
		return OctetsBuffer( mpBase )->mLength;
	}

	unsigned int CNSOctets::size( ) const
	{
		return OctetsBuffer( mpBase )->mSize;
	}

	CNSOctets& CNSOctets::clear( )
	{
		unique( );
		OctetsBuffer( mpBase )->mLength = 0;
		return *this;
	}

	CNSOctets& CNSOctets::erase( void* begin, void* end )
	{
		if ( begin != end )
		{
			void* tmp = mpBase;
			mpBase = OctetsBuffer( mpBase )->unique( );
			ptrdiff_t offset = (char*) mpBase - (char*) tmp;
			if ( offset != 0 )
			{
				begin = (char*) begin + offset;
				end = (char*) end + offset;
			}

			NSFunction::memmove_fast( (char*) begin, (char*) end, ( (char*) mpBase + length( ) ) - (char*) end );
			length( ) -= (unsigned int) ( (char*) end - (char*) begin );
		}
		return *this;
	}

	CNSOctets& CNSOctets::insert( void* pos, const void* data, unsigned int len )
	{
		ptrdiff_t offset = (char*) pos - (char*) mpBase;
		reserve( length( ) + len );
		pos = (char*) mpBase + offset;
		ptrdiff_t adjust = length( ) - offset;
		if ( adjust != 0 )
			NSFunction::memmove_fast( (char*) pos + len, pos, adjust );

		NSFunction::memmove_fast( pos, data, len );
		length( ) += len;
		return *this;
	}

	CNSOctets& CNSOctets::insert( void* pos, const void* begin, const void* end )
	{
		insert( pos, begin, (unsigned int) ( (char*) end - (char*) begin ) );
		return *this;
	}

	CNSOctets& CNSOctets::resize( unsigned int len )
	{
		reserve( len );
		length( ) = len;
		return *this;
	}

	CNSString CNSOctets::toString( ) const
	{
		static CNSString text;
		text.clear( );
		text.pushback( (const char*) begin( ), length( ) );
		return text;
	}

	CNSString CNSOctets::toValidString( ) const
	{
		static CNSString text;
		text.clear( );
		for ( unsigned int i = 0; i < length( ); i ++ )
		{
			unsigned char* dig = (unsigned char*) begin( ) + i;
			if ( isprint( *dig ) != 0 )
				text.pushback( *dig );
		}

		return text;
	}

	CNSString CNSOctets::toHexString( ) const
	{
		static CNSString hex;
		hex.clear( );
		for ( unsigned int i = 0; i < length( ); i ++ )
		{
			unsigned char* dig = (unsigned char*) begin( ) + i;
			static CNSString digText;
			digText.format( "%02x", *dig );
			hex.pushback( digText );
		}

		return hex;
	}

	//void CNSOctets::EncodeHMacSha1( const CNSString& key, CNSOctets& output ) const
	//{
	//	output.reserve( 20 );
	//	output.Length( ) = 20;
	//	NSBase::hmacSha1( (unsigned char*)key.getBuffer( ), key.getLength( ), (unsigned  char*)begin( ), Length( ), (unsigned char*)output.begin( ) );
	//}

	CNSOctets CNSOctets::decodeRsaByPrivkey( const CNSString& key ) const
	{
		static CNSString pemKey;
		pemKey.clear( );
		pemKey += "-----BEGIN PRIVATE KEY-----\r\n";
		pemKey += key;
		pemKey += "\r\n-----END PRIVATE KEY-----";

		mbedtls_pk_context pk;
		mbedtls_pk_init( &pk );
		int ret = 0;
		if ( ( ret = mbedtls_pk_parse_key( &pk, (const unsigned char*) key.getBuffer( ), key.getCount( ), NULL, 0 ) ) != 0 )
		{
			static CNSString errorDesc;
			errorDesc.format( "RSA - private key parse Error, errorCode - 0x%x", ret );
			NSException( errorDesc );
		}

		mbedtls_rsa_context* rsa = mbedtls_pk_rsa( pk );
		if ( ( ret = mbedtls_rsa_check_privkey( rsa ) ) != 0 )
		{
			static CNSString errorDesc;
			errorDesc.format( "RSA - translate private key to rsa context error, errorCode - 0x%x", ret );
			NSException( errorDesc );
		}

		static CNSOctets value;
		value.reserve( (unsigned int) rsa->len );
		value.length( ) = (unsigned int) rsa->len;
		if ( ( ret = mbedtls_rsa_private( rsa, NULL, 0, (const unsigned char*) begin( ), (unsigned char*) value.begin( ) ) ) != 0 )
		{
			static CNSString errorDesc;
			errorDesc.format( "RSA - private decode error, errorCode - 0x%x", ret );
			NSException( errorDesc );
		}

		mbedtls_pk_free( &pk );
		return value;
	}

	CNSOctets CNSOctets::decodeRsaByPubkey( const CNSString& key ) const
	{
		static CNSString pemKey;
		pemKey.clear( );
		pemKey += "-----BEGIN PUBLIC KEY-----\r\n";
		pemKey += key;
		pemKey += "\r\n-----END PUBLIC KEY-----";

		mbedtls_pk_context pk;
		mbedtls_pk_init( &pk );
		int ret = 0;
		if ( ( ret = mbedtls_pk_parse_public_key( &pk, (const unsigned char*) pemKey.getBuffer( ), pemKey.getCount( ) ) ) != 0 )
		{
			static CNSString errorDesc;
			errorDesc.format( "RSA - public key parse Error, errorCode - 0x%x", ret );
			NSException( errorDesc );
		}

		mbedtls_rsa_context* rsa = mbedtls_pk_rsa( pk );
		if ( ( ret = mbedtls_rsa_check_pubkey( rsa ) ) != 0 )
		{
			static CNSString errorDesc;
			errorDesc.format( "RSA - translate public key to rsa context error, errorCode - 0x%x", ret );
			NSException( errorDesc );
		}

		static CNSOctets value;
		value.reserve( (unsigned int) rsa->len );
		value.length( ) = (unsigned int) rsa->len;
		if ( ( ret = mbedtls_rsa_public( rsa, (const unsigned char*) begin( ), (unsigned char*) value.begin( ) ) ) != 0 )
		{
			static CNSString errorDesc;
			errorDesc.format( "RSA - public decode error, errorCode - 0x%x", ret );
			NSException( errorDesc );
		}

		mbedtls_pk_free( &pk );
		return value;
	}

	CNSOctets CNSOctets::gzipUncompress( unsigned int size ) const
	{
		CNSOctets outBuffer( size, size );
		unsigned int desSize = outBuffer.length( );
		unsigned int srcSize = length( );
		unsigned char* srcData = (unsigned char*) begin( );
		int tResult = NSBase::gzDecompress( (unsigned char*) srcData, srcSize, (unsigned char*) outBuffer.begin( ), (uLong*) &desSize );
		if ( tResult != SZ_OK )
		{
			static CNSString tErrorDesc;
			tErrorDesc.format( _UTF8( "GZip解压缩时发生错误, 错误码: %d" ), tResult );
			NSException( tErrorDesc.getBuffer( ) );
		}

		outBuffer.length( ) = desSize;
		return outBuffer;
	}

	CNSOctets CNSOctets::gzipCompress( ) const
	{
		CNSOctets cBuffer( length( ) * 2, length( ) * 2 );
		unsigned int size = cBuffer.length( );

		int result = NSBase::gzCompress( (unsigned char*) begin( ), length( ), (unsigned char*) cBuffer.begin( ), (uLong*) &size );
		if ( result != 0 )
		{
			static CNSString tErrorDesc;
			tErrorDesc.format( _UTF8( "GZip压缩时发生错误, 错误码: %d" ), result );
			NSException( tErrorDesc.getBuffer( ) );
		}

		cBuffer.length( ) = size;
		return cBuffer;
	}

	CNSOctets CNSOctets::uncompress( const void* begin, const void* end )
	{
		static CNSOctets value;
		size_t len = (ptrdiff_t) end - (ptrdiff_t) begin;
		if ( len == 0 )
			return value;

		unsigned char* data = (unsigned char*) begin;
		char* lzmaTag = (char*) data;
		if ( lzmaTag[ 0 ] != 'l' && lzmaTag[ 1 ] != 'z' )
		{
			static CNSString tErrorDesc;
			tErrorDesc.format( _UTF8( "Lzma解压缩时发生错误, 压缩数据被篡改" ) );
			NSException( tErrorDesc.getBuffer( ) );
		}

		size_t uSize = *( (unsigned int*) ( data + 2 ) );
		size_t cSize = len - LZMA_PROPS_SIZE - sizeof( unsigned int ) - 2;
		value.reserve( (unsigned int) uSize );
		value.length( ) = (unsigned int) uSize;

		unsigned char* propBuffer = data + sizeof( unsigned int ) + 2;
		int result = LzmaUncompress( (unsigned char*) value.begin( ), &uSize, (unsigned char*) propBuffer + LZMA_PROPS_SIZE, &cSize, propBuffer, LZMA_PROPS_SIZE );
		if ( result != SZ_OK )
		{
			static CNSString tErrorDesc;
			tErrorDesc.format( _UTF8( "Lzma解压缩时发生错误, 错误码: %d" ), result );
			NSException( tErrorDesc.getBuffer( ) );
		}

		return value;
	}

	CNSOctets CNSOctets::uncompress( ) const
	{
		return uncompress( begin( ), end( ) );
	}

	CNSOctets CNSOctets::compress( const void* begin, const void* end )
	{
		size_t uSize = (ptrdiff_t) end - (ptrdiff_t) begin;
		static char sLzmaTag[] = "lz";
		unsigned char prop[ LZMA_PROPS_SIZE ];
		size_t propSize = LZMA_PROPS_SIZE;
		size_t headerSize = sizeof( unsigned int ) + LZMA_PROPS_SIZE + 2;

		static CNSOctets value;
		value.reserve( (unsigned int) ( uSize * 3 + headerSize ) );
		value.length( ) = (unsigned int) ( uSize * 3 + headerSize );
		size_t cSize = (unsigned int) ( uSize * 3 + headerSize );
		unsigned char* tpCBuffer = (unsigned char*) value.begin( );
		int result = LzmaCompress( tpCBuffer + headerSize, &cSize, (unsigned char*) begin, uSize, prop, &propSize, 5, 1 << 16, 3, 0, 2, 32, 2 );
		if ( propSize != LZMA_PROPS_SIZE )
		{
			static CNSString tErrorDesc;
			tErrorDesc.format( _UTF8( "Lzma压缩时发生错误, 属性长度不正确[%d]" ), propSize );
			NSException( tErrorDesc.getBuffer( ) );
		}

		if ( result != SZ_OK )
		{
			static CNSString tErrorDesc;
			tErrorDesc.format( _UTF8( "Lzma压缩时发生错误, 错误码:%d" ), result );
			NSException( tErrorDesc.getBuffer( ) );
		}

		// 插入lzma标签
		NSFunction::memcpy_fast( tpCBuffer, sLzmaTag, 2 );

		// 插入数据原始大小
		NSFunction::memcpy_fast( tpCBuffer + 2, &uSize, sizeof( unsigned int ) );

		// 插入Lzma压缩信息头
		NSFunction::memcpy_fast( tpCBuffer + sizeof( unsigned int ) + 2, prop, LZMA_PROPS_SIZE );

		// 设置压缩后buffer大小
		value.length( ) = cSize + headerSize;
		return value;
	}

	CNSOctets CNSOctets::compress( ) const
	{
		return CNSOctets::compress( begin( ), end( ) );
	}

	CNSOctets CNSOctets::encodeBase64( ) const
	{
		static CNSOctets value;
		size_t olen = 0;
		mbedtls_base64_encode( NULL, 0, &olen, (const unsigned char*) begin( ), length( ) );

		value.reserve( olen );
		value.length( ) = olen - 1;
		int ret = mbedtls_base64_encode( (unsigned char*) value.begin( ), olen, &olen, (const unsigned char*) begin( ), length( ) );
		if ( ret != 0 )
		{
			static CNSString errorDesc;
			errorDesc.format( "base64 encode error, errorCode - 0x%x", ret );
			NSException( errorDesc );
		}

		return value;
	}

	CNSOctets CNSOctets::decodeBase64( ) const
	{
		static CNSOctets value;
		size_t olen = 0;
		mbedtls_base64_decode( NULL, 0, &olen, (const unsigned char*) begin( ), length( ) );

		value.reserve( olen );
		value.length( ) = olen;
		int ret = mbedtls_base64_decode( (unsigned char*) value.begin( ), olen, &olen, (const unsigned char*) begin( ), length( ) );
		if ( ret != 0 )
		{
			static CNSString errorDesc;
			errorDesc.format( "base64 encode error, errorCode - %d", ret );
			NSException( errorDesc );
		}

		return value;
	}

	CNSOctets CNSOctets::encodeMD5( ) const
	{
		static CNSOctets value;
		value.reserve( 16 );
		value.length( ) = 16;
		mbedtls_md5( (const unsigned char*) begin( ), length( ), (unsigned char*) value.begin( ) );
		return value;
	}

	// ********************************************************************** //
	// CNSOctetsStream
	// ********************************************************************** //
	CNSOctetsStream::CNSOctetsStream( EStreamType streamType, unsigned int maxSpare ) : mPos( 0 ), mTranPos( 0 ), mStreamType( streamType ), mMaxSpare( maxSpare )
	{
	}

	CNSOctetsStream::CNSOctetsStream( const CNSOctets octets, EStreamType streamType, unsigned int maxSpare ) : mBuffer( octets ), mPos( 0 ), mTranPos( 0 ), mStreamType( streamType ), mMaxSpare( maxSpare )
	{
	}

	CNSOctetsStream::CNSOctetsStream( const CNSOctetsStream& stream ) : mBuffer( stream.mBuffer ), mPos( stream.mPos ), mTranPos( stream.mTranPos ), mStreamType( stream.mStreamType ), mMaxSpare( stream.mMaxSpare )
	{
	}

	CNSOctetsStream& CNSOctetsStream::operator = ( const CNSOctetsStream& stream )
	{
		if ( &stream != this )
		{
			mPos = stream.mPos;
			mTranPos = stream.mTranPos;
			mBuffer = stream.mBuffer;
			mStreamType = stream.mStreamType;
			mMaxSpare = stream.mMaxSpare;
		}
		return *this;
	}

	bool CNSOctetsStream::operator == ( const CNSOctetsStream& rOStream ) const
	{
		return mBuffer == rOStream.mBuffer;
	}

	bool CNSOctetsStream::operator != ( const CNSOctetsStream& rOStream ) const
	{
		return mBuffer != rOStream.mBuffer;
	}

	unsigned int CNSOctetsStream::length( ) const
	{
		return mBuffer.length( );
	}

	void CNSOctetsStream::swap( CNSOctetsStream& rOStream )
	{
		mBuffer.swap( rOStream.mBuffer );
	}

	CNSOctetsStream::operator CNSOctets& ( )
	{
		return mBuffer;
	}

	void* CNSOctetsStream::begin( )
	{
		return mBuffer.begin( );
	}

	void* CNSOctetsStream::end( )
	{
		return mBuffer.end( );
	}

	void CNSOctetsStream::reserve( unsigned int size )
	{
		mBuffer.reserve( size );
	}

	const void* CNSOctetsStream::begin( ) const
	{
		return mBuffer.begin( );
	}

	const void* CNSOctetsStream::end( ) const
	{
		return mBuffer.end( );
	}

	void CNSOctetsStream::insert( void* pos, const void* begin, unsigned int size )
	{
		mBuffer.insert( pos, begin, size );
	}

	void CNSOctetsStream::insert( void* pos, const void* begin, const void* end )
	{
		mBuffer.insert( pos, begin, end );
	}

	void CNSOctetsStream::erase( void* begin, void* end )
	{
		mBuffer.erase( begin, end );
	}

	void CNSOctetsStream::clear( )
	{
		mBuffer.clear( );
		mPos = 0;
	}

	bool CNSOctetsStream::isEnd( ) const
	{
		return mPos == mBuffer.length( );
	}

	template< typename T > T CNSOctetsStream::popBytes( ) const
	{
		T retValue;
		if ( mPos + sizeof( T ) > mBuffer.length( ) )
			throw CNSMarshal::CException( );

		retValue = *(T*) ( (char*) mBuffer.begin( ) + mPos );
		mPos += sizeof( T );
		return retValue;
	}

	// 打包接口
	CNSOctetsStream& CNSOctetsStream::operator << ( const bool value )
	{
		pushBytes( value );
		return *this;
	}

	CNSOctetsStream& CNSOctetsStream::operator << ( const unsigned char value )
	{
		pushBytes( value );
		return *this;
	}

	CNSOctetsStream& CNSOctetsStream::operator << ( const char value )
	{
		pushBytes( value );
		return *this;
	}

	CNSOctetsStream& CNSOctetsStream::operator << ( const unsigned short value )
	{
		// 如果使用网络字节流
		if ( mStreamType == EStreamNet )
			pushBytes( swapByte16( value ) );

		// 如果使用内存字节流
		if ( mStreamType == EStreamMemory )
			pushBytes( value );

		return *this;
	}

	CNSOctetsStream& CNSOctetsStream::operator << ( const short value )
	{
		// 如果使用网络字节流
		if ( mStreamType == EStreamNet )
			pushBytes( swapByte16( value ) );

		// 如果使用内存字节流
		if ( mStreamType == EStreamMemory )
			pushBytes( value );

		return *this;
	}

	CNSOctetsStream& CNSOctetsStream::operator << ( const unsigned int value )
	{
		// 如果使用网络字节流
		if ( mStreamType == EStreamNet )
			compactUInt( value );

		// 如果使用内存字节流
		if ( mStreamType == EStreamMemory )
			pushBytes( value );

		return *this;
	}

	CNSOctetsStream& CNSOctetsStream::operator << ( const int value )
	{
		// 如果使用网络字节流
		if ( mStreamType == EStreamNet )
			compactSInt( value );

		// 如果使用内存字节流
		if ( mStreamType == EStreamMemory )
			pushBytes( value );

		return *this;
	}

	CNSOctetsStream& CNSOctetsStream::operator << ( const unsigned long long value )
	{
		// 如果使用网络字节流
		if ( mStreamType == EStreamNet )
			pushBytes( swapByte64( value ) );

		// 如果使用内存字节流
		if ( mStreamType == EStreamMemory )
			pushBytes( value );

		return *this;
	}

	CNSOctetsStream& CNSOctetsStream::operator << ( const long long value )
	{
		// 如果使用网络字节流
		if ( mStreamType == EStreamNet )
			pushBytes( swapByte64( value ) );

		// 如果使用内存字节流
		if ( mStreamType == EStreamMemory )
			pushBytes( value );

		return *this;
	}

	CNSOctetsStream& CNSOctetsStream::operator << ( const float value )
	{
		// 如果使用网络字节流
		if ( mStreamType == EStreamNet )
			pushBytes( swapByte32( NSFunction::aliasingCast<unsigned int>( value ) ) );

		// 如果使用内存字节流
		if ( mStreamType == EStreamMemory )
			pushBytes( NSFunction::aliasingCast<unsigned int>( value ) );

		return *this;
	}

	CNSOctetsStream& CNSOctetsStream::operator << ( const double value )
	{
		// 如果使用网络字节流
		if ( mStreamType == EStreamNet )
			pushBytes( swapByte64( NSFunction::aliasingCast<unsigned long long>( value ) ) );

		// 如果使用内存字节流
		if ( mStreamType == EStreamMemory )
			pushBytes( NSFunction::aliasingCast<unsigned long long>( value ) );
		return *this;
	}

	CNSOctetsStream& CNSOctetsStream::operator << ( const CNSMarshal& marshal )
	{
		return marshal.marshal( *this );
	}

	CNSOctetsStream& CNSOctetsStream::operator << ( const CNSOctets octets )
	{
		operator << ( (unsigned int) octets.length( ) );
		mBuffer.insert( mBuffer.end( ), octets.begin( ), octets.end( ) );
		return *this;
	}

	CNSOctetsStream& CNSOctetsStream::operator << ( const CNSString& text )
	{
		operator << ( (unsigned short) text.getLength( ) );
		mBuffer.insert( mBuffer.end( ), text.getBuffer( ), text.getLength( ) );
		return *this;
	}

	CNSOctetsStream& CNSOctetsStream::compactUInt( unsigned int value )
	{
		if ( value < 0x80 )
			return pushBytes( (unsigned char) value );
		else if ( value < 0x4000 )
			return pushBytes( swapByte16( value | 0x8000 ) );
		else if ( value < 0x20000000 )
			return pushBytes( swapByte32( value | 0xC0000000 ) );
		pushBytes( (unsigned char) 0xE0 );
		return pushBytes( swapByte32( value ) );
	}

	CNSOctetsStream& CNSOctetsStream::compactSInt( int value )
	{
		if ( value >= 0 )
		{
			if ( value < 0x40 )
				return pushBytes( (unsigned char) value );
			else if ( value < 0x2000 )
				return pushBytes( swapByte16( value | 0x8000 ) );
			else if ( value < 0x10000000 )
				return pushBytes( swapByte32( value | 0xC0000000 ) );

			pushBytes( (unsigned char) 0xE0 );
			return pushBytes( swapByte32( value ) );
		}
		if ( -value > 0 )
		{
			value = -value;
			if ( value < 0x40 )
				return pushBytes( (unsigned char) value | 0x40 );
			else if ( value < 0x2000 )
				return pushBytes( swapByte16( value | 0xA000 ) );
			else if ( value < 0x10000000 )
				return pushBytes( swapByte32( value | 0xD0000000 ) );

			pushBytes( (unsigned char) 0xF0 );
			return pushBytes( swapByte32( value ) );
		}

		pushBytes( (unsigned char) 0xF0 );
		return pushBytes( swapByte32( value ) );
	}

	// 解包接口
	const CNSOctetsStream& CNSOctetsStream::operator >> ( const bool& value ) const
	{
		NSFunction::removeConst( value ) = popBytes<bool>( );
		return *this;
	}

	const CNSOctetsStream& CNSOctetsStream::operator >> ( const unsigned char& value ) const
	{
		NSFunction::removeConst( value ) = popBytes<unsigned char>( );
		return *this;
	}

	const CNSOctetsStream& CNSOctetsStream::operator >> ( const char& value ) const
	{
		NSFunction::removeConst( value ) = popBytes<char>( );
		return *this;
	}

	const CNSOctetsStream& CNSOctetsStream::operator >> ( const unsigned short& value ) const
	{
		if ( mStreamType == EStreamNet )
			NSFunction::removeConst( value ) = swapByte16( popBytes<unsigned short>( ) );

		if ( mStreamType == EStreamMemory )
			NSFunction::removeConst( value ) = popBytes<unsigned short>( );

		return *this;
	}

	const CNSOctetsStream& CNSOctetsStream::operator >> ( const short& value ) const
	{
		if ( mStreamType == EStreamNet )
			NSFunction::removeConst( value ) = swapByte16( popBytes<short>( ) );

		if ( mStreamType == EStreamMemory )
			NSFunction::removeConst( value ) = popBytes<short>( );

		return *this;
	}

	const CNSOctetsStream& CNSOctetsStream::operator >> ( const unsigned int& value ) const
	{
		if ( mStreamType == EStreamNet )
			uncompactUInt( NSFunction::removeConst( value ) );

		if ( mStreamType == EStreamMemory )
			NSFunction::removeConst( value ) = popBytes<unsigned int>( );

		return *this;
	}

	const CNSOctetsStream& CNSOctetsStream::operator >> ( const int& value ) const
	{
		if ( mStreamType == EStreamNet )
			uncompactSInt( NSFunction::removeConst( value ) );

		if ( mStreamType == EStreamMemory )
			NSFunction::removeConst( value ) = popBytes<int>( );

		return *this;
	}

	const CNSOctetsStream& CNSOctetsStream::operator >> ( const unsigned long long& value ) const
	{
		if ( mStreamType == EStreamNet )
			NSFunction::removeConst( value ) = swapByte64( popBytes<unsigned long long>( ) );

		if ( mStreamType == EStreamMemory )
			NSFunction::removeConst( value ) = popBytes<unsigned long long>( );

		return *this;
	}

	const CNSOctetsStream& CNSOctetsStream::operator >> ( const long long& value ) const
	{
		if ( mStreamType == EStreamNet )
			NSFunction::removeConst( value ) = swapByte64( popBytes<long long>( ) );

		if ( mStreamType == EStreamMemory )
			NSFunction::removeConst( value ) = popBytes<long long>( );

		return *this;
	}

	const CNSOctetsStream& CNSOctetsStream::operator >> ( const float& value ) const
	{
		if ( mStreamType == EStreamNet )
			NSFunction::removeConst( value ) = NSFunction::aliasingCast<float>( swapByte32( popBytes<unsigned int>( ) ) );

		if ( mStreamType == EStreamMemory )
			NSFunction::removeConst( value ) = NSFunction::aliasingCast<float>( popBytes<unsigned int>( ) );

		return *this;
	}

	const CNSOctetsStream& CNSOctetsStream::operator >> ( const double& value ) const
	{
		if ( mStreamType == EStreamNet )
			NSFunction::removeConst( value ) = NSFunction::aliasingCast<double>( swapByte64( popBytes<unsigned long long>( ) ) );

		if ( mStreamType == EStreamMemory )
			NSFunction::removeConst( value ) = NSFunction::aliasingCast<double>( popBytes<unsigned long long>( ) );

		return *this;
	}

	const CNSOctetsStream& CNSOctetsStream::operator >> ( const CNSMarshal& marshal ) const
	{
		return NSFunction::removeConst( marshal ).unMarshal( *this );
	}

	const CNSOctetsStream& CNSOctetsStream::operator >> ( const CNSOctets& octets ) const
	{
		unsigned int len;
		operator >> ( len );
		unsigned int progress = mBuffer.length( ) - mPos;
		if ( len > progress )
			throw CNSMarshal::CException( len, progress );

		NSFunction::removeConst( octets ).replace( (char*) mBuffer.begin( ) + mPos, len );
		mPos += len;
		return *this;
	}

	const CNSOctetsStream& CNSOctetsStream::operator >> ( ETransaction trans ) const
	{
		switch ( trans )
		{
		case NSBase::EBegin:
			mTranPos = mPos;
			break;
		case NSBase::ERollback:	
			mPos = mTranPos;	
			break;
		case NSBase::ECommit:
			if ( mPos >= mMaxSpare )
			{
				NSFunction::removeConst( *this ).mBuffer.erase( (char*) mBuffer.begin( ), (char*) mBuffer.begin( ) + mPos );
				mPos = 0;
			}
		}
		return *this;
	}

	// 读取现在的字符串序列化格式
	const CNSOctetsStream& CNSOctetsStream::operator >> ( const CNSString& text ) const
	{
		unsigned short len;
		operator >> ( len );
		NSFunction::removeConst( text ).clear( );
		if ( len > mBuffer.size( ) - mPos )
			throw CNSMarshal::CException( );

		char* tpChar = (char*) ( (char*) mBuffer.begin( ) + mPos );
		NSFunction::removeConst( text ).pushback( tpChar, len );
		mPos += len;
		return *this;
	}

	const CNSOctetsStream& CNSOctetsStream::uncompactUInt( const unsigned int& value ) const
	{
		if ( mPos == mBuffer.length( ) )
			throw CNSMarshal::CException( );

		switch ( *( (unsigned char*) mBuffer.begin( ) + mPos ) & 0xE0 )
		{
			case 0xE0:
				popBytes<char>( );
				NSFunction::removeConst( value ) = swapByte32( popBytes<unsigned int>( ) );
				return *this;
			case 0xC0:
				NSFunction::removeConst( value ) = swapByte32( popBytes<unsigned int>( ) ) & ~0xC0000000;
				return *this;
			case 0xA0:
			case 0x80:
				NSFunction::removeConst( value ) = swapByte16( popBytes<unsigned short>( ) ) & ~0x8000;
				return *this;
		}
		NSFunction::removeConst( value ) = popBytes<unsigned char>( );
		return *this;
	}

	const CNSOctetsStream& CNSOctetsStream::uncompactSInt( const int& value ) const
	{
		if ( mPos == mBuffer.length( ) )
			throw CNSMarshal::CException( );

		switch ( *( (unsigned char*) mBuffer.begin( ) + mPos ) & 0xF0 )
		{
			case 0xF0:
				popBytes<char>( );
				NSFunction::removeConst( value ) = -swapByte32( popBytes<unsigned int>( ) );
				return *this;
			case 0xE0:
				popBytes<char>( );
				NSFunction::removeConst( value ) = swapByte32( popBytes<unsigned int>( ) );
				return *this;
			case 0xD0:
				NSFunction::removeConst( value ) = -( swapByte32( popBytes<unsigned int>( ) ) & ~0xD0000000 );
				return *this;
			case 0xC0:
				NSFunction::removeConst( value ) = swapByte32( popBytes<unsigned int>( ) ) & ~0xC0000000;
				return *this;
			case 0xB0:
			case 0xA0:
				NSFunction::removeConst( value ) = -( swapByte16( popBytes<unsigned short>( ) ) & ~0xA000 );
				return *this;
			case 0x90:
			case 0x80:
				NSFunction::removeConst( value ) = swapByte16( popBytes<unsigned short>( ) ) & ~0x8000;
				return *this;
			case 0x70:
			case 0x60:
			case 0x50:
			case 0x40:
				NSFunction::removeConst( value ) = -( popBytes<unsigned char>( ) & ~0x40 );
				return *this;
		}
		NSFunction::removeConst( value ) = popBytes<unsigned char>( );
		return *this;
	}
}
