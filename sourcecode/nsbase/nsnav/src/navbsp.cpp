#include <nsbase.h>
#include "nav.h"
namespace NSNav
{
	#define BSPDEPTH		16

	float CNSNavBsp::getWidth( ) const
	{
		return mBox.w;
	}

	float CNSNavBsp::getHeight( ) const
	{
		return mBox.h;
	}

	void CNSNavBsp::addNode( CNSNavNode* node )
	{
		CNSVector< CNSRect > rcList;
		mBox.splitRect( rcList );
		bool used[ 4 ] = { false, false, false, false };
		for ( int t = 0; t < 4; t ++ )
		{
			if ( used[ t ] == false && nodeInRect ( node, rcList [t] ) == true )
			{
				used[ t ] = true;
				CNSNavBsp* bsp = mBspNode[ t ];
				if ( bsp == NULL )
				{
					bsp = new CNSNavBsp( rcList[t] );
					mBspNode[ t ] = bsp;
					bsp->mShapes.insert( node->mNodeID, node );
					continue;
				}

				if ( bsp != NULL && bsp->mBox.w <= BSPDEPTH )
				{
					bsp->mShapes.insert( node->mNodeID, node );
					continue;
				}

				if ( bsp != NULL )
				{
					bsp->addNode( node );
					HLISTINDEX beginIndex = bsp->mShapes.getHead( );
					for ( ; beginIndex != NULL; bsp->mShapes.getNext( beginIndex ) )
					{
						CNSNavNode* subNode = bsp->mShapes.getValue( beginIndex );
						bsp->addNode( subNode );
					}

					bsp->mShapes.clear ( );
					continue;
				}
			}
		}
	}

	CNSNavNode* CNSNavBsp::pointInNav( const CNSVector2& pos )
	{
		for (int t = 0; t < 4; t ++) {
			if ( mBspNode[ t ] != NULL && mBspNode[ t ]->mBox.ptInRect ( pos ) == true)
				return mBspNode[ t ]->pointInNav( pos );
		}

		HLISTINDEX beginIndex = mShapes.getHead( );
		for ( ; beginIndex != NULL; mShapes.getNext( beginIndex ) )
		{
			CNSNavNode* subNode = mShapes.getValue( beginIndex );
			if ( subNode->ptInNode( pos ) == true )
				return subNode;
		}

		return NULL;
	}

	void CNSNavBsp::buildEdge( )
	{
		// 只有导航面bsp才需要添加到寻路图中
		for ( int i = 0; i < 4; i ++ )
		{
			if ( mBspNode[ i ] != NULL )
				mBspNode[ i ]->buildEdge( );
		}
	
		CNSVector< CNSNavNode* > nodeList;
		HLISTINDEX beginIndex = mShapes.getHead( );
		for ( ; beginIndex != NULL; mShapes.getNext( beginIndex ) )
			nodeList.pushback( mShapes.getValue( beginIndex ) );

		for ( unsigned int i = 0; i < nodeList.getCount( ); i ++ )
		{
			for ( unsigned int j = i + 1; j < nodeList.getCount( ); j ++ )
			{
				CNSNavNode* tpNode1 = nodeList[ i ];
				CNSNavNode* tpNode2 = nodeList[ j ];

				CNSNavEdge edge;
				if ( tpNode1->relation( tpNode2, edge ) )
				{
					for (int t = 0; t < 3; t ++) 
					{
						if (tpNode1->mNodeSib [t] == NULL ) 
						{
							tpNode1->mNodeSib [t] = tpNode2;
							tpNode1->mEdge [t] = edge;
							break;
						}
					}

					for (int t = 0; t < 3; t ++ )
					{
						if ( tpNode2->mNodeSib [t] == NULL )
						{
							tpNode2->mNodeSib [t] = tpNode1;
							tpNode2->mEdge [t] = edge;
							break;
						}
					}
				}
			}
		}
	}

	bool CNSNavBsp::nodeInRect( CNSNavNode* node, const CNSRect& rc )
	{
		for (int i = 0; i < 3; i ++) 
		{
			if ( rc.ptInRect( node->getVector( i ) ) == true )
				return true;
		}

		CNSVector2 pt[4];
		pt [0].setX( rc.x );
		pt [0].setZ( rc.y );
		pt [1].setX( rc.x );
		pt [1].setZ( rc.y + rc.h );
		pt [2].setX( rc.x + rc.w );
		pt [2].setZ( rc.y + rc.h );
		pt [3].setX( rc.x + rc.w );
		pt [3].setZ( rc.y );
		for (int i = 0; i < 4; i ++) 
		{
			if ( node->ptInNode( pt[i] ) == true )
				return true;
		}

		for (int i = 0; i < 3; i ++) {
			for (int t = 0; t < 4; t ++) {
				int s1 = i;
				int e1;
				if ( s1 == 2 )
					e1 = 0;
				else
					e1 = i + 1;

				int s2 = t;
				int e2;
				if ( s2 == 3 )
					e2 = 0;
				else
					e2 = t + 1;

				CNSSegment edge( node->getVector( s1 ), node->getVector( e1 ) );
				if ( edge.intersectLine ( CNSSegment( pt[ s2 ], pt[ e2 ] ) ) == true )
					return true;
			}
		}

		return false;
	}
}
