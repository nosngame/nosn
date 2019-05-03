#pragma once

namespace NSBase
{
	class CNSException
	{
	public:
		char mErrorDesc[ 2048 ];

	public:
		CNSException( const char* errorDesc )
		{
			if ( errorDesc == NULL )
				return;

			// 异常描述只保留127个字符，最后一个字符作为终止符
			unsigned int len = min( strlen( errorDesc ), 2047 );
			NSFunction::memcpy_fast( mErrorDesc, errorDesc, len );
			mErrorDesc[ len ] = 0;
		}
	public:
		virtual ~CNSException( )
		{
		}
	};

	class CNSCallCounter
	{
	public:
		static unsigned int mCounter[ 8 ];
		static unsigned int mTickCounter[ 8 ];

	public:
		CNSCallCounter( int index, unsigned int duration, const char* name );
	};

	class CAllocator
	{
	public:
		inline static void* alloc( const char* pFileName, const int vLineNumber, size_t size )
		{
#if SHOW_MEM_ALLOC == 1
			CNSCallCounter allocCounter( 0, SHOW_TICK, "fbbase memory alloc" );
#endif
			void* p = ::malloc( size );
			return p;
		}

		inline static void free( void *p )
		{
#if SHOW_MEM_ALLOC == 1
			CNSCallCounter allocCounter( 1, SHOW_TICK, "fbbase memory free" );
#endif
			::free( p );
		}
	};

	// *******************************************************************	//
	// CNSPair																//
	// *******************************************************************	//

	template < typename FIRST, typename SECOND >
	class CNSPair
	{
	public:
		FIRST	mFirst;
		SECOND	mSecond;

	public:
		CNSPair( )
		{
		}

	public:
		CNSPair( const FIRST& rFirst, const SECOND& rSecond ) : mFirst( rFirst ), mSecond( rSecond )
		{
		}
	};

	// *******************************************************************	//
	// CNSVector															//
	// *******************************************************************	//
	template< typename T, typename Allocator >
	class CNSVector
	{
	protected:
		T*				mpData;
		unsigned int	mSize;
		unsigned int	mCount;

	public:
		CNSVector( unsigned int size = 16 ) : mCount( 0 ), mSize( size )
		{
			mpData = (T*) Allocator::alloc( __FILE__, __LINE__, sizeof( T ) * mSize );
		}

		CNSVector( const CNSVector& v )
		{
			if ( this == &v )
				return;

			mCount = v.mCount;
			mSize = v.mSize;
			mpData = (T*) Allocator::alloc( __FILE__, __LINE__, sizeof( T ) * v.mSize );
			for ( unsigned int i = 0; i < mCount; i ++ )
				new( (void*) ( &mpData[ i ] ) ) T( v.mpData[ i ] );
		}

	public:
		~CNSVector( )
		{
			for ( unsigned int i = 0; i < mCount; i ++ )
				mpData[ i ].~T( );
			Allocator::free( (void*) mpData );
		}

	public:
		// 清除所有节点
		void clear( )
		{
			for ( unsigned int i = 0; i < mCount; i ++ )
				mpData[ i ].~T( );

			mCount = 0;
		}

		// 删除指定索引的节点
		void erase( unsigned int index )
		{
			if ( index >= mCount )
				NSException( _UTF8( "超出CNSVector数组范围" ) );

			// 析构删除对象
			for ( unsigned int i = index; i < mCount - 1; i ++ )
				mpData[ i ] = mpData[ i + 1 ];
			mpData[ mCount - 1 ].~T( );
			mCount --;
		}

		// 插入节点
		T* insert( unsigned int index, const T& value )
		{
			if ( index > mCount )
				NSException( _UTF8( "超出CNSVector数组范围" ) );

			if ( mCount == mSize )
			{
				mSize = mSize << 1;
				T* tpTemp = (T*) Allocator::alloc( __FILE__, __LINE__, sizeof( T ) * mSize );
				for ( unsigned int i = 0; i < mCount; i ++ )
				{
					new ( &tpTemp[ i ] ) T( mpData[ i ] );
					mpData[ i ].~T( );
				}

				Allocator::free( mpData );
				mpData = tpTemp;
			}

			for ( int i = (int) mCount - 1; i >= (int) index; i -- )
			{
				if ( i + 1 == mCount )
					new ( &mpData[ i + 1 ] ) T( mpData[ i ] );
				else
					mpData[ i + 1 ] = mpData[ i ];
			}

			if ( index == mCount )
				new ( &mpData[ index ] ) T( value );
			else
				mpData[ index ] = value;

			mCount ++;
			return &mpData[ index ];
		}

		// 添加节点
		T* pushback( const T& rValue )
		{
			if ( mCount == mSize )
			{
				mSize = mSize << 1;
				T* tpTemp = (T*) Allocator::alloc( __FILE__, __LINE__, sizeof( T ) * mSize );
				for ( unsigned int i = 0; i < mCount; i ++ )
				{
					new ( (void*) &tpTemp[ i ] ) T( mpData[ i ] );
					mpData[ i ].~T( );
				}

				Allocator::free( (void*) mpData );
				mpData = tpTemp;
			}

			new( (void*) &mpData[ mCount ] )T( rValue );
			mCount ++;
			return &mpData[ mCount - 1 ];
		}

		// 判断是否为空数组
		bool isEmpty( ) const
		{
			return mCount == 0;
		}

		// 得到缓冲区大小
		unsigned int getSize( ) const { return mSize; }

		// 得到节点个数
		unsigned int getCount( ) const { return mCount; }

		// 得到节点
		T& operator [] ( unsigned int index )
		{
			if ( index >= mCount )
				NSException( _UTF8( "超出CNSVector数组范围" ) );

			return mpData[ index ];
		}

		// 得到节点
		const T& operator [] ( unsigned int index ) const
		{
			if ( index >= mCount )
				NSException( _UTF8( "超出CNSVector数组范围" ) );

			return mpData[ index ];
		}

		// 赋值运算符
		CNSVector& operator = ( const CNSVector& v )
		{
			if ( this == &v )
				return *this;

			for ( unsigned int i = 0; i < mCount; i ++ )
				mpData[ i ].~T( );

			if ( mSize < v.mSize )
			{
				Allocator::free( (void*) mpData );
				mpData = (T*) Allocator::alloc( __FILE__, __LINE__, sizeof( T ) * v.mSize );
			}

			mCount = v.mCount;
			mSize = v.mSize;
			for ( unsigned int i = 0; i < mCount; i ++ )
				new( (void*) &mpData[ i ] ) T( v.mpData[ i ] );
			return *this;
		}
	};

	// *******************************************************************	//
	// CNSBinaryVector														//
	// *******************************************************************	//
	template< typename T, typename Allocator >
	class CNSBinaryVector
	{
	protected:
		T*				mpData;
		unsigned int	mSize;
		unsigned int	mCount;
		bool			mKeepUnique;

	public:
		CNSBinaryVector( unsigned int len = 16, bool keepUnique = true ) : mCount( 0 ), mSize( len ), mKeepUnique( keepUnique )
		{
			mpData = (T*) Allocator::alloc( __FILE__, __LINE__, sizeof( T ) * mSize );
		}

		CNSBinaryVector( const CNSBinaryVector& vec )
		{
			if ( this == &vec )
				return;

			mCount = vec.mCount;
			mSize = vec.mSize;
			mpData = (T*) Allocator::alloc( __FILE__, __LINE__, sizeof( T ) * vec.mSize );
			for ( unsigned int i = 0; i < mCount; i ++ )
				new( (void*) ( &mpData[ i ] ) ) T( vec.mpData[ i ] );
		}

	public:
		~CNSBinaryVector( )
		{
			for ( unsigned int i = 0; i < mCount; i ++ )
				mpData[ i ].~T( );
			Allocator::free( (void*) mpData );
		}

	protected:
		// 唯一数组如果发生冲突返回-1, 如果未发生冲突，返回可以插入的位置
		// 非唯一数组如果发生冲突等价于Find，如果未发生冲突，返回可以插入的位置
		int findIndex( const T& value ) const
		{
			if ( mCount == 0 )
				return 0;

			int leftIndex = 0;
			int rightIndex = mCount - 1;
			int count = 0;
			for ( ; 1; )
			{
				int halfIndex = leftIndex + ( ( rightIndex - leftIndex + 1 ) >> 1 );
				T& tempValue = mpData[ halfIndex ];
				if ( tempValue > value )
				{
					if ( leftIndex == rightIndex )
						return halfIndex;

					rightIndex = halfIndex - 1;
				}
				else if ( tempValue < value )
				{
					if ( leftIndex == rightIndex )
						return halfIndex + 1;

					leftIndex = halfIndex;
				}
				else
				{
					if ( mKeepUnique == true )
						return -1;

					return halfIndex;
				}
			}

			return 0;
		}

	public:
		// 清除所有节点
		void clear( )
		{
			for ( unsigned int i = 0; i < mCount; i ++ )
				mpData[ i ].~T( );

			mCount = 0;
		}

		bool find( const T& value ) const
		{
			if ( mCount == 0 )
				return false;

			int leftIndex = 0;
			int rightIndex = mCount - 1;
			for ( ; 1; )
			{
				int halfIndex = leftIndex + ( ( rightIndex - leftIndex + 1 ) >> 1 );
				T& tempValue = mpData[ halfIndex ];
				if ( tempValue > value )
				{
					if ( leftIndex == rightIndex )
						return false;

					rightIndex = halfIndex - 1;
				}
				else if ( tempValue < value )
				{
					if ( leftIndex == rightIndex )
						return false;

					leftIndex = halfIndex;
				}
				else
					return true;
			}

			return false;
		}

		// 删除指定索引的节点
		void erase( const T& value )
		{
			int index = find( value );
			if ( index == -1 )
				return;

			// 析构删除对象
			for ( unsigned int i = index; i < mCount - 1; i ++ )
				mpData[ i ] = mpData[ i + 1 ];
			mpData[ mCount - 1 ].~T( );
			mCount --;
		}

		// 插入节点
		void insert( const T& value )
		{
			if ( mCount == mSize )
			{
				mSize = mSize << 1;
				T* tpTemp = (T*) Allocator::alloc( __FILE__, __LINE__, sizeof( T ) * mSize );
				for ( unsigned int i = 0; i < mCount; i ++ )
				{
					new ( &tpTemp[ i ] ) T( mpData[ i ] );
					mpData[ i ].~T( );
				}

				Allocator::free( mpData );
				mpData = tpTemp;
			}

			int index = findIndex( value );
			// 如果该值已经存在，那么什么也不做
			if ( index == -1 )
				return;

			for ( int i = (int) mCount - 1; i >= (int) index; i -- )
			{
				if ( i + 1 == mCount )
					new ( &mpData[ i + 1 ] ) T( mpData[ i ] );
				else
					mpData[ i + 1 ] = mpData[ i ];
			}

			if ( index == mCount )
				new ( &mpData[ index ] ) T( value );
			else
				mpData[ index ] = value;
			mCount ++;
		}

		// 判断是否为空数组
		inline bool isEmpty( ) const
		{
			return mCount == 0;
		}

		// 得到缓冲区大小
		inline unsigned int getSize( ) const { return mSize; }

		// 得到节点个数
		inline unsigned int getCount( ) const { return mCount; }

		// 得到节点
		const T& operator [] ( unsigned int index ) const
		{
			if ( index < 0 || index >= mCount )
				NSException( _UTF8( "超出CNSBinaryVector数组范围" ) );

			return mpData[ index ];
		}

		// 赋值运算符
		CNSBinaryVector& operator = ( const CNSBinaryVector& v )
		{
			if ( this == &v )
				return *this;

			if ( mCount < v.mCount )
			{
				for ( unsigned int i = 0; i < mCount; i ++ )
					mpData[ i ].~T( );
				Allocator::free( (void*) mpData );

				mCount = v.mCount;
				mSize = v.mSize;
				mpData = (T*) Allocator::alloc( __FILE__, __LINE__, sizeof( T ) * v.mSize );
				for ( unsigned int i = 0; i < mCount; i ++ )
					new( (void*) &mpData[ i ] ) T( v.mpData[ i ] );
			}
			else
			{
				for ( unsigned int i = 0; i < mCount; i ++ )
					mpData[ i ].~T( );

				mCount = v.mCount;
				mSize = v.mSize;
				for ( unsigned int i = 0; i < mCount; i ++ )
					new( (void*) &mpData[ i ] ) T( v.mpData[ i ] );
			}
			return *this;
		}
	};

	// *******************************************************************	//
	// CNSBitPool															//
	// *******************************************************************	//
	template< typename T, typename Allocator >
	class CNSBitPool
	{
	public:
		T*				mpData;
		unsigned int	mSize;
	public:
		CNSBitPool( unsigned int vSize = 1 ) : mSize( vSize )
		{
			mpData = (T*) Allocator::alloc( __FILE__, __LINE__, sizeof( T ) * vSize );
			::memset( mpData, 0, sizeof( T ) * vSize );
		}

	public:
		// 清除所有节点
		void clear( )
		{
			::memset( mpData, 0, sizeof( T ) * mSize );
		}

		// 得到缓冲区大小字节单位
		unsigned int getSize( ) const { return mSize; }

		// 得到节点
		void setBit( unsigned int index, bool vValue )
		{
			unsigned int tIndexY = index / ( sizeof( T ) * 8 );
			unsigned int tIndexX = index % ( sizeof( T ) * 8 );
			unsigned int tSize = tIndexX > 0 ? tIndexY + 1 : tIndexY;
			if ( tSize > mSize )
			{
				T* tpData = (T*) Allocator::alloc( __FILE__, __LINE__, sizeof( T ) * tSize );
				::memset( tpData, 0, sizeof( T ) * tSize );
				NSFunction::memcpy_fast( tpData, mpData, sizeof( T ) * mSize );
				mSize = tSize;
			}

			if ( vValue == true )
				mpData[ tIndexY ] |= ( 1 << tIndexX );
			else
				mpData[ tIndexY ] &= ~( 1 << tIndexX );
		}

