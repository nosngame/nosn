#pragma once

namespace NSBase
{
	typedef void* HLOCK;
	class CNSLock
	{
	public:
		HLOCK	mLock;

	public:
		CNSLock( HLOCK pLock );
		~CNSLock( );
	};

	HLOCK createLock( );
	void destroyLock( HLOCK pLock );
}
