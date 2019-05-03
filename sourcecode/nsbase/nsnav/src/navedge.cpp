#include <nsbase.h>
#include "nav.h"

namespace NSNav
{
	CNSNavEdge::CNSNavEdge( CNSNavBsp* pLayer, unsigned int p1, unsigned int p2 ) : mpBspLayer( pLayer ), mSegmentDirty( true )
	{
		mPtIndex[0] = p1;
		mPtIndex[1] = p2;
	}

	CNSSegment& CNSNavEdge::getSegment( ) const
	{
		if ( mSegmentDirty == true )
		{
			mSegment = CNSSegment( (*mpBspLayer)[ mPtIndex[ 0 ] ], (*mpBspLayer)[ mPtIndex[ 1 ] ] );
			mSegmentDirty = false;
		}

		return mSegment;
	}
}