		// 得到节点
		const bool getBit( unsigned int index ) const
		{
			unsigned int tIndexY = index / ( sizeof( T ) * 8 );
			unsigned int tIndexX = index % ( sizeof( T ) * 8 );
			if ( tIndexY < 0 || tIndexY >= mSize )
				NSException( _UTF8( "超出CNSBitPool数组范围" ) );

			return bool( mpData[ tIndexY ] & ( 1 << tIndexX ) );
		}

		void getBits( CNSVector< unsigned int >& rBits ) const
		{
			for ( unsigned int i = 0; i < mSize; i ++ )
			{
				for ( int tIndex = 0; tIndex < 32; tIndex ++ )
				{
					if ( ( mpData[ i ] & ( 1 << tIndex ) ) != 0 )
						rBits.pushback( tIndex + i * ( sizeof( T ) * 8 ) );
				}
			}
		}
	};

	// *******************************************************************	//
	// CNSList																//
	// *******************************************************************	//
	typedef void* HLISTINDEX;

	template< typename T >
	class CNSListNode
	{
	public:
		CNSListNode*	mpNext;
		CNSListNode*	mpPrev;
		T				mData;
	public:
		CNSListNode( ) : mpNext( NULL ), mpPrev( NULL )
		{
		}
	};

	template< typename T, typename Allocator >
	class CNSList
	{
	protected:
		CNSListNode< T >*	mpHead;
		CNSListNode< T >*	mpTail;
		unsigned int		mCount;

	public:
		CNSList( ) : mpHead( NULL ), mpTail( NULL ), mCount( 0 )
		{
		}

		CNSList( const CNSList< T >& rList ) : mpHead( NULL ), mpTail( NULL ), mCount( 0 )
		{
			HLISTINDEX tBeginIndex = rList.getHead( );
			for ( ; tBeginIndex != NULL; rList.getNext( tBeginIndex ) )
				pushback( rList.getData( tBeginIndex ) );
		}

	public:
		CNSList< T >& operator = ( const CNSList< T >& rList )
		{
			if ( this == &rList )
				return *this;

			clear( );
			HLISTINDEX tBeginIndex = rList.getHead( );
			for ( ; tBeginIndex != NULL; rList.getNext( tBeginIndex ) )
				pushback( rList.getData( tBeginIndex ) );

			return *this;
		}

	public:
		~CNSList( )
		{
			clear( );
		}

	public:
		// 清除所有节点
		void clear( )
		{
			while ( mpHead != NULL )
			{
				CNSListNode< T >* tpNextNode = mpHead->mpNext;
				mpHead->~CNSListNode< T >( );
				Allocator::free( mpHead );
				mpHead = tpNextNode;
			}

			mpHead = NULL;
			mpTail = NULL;
			mCount = 0;
		}

		// 添加一个节点到末尾
		HLISTINDEX pushback( const T& rValue )
		{
			if ( mpHead == NULL )
			{
				mpHead = ( CNSListNode< T >* ) Allocator::alloc( __FILE__, __LINE__, sizeof( CNSListNode< T > ) );
				new( mpHead ) CNSListNode< T >( );
				mpTail = mpHead;
				mpTail->mData = rValue;
			}
			else
			{
				mpTail->mpNext = ( CNSListNode< T >* ) Allocator::alloc( __FILE__, __LINE__, sizeof( CNSListNode< T > ) );
				new( mpTail->mpNext ) CNSListNode< T >( );
				mpTail->mpNext->mpPrev = mpTail;
				mpTail = mpTail->mpNext;
				mpTail->mData = rValue;
			}
			mCount ++;
			return mpTail;
		}

		// 通过索引得到节点索引
		HLISTINDEX getIndex( unsigned int index )
		{
			CNSListNode< T >* tpCurr = mpHead;
			unsigned int tIndex = 0;
			while ( tpCurr != NULL )
			{
				if ( tIndex ++ == index )
					return tpCurr;

				tpCurr = tpCurr->mpNext;
			}

			return (HLISTINDEX) NULL;
		}

		// 插入一个节点
		void insertAfter( HLISTINDEX index, const T& rValue )
		{
			CNSListNode< T >* tpNewNode = ( CNSListNode< T >* ) Allocator::alloc( __FILE__, __LINE__, sizeof( CNSListNode< T > ) );
			new( tpNewNode ) CNSListNode< T >( );
			CNSListNode< T >* tpNode = ( CNSListNode< T >* ) index;
			if ( tpNode == NULL )
				return;

			tpNewNode->mpNext = tpNode->mpNext;
			tpNewNode->mpPrev = tpNode;
			tpNewNode->mData = rValue;
			tpNode->mpNext = tpNewNode;

			if ( tpNewNode->mpNext != NULL )
				tpNewNode->mpNext->mpPrev = tpNewNode;

			if ( tpNewNode->mpNext == NULL )
				mpTail = tpNewNode;

			mCount ++;
		}

		// 插入一个节点
		void insertBefore( HLISTINDEX index, const T& rValue )
		{
			CNSListNode< T >* tpNewNode = ( CNSListNode< T >* ) Allocator::alloc( __FILE__, __LINE__, sizeof( CNSListNode< T > ) );
			new( tpNewNode ) CNSListNode< T >( );
			CNSListNode< T >* tpNode = ( CNSListNode< T >* ) index;
			if ( tpNode == NULL )
				return;

			tpNewNode->mpPrev = tpNode->mpPrev;
			tpNewNode->mpNext = tpNode;
			tpNewNode->mData = rValue;
			tpNode->mpPrev = tpNewNode;

			if ( tpNewNode->mpPrev != NULL )
				tpNewNode->mpPrev->mpNext = tpNewNode;

			if ( tpNewNode->mpPrev == NULL )
				mpHead = tpNewNode;

			mCount ++;
		}

		// 通过索引删除节点
		void erase( HLISTINDEX index )
		{
			CNSListNode< T >* tpListNode = ( CNSListNode< T >* ) index;
			CNSListNode< T >* tpPrevNode = tpListNode->mpPrev;
			CNSListNode< T >* tpNextNode = tpListNode->mpNext;

			if ( mpHead == tpListNode )
				mpHead = tpListNode->mpNext;

			if ( mpTail == tpListNode )
				mpTail = tpListNode->mpPrev;

			if ( tpPrevNode != NULL )
				tpPrevNode->mpNext = tpNextNode;

			if ( tpNextNode != NULL )
				tpNextNode->mpPrev = tpPrevNode;

			// 调用析构函数
			tpListNode->~CNSListNode< T >( );

			// 释放内存
			Allocator::free( tpListNode );

			mCount --;
		}

		// 得到下一个节点
		void getNext( HLISTINDEX& index ) const { index = ( ( CNSListNode< T >* ) index )->mpNext; }

		// 得到上一个节点
		void getPrev( HLISTINDEX& index ) const { index = ( ( CNSListNode< T >* ) index )->mpPrev; }

		// 得到节点数据
		const T& getData( HLISTINDEX rIndex ) const { return ( ( CNSListNode< T >* ) rIndex )->mData; }

		// 得到节点数据
		T& getData( HLISTINDEX rIndex ) { return ( ( CNSListNode< T >* ) rIndex )->mData; }

		// 得到头节点
		HLISTINDEX getHead( ) const { return mpHead; }

		// 得到尾节点
		HLISTINDEX getTail( ) const { return mpTail; }

		// 得到节点数量
		unsigned int getCount( ) const { return mCount; }
	};

	// *******************************************************************	//
	// CNSMap																//
	// *******************************************************************	//
	typedef void* HMAPINDEX;

	template < typename KEY, typename T >
	class CNSMapNode
	{
	public:
		enum ENodeColour
		{
			NODE_RED,
			NODE_BLACK,
		};

	public:
		KEY						mKey;			// 键值
		T						mValue;			// 数值
		CNSMapNode< KEY, T >*	mpLeft;			// 左子节点
		CNSMapNode< KEY, T >*	mpRight;		// 右子节点
		CNSMapNode< KEY, T >*	mpParent;		// 父节点
		ENodeColour				mNodeColour;	// 节点颜色

	public:
		CNSMapNode( ) : mpLeft( NULL ), mpRight( NULL ), mpParent( NULL ), mNodeColour( NODE_BLACK )
		{
		}

		ENodeColour getColor( ) const { return mNodeColour; }
	};

	template < typename KEY, typename T, typename Allocator >
	class CNSMap
	{
	public:
		CNSMapNode< KEY, HLISTINDEX >			mSentinel;
		CNSMapNode< KEY, HLISTINDEX >*			mpRoot;
		CNSList< CNSPair< KEY, T > >			mList;

	public:
		CNSMap( )
		{
			mSentinel.mNodeColour = CNSMapNode< KEY, HLISTINDEX >::NODE_BLACK;
			mSentinel.mpLeft = &mSentinel;
			mSentinel.mpRight = &mSentinel;
			mSentinel.mpParent = NULL;
			mpRoot = &mSentinel;
		}

	public:
		CNSMap( const CNSMap< KEY, T >& rMap )
		{
			mSentinel.mNodeColour = CNSMapNode< KEY, HLISTINDEX >::NODE_BLACK;
			mSentinel.mpLeft = &mSentinel;
			mSentinel.mpRight = &mSentinel;
			mSentinel.mpParent = NULL;
			mpRoot = &mSentinel;

			HLISTINDEX tBeginIndex = rMap.getHead( );
			for ( ; tBeginIndex != NULL; rMap.getNext( tBeginIndex ) )
			{
				KEY tKey = rMap.getKey( tBeginIndex );
				T tValue = rMap.getValue( tBeginIndex );
				insert( tKey, tValue );
			}
		}

	public:
		CNSMap< KEY, T >& operator = ( const CNSMap< KEY, T >& rMap )
		{
			if ( this == &rMap )
				return *this;

			clear( );
			HLISTINDEX tBeginIndex = rMap.getHead( );
			for ( ; tBeginIndex != NULL; rMap.getNext( tBeginIndex ) )
			{
				KEY tKey = rMap.getKey( tBeginIndex );
				T tValue = rMap.getValue( tBeginIndex );
				insert( tKey, tValue );
			}

			return *this;
		}

	public:
		~CNSMap( )
		{
			clear( );
		}

	protected:
		// 释放指定节点的所有子节点
		void eraseTree( CNSMapNode< KEY, HLISTINDEX >* pRoot )
		{
			if ( pRoot->mpLeft != &mSentinel )
				eraseTree( pRoot->mpLeft );

			if ( pRoot->mpRight != &mSentinel )
				eraseTree( pRoot->mpRight );

			if ( pRoot != &mSentinel )
			{
				// 调用析构函数
				pRoot->~CNSMapNode< KEY, HLISTINDEX >( );

				// 释放内存
				Allocator::free( pRoot );
			}
			mList.clear( );
		}

		void rotateLeft( CNSMapNode< KEY, HLISTINDEX >* pNode )
		{
			CNSMapNode< KEY, HLISTINDEX >* tpRight = pNode->mpRight;

			pNode->mpRight = tpRight->mpLeft;
			if ( tpRight->mpLeft != &mSentinel )
				tpRight->mpLeft->mpParent = pNode;

			if ( tpRight != &mSentinel )
				tpRight->mpParent = pNode->mpParent;

			if ( pNode->mpParent )
			{
				if ( pNode == pNode->mpParent->mpLeft )
					pNode->mpParent->mpLeft = tpRight;
				else
					pNode->mpParent->mpRight = tpRight;
			}
			else
				mpRoot = tpRight;

			tpRight->mpLeft = pNode;
			if ( pNode != &mSentinel )
				pNode->mpParent = tpRight;
		}

		void rotateRight( CNSMapNode< KEY, HLISTINDEX >* pNode )
		{
			CNSMapNode< KEY, HLISTINDEX >* tpLeft = pNode->mpLeft;

			pNode->mpLeft = tpLeft->mpRight;
			if ( tpLeft->mpRight != &mSentinel )
				tpLeft->mpRight->mpParent = pNode;

			if ( tpLeft != &mSentinel )
				tpLeft->mpParent = pNode->mpParent;

			if ( pNode->mpParent )
			{
				if ( pNode == pNode->mpParent->mpRight )
					pNode->mpParent->mpRight = tpLeft;
				else
					pNode->mpParent->mpLeft = tpLeft;
			}
			else
				mpRoot = tpLeft;

			tpLeft->mpRight = pNode;
			if ( pNode != &mSentinel )
				pNode->mpParent = tpLeft;
		}

		void insertFixup( CNSMapNode< KEY, HLISTINDEX >* pNode )
		{
			while ( pNode != mpRoot && pNode->mpParent->mNodeColour == CNSMapNode< KEY, HLISTINDEX >::NODE_RED )
			{
				if ( pNode->mpParent == pNode->mpParent->mpParent->mpLeft )
				{
					CNSMapNode< KEY, HLISTINDEX >* tpUncle = pNode->mpParent->mpParent->mpRight;
					if ( tpUncle->mNodeColour == CNSMapNode< KEY, HLISTINDEX >::NODE_RED )
					{
						pNode->mpParent->mNodeColour = CNSMapNode< KEY, HLISTINDEX >::NODE_BLACK;
						tpUncle->mNodeColour = CNSMapNode< KEY, HLISTINDEX >::NODE_BLACK;
						pNode->mpParent->mpParent->mNodeColour = CNSMapNode< KEY, HLISTINDEX >::NODE_RED;
						pNode = pNode->mpParent->mpParent;
					}
					else
					{
						if ( pNode == pNode->mpParent->mpRight )
						{
							pNode = pNode->mpParent;
							rotateLeft( pNode );
						}

						pNode->mpParent->mNodeColour = CNSMapNode< KEY, HLISTINDEX >::NODE_BLACK;
						pNode->mpParent->mpParent->mNodeColour = CNSMapNode< KEY, HLISTINDEX >::NODE_RED;
						rotateRight( pNode->mpParent->mpParent );
					}
				}
				else
				{
					CNSMapNode< KEY, HLISTINDEX >* tpUncle = pNode->mpParent->mpParent->mpLeft;
					if ( tpUncle->mNodeColour == CNSMapNode< KEY, HLISTINDEX >::NODE_RED )
					{
						pNode->mpParent->mNodeColour = CNSMapNode< KEY, HLISTINDEX >::NODE_BLACK;
						tpUncle->mNodeColour = CNSMapNode< KEY, HLISTINDEX >::NODE_BLACK;
						pNode->mpParent->mpParent->mNodeColour = CNSMapNode< KEY, HLISTINDEX >::NODE_RED;
						pNode = pNode->mpParent->mpParent;
					}
					else
					{
						if ( pNode == pNode->mpParent->mpLeft )
						{
							pNode = pNode->mpParent;
							rotateRight( pNode );
						}
						pNode->mpParent->mNodeColour = CNSMapNode< KEY, HLISTINDEX >::NODE_BLACK;
						pNode->mpParent->mpParent->mNodeColour = CNSMapNode< KEY, HLISTINDEX >::NODE_RED;
						rotateLeft( pNode->mpParent->mpParent );
					}
				}
			}

			mpRoot->mNodeColour = CNSMapNode< KEY, HLISTINDEX >::NODE_BLACK;
		}

