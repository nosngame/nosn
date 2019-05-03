#pragma once
namespace NSNav
{
	class CNSPathNode
	{
	public:
		CNSVector2	mPathPos;
		CNSNavNode*	mPathNode;

	public:
		CNSPathNode( const CNSVector2& pos, CNSNavNode* node )
		{
			mPathPos = pos;
			mPathNode = node;
		}
	};
}
