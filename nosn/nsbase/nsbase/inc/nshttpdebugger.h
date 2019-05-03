#pragma once
namespace NSHttpDebugger
{
	typedef CNSString (* FDebuggerProc )(  
			const CNSString&,
			TSessionID sessionID,
			const CNSString&,
			const CNSVector< CNSString >&
			);


	void init( const CNSString& name, const CNSString& address );
	void exit( );
	void setDebuggerHandler( FDebuggerProc proc );
	CNSString getAddress( const CNSString& name );
}
