#pragma once
namespace NSBase
{
	enum ETransaction
	{
		EBegin,
		ECommit,
		ERollback
	};

	class CNSMarshal
	{
	public:
		class CException : public CNSException
		{
		public:
			unsigned int mTotal;
			unsigned int mProgress;

		public:
			CException( unsigned int total = 0, unsigned int progress = 0 ) : CNSException( _UTF8( "流对象溢出" ) ), mTotal( total ), mProgress( progress )
			{
			}
		};

	public:
		virtual ~CNSMarshal( )
		{
		}

	public:
		virtual CNSOctetsStream& marshal( CNSOctetsStream& ) const = 0;
		virtual const CNSOctetsStream& unMarshal( const CNSOctetsStream& ) = 0;
	};
	
	// 只支持32位长度
	class CNSOctets
	{
	protected:
		void*		mpBase;		// 数据地址

	public:
		CNSOctets( );
		CNSOctets( unsigned int size, unsigned int len = 0 );
		CNSOctets( const void* data, unsigned int size );
		// 只支持32位长度，间隔超过32位将溢出
		CNSOctets( const void* begin, const void* end );
		CNSOctets( const CNSOctets& octets );
		CNSOctets( const CNSString& text );

	public:
		virtual ~CNSOctets( );

	public:
		// 归一化引用技术
		void unique( );

		// 预约指定大小的空间
		CNSOctets& reserve( unsigned int size );

		// 替换数据
		CNSOctets& replace( const void* data, unsigned int size );
		CNSOctets& swap( CNSOctets& octets );
		void* begin( );
		void* end( );
		const void* begin( ) const;
		const void* end( ) const;

		unsigned int length( ) const;
		unsigned int& length( );
		unsigned int size( ) const;
		CNSOctets& clear( );
		// 只支持32位长度，间隔超过32位将溢出
		CNSOctets& erase( void* begin, void* end );
		CNSOctets& insert( void* pos, const void* data, unsigned int len );
		// 只支持32位长度，间隔超过32位将溢出
		CNSOctets& insert( void* pos, const void* begin, const void* end );
		CNSOctets& resize( unsigned int size );

		CNSOctets& operator = ( const CNSOctets& octets );
		CNSOctets& operator = ( const CNSString& text );
		bool operator == ( const CNSOctets& octets ) const;
		bool operator != ( const CNSOctets& octets ) const;

		CNSOctets compress( ) const;
		CNSOctets uncompress( ) const;

		CNSOctets gzipCompress( ) const;
		CNSOctets gzipUncompress( unsigned int size ) const;

		CNSString toHexString( ) const;
		CNSString toString( ) const;
		CNSString toValidString( ) const;

		CNSOctets decodeBase64( ) const;
		CNSOctets encodeBase64( ) const;
		CNSOctets encodeMD5( ) const;

		CNSOctets decodeRsaByPrivkey( const CNSString& key ) const;
		CNSOctets decodeRsaByPubkey( const CNSString& key ) const;

	public:
		static CNSOctets compress( const void* begin, const void* end );
		static CNSOctets uncompress( const void* begin, const void* end );
	};

	// 只支持32位长度
	class CNSOctetsShadow
	{
		friend class CNSOctetsStream;

	protected:
		unsigned int*			mpSize;
		void*					mpBase;
		mutable unsigned int	mPos;
		mutable unsigned int	mTranPos;

	public:
		CNSOctetsShadow( ) : mpSize( NULL ), mpBase( NULL ), mPos( 0 ), mTranPos( 0 )
		{
		}

	private:
		CNSOctetsShadow( const CNSOctetsShadow& buffer ) : mpSize( NULL ), mpBase( NULL ), mPos( 0 ), mTranPos( 0 )
		{
		}

	public:
		inline bool isEnd( ) const
		{
			return mPos == *mpSize;
		}

