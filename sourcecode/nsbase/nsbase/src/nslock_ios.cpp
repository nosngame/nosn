#include <nsbase.h>
#include <pthread.h>
namespace NSBase
{
    CNSLock::CNSLock( HLOCK pLock ) : mLock( pLock )
    {
        pthread_mutex_lock( (pthread_mutex_t*) mLock );
    }
    
    CNSLock::~CNSLock( )
    {
        pthread_mutex_unlock( (pthread_mutex_t*) mLock );
    }

    HLOCK CreateLock( )
    {
        pthread_mutex_t* tpSection = new pthread_mutex_t;
        pthread_mutex_init( tpSection, NULL );
        return tpSection;
    }

    void DestroyLock( HLOCK pLock )
    {
        pthread_mutex_t* tpSection = (pthread_mutex_t*) pLock;
        pthread_mutex_destroy( tpSection );
        delete tpSection;
    }
};