		void deleteFixup( CNSMapNode< KEY, HLISTINDEX >* pNode )
		{
			while ( pNode != mpRoot && pNode->mNodeColour == CNSMapNode< KEY, HLISTINDEX >::NODE_BLACK )
			{
				if ( pNode == pNode->mpParent->mpLeft )
				{
					CNSMapNode< KEY, HLISTINDEX >* tpUncle = pNode->mpParent->mpRight;
					if ( tpUncle->mNodeColour == CNSMapNode< KEY, HLISTINDEX >::NODE_RED )
					{
						tpUncle->mNodeColour = CNSMapNode< KEY, HLISTINDEX >::NODE_BLACK;
						tpUncle->mpParent->mNodeColour = CNSMapNode< KEY, HLISTINDEX >::NODE_RED;
						rotateLeft( pNode->mpParent );
						tpUncle = pNode->mpParent->mpRight;
					}

					if ( tpUncle->mpLeft->mNodeColour == CNSMapNode< KEY, HLISTINDEX >::NODE_BLACK && tpUncle->mpRight->mNodeColour == CNSMapNode< KEY, HLISTINDEX >::NODE_BLACK )
					{
						tpUncle->mNodeColour = CNSMapNode< KEY, HLISTINDEX >::NODE_RED;
						pNode = pNode->mpParent;
					}
					else
					{
						if ( tpUncle->mpRight->mNodeColour == CNSMapNode< KEY, HLISTINDEX >::NODE_BLACK )
						{
							tpUncle->mpLeft->mNodeColour = CNSMapNode< KEY, HLISTINDEX >::NODE_BLACK;
							tpUncle->mNodeColour = CNSMapNode< KEY, HLISTINDEX >::NODE_RED;
							rotateRight( tpUncle );
							tpUncle = pNode->mpParent->mpRight;
						}
						tpUncle->mNodeColour = pNode->mpParent->mNodeColour;
						pNode->mpParent->mNodeColour = CNSMapNode< KEY, HLISTINDEX >::NODE_BLACK;
						tpUncle->mpRight->mNodeColour = CNSMapNode< KEY, HLISTINDEX >::NODE_BLACK;
						rotateLeft( pNode->mpParent );
						pNode = mpRoot;
					}
				}
				else
				{
					CNSMapNode< KEY, HLISTINDEX >* tpUncle = pNode->mpParent->mpLeft;
					if ( tpUncle->mNodeColour == CNSMapNode< KEY, HLISTINDEX >::NODE_RED )
					{
						tpUncle->mNodeColour = CNSMapNode< KEY, HLISTINDEX >::NODE_BLACK;
						pNode->mpParent->mNodeColour = CNSMapNode< KEY, HLISTINDEX >::NODE_RED;
						rotateRight( pNode->mpParent );
						tpUncle = pNode->mpParent->mpLeft;
					}
					if ( tpUncle->mpRight->mNodeColour == CNSMapNode< KEY, HLISTINDEX >::NODE_BLACK && tpUncle->mpLeft->mNodeColour == CNSMapNode< KEY, HLISTINDEX >::NODE_BLACK )
					{
						tpUncle->mNodeColour = CNSMapNode< KEY, HLISTINDEX >::NODE_RED;
						pNode = pNode->mpParent;
					}
					else
					{
						if ( tpUncle->mpLeft->mNodeColour == CNSMapNode< KEY, HLISTINDEX >::NODE_BLACK )
						{
							tpUncle->mpRight->mNodeColour = CNSMapNode< KEY, HLISTINDEX >::NODE_BLACK;
							tpUncle->mNodeColour = CNSMapNode< KEY, HLISTINDEX >::NODE_RED;
							rotateLeft( tpUncle );
							tpUncle = pNode->mpParent->mpLeft;
						}
						tpUncle->mNodeColour = pNode->mpParent->mNodeColour;
						pNode->mpParent->mNodeColour = CNSMapNode< KEY, HLISTINDEX >::NODE_BLACK;
						tpUncle->mpLeft->mNodeColour = CNSMapNode< KEY, HLISTINDEX >::NODE_BLACK;
						rotateRight( pNode->mpParent );
						pNode = mpRoot;
					}
				}
			}

			pNode->mNodeColour = CNSMapNode< KEY, HLISTINDEX >::NODE_BLACK;
		}

	public:
		void clear( )
		{
			if ( mpRoot != &mSentinel )
			{
				eraseTree( mpRoot );
				mpRoot = &mSentinel;
			}
			mList.clear( );
		}

		void erase( const KEY& rKey )
		{
			CNSMapNode< KEY, HLISTINDEX >* tpEraseNode = ( CNSMapNode< KEY, HLISTINDEX >* ) findNode( rKey );
			if ( tpEraseNode == NULL )
				return;

			eraseNode( tpEraseNode );
		}

		void eraseNode( HMAPINDEX vNodeIndex )
		{
			CNSMapNode< KEY, HLISTINDEX >* tpNode = ( CNSMapNode< KEY, HLISTINDEX >* ) vNodeIndex;
			CNSMapNode< KEY, HLISTINDEX >* tpTempNode1 = NULL;
			CNSMapNode< KEY, HLISTINDEX >* tpTempNode2 = NULL;

			if ( tpNode->mpLeft == &mSentinel || tpNode->mpRight == &mSentinel )
				tpTempNode2 = tpNode;
			else
			{
				tpTempNode2 = tpNode->mpRight;
				while ( tpTempNode2->mpLeft != &mSentinel )
					tpTempNode2 = tpTempNode2->mpLeft;
			}

			if ( tpTempNode2->mpLeft != &mSentinel )
				tpTempNode1 = tpTempNode2->mpLeft;
			else
				tpTempNode1 = tpTempNode2->mpRight;

			tpTempNode1->mpParent = tpTempNode2->mpParent;
			if ( tpTempNode2->mpParent != NULL )
			{
				if ( tpTempNode2 == tpTempNode2->mpParent->mpLeft )
					tpTempNode2->mpParent->mpLeft = tpTempNode1;
				else
					tpTempNode2->mpParent->mpRight = tpTempNode1;
			}
			else
				mpRoot = tpTempNode1;

			HLISTINDEX tLastIndex = tpNode->mValue;
			if ( tpTempNode2 != tpNode )
			{
				tpNode->mKey = tpTempNode2->mKey;
				tpNode->mValue = tpTempNode2->mValue;
			}

			if ( tpTempNode2->mNodeColour == CNSMapNode< KEY, HLISTINDEX >::NODE_BLACK )
				deleteFixup( tpTempNode1 );

			mList.erase( tLastIndex );
			tpTempNode2->~CNSMapNode< KEY, HLISTINDEX >( );
			Allocator::free( tpTempNode2 );
		}

		const T* get( const KEY& rKey ) const
		{
			CNSMapNode< KEY, HLISTINDEX >* tpCurr = mpRoot;
			while ( tpCurr != &mSentinel )
			{
				if ( rKey == tpCurr->mKey )
				{
					if ( tpCurr->mValue == NULL )
						return NULL;

					return &getValue( tpCurr->mValue );
				}
				else
					tpCurr = rKey < tpCurr->mKey ? tpCurr->mpLeft : tpCurr->mpRight;
			}

			return NULL;
		}

		T* get( const KEY& rKey )
		{
			CNSMapNode< KEY, HLISTINDEX >* tpCurr = mpRoot;
			while ( tpCurr != &mSentinel )
			{
				if ( rKey == tpCurr->mKey )
				{
					if ( tpCurr->mValue == NULL )
						return NULL;

					return &getValue( tpCurr->mValue );
				}
				else
					tpCurr = rKey < tpCurr->mKey ? tpCurr->mpLeft : tpCurr->mpRight;
			}

			return NULL;
		}

		HMAPINDEX findNode( const KEY& rKey ) const
		{
			CNSMapNode< KEY, HLISTINDEX >* tpCurr = mpRoot;
			while ( tpCurr != &mSentinel )
			{
				if ( rKey == tpCurr->mKey )
					return tpCurr;
				else
					tpCurr = rKey < tpCurr->mKey ? tpCurr->mpLeft : tpCurr->mpRight;
			}

			return NULL;
		}

		HLISTINDEX findNodeIndex( const KEY& rKey ) const
		{
			CNSMapNode< KEY, HLISTINDEX >* tpCurr = mpRoot;
			while ( tpCurr != &mSentinel )
			{
				if ( rKey == tpCurr->mKey )
					return tpCurr->mValue;
				else
					tpCurr = rKey < tpCurr->mKey ? tpCurr->mpLeft : tpCurr->mpRight;
			}

			return NULL;
		}

		bool find( const KEY& rKey, T& rValue )
		{
			CNSMapNode< KEY, HLISTINDEX >* tpCurr = mpRoot;
			while ( tpCurr != &mSentinel )
			{
				if ( rKey == tpCurr->mKey )
				{
					rValue = mList.getData( tpCurr->mValue ).mSecond;
					return true;
				}
				else
					tpCurr = rKey < tpCurr->mKey ? tpCurr->mpLeft : tpCurr->mpRight;
			}

			return false;
		}

		T& insert( const KEY& rKey, const T& rValue )
		{
			CNSMapNode< KEY, HLISTINDEX >* tpCurr;
			CNSMapNode< KEY, HLISTINDEX >* tpParent;
			tpCurr = mpRoot;
			tpParent = NULL;
			while ( tpCurr != &mSentinel )
			{
				if ( rKey == tpCurr->mKey )
					return getValueEx( tpCurr ) = rValue;

				tpParent = tpCurr;
				tpCurr = rKey < tpCurr->mKey ? tpCurr->mpLeft : tpCurr->mpRight;
			}

			CNSMapNode< KEY, HLISTINDEX >* tpNewNode = ( CNSMapNode< KEY, HLISTINDEX >* ) Allocator::alloc( __FILE__, __LINE__, sizeof( CNSMapNode< KEY, HLISTINDEX > ) );
			new ( tpNewNode ) CNSMapNode< KEY, HLISTINDEX >( );
			tpNewNode->mpParent = tpParent;
			tpNewNode->mpLeft = &mSentinel;
			tpNewNode->mpRight = &mSentinel;
			tpNewNode->mNodeColour = CNSMapNode< KEY, HLISTINDEX >::NODE_RED;
			tpNewNode->mKey = rKey;
			tpNewNode->mValue = mList.pushback( CNSPair< KEY, T >( rKey, rValue ) );

			if ( tpParent )
			{
				if ( rKey < tpParent->mKey )
					tpParent->mpLeft = tpNewNode;
				else
					tpParent->mpRight = tpNewNode;
			}
			else
				mpRoot = tpNewNode;

			insertFixup( tpNewNode );
			return getValueEx( tpNewNode );
		}

		// 是否为空Map
		bool isEmpty( ) const { return getCount( ) == 0; }

		// 得到节点数量
		unsigned int getCount( ) const { return mList.getCount( ); }

		// 得到第一个添加的节点
		HLISTINDEX getHead( ) const { return mList.getHead( ); }

		// 得到最后一个添加的节点
		HLISTINDEX getTail( ) const { return mList.getTail( ); }

		// 得到下一个节点
		void getNext( HLISTINDEX& index ) const { index = ( ( CNSListNode< CNSPair< KEY, T > >* ) index )->mpNext; }

		// 得到上一个节点
		void getPrev( HLISTINDEX& index ) const { index = ( ( CNSListNode< CNSPair< KEY, T > >* ) index )->mpPrev; }

		// 根据节点索引得到节点的数值
		KEY& getKey( HLISTINDEX index )
		{
			if ( index == NULL )
				NSException( _UTF8( "Map索引无效" ) );

			CNSListNode< CNSPair< KEY, T > >* tpListNode = ( CNSListNode< CNSPair< KEY, T > >* ) index;
			return tpListNode->mData.mFirst;
		}

		// 根据节点索引得到节点的数值
		const KEY& getKey( HLISTINDEX index ) const
		{
			if ( index == NULL )
				NSException( _UTF8( "Map索引无效" ) );

			CNSListNode< CNSPair< KEY, T > >* tpListNode = ( CNSListNode< CNSPair< KEY, T > >* ) index;
			return tpListNode->mData.mFirst;
		}

		// 根据节点索引得到节点的数值
		KEY& getKeyEx( HMAPINDEX index )
		{
			if ( index == NULL )
				NSException( _UTF8( "Map索引无效" ) );

			CNSMapNode< KEY, HLISTINDEX >* tpMapNode = ( CNSMapNode< KEY, HLISTINDEX >* ) index;
			return tpMapNode->mKey;
		}

