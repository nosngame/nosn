#include <nsbase.h>

namespace NSBase
{

	CNSLock::CNSLock( HLOCK pLock ) : mLock( pLock )
	{
		EnterCriticalSection( (CRITICAL_SECTION*)mLock );
	}

	CNSLock::~CNSLock( )
	{
		LeaveCriticalSection( (CRITICAL_SECTION*)mLock );
	}

	HLOCK createLock( )
	{
		CRITICAL_SECTION* tpSection = new CRITICAL_SECTION;
		::InitializeCriticalSection( tpSection );
		return tpSection;
	}

	void destroyLock( HLOCK pLock )
	{
		CRITICAL_SECTION* tpSection = (CRITICAL_SECTION*)pLock;
		delete tpSection;
	}

}