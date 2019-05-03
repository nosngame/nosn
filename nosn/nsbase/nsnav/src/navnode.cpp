#include <nsbase.h>
#include "nav.h"
namespace NSNav
{

	const CNSVector2& CNSNavNode::getVector( int index ) const
	{
		if ( index < 0 || index > 2 )
			return CNSVector2::zero;

		return (*mpBspLayer)[ mPtIndex[ index ] ];
	}

	CNSVector2 CNSNavNode::getCenter( ) const
	{
		if ( mCenterDirty == true )
		{
			CNSVector2 p1 = (*mpBspLayer)[ mPtIndex[ 0 ] ];
			CNSVector2 p2 = (*mpBspLayer)[ mPtIndex[ 1 ] ];
			CNSVector2 p3 = (*mpBspLayer)[ mPtIndex[ 2 ] ];
			mCenter = CNSVector2( ( p1.getX( ) + p2.getX( ) + p3.getX( ) ) / 3.0f, ( p1.getZ( ) + p2.getZ( ) + p3.getZ( ) ) / 3.0f );
			mCenterDirty = false;
		}

		return mCenter;
	}

	void CNSNavNode::reset( )
	{
		mFValue = 0;
		mPrev = NULL;
		mNext = NULL;
	}

	// 得到相连边
	CNSNavEdge* CNSNavNode::findConnectEdge( CNSNavNode* node )
	{
		for (int i = 0; i < 3; i ++) 
		{
			if ( mNodeSib[ i ] == node )
				return &mEdge[ i ];
		}
		
		return NULL;
	}

	// 查找距离指定点最近的点
	float CNSNavNode::findCloset( const CNSVector2& src )
	{
		CNSVector2 p1 = (*mpBspLayer)[ mPtIndex[ 0 ] ];
		CNSVector2 p2 = (*mpBspLayer)[ mPtIndex[ 1 ] ];
		CNSVector2 p3 = (*mpBspLayer)[ mPtIndex[ 2 ] ];
		float t1 = (p1 - src).magnitude( );
		float t2 = (p2 - src).magnitude( );
		float t3 = (p3 - src).magnitude( );
		float value = FLT_MAX;
		if ( t1 < value ) 
			value = t1;
		
		if ( t2 < value ) 
			value = t2;
		
		if ( t3 < value ) 
			value = t3;
		
		return value;
	}

	// 计算评估值
	float CNSNavNode::computeValue( const CNSVector2& src, const CNSVector2& des )
	{
		CNSVector2 p1;
		CNSVector2 p2;
		float v1 = findCloset( src );
		float v2 = findCloset( des );
		return v1 + v2;
	}

	float CNSNavNode::getFValue( )
	{
		return mFValue;
	}

	// 判断以指定节点关系，如果相连，edge是相连边
	bool CNSNavNode::relation( CNSNavNode* node, CNSNavEdge& edge )
	{
		int pairCount = 0;
		bool used[3];
		for (int i = 0; i < 3; i ++) 
		{
			if ( pairCount == 2 )
				break;

			for ( int t = 0; t < 3; t ++ )
			{
				if ( used[ t ] == false && mPtIndex[ i ] == node->mPtIndex[ t ] )
				{
					edge.mPtIndex[ pairCount ] = mPtIndex[ i ];
					pairCount ++;
					used[ t ] = true;
					break;
				}
			}
		}

		if (pairCount == 0)
			return false;

		if (pairCount == 1)
			return false;

		return true;
	}

	// 判断指定坐标是否在节点范围内
	bool CNSNavNode::ptInNode( CNSVector2 pos )
	{
		CNSVector2 ptStart	= CNSVector2::zero;
		CNSVector2 ptEnd 	= CNSVector2::zero;
		int cross 		= 0;
		for ( int i = 0; i < 3; i ++ )
		{
			ptStart = (*mpBspLayer)[ mPtIndex[ i ] ];
			if ( i == 2 )
				ptEnd = (*mpBspLayer)[ mPtIndex[ 0 ] ];
			else
				ptEnd = (*mpBspLayer)[ mPtIndex[ i + 1 ] ];
			
			// 邻的两个点是平行于x轴的，肯定不相交，忽略  
			if ( ptStart.getZ( ) != ptEnd.getZ( ) )
			{
				// 交点在pStart,pEnd的延长线上，pCur肯定不会与pStart.pEnd相交，忽略  
				if ( pos.getZ( ) >= min( ptStart.getZ( ), ptEnd.getZ( ) ) && pos.getZ( ) <= max( ptStart.getZ( ), ptEnd.getZ( ) ) )
				{
					// 求当前点和x轴的平行线与pStart,pEnd直线的交点的x坐标  
					float x = (pos.getZ( ) - ptStart.getZ( ) ) * (ptEnd.getX( ) - ptStart.getX( ) ) / (ptEnd.getZ( ) - ptStart.getZ( ) ) + ptStart.getX( );
								
					// 若x坐标大于当前点的坐标，则有交点  
					if ( x >= pos.getX( ) )
						cross = cross + 1;
				}
			}
		}

		return (cross % 2) == 1;
	}
}