		// 根据节点索引得到节点的数值
		const KEY& getKeyEx( HMAPINDEX index ) const
		{
			if ( index == NULL )
				NSException( _UTF8( "Map索引无效" ) );

			CNSMapNode< KEY, HLISTINDEX >* tpMapNode = ( CNSMapNode< KEY, HLISTINDEX >* ) index;
			return tpMapNode->mKey;
		}

		// 根据节点索引得到节点的数值
		T& getValue( HLISTINDEX index )
		{
			if ( index == NULL )
				NSException( _UTF8( "Map索引无效" ) );

			return mList.getData( index ).mSecond;
		}

		// 根据节点索引得到节点的数值
		const T& getValue( HLISTINDEX index ) const
		{
			if ( index == NULL )
				NSException( _UTF8( "Map索引无效" ) );

			return mList.getData( index ).mSecond;
		}

		// 根据节点索引得到节点的数值
		T& getValueEx( HMAPINDEX index )
		{
			if ( index == NULL )
				NSException( _UTF8( "Map索引无效" ) );

			CNSMapNode< KEY, HLISTINDEX >* tpMapNode = ( CNSMapNode< KEY, HLISTINDEX >* ) index;
			return getValue( tpMapNode->mValue );
		}

		// 根据节点索引得到节点的数值
		const T& getValueEx( HMAPINDEX index ) const
		{
			if ( index == NULL )
				NSException( _UTF8( "Map索引无效" ) );

			CNSMapNode< KEY, HLISTINDEX >* tpMapNode = ( CNSMapNode< KEY, HLISTINDEX >* ) index;
			return getValue( tpMapNode->mValue );
		}
	};

	// *******************************************************************	//
	// CNSSet																//
	// *******************************************************************	//
	template < typename KEY, typename Allocator >
	class CNSSet
	{
	public:
		CNSMapNode< KEY, HLISTINDEX >		mSentinel;
		CNSMapNode< KEY, HLISTINDEX >*		mpRoot;
		CNSList< KEY >						mList;

	public:
		CNSSet( )
		{
			mSentinel.mNodeColour = CNSMapNode< KEY, HLISTINDEX >::NODE_BLACK;
			mSentinel.mpLeft = &mSentinel;
			mSentinel.mpRight = &mSentinel;
			mSentinel.mpParent = NULL;
			mpRoot = &mSentinel;
		}

	public:
		CNSSet( const CNSSet< KEY >& rSet )
		{
			mSentinel.mNodeColour = CNSMapNode< KEY, HLISTINDEX >::NODE_BLACK;
			mSentinel.mpLeft = &mSentinel;
			mSentinel.mpRight = &mSentinel;
			mSentinel.mpParent = NULL;
			mpRoot = &mSentinel;

			HLISTINDEX tBeginIndex = rSet.getHead( );
			for ( ; tBeginIndex != NULL; rSet.getNext( tBeginIndex ) )
			{
				KEY tKey = rSet.getKey( tBeginIndex );
				insert( tKey );
			}
		}

		CNSSet( const CNSVector< KEY >& vector )
		{
			clear( );
			for ( unsigned int i = 0; i < vector.getCount( ); i ++ )
				insert( vector[ i ] );
		}

	public:
		CNSSet< KEY >& operator = ( const CNSSet< KEY >& rSet )
		{
			if ( this == &rSet )
				return *this;

			clear( );
			HLISTINDEX tBeginIndex = rSet.getHead( );
			for ( ; tBeginIndex != NULL; rSet.getNext( tBeginIndex ) )
			{
				KEY tKey = rSet.getKey( tBeginIndex );
				insert( tKey );
			}

			return *this;
		}

		CNSSet< KEY >& operator = ( const CNSVector< KEY >& vector )
		{
			clear( );
			for ( unsigned int i = 0; i < vector.getCount( ); i ++ )
				insert( vector[ i ] );

			return *this;
		}
	public:
		~CNSSet( )
		{
			clear( );
		}

	protected:
		// 释放指定节点的所有子节点
		void eraseTree( CNSMapNode< KEY, HLISTINDEX >* pRoot )
		{
			if ( pRoot->mpLeft != &mSentinel )
				eraseTree( pRoot->mpLeft );

			if ( pRoot->mpRight != &mSentinel )
				eraseTree( pRoot->mpRight );

			if ( pRoot != &mSentinel )
			{
				// 调用析构函数
				pRoot->~CNSMapNode< KEY, HLISTINDEX >( );

				// 释放内存
				Allocator::free( pRoot );
			}
			mList.clear( );
		}

		void rotateLeft( CNSMapNode< KEY, HLISTINDEX >* pNode )
		{
			CNSMapNode< KEY, HLISTINDEX >* tpRight = pNode->mpRight;

			pNode->mpRight = tpRight->mpLeft;
			if ( tpRight->mpLeft != &mSentinel )
				tpRight->mpLeft->mpParent = pNode;

			if ( tpRight != &mSentinel )
				tpRight->mpParent = pNode->mpParent;

			if ( pNode->mpParent )
			{
				if ( pNode == pNode->mpParent->mpLeft )
					pNode->mpParent->mpLeft = tpRight;
				else
					pNode->mpParent->mpRight = tpRight;
			}
			else
				mpRoot = tpRight;

			tpRight->mpLeft = pNode;
			if ( pNode != &mSentinel )
				pNode->mpParent = tpRight;
		}

		void rotateRight( CNSMapNode< KEY, HLISTINDEX >* pNode )
		{
			CNSMapNode< KEY, HLISTINDEX >* tpLeft = pNode->mpLeft;

			pNode->mpLeft = tpLeft->mpRight;
			if ( tpLeft->mpRight != &mSentinel )
				tpLeft->mpRight->mpParent = pNode;

			if ( tpLeft != &mSentinel )
				tpLeft->mpParent = pNode->mpParent;

			if ( pNode->mpParent )
			{
				if ( pNode == pNode->mpParent->mpRight )
					pNode->mpParent->mpRight = tpLeft;
				else
					pNode->mpParent->mpLeft = tpLeft;
			}
			else
				mpRoot = tpLeft;

			tpLeft->mpRight = pNode;
			if ( pNode != &mSentinel )
				pNode->mpParent = tpLeft;
		}

		void insertFixup( CNSMapNode< KEY, HLISTINDEX >* pNode )
		{
			while ( pNode != mpRoot && pNode->mpParent->mNodeColour == CNSMapNode< KEY, HLISTINDEX >::NODE_RED )
			{
				if ( pNode->mpParent == pNode->mpParent->mpParent->mpLeft )
				{
					CNSMapNode< KEY, HLISTINDEX >* tpUncle = pNode->mpParent->mpParent->mpRight;
					if ( tpUncle->mNodeColour == CNSMapNode< KEY, HLISTINDEX >::NODE_RED )
					{
						pNode->mpParent->mNodeColour = CNSMapNode< KEY, HLISTINDEX >::NODE_BLACK;
						tpUncle->mNodeColour = CNSMapNode< KEY, HLISTINDEX >::NODE_BLACK;
						pNode->mpParent->mpParent->mNodeColour = CNSMapNode< KEY, HLISTINDEX >::NODE_RED;
						pNode = pNode->mpParent->mpParent;
					}
					else
					{
						if ( pNode == pNode->mpParent->mpRight )
						{
							pNode = pNode->mpParent;
							rotateLeft( pNode );
						}

						pNode->mpParent->mNodeColour = CNSMapNode< KEY, HLISTINDEX >::NODE_BLACK;
						pNode->mpParent->mpParent->mNodeColour = CNSMapNode< KEY, HLISTINDEX >::NODE_RED;
						rotateRight( pNode->mpParent->mpParent );
					}
				}
				else
				{
					CNSMapNode< KEY, HLISTINDEX >* tpUncle = pNode->mpParent->mpParent->mpLeft;
					if ( tpUncle->mNodeColour == CNSMapNode< KEY, HLISTINDEX >::NODE_RED )
					{
						pNode->mpParent->mNodeColour = CNSMapNode< KEY, HLISTINDEX >::NODE_BLACK;
						tpUncle->mNodeColour = CNSMapNode< KEY, HLISTINDEX >::NODE_BLACK;
						pNode->mpParent->mpParent->mNodeColour = CNSMapNode< KEY, HLISTINDEX >::NODE_RED;
						pNode = pNode->mpParent->mpParent;
					}
					else
					{
						if ( pNode == pNode->mpParent->mpLeft )
						{
							pNode = pNode->mpParent;
							rotateRight( pNode );
						}
						pNode->mpParent->mNodeColour = CNSMapNode< KEY, HLISTINDEX >::NODE_BLACK;
						pNode->mpParent->mpParent->mNodeColour = CNSMapNode< KEY, HLISTINDEX >::NODE_RED;
						rotateLeft( pNode->mpParent->mpParent );
					}
				}
			}

			mpRoot->mNodeColour = CNSMapNode< KEY, HLISTINDEX >::NODE_BLACK;
		}

		void deleteFixup( CNSMapNode< KEY, HLISTINDEX >* pNode )
		{
			while ( pNode != mpRoot && pNode->mNodeColour == CNSMapNode< KEY, HLISTINDEX >::NODE_BLACK )
			{
				if ( pNode == pNode->mpParent->mpLeft )
				{
					CNSMapNode< KEY, HLISTINDEX >* tpUncle = pNode->mpParent->mpRight;
					if ( tpUncle->mNodeColour == CNSMapNode< KEY, HLISTINDEX >::NODE_RED )
					{
						tpUncle->mNodeColour = CNSMapNode< KEY, HLISTINDEX >::NODE_BLACK;
						tpUncle->mpParent->mNodeColour = CNSMapNode< KEY, HLISTINDEX >::NODE_RED;
						rotateLeft( pNode->mpParent );
						tpUncle = pNode->mpParent->mpRight;
					}

					if ( tpUncle->mpLeft->mNodeColour == CNSMapNode< KEY, HLISTINDEX >::NODE_BLACK && tpUncle->mpRight->mNodeColour == CNSMapNode< KEY, HLISTINDEX >::NODE_BLACK )
					{
						tpUncle->mNodeColour = CNSMapNode< KEY, HLISTINDEX >::NODE_RED;
						pNode = pNode->mpParent;
					}
					else
					{
						if ( tpUncle->mpRight->mNodeColour == CNSMapNode< KEY, HLISTINDEX >::NODE_BLACK )
						{
							tpUncle->mpLeft->mNodeColour = CNSMapNode< KEY, HLISTINDEX >::NODE_BLACK;
							tpUncle->mNodeColour = CNSMapNode< KEY, HLISTINDEX >::NODE_RED;
							rotateRight( tpUncle );
							tpUncle = pNode->mpParent->mpRight;
						}
						tpUncle->mNodeColour = pNode->mpParent->mNodeColour;
						pNode->mpParent->mNodeColour = CNSMapNode< KEY, HLISTINDEX >::NODE_BLACK;
						tpUncle->mpRight->mNodeColour = CNSMapNode< KEY, HLISTINDEX >::NODE_BLACK;
						rotateLeft( pNode->mpParent );
						pNode = mpRoot;
					}
				}
				else
				{
					CNSMapNode< KEY, HLISTINDEX >* tpUncle = pNode->mpParent->mpLeft;
					if ( tpUncle->mNodeColour == CNSMapNode< KEY, HLISTINDEX >::NODE_RED )
					{
						tpUncle->mNodeColour = CNSMapNode< KEY, HLISTINDEX >::NODE_BLACK;
						pNode->mpParent->mNodeColour = CNSMapNode< KEY, HLISTINDEX >::NODE_RED;
						rotateRight( pNode->mpParent );
						tpUncle = pNode->mpParent->mpLeft;
					}
					if ( tpUncle->mpRight->mNodeColour == CNSMapNode< KEY, HLISTINDEX >::NODE_BLACK && tpUncle->mpLeft->mNodeColour == CNSMapNode< KEY, HLISTINDEX >::NODE_BLACK )
					{
						tpUncle->mNodeColour = CNSMapNode< KEY, HLISTINDEX >::NODE_RED;
						pNode = pNode->mpParent;
					}
					else
					{
						if ( tpUncle->mpLeft->mNodeColour == CNSMapNode< KEY, HLISTINDEX >::NODE_BLACK )
						{
							tpUncle->mpRight->mNodeColour = CNSMapNode< KEY, HLISTINDEX >::NODE_BLACK;
							tpUncle->mNodeColour = CNSMapNode< KEY, HLISTINDEX >::NODE_RED;
							rotateLeft( tpUncle );
							tpUncle = pNode->mpParent->mpLeft;
						}
						tpUncle->mNodeColour = pNode->mpParent->mNodeColour;
						pNode->mpParent->mNodeColour = CNSMapNode< KEY, HLISTINDEX >::NODE_BLACK;
						tpUncle->mpLeft->mNodeColour = CNSMapNode< KEY, HLISTINDEX >::NODE_BLACK;
						rotateRight( pNode->mpParent );
						pNode = mpRoot;
					}
				}
			}

			pNode->mNodeColour = CNSMapNode< KEY, HLISTINDEX >::NODE_BLACK;
		}

	public:
		void clear( )
		{
			if ( mpRoot != &mSentinel )
			{
				eraseTree( mpRoot );
				mpRoot = &mSentinel;
			}
		}

		void erase( const KEY& rKey )
		{
			CNSMapNode< KEY, HLISTINDEX >* tpEraseNode = ( CNSMapNode< KEY, HLISTINDEX >* ) findNode( rKey );
			if ( tpEraseNode == NULL )
				return;

			eraseNode( tpEraseNode );
		}

