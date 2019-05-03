#pragma once

namespace NSNav
{
	class CNSNavNode
	{
		friend class CNSNavBsp;

	private:
		unsigned int		mPtIndex[3];
		CNSNavBsp*			mBspRoot;
		CNSNavBsp*			mpBspLayer;

	public:
		CNSNavNode*			mNodeSib[ 3 ];
		CNSNavEdge			mEdge[ 3 ];
		unsigned int		mNodeID;
		float				mFValue;
		CNSNavNode*			mPrev;
		CNSNavNode*			mNext;
		mutable CNSVector2	mCenter;
		mutable bool		mCenterDirty;

	public:
		CNSNavNode( CNSNavBsp* pBspLayer, unsigned int nodeID, unsigned int p1, unsigned int p2, unsigned int p3 ) : mpBspLayer( pBspLayer ), mPrev( NULL ), mNext( NULL ), mFValue( 0 ), mCenterDirty( true ), mBspRoot( NULL )
		{
			mNodeID = nodeID;
			mPtIndex[ 0 ] = p1;
			mPtIndex[ 1 ] = p2;
			mPtIndex[ 2 ] = p3;
		}

		const CNSVector2& getVector( int index ) const;

		CNSVector2 getCenter( ) const;

		void reset( );

		// 得到相连边
		CNSNavEdge* findConnectEdge( CNSNavNode* node );

		// 查找距离指定点最近的点
		float findCloset( const CNSVector2& src );

		// 计算评估值
		float computeValue( const CNSVector2& src, const CNSVector2& des );

		float getFValue( );

	public:
		// 判断以指定节点关系，如果相连，edge是相连边
		bool relation( CNSNavNode* node, CNSNavEdge& edge );

		// 判断指定坐标是否在节点范围内
		bool ptInNode( CNSVector2 pos );
	};
}