		template< typename T > inline T popBytes( ) const
		{
			if ( mpBase == NULL || mpSize == NULL )
				throw CNSMarshal::CException( );

			T tRetValue;
			if ( mPos + sizeof( T ) > *mpSize )
				throw CNSMarshal::CException( );

			tRetValue = *(T*) ( (char*) mpBase + mPos );
			mPos += sizeof( T );
			return tRetValue;
		}

		inline const CNSOctetsShadow& operator >> ( const unsigned char& value ) const
		{
			NSFunction::removeConst( value ) = popBytes<unsigned char>( );
			return *this;
		}

		inline const CNSOctetsShadow& operator >> ( const bool& value ) const
		{
			NSFunction::removeConst( value ) = popBytes<bool>( );
			return *this;
		}

		inline const CNSOctetsShadow& operator >> ( const char& value ) const
		{
			NSFunction::removeConst( value ) = popBytes<char>( );
			return *this;
		}

		inline const CNSOctetsShadow& operator >> ( const unsigned short& value ) const
		{
			NSFunction::removeConst( value ) = popBytes<unsigned short>( );
			return *this;
		}

		inline const CNSOctetsShadow& operator >> ( const short& value ) const
		{
			NSFunction::removeConst( value ) = popBytes<short>( );
			return *this;
		}

		inline const CNSOctetsShadow& operator >> ( const unsigned int& value ) const
		{
			NSFunction::removeConst( value ) = popBytes<unsigned int>( );
			return *this;
		}

		inline const CNSOctetsShadow& operator >> ( const int& value ) const
		{
			NSFunction::removeConst( value ) = popBytes<int>( );
			return *this;
		}

		inline const CNSOctetsShadow& operator >> ( const unsigned long long& value ) const
		{
			NSFunction::removeConst( value ) = popBytes<unsigned long long>( );
			return *this;
		}

		inline const CNSOctetsShadow& operator >> ( const long long& value ) const
		{
			NSFunction::removeConst( value ) = popBytes<long long>( );
			return *this;
		}

		inline const CNSOctetsShadow& operator >> ( const float& value ) const
		{
			NSFunction::removeConst( value ) = popBytes<float>( );
			return *this;
		}

		inline const CNSOctetsShadow& operator >> ( const double& value ) const
		{
			NSFunction::removeConst( value ) = popBytes<double>( );
			return *this;
		}

		inline const CNSOctetsShadow& operator >> ( const CNSOctets& octets ) const
		{
			if ( mpBase == NULL || mpSize == NULL )
				throw CNSMarshal::CException( );

			unsigned int len = 0;
			*this >> len;
			unsigned int progress = *mpSize - mPos;
			if ( len > progress )
				throw CNSMarshal::CException( len, progress );

			NSFunction::removeConst( octets ).replace( (char*) mpBase + mPos, len );
			mPos += len;
			return *this;
		}

		inline const CNSOctetsShadow& operator >> ( const CNSString& text ) const
		{
			if ( mpBase == NULL || mpSize == NULL )
				throw CNSMarshal::CException( );

			unsigned short len = 0;
			operator >> ( len );
			NSFunction::removeConst( text ).clear( );
			if ( len > *mpSize - mPos )
				throw CNSMarshal::CException( );

			char* charBuffer = (char*) ( (char*) mpBase + mPos );
			NSFunction::removeConst( text ).pushback( charBuffer, len );
			mPos += len;
			return *this;
		}