		void eraseNode( HMAPINDEX pNode )
		{
			CNSMapNode< KEY, HLISTINDEX >* tpNode = ( CNSMapNode< KEY, HLISTINDEX >* ) pNode;
			CNSMapNode< KEY, HLISTINDEX >* tpTempNode1 = NULL;
			CNSMapNode< KEY, HLISTINDEX >* tpTempNode2 = NULL;

			if ( tpNode->mpLeft == &mSentinel || tpNode->mpRight == &mSentinel )
				tpTempNode2 = tpNode;
			else
			{
				tpTempNode2 = tpNode->mpRight;
				while ( tpTempNode2->mpLeft != &mSentinel )
					tpTempNode2 = tpTempNode2->mpLeft;
			}

			if ( tpTempNode2->mpLeft != &mSentinel )
				tpTempNode1 = tpTempNode2->mpLeft;
			else
				tpTempNode1 = tpTempNode2->mpRight;

			tpTempNode1->mpParent = tpTempNode2->mpParent;
			if ( tpTempNode2->mpParent != NULL )
			{
				if ( tpTempNode2 == tpTempNode2->mpParent->mpLeft )
					tpTempNode2->mpParent->mpLeft = tpTempNode1;
				else
					tpTempNode2->mpParent->mpRight = tpTempNode1;
			}
			else
				mpRoot = tpTempNode1;

			HLISTINDEX tLastIndex = tpNode->mValue;
			if ( tpTempNode2 != tpNode )
			{
				tpNode->mKey = tpTempNode2->mKey;
				tpNode->mValue = tpTempNode2->mValue;
			}

			if ( tpTempNode2->mNodeColour == CNSMapNode< KEY, HLISTINDEX >::NODE_BLACK )
				deleteFixup( tpTempNode1 );

			mList.erase( tLastIndex );
			tpTempNode2->~CNSMapNode< KEY, HLISTINDEX >( );
			Allocator::free( tpTempNode2 );
		}

		HMAPINDEX findNode( const KEY& rKey ) const
		{
			CNSMapNode< KEY, HLISTINDEX >* tpCurr = mpRoot;
			while ( tpCurr != &mSentinel )
			{
				if ( rKey == tpCurr->mKey )
				{
					return tpCurr;
				}
				else
					tpCurr = rKey < tpCurr->mKey ? tpCurr->mpLeft : tpCurr->mpRight;
			}

			return NULL;
		}

		HLISTINDEX findNodeIndex( const KEY& rKey ) const
		{
			CNSMapNode< KEY, HLISTINDEX >* tpCurr = mpRoot;
			while ( tpCurr != &mSentinel )
			{
				if ( rKey == tpCurr->mKey )
					return tpCurr->mValue;
				else
					tpCurr = rKey < tpCurr->mKey ? tpCurr->mpLeft : tpCurr->mpRight;
			}

			return NULL;
		}

		bool find( const KEY& rKey ) const
		{
			CNSMapNode< KEY, HLISTINDEX >* tpCurr = mpRoot;
			while ( tpCurr != &mSentinel )
			{
				if ( rKey == tpCurr->mKey )
					return true;
				else
					tpCurr = rKey < tpCurr->mKey ? tpCurr->mpLeft : tpCurr->mpRight;
			}

			return false;
		}

		KEY& insert( const KEY& rKey )
		{
			CNSMapNode< KEY, HLISTINDEX >* tpCurr;
			CNSMapNode< KEY, HLISTINDEX >* tpParent;
			tpCurr = mpRoot;
			tpParent = NULL;
			while ( tpCurr != &mSentinel )
			{
				if ( rKey == tpCurr->mKey )
					return tpCurr->mKey;

				tpParent = tpCurr;
				tpCurr = rKey < tpCurr->mKey ? tpCurr->mpLeft : tpCurr->mpRight;
			}

			CNSMapNode< KEY, HLISTINDEX >* tpNewNode = ( CNSMapNode< KEY, HLISTINDEX >* ) Allocator::alloc( __FILE__, __LINE__, sizeof( CNSMapNode< KEY, HLISTINDEX > ) );
			new ( tpNewNode ) CNSMapNode< KEY, HLISTINDEX >( );
			tpNewNode->mpParent = tpParent;
			tpNewNode->mpLeft = &mSentinel;
			tpNewNode->mpRight = &mSentinel;
			tpNewNode->mNodeColour = CNSMapNode< KEY, HLISTINDEX >::NODE_RED;
			tpNewNode->mKey = rKey;
			tpNewNode->mValue = mList.pushback( rKey );

			if ( tpParent )
			{
				if ( rKey < tpParent->mKey )
					tpParent->mpLeft = tpNewNode;
				else
					tpParent->mpRight = tpNewNode;
			}
			else
				mpRoot = tpNewNode;

			insertFixup( tpNewNode );
			return tpNewNode->mKey;
		}

		bool operator [] ( const KEY& rKey ) const
		{
			HMAPINDEX tFindIndex = findNode( rKey );
			if ( tFindIndex == NULL )
				return false;

			return true;
		}

		// 是否为空Map
		bool isEmpty( ) const { return getCount( ) == 0; }

		// 得到节点数量
		unsigned int getCount( ) const { return mList.getCount( ); }

		// 得到第一个添加的节点
		HLISTINDEX getHead( ) const { return mList.getHead( ); }

		// 得到最后一个添加的节点
		HLISTINDEX getTail( ) const { return mList.getTail( ); }

		// 得到下一个节点
		void getNext( HLISTINDEX& index ) const { index = ( ( CNSListNode< KEY >* ) index )->mpNext; }

		// 得到上一个节点
		void getPrev( HLISTINDEX& index ) const { index = ( ( CNSListNode< KEY >* ) index )->mpPrev; }

		// 根据节点索引得到节点的数值
		KEY& getKey( HLISTINDEX index )
		{
			if ( index == NULL )
				NSException( _UTF8( "Map索引无效" ) );

			CNSListNode< KEY >* tpListNode = ( CNSListNode< KEY >* ) index;
			return tpListNode->mData;
		}

		// 根据节点索引得到节点的数值
		const KEY& getKey( HLISTINDEX index ) const
		{
			if ( index == NULL )
				NSException( _UTF8( "Map索引无效" ) );

			CNSListNode< KEY >* tpListNode = ( CNSListNode< KEY >* ) index;
			return tpListNode->mData;
		}

		// 根据节点索引得到节点的数值
		KEY& getKeyEx( HMAPINDEX index )
		{
			if ( index == NULL )
				NSException( _UTF8( "Map索引无效" ) );

			CNSMapNode< KEY, HLISTINDEX >* tpMapNode = ( CNSMapNode< KEY, HLISTINDEX >* ) index;
			return tpMapNode->mKey;
		}

		// 根据节点索引得到节点的数值
		const KEY& getKeyEx( HMAPINDEX index ) const
		{
			if ( index == NULL )
				NSException( _UTF8( "Map索引无效" ) );

			CNSMapNode< KEY, HLISTINDEX >* tpMapNode = ( CNSMapNode< KEY, HLISTINDEX >* ) index;
			return tpMapNode->mKey;
		}
	};

	// *******************************************************************	//
	// CNSHashMap															//
	// *******************************************************************	//
	template < typename Key, typename T >
	class CNSHashMapNode
	{
	public:
		Key		mKey;					// 键值
		T		mValue;					// 数值

	public:
		CNSHashMapNode( const Key& rKey, const T& rValue ) : mKey( rKey ), mValue( rValue )
		{
		}
	};

	template< typename Key, typename T, typename Allocator >
	class CNSHashMap
	{
	public:
		CNSMap< Key, HLISTINDEX >*				mpHashTable;
		CNSList< CNSHashMapNode< Key, T >* >	mList;
		unsigned int									mTableSize;

	private:
		CNSHashMap< Key, T >& operator = ( const CNSHashMap< Key, T >& rList )
		{
			return *this;
		}

		CNSHashMap( const CNSHashMap< Key, T >& rList )
		{
		}

	public:
		CNSHashMap( unsigned int tableSize ) : mTableSize( tableSize )
		{
			// 分配足够的内存
			mpHashTable = ( CNSMap< Key, HLISTINDEX >* ) Allocator::alloc( __FILE__, __LINE__, sizeof( CNSMap< Key, HLISTINDEX > ) * mTableSize );

			// 逐个调用构造函数
			for ( unsigned int i = 0; i < mTableSize; i ++ )
				new( mpHashTable + i ) CNSMap< Key, HLISTINDEX >( );
		}

	public:
		~CNSHashMap( )
		{
			// 逐个调用析构函数
			for ( unsigned int i = 0; i < mTableSize; i ++ )
				( mpHashTable + i )->~CNSMap< Key, HLISTINDEX >( );

			// 释放内存
			Allocator::free( mpHashTable );

			// 释放List
			HLISTINDEX tBeginIndex = mList.getHead( );
			for ( ; tBeginIndex != NULL; mList.getNext( tBeginIndex ) )
			{
				CNSListNode< CNSHashMapNode< Key, T >* >* tpListNode = ( CNSListNode< CNSHashMapNode< Key, T >* >* ) tBeginIndex;

				// 调用析构函数
				tpListNode->mData->~CNSHashMapNode< Key, T >( );

				// 释放内存
				Allocator::free( tpListNode->mData );
			}
		}

	public:
		// 清除所有节点
		void clear( )
		{
			// 逐个调用析构函数
			for ( unsigned int i = 0; i < mTableSize; i ++ )
				( mpHashTable + i )->~CNSMap< Key, HLISTINDEX >( );

			// 释放内存
			Allocator::free( mpHashTable );

			// 释放List
			HLISTINDEX tBeginIndex = mList.getHead( );
			for ( ; tBeginIndex != NULL; mList.getNext( tBeginIndex ) )
			{
				CNSListNode< CNSHashMapNode< Key, T >* >* tpListNode = ( CNSListNode< CNSHashMapNode< Key, T >* >* ) tBeginIndex;

				// 调用析构函数
				tpListNode->mData->~CNSHashMapNode< Key, T >( );

				// 释放内存
				Allocator::free( tpListNode->mData );
			}
			mList.clear( );

			// 分配足够的内存
			mpHashTable = ( CNSMap< Key, HLISTINDEX >* ) Allocator::alloc( __FILE__, __LINE__, sizeof( CNSMap< Key, HLISTINDEX > ) * mTableSize );

			// 逐个调用构造函数
			for ( unsigned int i = 0; i < mTableSize; i ++ )
				new( mpHashTable + i ) CNSMap< Key, HLISTINDEX >( );
		}

		// 根据键值删除节点
		void erase( const Key& rKey )
		{
			// 计算hash值
			unsigned int tTableIndex = rKey % mTableSize;

			// 遍历索引到Hash桶
			for ( unsigned int i = 0; i < mpHashTable[ tTableIndex ].getCount( ); i ++ )
			{
				// Hash桶中的每一个节点都是List的节点指针
				HMAPINDEX tMapIndex = mpHashTable[ tTableIndex ].findNode( rKey );
				if ( tMapIndex != NULL )
				{
					// 释放List中的节点
					mList.erase( mpHashTable[ tTableIndex ].getValueEx( tMapIndex ) );

					// 释放Hash桶中的节点
					mpHashTable[ tTableIndex ].erase( rKey );
					return;
				}
			}
		}

		// 插入节点，指定键值和数值，键值要自己保证唯一，否则后果自负！
		void insert( const Key& rKey, const T& rValue )
		{
			// 计算hash值
			unsigned int tTableIndex = rKey % mTableSize;

			// 分配内存
			CNSHashMapNode< Key, T >* tpNode = ( CNSHashMapNode< Key, T >* ) Allocator::alloc( __FILE__, __LINE__, sizeof( CNSHashMapNode< Key, T > ) );

			// 调用构造函数
			new( tpNode ) ( CNSHashMapNode< Key, T > )( rKey, rValue );

			// 放入Hash表中
			mpHashTable[ tTableIndex ].insert( rKey, mList.pushback( tpNode ) );
		}

		// 根据键值查找数值，返回是否找到
		bool find( const Key& rKey, T& rValue )
		{
			// 计算hash值
			unsigned int tTableIndex = rKey % mTableSize;

			HLISTINDEX tListIndex;
			if ( mpHashTable[ tTableIndex ].find( rKey, tListIndex ) == true )
			{
				CNSListNode< CNSHashMapNode< Key, T >* >* tpListNode = ( CNSListNode< CNSHashMapNode< Key, T >* >* ) tListIndex;
				// 返回找到的数值
				rValue = tpListNode->mData->mValue;
				return true;
			}

			return false;
		}

		// 得到节点数量
		unsigned int getCount( ) const { return mList.getCount( ); }

		// 得到第一个添加的节点
		HLISTINDEX getHead( ) const { return mList.getHead( ); }

		// 得到最后一个添加的节点
		HLISTINDEX getTail( ) const { return mList.getTail( ); }

		// 得到下一个节点
		void getNext( HLISTINDEX& rIndex ) const { rIndex = ( ( CNSListNode< CNSHashMapNode< Key, T >* >* ) rIndex )->mpNext; }

		// 根据节点索引得到节点的键值
		const Key& getKey( HLISTINDEX index ) const
		{
			CNSListNode< CNSHashMapNode< Key, T >* >* tpListNode = ( CNSListNode< CNSHashMapNode< Key, T >* >* ) index;
			return tpListNode->mData->mKey;
		}

		// 根据节点索引得到节点的数值
		const T& getValue( HLISTINDEX index ) const
		{
			CNSListNode< CNSHashMapNode< Key, T >* >* tpListNode = ( CNSListNode< CNSHashMapNode< Key, T >* >* ) index;
			return tpListNode->mData->mValue;
		}

		// 根据节点索引得到节点的数值
		T& getValue( HLISTINDEX index )
		{
			CNSListNode< CNSHashMapNode< Key, T >* >* tpListNode = ( CNSListNode< CNSHashMapNode< Key, T >* >* ) index;
			return tpListNode->mData->mValue;
		}
	};

	// *******************************************************************	//
	// CNSString
	// *******************************************************************	//
	class CNSString
	{
	protected:
		char*	mpText;			// 字符串缓冲区
		size_t	mSize;			// 缓冲区长度(字符为单位)
		size_t	mCount;			// 字符长度，包含0结束

	public:
		// 默认构造函数
		CNSString( ) : mSize( 16 ), mCount( 1 )
		{
			mpText = (char*) CAllocator::alloc( __FILE__, __LINE__, mSize );
			mpText[ 0 ] = '\0';
		}

