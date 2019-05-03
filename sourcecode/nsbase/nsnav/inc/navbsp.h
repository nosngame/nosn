#pragma once

namespace NSNav
{
	class CNSNavBsp
	{
	private:
		CNSRect						mBox;
		CNSVector< CNSVector2 >		mVertices;

	private:
		CNSMap< unsigned int, CNSNavNode* > mShapes;
		CNSNavBsp*	mBspNode[ 4 ];

	public:
		CNSNavBsp( const CNSRect& rc )
		{
			mBox = rc;
			mBspNode[ 0 ] = NULL;
			mBspNode[ 1 ] = NULL;
			mBspNode[ 2 ] = NULL;
			mBspNode[ 3 ] = NULL;
		}

	public:
		~CNSNavBsp( )
		{
			delete mBspNode[ 0 ];
			delete mBspNode[ 1 ];
			delete mBspNode[ 2 ];
			delete mBspNode[ 3 ];
		}

		float getWidth( ) const;
		float getHeight( ) const;
		void addNode( CNSNavNode* node );
		CNSNavNode* pointInNav( const CNSVector2& pos );
		void addVertice( const CNSVector2& v )
		{
			mVertices.pushback( v );
		}

		const CNSVector2& operator[] ( int index ) const
		{
			return mVertices[ index ];
		}

		void buildEdge( );

	private:
		bool nodeInRect( CNSNavNode* node, const CNSRect& rc );
	};
}