#include <fbbase.h>

CFBLock::CFBLock( HLOCK pLock ) : mLock( pLock )
{
	pthread_mutex_lock( (pthread_mutex_t*) mLock );
}

CFBLock::~CFBLock( )
{
	pthread_mutex_unlock( (pthread_mutex_t*) mLock );
}

namespace FBBase
{

HLOCK CreateLock( )
{
	pthread_mutex_t* tpSection = NEW pthread_mutex_t;
	pthread_mutex_init( tpSection, NULL );
	return tpSection;
}

void DestroyLock( HLOCK pLock )
{
	pthread_mutex_t* tpSection = (pthread_mutex_t*) pLock;
	pthread_mutex_destroy( tpSection );
	DELETEPTR tpSection;
}

};