		// 通过字符串指针构造，调用方保证text地址正确
		CNSString( const char* text, size_t len = -1 ) : mpText( NULL ), mSize( 16 ), mCount( 1 )
		{
			if ( text == NULL )
			{
				mpText = (char*) CAllocator::alloc( __FILE__, __LINE__, mSize );
				mpText[ 0 ] = '\0';
				return;
			}

			if ( len == -1 )
				len = strlen( text );

			// 记录缓冲区长度
			mSize = NSBase::NSFunction::forbsize( len + 1 );

			// 记录字符串长度
			mCount = len + 1;

			// 分配足够的内存
			mpText = (char*) CAllocator::alloc( __FILE__, __LINE__, mSize );

			// 拷贝字符串到缓冲区中
			NSFunction::memcpy_fast( mpText, text, len );

			// 设置NULL结束
			mpText[ len ] = '\0';
		}

		// 通过对象本身构造
		CNSString( const CNSString& text ) : mpText( NULL ), mSize( 16 ), mCount( 1 )
		{
			// 分配足够的内存
			mpText = (char*) CAllocator::alloc( __FILE__, __LINE__, text.mSize );

			// 拷贝字符串到缓冲区中
			NSFunction::memcpy_fast( mpText, text.mpText, text.getCount( ) );

			// 记录缓冲区长度
			mSize = text.mSize;

			// 记录字符串长度
			mCount = text.mCount;
		}

	public:
		// 析构函数
		~CNSString( )
		{
			CAllocator::free( mpText );
			mSize = 0;
			mCount = 0;
			mpText = NULL;
		}

	public:
		// 判断有几个字节
		static inline int getUtf8Length( unsigned char ch )
		{
			if ( ch >= 0x00 && ch <= 0x7f )
			{
				//说明最高位为'0'，这意味着utf8编码只有1个字节！  
				return 1;
			}
			else if ( ( ch & 0xe0 ) == 0xc0 )
			{
				//只保留最高三位，看最高三位是不是110，如果是则意味着utf8编码有2个字节！  
				return 2;
			}
			else if ( ( ch & 0xf0 ) == 0xe0 )
			{
				//只保留最高四位，看最高三位是不是1110，如果是则意味着utf8编码有3个字节！  
				return 3;
			}
			else if ( ( ch & 0xf8 ) == 0xf0 )
			{
				//只保留最高五位，看最高四位是不是1111，如果是则意味着utf8编码有4个字节！  
				return 4;
			}
			else if ( ( ch & 0xfc ) == 0xf8 )
			{
				//只保留最高六位，看最高五位是不是1111 10，如果是则意味着utf8编码有5个字节！  
				return 5;
			}
			else if ( ( ch & 0xfe ) == 0xfc )
			{
				//只保留最高七位，看最高六位是不是1111 110，如果是则意味着utf8编码有6个字节！  
				return 5;
			}
			return 1;
		}

#ifdef PLATFORM_WIN32
	protected:
		// 转换unicode, 返回unicode字节长度
		static inline int _convertUtf8ToUni32( const char* utf8, int len, unsigned int* unicode )
		{
			int i = 0;
			int j = 0;
			char* temp = (char*) unicode;
			//循环解析
			while ( i < len )
			{
				int byteNum = getUtf8Length( utf8[ i ] );
				if ( byteNum == 0 )
					return 0;

				if ( temp != NULL )
				{
					switch ( byteNum )
					{
					case 1:
						temp[ j ] = utf8[ i ];
						temp[ j + 1 ] = 0;
						temp[ j + 2 ] = 0;
						temp[ j + 3 ] = 0;
						break;
					case 2:
						temp[ j ] = ( utf8[ i ] << 6 ) + ( utf8[ i + 1 ] & 0x3F );
						temp[ j + 1 ] = ( utf8[ i ] >> 2 ) & 0x07;
						temp[ j + 2 ] = 0;
						temp[ j + 3 ] = 0;
						break;
					case 3:
						temp[ j ] = ( ( utf8[ i + 1 ] & 0x03 ) << 6 ) + ( utf8[ i + 2 ] & 0x3F );
						temp[ j + 1 ] = ( ( utf8[ i ] & 0x0F ) << 4 ) | ( ( utf8[ i + 1 ] >> 2 ) & 0x0F );
						temp[ j + 2 ] = 0;
						temp[ j + 3 ] = 0;
						break;
					case 4:
						temp[ j ] = ( utf8[ i + 2 ] << 6 ) + ( utf8[ i + 3 ] & 0x3F );
						temp[ j + 1 ] = ( utf8[ i + 1 ] << 4 ) + ( ( utf8[ i + 2 ] >> 2 ) & 0x0F );
						temp[ j + 2 ] = ( ( utf8[ i ] << 2 ) & 0x1C ) + ( ( utf8[ i + 1 ] >> 4 ) & 0x03 );
						temp[ j + 3 ] = 0;
						break;
					case 5:
						temp[ j ] = ( utf8[ i + 3 ] << 6 ) + ( utf8[ i + 4 ] & 0x3F );
						temp[ j + 1 ] = ( utf8[ i + 2 ] << 4 ) + ( ( utf8[ i + 3 ] >> 2 ) & 0x0F );
						temp[ j + 2 ] = ( utf8[ i + 1 ] << 2 ) + ( ( utf8[ i + 2 ] >> 4 ) & 0x03 );
						temp[ j + 3 ] = ( utf8[ i ] & 0x03 );
						break;
					case 6:
						temp[ j ] = ( utf8[ i + 4 ] << 6 ) + ( utf8[ i + 5 ] & 0x3F );
						temp[ j + 1 ] = ( utf8[ i + 3 ] << 4 ) + ( ( utf8[ i + 4 ] >> 2 ) & 0x0F );
						temp[ j + 2 ] = ( utf8[ i + 2 ] << 2 ) + ( ( utf8[ i + 3 ] >> 4 ) & 0x03 );
						temp[ j + 3 ] = ( ( utf8[ i ] << 6 ) & 0x40 ) + ( utf8[ i + 1 ] & 0x3F );
						break;
					default:
						break;
					}
				}

				j += 4;
				i += byteNum;
			}
			if ( temp != NULL )
			{
				temp[ j ] = 0;
				temp[ j + 1 ] = 0;
				temp[ j + 2 ] = 0;
				temp[ j + 3 ] = 0;
			}
			return j + 4;
		}

		static char* _convertUni32ToUtf8( unsigned int uniChar )
		{
			static char value[ 7 ];
			if ( uniChar <= 0x0000007F )
			{
				// * U-00000000 - U-0000007F:  0xxxxxxx  
				value[ 0 ] = ( uniChar & 0x7F );
				value[ 1 ] = 0;
			}
			else if ( uniChar >= 0x00000080 && uniChar <= 0x000007FF )
			{
				// * U-00000080 - U-000007FF:  110xxxxx 10xxxxxx
				value[ 2 ] = 0;
				value[ 1 ] = ( uniChar & 0x3F ) | 0x80;
				value[ 0 ] = ( ( uniChar >> 6 ) & 0x1F ) | 0xC0;
			}
			else if ( uniChar >= 0x00000800 && uniChar <= 0x0000FFFF )
			{
				// * U-00000800 - U-0000FFFF:  1110xxxx 10xxxxxx 10xxxxxx
				value[ 3 ] = 0;
				value[ 2 ] = ( uniChar & 0x3F ) | 0x80;
				value[ 1 ] = ( ( uniChar >> 6 ) & 0x3F ) | 0x80;
				value[ 0 ] = ( ( uniChar >> 12 ) & 0x0F ) | 0xE0;
			}
			else if ( uniChar >= 0x00010000 && uniChar <= 0x001FFFFF )
			{
				// * U-00010000 - U-001FFFFF:  11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
				value[ 4 ] = 0;
				value[ 3 ] = ( uniChar & 0x3F ) | 0x80;
				value[ 2 ] = ( ( uniChar >> 6 ) & 0x3F ) | 0x80;
				value[ 1 ] = ( ( uniChar >> 12 ) & 0x3F ) | 0x80;
				value[ 0 ] = ( ( uniChar >> 18 ) & 0x07 ) | 0xF0;
			}
			else if ( uniChar >= 0x00200000 && uniChar <= 0x03FFFFFF )
			{
				// * U-00200000 - U-03FFFFFF:  111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
				value[ 5 ] = 0;
				value[ 4 ] = ( uniChar & 0x3F ) | 0x80;
				value[ 3 ] = ( ( uniChar >> 6 ) & 0x3F ) | 0x80;
				value[ 2 ] = ( ( uniChar >> 12 ) & 0x3F ) | 0x80;
				value[ 1 ] = ( ( uniChar >> 18 ) & 0x3F ) | 0x80;
				value[ 0 ] = ( ( uniChar >> 24 ) & 0x03 ) | 0xF8;
			}
			else if ( uniChar >= 0x04000000 && uniChar <= 0x7FFFFFFF )
			{
				// * U-04000000 - U-7FFFFFFF:  1111110x 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
				value[ 6 ] = 0;
				value[ 5 ] = ( uniChar & 0x3F ) | 0x80;
				value[ 4 ] = ( ( uniChar >> 6 ) & 0x3F ) | 0x80;
				value[ 3 ] = ( ( uniChar >> 12 ) & 0x3F ) | 0x80;
				value[ 2 ] = ( ( uniChar >> 18 ) & 0x3F ) | 0x80;
				value[ 1 ] = ( ( uniChar >> 24 ) & 0x3F ) | 0x80;
				value[ 0 ] = ( ( uniChar >> 30 ) & 0x01 ) | 0xFC;
			}

			return value;
		}

		// 这个函数的返回值不能保存
		static char* _convertUni16ToUtf8( unsigned int uniChar )
		{
			static char value[ 6 ];
			if ( uniChar <= 0x0000007F )
			{
				// * U-00000000 - U-0000007F:  0xxxxxxx  
				value[ 0 ] = ( uniChar & 0x7F );
				value[ 1 ] = 0;
			}
			else if ( uniChar >= 0x00000080 && uniChar <= 0x000007FF )
			{
				// * U-00000080 - U-000007FF:  110xxxxx 10xxxxxx
				value[ 2 ] = 0;
				value[ 1 ] = ( uniChar & 0x3F ) | 0x80;
				value[ 0 ] = ( ( uniChar >> 6 ) & 0x1F ) | 0xC0;
			}
			else if ( uniChar >= 0x00000800 && uniChar <= 0x0000FFFF )
			{
				// * U-00000800 - U-0000FFFF:  1110xxxx 10xxxxxx 10xxxxxx
				value[ 3 ] = 0;
				value[ 2 ] = ( uniChar & 0x3F ) | 0x80;
				value[ 1 ] = ( ( uniChar >> 6 ) & 0x3F ) | 0x80;
				value[ 0 ] = ( ( uniChar >> 12 ) & 0x0F ) | 0xE0;
			}

			return value;
		}

		// 转换unicode, 返回unicode字节长度
		static inline int _convertUtf8ToUni16( const char* utf8, int len, unsigned short* unicode )
		{
			int i = 0;
			int j = 0;
			char* temp = (char*) unicode;
			//循环解析
			while ( i < len )
			{
				int byteNum = getUtf8Length( utf8[ i ] );
				if ( byteNum == 0 )
					return 0;

				if ( temp != NULL )
				{
					switch ( byteNum )
					{
					case 1:
						temp[ j ] = utf8[ i ];
						temp[ j + 1 ] = 0;
						break;
					case 2:
						temp[ j ] = ( utf8[ i ] << 6 ) + ( utf8[ i + 1 ] & 0x3F );
						temp[ j + 1 ] = ( utf8[ i ] >> 2 ) & 0x07;
						break;
					case 3:
						temp[ j ] = ( ( utf8[ i + 1 ] & 0x03 ) << 6 ) + ( utf8[ i + 2 ] & 0x3F );
						temp[ j + 1 ] = ( ( utf8[ i ] & 0x0F ) << 4 ) | ( ( utf8[ i + 1 ] >> 2 ) & 0x0F );
						break;
					case 4:
						NSException( _UTF8( "不支持unicode32" ) );
						break;
					case 5:
						NSException( _UTF8( "不支持unicode32" ) );
						break;
					case 6:
						NSException( _UTF8( "不支持unicode32" ) );
						break;
					default:
						break;
					}
				}

				j += 2;
				i += byteNum;
			}
			if ( temp != NULL )
			{
				temp[ j ] = 0;
				temp[ j + 1 ] = 0;
			}
			return j + 2;
		}

	public:
		// utf8只是unicode一种可变长存储方式，utf8使用和unicode同样的字符集，叫做unicode字符集，也就是.65001
		// vLength代表text的unicode字符个数(不是字节个数)
		// 不能再多线程中使用
		static CNSString convertUni16ToUtf8( wchar_t* text, size_t len = -1 )
		{
			if ( len == -1 )
				len = wcslen( (const wchar_t*) text );

			static CNSString value;
			value.clear( );
			for ( size_t i = 0; i < len; i ++ )
			{
				unsigned int uniChar = *( text + i );
				value.pushback( CNSString::_convertUni16ToUtf8( uniChar ) );
			}
			return value;
		}
		static CNSOctets convertUtf8ToUni16( const CNSString& text );
		static CNSOctets convertUtf8ToUni16( const char* text );
		static CNSOctets convertUtf8ToUni16( const char* text, size_t start, size_t end );
		// 不能再多线程中使用
		static CNSString convertUni32ToUtf8( unsigned int* text, size_t len )
		{
			static CNSString value;
			value.clear( );
			for ( size_t i = 0; i < len; i ++ )
			{
				unsigned int uniChar = *( text + i );
				value.pushback( CNSString::_convertUni32ToUtf8( uniChar ) );
			}
			return value;
		}

