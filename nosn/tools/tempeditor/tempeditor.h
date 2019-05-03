#pragma once

class CTempApp : public NSWin32::CNSBaseApp
{
public:
	CTempApp( bool enableDebug ) : CNSBaseApp( "tempeditor", enableDebug )
	{
	}

public:
	virtual void onInitApp( );
	virtual void onExitApp( );

public:
	void registerLua( );
};
