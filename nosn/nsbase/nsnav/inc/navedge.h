#pragma once
namespace NSNav
{
	class CNSNavEdge
	{
	public:
		unsigned int mPtIndex[2];
		mutable CNSSegment	mSegment;
		mutable bool		mSegmentDirty;
		CNSNavBsp*			mpBspLayer;

	public:
		CNSNavEdge( CNSNavBsp* pLayer = NULL, unsigned int p1 = -1, unsigned int p2 = -1 );
		CNSSegment& getSegment( ) const;
	};
}