		static CNSString convertMbcsToUtf8( const CNSString& text );
		static CNSString convertMbcsToUtf8( const char* text );
		static CNSString convertUtf8ToMbcs( const CNSString& text );
		static CNSString convertUtf8ToMbcs( const char* text );

		// 这个函数假定参数text 是unicode(wchar_t类型) 或者 mbcs(char类型)，不能传入utf8文本
		// 不能再多线程中使用
		static CNSString fromTChar( TCHAR* text )
		{
			static CNSString utf8String;
#ifdef _UNICODE
			utf8String = CNSString::convertUni16ToUtf8( text );
#elif _MBCS
			utf8String = CNSString::convertMbcsToUtf8( text );
#endif

			return utf8String;
		}

		// 这个函数假定参数text 是utf(char类型)，不能传入非utf8文本
		// 不能再多线程中使用
		static TCHAR* toTChar( const CNSString& text );

		// 可以再多线程中使用
		static CNSOctets toTCharOctets( const CNSString& text );
		// 不能再多线程中使用
		static TCHAR* toTChar( const CNSString& text, int start, int end );
#endif
		static CNSString bool2String( bool value )
		{
			static CNSString stringValue;
			static CNSString falseValue = "false";
			if ( value == true )
				stringValue = "true";
			else
				stringValue = "false";

			return stringValue;
		}

		static CNSString number2String( double value )
		{
			static CNSString stringValue;
			if ( value == floor( value ) )
				stringValue.format( "%lld", (long long) value );
			else
				stringValue.format( "%1.2f", value );

			return stringValue;
		}

		static CNSString number2String( float value )
		{
			static CNSString stringValue;
			if ( value == floorf( value ) )
				stringValue.format( "%d", (int) value );
			else
				stringValue.format( "%1.2f", value );

			return stringValue;
		}

		static CNSString number2String( int value )
		{
			static CNSString tValue;
			tValue.format( "%d", value );
			return tValue;
		}

		static CNSString number2String( unsigned int value )
		{
			static CNSString tValue;
			tValue.format( "%u", value );
			return tValue;
		}

		static CNSString address2String( const void* value )
		{
			static CNSString tValue;
			tValue.format( "0x%016x", value );
			return tValue;
		}

		static CNSString number2String( long long value )
		{
			static CNSString tValue;
			tValue.format( "%lld", value );
			return tValue;
		}

		static CNSString number2String( unsigned long long value )
		{
			static CNSString tValue;
			tValue.format( "%lld", value );
			return tValue;
		}