		const CNSOctetsShadow& operator >> ( ETransaction trans ) const
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
                    break;
			}
			return *this;
		}

	public:
		inline CNSOctets compress( ) const
		{
			return CNSOctets::compress( begin( ), end( ) );
		}

		inline CNSOctets uncompress( ) const
		{
			return CNSOctets::uncompress( begin( ), end( ) );
		}

		inline void clear( )
		{
			mpSize = NULL;
			mpBase = NULL;
			mPos = 0;
			mTranPos = 0;
		}

		inline unsigned int length( ) const
		{
			if ( mpSize == NULL )
				throw CNSMarshal::CException( );

			return *mpSize;
		}

		inline void* begin( )
		{
			if ( mpBase == NULL )
				throw CNSMarshal::CException( );

			return mpBase;
		}

		inline void* end( )
		{
			if ( mpBase == NULL || mpSize == NULL )
				throw CNSMarshal::CException( );

			return (char*) mpBase + *mpSize;
		}

		inline const void* begin( ) const
		{
			if ( mpBase == NULL )
				throw CNSMarshal::CException( );

			return mpBase;
		}

		inline void* end( ) const
		{
			if ( mpBase == NULL || mpSize == NULL )
				throw CNSMarshal::CException( );

			return (char*) mpBase + *mpSize;
		}

		template< typename T > inline const CNSOctetsShadow& unmarshalToPointer( const CNSVector< T* >& pointer ) const
		{
			if ( mpBase == NULL || mpSize == NULL )
				throw CNSMarshal::CException( );

			NSFunction::removeConst( pointer ).clear( );
			unsigned int* len = NULL;
			unmarshalToPointer( len );
			for ( unsigned int i = 0; i < *len; i ++ )
			{
				T* data = NULL;
				unmarshalToPointer( data );
				NSFunction::removeConst( pointer ).pushback( data );
			}

			return *this;
		}

		template< typename T > inline const CNSOctetsShadow& unmarshalToPointer( T*& pointer ) const
		{
			if ( mpBase == NULL || mpSize == NULL )
				throw CNSMarshal::CException( );

			if ( mPos + sizeof( T ) > *mpSize )
				throw CNSMarshal::CException( );

			pointer = (T*) ( (char*) mpBase + mPos );
			mPos += sizeof( T );
			return *this;
		}

		inline const CNSOctetsShadow& unmarshalToPointer( const CNSOctetsShadow& buffer ) const
		{
			if ( mpBase == NULL || mpSize == NULL )
				throw CNSMarshal::CException( );

			NSFunction::removeConst( buffer ).clear( );
			unmarshalToPointer( NSFunction::removeConst( buffer ).mpSize );
			if ( mPos + *( NSFunction::removeConst( buffer ).mpSize ) > *mpSize )
				throw CNSMarshal::CException( );

			NSFunction::removeConst( buffer ).mpBase = (void*) ( (char*) mpBase + mPos );
			mPos += *NSFunction::removeConst( buffer ).mpSize;
			return *this;
		}
	};

	// 只支持32位长度
	class CNSOctetsStream
	{
	public:
		enum EStreamType
		{
			EStreamNet,
			EStreamMemory,
		};

	public:
		CNSOctets				mBuffer;
		mutable unsigned int	mPos;
		mutable unsigned int	mTranPos;
		mutable EStreamType		mStreamType;
		unsigned int			mMaxSpare;

	public:
		CNSOctetsStream( EStreamType streamType = EStreamMemory, unsigned int maxSpare = 16384 );
		CNSOctetsStream( const CNSOctets octets, EStreamType streamType = EStreamMemory, unsigned int maxSpare = 16384 );
		CNSOctetsStream( const CNSOctetsStream& stream );

	public:
		CNSOctetsStream& operator = ( const CNSOctetsStream& stream );
		bool operator == ( const CNSOctetsStream& stream ) const;
		bool operator != ( const CNSOctetsStream& stream ) const;

	public:
		unsigned int length( ) const;
		void swap( CNSOctetsStream& stream );
		operator CNSOctets& ( );
		void* begin( );
		void* end( );
		void reserve( unsigned int size );
		const void* begin( ) const;
		const void *end( ) const;
		void insert( void* pos, const void* begin, unsigned int size );
		void insert( void* pos, const void* begin, const void* end );
		void erase( void* begin, void* end );
		void clear( );
		template< typename T > CNSOctetsStream& repBytes( unsigned int pos, T value )
		{
			if ( pos + sizeof( T ) > mBuffer.length( ) )
				throw CNSMarshal::CException( );

			NSFunction::memcpy_fast( (char*) mBuffer.begin( ) + pos, &value, sizeof( T ) );
			return *this;
		}

		template< typename T > CNSOctetsStream& pushBytes( T value )
		{
			mBuffer.insert( mBuffer.end( ), &value, sizeof( T ) );
			return *this;
		}

		template< typename T > T popBytes( ) const;
		bool isEnd( ) const;

		// 打包接口
		CNSOctetsStream& operator << ( const bool value );
		CNSOctetsStream& operator << ( const unsigned char value );
		CNSOctetsStream& operator << ( const char value );
		CNSOctetsStream& operator << ( const unsigned short value );
		CNSOctetsStream& operator << ( const short value );
		CNSOctetsStream& operator << ( const unsigned int value );
		CNSOctetsStream& operator << ( const int value );
		CNSOctetsStream& operator << ( const unsigned long long value );
		CNSOctetsStream& operator << ( const long long value );
		CNSOctetsStream& operator << ( const float value );
		CNSOctetsStream& operator << ( const double value );
		CNSOctetsStream& operator << ( const CNSOctets octets );
		CNSOctetsStream& operator << ( const CNSMarshal& marshal );
		CNSOctetsStream& operator << ( const CNSString& text );

		template< typename T1, typename T2 > CNSOctetsStream& operator << ( const CNSPair< T1, T2 >& pair )
		{
			operator << ( pair.mFirst );
			operator << ( pair.mSecond );
			return *this;
		}

		template< typename T > CNSOctetsStream& operator << ( const CNSVector< T >& vector )
		{
			operator << ( (unsigned int) vector.getCount( ) );
			for ( unsigned int i = 0; i < vector.getCount( ); i ++ )
				operator << ( vector[ i ] );
			return *this;
		}

		template< typename T > CNSOctetsStream& operator << ( const CNSList< T >& list )
		{
			operator << ( (unsigned int) list.getCount( ) );
			HLISTINDEX beginIndex = list.getHead( );
			for ( ; beginIndex != NULL; list.getNext( beginIndex ) )
				operator << ( list.getData( beginIndex ) );

			return *this;
		}

		template< typename Key, typename T > CNSOctetsStream& operator << ( const CNSMap< Key, T >& map )
		{
			operator << ( (unsigned int) map.getCount( ) );
			HLISTINDEX beginIndex = map.getHead( );
			for ( ; beginIndex != NULL; map.getNext( beginIndex ) )
				operator << ( CNSPair< Key, T >( map.getKey( beginIndex ), map.getValue( beginIndex ) ) );

			return *this;
		}

		template< typename Key > CNSOctetsStream& operator << ( const CNSSet< Key >& set )
		{
			operator << ( (unsigned int) set.getCount( ) );
			HLISTINDEX beginIndex = set.getHead( );
			for ( ; beginIndex != NULL; set.getNext( beginIndex ) )
				operator << ( set.getKey( beginIndex ) );

			return *this;
		}

		template< typename Key, typename T > CNSOctetsStream& operator << ( const CNSHashMap< Key, T >& hashMap )
		{
			operator << ( (unsigned int) hashMap.getCount( ) );
			HLISTINDEX beginIndex = hashMap.getHead( );
			for ( ; beginIndex != NULL; hashMap.rHashMap( beginIndex ) )
				operator << ( CNSPair< Key, T >( hashMap.getKey( beginIndex ), hashMap.getValue( beginIndex ) ) );

			return *this;
		}

		CNSOctetsStream& compactUInt( unsigned int value );
		CNSOctetsStream& compactSInt( int value );

		template< typename T > const CNSOctetsStream& unmarshalToPointer( T*& pointer ) const
		{
			if ( mPos + sizeof( T ) > mBuffer.length( ) )
				throw CNSMarshal::CException( );

			pointer = (T*) ( (char*) mBuffer.begin( ) + mPos );
			mPos += sizeof( T );
			return *this;
		}

		const CNSOctetsStream& unmarshalToPointer( const CNSOctetsShadow& buffer ) const
		{
			unmarshalToPointer( (unsigned int*&) NSFunction::removeConst( buffer ).mpSize );
			if ( mPos + *( NSFunction::removeConst( buffer ).mpSize ) > mBuffer.length( ) )
				throw CNSMarshal::CException( );

			NSFunction::removeConst( buffer ).mpBase = (void*) ( (char*) mBuffer.begin( ) + mPos );
			mPos += *NSFunction::removeConst( buffer ).mpSize;
			return *this;
		}

		// 解包接口
		const CNSOctetsStream& operator >> ( const unsigned char& value ) const;
		const CNSOctetsStream& operator >> ( const bool& value ) const;
		const CNSOctetsStream& operator >> ( const char& value ) const;
		const CNSOctetsStream& operator >> ( const unsigned short& value ) const;
		const CNSOctetsStream& operator >> ( const short& value ) const;
		const CNSOctetsStream& operator >> ( const unsigned int& value ) const;
		const CNSOctetsStream& operator >> ( const int& value ) const;
		const CNSOctetsStream& operator >> ( const unsigned long long& value ) const;
		const CNSOctetsStream& operator >> ( const long long& value ) const;
		const CNSOctetsStream& operator >> ( const float& value ) const;
		const CNSOctetsStream& operator >> ( const double& value ) const;
		const CNSOctetsStream& operator >> ( const CNSOctets& octets ) const;
		const CNSOctetsStream& operator >> ( const CNSMarshal& marshal ) const;
		const CNSOctetsStream& operator >> ( const CNSString& text ) const;
		const CNSOctetsStream& operator >> ( ETransaction trans ) const;

		template< typename T1, typename T2 > const CNSOctetsStream& operator >> ( const CNSPair< T1, T2 >& pair ) const
		{
			operator >> ( pair.mFirst );
			operator >> ( pair.mSecond );
			return *this;
		}

		template< typename T > const CNSOctetsStream& operator >> ( const CNSVector< T >& vector ) const
		{
			unsigned int len = 0;
			NSFunction::removeConst( vector ).clear( );
			for ( operator >> ( len ); len > 0; len -- )
			{
				T value;
				operator >> ( value );
				NSFunction::removeConst( vector ).pushback( value );
			}

			return *this;
		}

		template< typename T > const CNSOctetsStream& operator >> ( const CNSList< T >& list ) const
		{
			unsigned int len = 0;
			NSFunction::removeConst( list ).clear( );
			for ( operator >> ( len ); len > 0; len -- )
			{
				T value;
				operator >> ( value );
				NSFunction::removeConst( list ).pushback( value );
			}

			return *this;
		}

		template< typename Key, typename T > const CNSOctetsStream& operator >> ( const CNSMap< Key, T >& map ) const
		{
			unsigned int len = 0;
			NSFunction::removeConst( map ).clear( );
			for ( operator >> ( len ); len > 0; len -- )
			{
				CNSPair< Key, T > value;
				operator >> ( value );
				NSFunction::removeConst( map ).insert( value.mFirst, value.mSecond );
			}

			return *this;
		}

		template< typename Key > const CNSOctetsStream& operator >> ( const CNSSet< Key >& set ) const
		{
			unsigned int len = 0;
			NSFunction::removeConst( set ).clear( );
			for ( operator >> ( len ); len > 0; len -- )
			{
				Key value;
				operator >> ( value );
				NSFunction::removeConst( set ).insert( value );
			}

			return *this;
		}

		template< typename Key, typename T > const CNSOctetsStream& operator >> ( const CNSHashMap< Key, T >& hashMap ) const
		{
			unsigned int len = 0;
			NSFunction::removeConst( hashMap ).clear( );
			for ( operator >> ( len ); len > 0; len -- )
			{
				CNSPair< Key, T > value;
				operator >> ( value );
				NSFunction::removeConst( hashMap ).insert( value.mFirst, value.mSecond );
			}

			return *this;
		}

		const CNSOctetsStream& uncompactUInt( const unsigned int& value ) const;
		const CNSOctetsStream& uncompactSInt( const int& value ) const;
	};

};
