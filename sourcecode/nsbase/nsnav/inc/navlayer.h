#pragma once
namespace NSNav
{
	class CNSNavLayer
	{
	public:
		CNSMap< unsigned int, CNSNavNode* > openList;
		CNSMap< unsigned int, CNSNavNode* > closeList;
		CNSVector< CNSPathNode >			mPathNode;
		CNSNavNode*							mBestNode;
		CNSNavBsp*							mpBspNav;		// 导航寻路bsp
		CNSMap< CNSString, CNSNavBsp* >		mBsp;			// 其他逻辑需要的bsp, 比如可pk区域等
		float*								mpHeightMap;
		unsigned short						mWidth;
		unsigned short						mHeight;

	public:
		CNSNavLayer( unsigned short w, unsigned short h ) : mBestNode( NULL ), mpHeightMap( NULL ), mWidth( w ), mHeight( h )
		{
		}
	
		~CNSNavLayer( )
		{
			HLISTINDEX beginIndex = mBsp.getHead( );
			for ( ; beginIndex != NULL; mBsp.getNext( beginIndex ) )
				delete mBsp.getValue( beginIndex );

			delete [] mpHeightMap;
		}

	public:
		static CNSNavLayer* create( const char* navFile, bool buildNav );
		static void loadBsp( CNSNavLayer* layer, const CNSString& navName, const CNSOctetsStream& stream, unsigned short w, unsigned short h, bool buildNav );
		static void loadHeightMap( CNSNavLayer* layer, const CNSString& navName, const CNSOctetsStream& stream, unsigned short w, unsigned short h );

		bool pointInNav( const CNSString& navName, const CNSVector2& v )
		{
			CNSNavBsp** tpBspRef = mBsp.get( navName );
			if ( tpBspRef == NULL )
				return false;

			return (*tpBspRef)->pointInNav( v ) != NULL;
		}

		inline unsigned short getWidth( ) const
		{
			return mWidth;
		}

		inline unsigned short getHeight( ) const
		{
			return mHeight;
		}

		inline float sampleHeight( unsigned short x, unsigned short z ) const
		{
			if ( mpHeightMap == NULL )
				return 0.0f;

			unsigned int index = x * (int) getHeight( ) + z;
			if ( index > ( (unsigned int) getWidth( ) ) * getHeight( ) )
				return 0.0f;

			return mpHeightMap[ index ];
		}

		bool isBlock( const CNSVector2& pos );
		void findPath( const CNSVector2& src, const CNSVector2& des, bool needPath );
		CNSVector< CNSPathNode >& getPath( )
		{
			return mPathNode;
		}

	private:
		void createHeightMap( unsigned short w, unsigned short h, void* pBuffer )
		{
			int size = (unsigned int) w * (unsigned int) h;
			mpHeightMap = new float[ size ];
			NSFunction::memcpy_fast( mpHeightMap, pBuffer, size * sizeof( float ) );
		}

		void createBsp( const CNSString& navName, unsigned short w, unsigned short h )
		{
			CNSNavBsp* tpBsp = new CNSNavBsp( CNSRect( 0, 0, w, h ) );
			addBsp( navName, tpBsp );
		}

		void addVertice( const CNSString& navName, const CNSVector2& v )
		{
			CNSNavBsp** tpBspRef = mBsp.get( navName );
			if ( tpBspRef == NULL )
				return;

			(*tpBspRef)->addVertice( v );
		}

		void addBsp( const CNSString& name, CNSNavBsp* bsp )
		{
			mBsp.insert( name, bsp );
			if ( name == "nav" )
				mpBspNav = bsp;
		}
	
		void addOpenList( CNSNavNode* node );
		CNSNavNode* findBestNode( );
		// 判断直线是否穿透所有节点(从srcNode到desNode, navnode之间通过mPrev,mNext链接)
		bool lineCross( const CNSVector2& src, const CNSVector2& des, CNSNavNode* srcNode, CNSNavNode* desNode );
		// 寻找拐点
		void fillPath( const CNSVector2& src, const CNSVector2& des, CNSNavNode* srcNode, CNSNavNode* desNode );
		// 添加节点到寻路图中
		void addNode( const CNSString& bspName, unsigned int nodeID, unsigned short indices1, unsigned short indices2, unsigned short indices3 );
	};
}