		CNSString encodeURI( ) const
		{
			static const char safe[] = { 
				0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 
				0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 
				1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 0, 1, 1, 1, 1, 1, 
				1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
				0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
				0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
				0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
				0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
			static CNSString retValue;
			retValue.clear( );
			for ( size_t i = 0; i < getLength( ); i ++ )
			{
				if ( safe[ (unsigned char) mpText[ i ] ] == 0 )
				{
					CNSString uriCode;
					uriCode.format( "%%%X", (unsigned char) mpText[ i ] );
					retValue += uriCode;
				}
				else if ( mpText[ i ] == ' ' )
					retValue += '+';
				else
					retValue += mpText[ i ];
			}
			return retValue;
		}

		CNSString decodeURI( ) const
		{
			const unsigned char *ptr = (const unsigned char *) getBuffer( );
			static CNSString retValue;
			retValue.clear( );
			for ( ; *ptr; ++ptr )
			{
				if ( *ptr == '%' )
				{
					if ( *( ptr + 1 ) )
					{
						char a = *( ptr + 1 );
						char b = *( ptr + 2 );
						if ( !( ( a >= 0x30 && a < 0x40 ) || ( a >= 0x41 && a < 0x47 ) ) )
							continue;
						if ( !( ( b >= 0x30 && b < 0x40 ) || ( b >= 0x41 && b < 0x47 ) ) )
							continue;
						char buf[ 3 ];
						buf[ 0 ] = a;
						buf[ 1 ] = b;
						buf[ 2 ] = 0;
						retValue += (char) strtoul( buf, NULL, 16 );
						ptr += 2;
						continue;
					}
				}
				if ( *ptr == '+' )
				{
					retValue += ' ';
					continue;
				}
				retValue += *ptr;
			}
			return retValue;
		}

	public:
		static CNSString getCurDir( )
		{
#ifdef PLATFORM_WIN32
			TCHAR path[ 256 ] = { 0 };
			_tgetcwd( path, 256 );
			return CNSString::fromTChar( path );
#else
			char path[ 256 ] = { 0 };
			getcwd( path, 256 );
			return CNSString( path );
#endif
		}

		CNSString getFileExtent( ) const
		{
			static CNSString result;
			int pos = findLastOf( '.' );
			if ( pos == -1 )
			{
				result = *this;
				return result;
			}

			result.clear( );
			copy( result, getLength( ) - pos - 1, pos + 1 );
			return result;
		}

		CNSString getFileName( const char split ) const
		{
			static CNSString result;
			int pos = findLastOf( split );
			if ( pos == -1 )
			{
				result = *this;
				return result;
			}

			result.clear( );
			copy( result, getLength( ) - pos - 1, pos + 1 );
			return result;
		}

		CNSString getFilePath( const char split ) const
		{
			static CNSString result;
			int pos = findLastOf( split );
			if ( pos == -1 )
			{
				result = *this;
				return result;
			}

			result.clear( );
			copy( result, pos, 0 );
			return result;
		}

		operator char*( )
		{
			return mpText;
		}

		operator const char* ( ) const
		{
			return mpText;
		}

		bool isTextUtf8( ) const
		{
			int encodingBytesCount = 0;
			for ( size_t i = 0; i < getLength( ); i++ )
			{
				unsigned char current = mpText[ i ];
				//First byte
				if ( encodingBytesCount == 0 )
				{
					if ( ( current & 0x80 ) == 0 )
					{
						//ASCII chars, from 0x00 - 0x7F
						continue;
					}

					if ( ( current & 0xC0 ) == 0xC0 )
					{
						encodingBytesCount = 1;
						current <<= 2;

						// More than two bytes used to encoding a unicode char.
						// Calculate the real length.
						while ( ( current & 0x80 ) == 0x80 )
						{
							current <<= 1;
							encodingBytesCount++;
						}
					}
					else
					{
						// Invalid bits structure for UTF8 encoding rule.
						return false;
					}
				}
				else
				{
					// Following bytes, must start with 10.
					if ( ( current & 0xC0 ) == 0x80 )
					{
						encodingBytesCount--;
					}
					else
					{
						//Invalid bits structure for UTF8 encoding rule.
						return false;
					}
				}
			}

			if ( encodingBytesCount != 0 )
			{
				//Invalid bits structure for UTF8 encoding rule.
				//Wrong following bytes count.
				return false;
			}

			//Although UTF8 supports encoding for ASCII chars, we regard as a input stream, whose contents are all ASCII as default encoding.
			return true;
		}

		void replace( const CNSString& match, const CNSString& text )
		{
			int beginIndex = 0;
			for ( ;; )
			{
				beginIndex = findFirstOf( match.getBuffer( ), beginIndex );
				if ( beginIndex == -1 )
					return;

				erase( beginIndex, beginIndex + match.getLength( ) - 1 );
				insert( beginIndex, text );
				beginIndex += text.getLength( );
			}
		}

		size_t hashCode( ) const
		{
			register size_t hashValue = 0;
			for ( size_t i = 0; i < getLength( ); i ++ )
				hashValue = hashValue * 131 + (size_t) mpText[ i ];

			return hashValue;
		}

		// 赋值运算符，通过对象本身赋值
		CNSString& operator = ( const CNSString& text )
		{
			operator = ( text.getBuffer( ) );
			return *this;
		}

		// 赋值运算符，通过字符串指针赋值
		CNSString& operator = ( const char* text )
		{
			if ( text == NULL )
			{
				clear( );
				return *this;
			}

			// 计算传入的文本长度
			size_t textLength = (size_t) strlen( text );

			// 空间不够时，需要重新分配内存
			if ( mSize < textLength + 1 )
			{
				// 保证扩大一倍
				mSize = NSBase::NSFunction::forbsize( textLength + 1 );
				// 重构内存
				CAllocator::free( mpText );
				mpText = (char*) CAllocator::alloc( __FILE__, __LINE__, mSize );
			}

			// 拷贝字符串
			NSFunction::memcpy_fast( mpText, text, textLength );

			// 记录字符串长度
			mCount = textLength + 1;

			// 设置结束符
			mpText[ textLength ] = '\0';
			return *this;
		}

		// 字符串相加，调用的对象和指定字符串相加
		void operator += ( const char* text )
		{
			pushback( text );
		}

		// 字符串相加，调用的对象和指定字符串相加
		void operator += ( char c )
		{
			pushback( c );
		}

		// 字符串相加，调用的对象和指定字符串相加
		void operator += ( const CNSString& text )
		{
			pushback( text );
		}

		CNSString operator + ( const CNSString& text ) const
		{
			CNSString value = *this;
			value += text;
			return value;
		}

		CNSString operator + ( const char* text ) const
		{
			CNSString value = *this;
			value += text;
			return value;
		}

		// 重载运算符 []
		char& operator [] ( size_t index )
		{
			if ( index >= mCount )
				NSException( _UTF8( "访问 CNSString 越界" ) );

			return mpText[ index ];
		}

		// 重载运算符 []
		const char operator [] ( size_t index ) const
		{
			if ( index >= mCount )
				NSException( _UTF8( "访问 CNSString 越界" ) );

			return mpText[ index ];
		}

		// 重载运算符 >
		bool operator > ( const CNSString& text ) const
		{
			if ( strcmp( mpText, text.mpText ) > 0 )
				return true;

			return false;
		}

		// 重载运算符 >=
		bool operator >= ( const CNSString& text ) const
		{
			if ( strcmp( mpText, text.mpText ) >= 0 )
				return true;

			return false;
		}

		// 重载运算符 <
		bool operator < ( const CNSString& text ) const
		{
			if ( strcmp( mpText, text.mpText ) < 0 )
				return true;

			return false;
		}

		// 重载运算符 <=
		bool operator <= ( const CNSString& text ) const
		{
			if ( strcmp( mpText, text.mpText ) <= 0 )
				return true;

			return false;
		}

		// 重载运算符 !=
		bool operator != ( const char* text ) const
		{
			if ( strcmp( mpText, text ) != 0 )
				return true;

			return false;
		}

		// 重载运算符 !=
		bool operator != ( const CNSString& text ) const
		{
			if ( strcmp( mpText, text.mpText ) != 0 )
				return true;

			return false;
		}

		// 重载运算符 ==
		bool operator == ( const char* text ) const
		{
			if ( strcmp( mpText, text ) == 0 )
				return true;

			return false;
		}

		// 重载运算符 ==
		bool operator == ( char* text ) const
		{
			if ( strcmp( mpText, text ) == 0 )
				return true;

			return false;
		}

		// 重载运算符 ==
		bool operator == ( const CNSString& text ) const
		{
			if ( strcmp( mpText, text.mpText ) == 0 )
				return true;

			return false;
		}

		// 重载运算符 %
		size_t operator % ( size_t key ) const
		{
			return hashCode( ) % key;
		}

		void format( const char* format, ... )
		{
			clear( );
			va_list args;
			va_start( args, format );
			// 计算需要得长度
			size_t needLen = vsnprintf( NULL, 0, format, args ) + 1;

			// 如果当前空间不能满足所需要得空间
			if ( needLen > mSize )
			{
				mSize = NSBase::NSFunction::forbsize( needLen );
				mpText = (char*) CAllocator::alloc( __FILE__, __LINE__, mSize );
			}

			// 格式化文本
			vsnprintf( mpText, mSize, format, args );
			va_end( args );
			mCount = needLen;
		}

		// 按照指定区域删除,vEnd等于-1表示删除vStart后面所有字符
		void erase( size_t start, size_t end )
		{
			if ( start < 0 || start >= mCount - 1 )
			{
				CNSString tErrorDesc;
				tErrorDesc.format( _UTF8( "Erase2函数 字符串数组访问越界 index - %d" ), start );
				NSException( tErrorDesc );
			}

			if ( end < 0 || end >= mCount - 1 )
			{
				CNSString tErrorDesc;
				tErrorDesc.format( _UTF8( "Erase2函数 字符串数组访问越界 index - %d" ), end );
				NSException( tErrorDesc );
			}

			// 0代表删除从vStart后面所有得字符
			if ( end == -1 )
			{
				mCount = start + 1;
				mpText[ start ] = 0;
				return;
			}

			NSFunction::memmove_fast( mpText + start, mpText + end + 1, mCount - end - 1 );
			mCount -= end + 1 - start;
		}

		// 通过索引删除指定字符，保持数组紧凑
		void erase( size_t index )
		{
			if ( index < 0 || index >= mCount - 1 )
			{
				CNSString tErrorDesc;
				tErrorDesc.format( _UTF8( "erase函数 字符串数组访问越界 index - %d" ), index );
				NSException( tErrorDesc );
			}

			NSFunction::memmove_fast( mpText + index, mpText + index + 1, mCount - index - 1 );
			mCount --;
		}

		// 清空字符串
		void clear( )
		{
			mCount = 1;
			mpText[ 0 ] = '\0';
		}

		// 是否为空字符串
		bool isEmpty( ) const
		{
			return mCount <= 1;
		}

		// 在指定索引，插入字符串
		void insert( size_t index, const CNSString& text )
		{
			insert( index, text.getBuffer( ) );
		}

		// 在指定索引，插入字符串
		void insert( size_t index, const char* text, size_t len = -1 )
		{
			if ( len == -1 )
				len = strlen( text );

			// 不包含0结束的长度
			size_t oldLen = getLength( );

			size_t newCount = oldLen + len + 1;
			if ( mSize >= newCount )
			{
				if ( oldLen - index > 0 )
					NSFunction::memmove_fast( mpText + index + len, mpText + index, oldLen - index );

				NSFunction::memcpy_fast( mpText + index, text, len );
			}
			else
			{
				mSize = NSBase::NSFunction::forbsize( newCount );
				char* temp = (char*) CAllocator::alloc( __FILE__, __LINE__, mSize );
				if ( index > 0 )
					NSFunction::memcpy_fast( temp, mpText, index );

				NSFunction::memcpy_fast( temp + index, text, len );

				if ( oldLen - index > 0 )
					NSFunction::memcpy_fast( temp + index + len, mpText + index, oldLen - index );

				CAllocator::free( mpText );
				mpText = temp;
			}

			mCount = newCount;
			mpText[ newCount - 1 ] = '\0';
		}

		// 在指定索引，插入字符
		void insert( size_t index, char c )
		{
			insert( index, &c, 1 );
		}

		// 添加字符串
		void pushback( const CNSString& text )
		{
			pushback( text.getBuffer( ) );
		}

		// 添加字符串
		void pushback( const char* text, size_t len = -1 )
		{
			if ( len == -1 )
				len = (size_t) strlen( text );

			if ( len == 0 )
				return;

			// 不包含0结束的长度
			size_t oldLen = getLength( );

			size_t newCount = oldLen + len + 1;
			if ( mSize >= newCount )
			{
				NSFunction::memcpy_fast( mpText + oldLen, text, len );
			}
			else
			{
				mSize = NSBase::NSFunction::forbsize( newCount );
				char* temp = (char*) CAllocator::alloc( __FILE__, __LINE__, mSize );
				if ( oldLen > 0 )
					NSFunction::memcpy_fast( temp, mpText, oldLen );

				NSFunction::memcpy_fast( temp + oldLen, text, len );

				CAllocator::free( mpText );
				mpText = temp;
			}

			mCount = newCount;
			mpText[ newCount - 1 ] = '\0';
		}

		// 添加字符
		void pushback( char c )
		{
			pushback( &c, 1 );
		}

		// 得到缓冲区长度
		size_t getSize( ) const { return mSize; }

		// 得到字符串长度，包含0结束
		size_t getCount( ) const { return mCount; }

		// 得到字符串长度，不包含0结束
		size_t getLength( ) const { return max( 0, (int) mCount - 1 ); }

		// 得到缓冲区指针
		const char* getBuffer( ) const { return mpText; }

		// 按照指定位置拷贝字符串
		void copy( CNSString& text, size_t len, size_t offset = 0 ) const
		{
			text.clear( );
			if ( len == 0 )
				return;

			if ( offset >= getLength( ) )
				return;

			if ( len != -1 && offset + len - 1 >= getLength( ) )
				return;

			text.pushback( mpText + offset, len );
		}

		// 16进制字符串数转换10进制数值
		unsigned int hex2Dec( ) const
		{
			int tRetValue = 0;
			for ( size_t i = 0; i < getLength( ); i ++ )
			{
				int tValue = 0;
				if ( mpText[ i ] >= '0' && mpText[ i ] <= '9' )
					tValue = mpText[ i ] - '0';

				if ( mpText[ i ] >= 'a' && mpText[ i ] <= 'f' )
					tValue = mpText[ i ] - 'a';

				if ( mpText[ i ] >= 'A' && mpText[ i ] <= 'F' )
					tValue = mpText[ i ] - 'A' + 10;

				tRetValue |= ( tValue << ( ( getLength( ) - i - 1 ) * 4 ) );
			}

			return tRetValue;
		}

		// 得到布尔值
		bool toBoolean( ) const { return toLower( ) == "true" ? true : false; }

		// 得到整数值
		int toInteger( ) const { return atoi( mpText ); }

		// 得到浮点数
		double toDouble( ) const { return atof( mpText ); }

		bool isAscii( ) const
		{
			for ( size_t i = 1; i < getLength( ); i ++ )
			{
				if ( isascii( (unsigned char) mpText[ i ] ) == 0 )
					return false;
			}

			return true;
		}

		bool isNumber( ) const
		{
			if ( ::isdigit( (unsigned char) mpText[ 0 ] ) == 0 && mpText[ 0 ] != '-' )
				return false;

			for ( size_t i = 1; i < getLength( ); i ++ )
			{
				if ( ::isdigit( (unsigned char) mpText[ i ] ) == 0 && mpText[ i ] != '.' )
					return false;
			}

			return true;
		}

		// 转换为小写
		const CNSString toLower( ) const
		{
			CNSString tLower = ( *this );
			tLower.toLower( );
			return tLower;
		}

		// 转换为小写
		const CNSString& toLower( )
		{
			for ( size_t i = 0; i < mCount; i ++ )
				mpText[ i ] = tolower( mpText[ i ] );
			return ( *this );
		}

		// 忽略大小写对比
		bool compareNoCase( const CNSString& rText ) const
		{
#ifdef _WIN32
			if ( _stricmp( mpText, rText.getBuffer( ) ) == 0 )
#else
			if ( strcasecmp( mpText, rText.getBuffer( ) ) == 0 )
#endif	
				return true;

			return false;
		}

		// 按照指定字符串切割文本
		CNSVector< CNSString >& split( const CNSString& split, CNSVector< CNSString >& result ) const
		{
			size_t beginIndex = 0;
			size_t endIndex = 0;
			for ( size_t i = 0; i < mCount - 1; )
			{
				bool find = true;
				for ( size_t j = 0; j < split.getLength( ); j ++ )
				{
					if ( i + j >= getLength( ) || mpText[ i + j ] != split[ j ] )
					{
						find = false;
						break;
					}
				}

				int offset = getUtf8Length( mpText[ i ] );
				if ( find == true )
				{
					static CNSString temp;
					temp.clear( );

					if ( endIndex > beginIndex )
						copy( temp, endIndex - beginIndex, beginIndex );

					result.pushback( temp );
					offset = split.getLength( );
					beginIndex = i + offset;
				}

				i += offset;
				endIndex = i;
			}

			if ( beginIndex >= getLength( ) || beginIndex == endIndex )
			{
				static CNSString temp;
				temp.clear( );
				result.pushback( temp );
			}
			else if ( beginIndex < endIndex )
			{
				static CNSString temp;
				temp.clear( );
				if ( endIndex > beginIndex )
					copy( temp, endIndex - beginIndex, beginIndex );
				result.pushback( temp );
			}

			return result;
		}

		// 查找第一个指定字符，off表示开始位置
		int findFirstOf( char ch, int off = 0 ) const
		{
			for ( size_t i = off; i < mCount; i ++ )
			{
				if ( mpText[ i ] == ch )
					return i;
			}

			return -1;
		}

		// 查找第一个指定字符，off表示开始位置
		int nocaseFindFirstOf( char ch, int off = 0 ) const
		{
			for ( size_t i = off; i < mCount; i ++ )
			{
				if ( tolower( mpText[ i ] ) == tolower( ch ) )
					return i;
			}

			return -1;
		}

		// 查找第一个非指定字符，off表示开始位置
		int findFirstNotOf( char ch, int off = 0 ) const
		{
			for ( size_t i = off; i < mCount; i ++ )
			{
				if ( mpText[ i ] != ch )
					return i;
			}

			return -1;
		}

		// 查找第一个非指定字符，off表示开始位置
		int nocaseFindFirstNotOf( char ch, int off = 0 ) const
		{
			for ( size_t i = off; i < mCount; i ++ )
			{
				if ( tolower( mpText[ i ] ) != tolower( ch ) )
					return i;
			}

			return -1;
		}

		// 查找最后一个指定字符，off表示结束位置
		int findLastOf( char ch, int off = 0 ) const
		{
			for ( int i = mCount - 2; i >= off; i -- )
			{
				if ( mpText[ i ] == ch )
					return i;
			}

			return -1;
		}

		// 查找最后一个指定字符，off表示结束位置
		int nocaseFindLastOf( char ch, int off = 0 ) const
		{
			for ( int i = mCount - 2; i >= off; i -- )
			{
				if ( tolower( mpText[ i ] ) == tolower( ch ) )
					return i;
			}

			return -1;
		}

		// 查找最后一个非指定字符，off表示结束位置
		int findLastNotOf( char ch, int off = 0 ) const
		{
			for ( int i = mCount - 2; i >= off; i -- )
			{
				if ( mpText[ i ] != ch )
					return i;
			}

			return -1;
		}

		// 查找最后一个非指定字符，off表示结束位置
		int nocaseFindLastNotOf( char ch, int off = 0 ) const
		{
			for ( int i = mCount - 2; i >= off; i -- )
			{
				if ( tolower( mpText[ i ] ) != tolower( ch ) )
					return i;
			}

			return -1;
		}

		// 查找第一个指定字符串，off表示开始位置
		int findFirstOf( const char* text, int off = 0 ) const
		{
			return findFirstOf( text, off, strlen( text ) );
		}

		// 查找第一个指定字符串，off表示开始位置
		int nocaseFindFirstOf( const char* text, int offset = 0 ) const
		{
			return nocaseFindFirstOf( text, offset, strlen( text ) );
		}

		// 查找第一个指定字符串，off表示开始位置, count表示匹配几个字符
		int findFirstOf( const char* text, int offset, size_t count ) const
		{
			size_t textLen = strlen( text );
			for ( size_t i = offset; i < mCount; i ++ )
			{
				bool find = true;
				size_t index = 0;
				for ( size_t j = i; j < mCount && index < min( textLen, count ); j ++, index ++ )
				{
					if ( mpText[ j ] != text[ index ] )
					{
						find = false;
						break;
					}
				}

				if ( find == true )
					return i;
			}

			return -1;
		}

		// 查找第一个指定字符串，off表示开始位置, count表示匹配几个字符
		int nocaseFindFirstOf( const char* text, int off, size_t count ) const
		{
			for ( size_t i = off; i < mCount; i ++ )
			{
				bool tFind = true;
				size_t tIndex = 0;
				for ( size_t j = i; j < mCount && tIndex < min( strlen( text ), count ); j ++, tIndex ++ )
				{
					if ( tolower( mpText[ j ] ) != tolower( text[ tIndex ] ) )
					{
						tFind = false;
						break;
					}
				}

				if ( tFind == true )
					return i;
			}

			return -1;
		}

		// 查找最后一个指定字符串，off表示结束位置
		int findLastOf( const char* text, int off = 0 ) const
		{
			return findLastOf( text, off, strlen( text ) );
		}

		// 查找最后一个指定字符串，off表示结束位置
		int nocaseFindLastOf( const char* text, int off = 0 ) const
		{
			return nocaseFindLastOf( text, off, strlen( text ) );
		}

		// 查找最后一个指定字符串，off表示结束位置, count表示匹配几个字符
		int findLastOf( const char* text, int off, size_t count ) const
		{
			for ( int i = mCount - 2; i >= off; i -- )
			{
				bool tFind = true;
				size_t tIndex = 0;
				for ( size_t j = i; j <= mCount - 2 && tIndex < min( strlen( text ), count ); j ++, tIndex ++ )
				{
					if ( mpText[ j ] != text[ tIndex ] )
					{
						tFind = false;
						break;
					}
				}

				if ( tFind == true )
					return i;
			}

			return -1;
		}

		// 查找最后一个指定字符串，off表示结束位置, count表示匹配几个字符
		int nocaseFindLastOf( const char* text, int off, size_t count ) const
		{
			for ( int i = mCount - 2; i >= off; i -- )
			{
				bool tFind = true;
				size_t tIndex = 0;
				for ( size_t j = i; j <= mCount - 2 && tIndex < min( strlen( text ), count ); j ++, tIndex ++ )
				{
					if ( tolower( mpText[ j ] ) != tolower( text[ tIndex ] ) )
					{
						tFind = false;
						break;
					}
				}

				if ( tFind == true )
					return i;
			}

			return -1;
		}

		// 查找第一个非指定字符串，off表示开始位置
		int findFirstNotOf( const char* text, int off = 0 ) const
		{
			return findFirstNotOf( text, off, strlen( text ) );
		}

		// 查找第一个非指定字符串，off表示开始位置
		int nocaseFindFirstNotOf( const char* text, int off = 0 ) const
		{
			return nocaseFindFirstNotOf( text, off, strlen( text ) );
		}

		// 查找第一个非指定字符串，off表示开始位置，count表示匹配个数
		int findFirstNotOf( const char* text, int off, size_t count ) const
		{
			for ( size_t i = off; i < mCount; i ++ )
			{
				bool tFind = true;
				size_t tIndex = 0;
				for ( size_t j = i; j < mCount && tIndex < strlen( text ) && tIndex < count; j ++, tIndex ++ )
				{
					if ( mpText[ j ] != text[ tIndex ] )
					{
						tFind = false;
						break;
					}
				}

				if ( tFind == false )
					return i;
			}

			return -1;
		}

		// 查找第一个非指定字符串，off表示开始位置，count表示匹配个数
		int nocaseFindFirstNotOf( const char* text, int off, size_t count ) const
		{
			for ( size_t i = off; i < mCount; i ++ )
			{
				bool tFind = true;
				size_t tIndex = 0;
				for ( size_t j = i; j < mCount && tIndex < strlen( text ) && tIndex < count; j ++, tIndex ++ )
				{
					if ( tolower( mpText[ j ] ) != tolower( text[ tIndex ] ) )
					{
						tFind = false;
						break;
					}
				}

				if ( tFind == false )
					return i;
			}

			return -1;
		}
	};

	// *******************************************************************	//
	// CNSMapTomb															//
	// *******************************************************************	//
	template < typename KEY, typename T, typename Allocator >
	class CNSMapTomb
	{
	protected:
		CNSMap< KEY, T >&	mMap;
		CNSVector< KEY >	mTombs;

	public:
		CNSMapTomb( CNSMap< KEY, T >& rMap ) : mMap( rMap )
		{
		}

		~CNSMapTomb( )
		{
			for ( unsigned int i = 0; i < mTombs.getCount( ); i ++ )
				mMap.erase( mTombs[ i ] );
		}

	public:
		void pushTomb( KEY vKey )
		{
			mTombs.pushback( vKey );
		}
	};

};
