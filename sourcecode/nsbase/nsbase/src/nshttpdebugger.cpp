#include <nsbase.h>

namespace NSHttpDebugger
{
	class CDebuggerServer : public CNSNetManager
	{
	public:
		CDebuggerServer( const CNSString& name, const CNSString& address )
			: CNSNetManager( name, address, 10, 4096, 16384, true )
		{
		}

	public:
		virtual void onRegisterProtocol( const CNSString& name ) {}
		virtual void onAddSession( const CNSString& name, TSessionID sessionID, const CNSSockAddr& local, const CNSSockAddr& peer ) {}
		virtual void onDelSession( const CNSString& name, TSessionID sessionID ) {}
		virtual void onProtocol( const CNSString& name, CNSProtocol* proto, TSessionID sessionID ) {}
		virtual void onProtocol( const CNSString& name, TSessionID sessionID, const CNSMap< CNSString, CNSString >& headers, const CNSString& text, const CNSString& postData );
	};

	bool				sDebuggerLoaded = false;
	CDebuggerServer*	spHttpDebugger = NULL;
	FDebuggerProc		gDebuggerHandler = NULL;
	CNSString			gHttpReturn;

	CNSString LuaHttpDebug( const CNSString& command, const CNSVector< CNSString >& paramList );
	void CDebuggerServer::onProtocol( const CNSString& name, TSessionID sessionID, const CNSMap< CNSString, CNSString >& headers, const CNSString& text, const CNSString& postData )
	{
		try
		{
			CNSString& tText = text.decodeURI( );
			int tStartIndex = tText.findFirstOf( "(" );
			int tEndIndex = tText.findLastOf( ")", tStartIndex );
			if (tStartIndex > 10)
			{
				close( name, sessionID );
				return;
			}

			if (tStartIndex == -1 || tEndIndex == -1)
			{
				close( name, sessionID );
				return;
			}

			tStartIndex = tStartIndex + 1;
			tEndIndex = tEndIndex - 1;
			static CNSString tCommand;
			tCommand.clear( );
			tText.copy( tCommand, tEndIndex - tStartIndex + 1, tStartIndex );

			static CNSVector< CNSString > stringList;
			stringList.clear( );
			tCommand.split( ":", stringList );

			CNSString cmd = stringList[ 0 ];
			if (gDebuggerHandler != NULL)
			{
				CNSString text = gDebuggerHandler( name, sessionID, cmd, stringList );
				if (text.isEmpty( ) == false)
				{
					CNSString retValue;
					retValue.format( "<h2>%s Control</h2>%s", spHttpDebugger->mName.getBuffer( ), text.getBuffer( ) );
					NSHttp::returnUtf8( name, sessionID, retValue );
					return;
				}
			}

			CNSString debugText = LuaHttpDebug( cmd, stringList );
			if (debugText.isEmpty( ) == false)
			{
				if (debugText == "none")
					return;

				CNSString retValue;
				retValue.format( "<h2>%s Control</h2>%s", spHttpDebugger->mName.getBuffer( ), debugText.getBuffer( ) );
				NSHttp::returnUtf8( name, sessionID, retValue );
				return;
			}

			CNSString retValue;
			retValue.format( "<h2>%s Control</h2>command not support", spHttpDebugger->mName.getBuffer( ) );
			NSHttp::returnUtf8( name, sessionID, retValue );
		}
		catch (CNSException& e)
		{
			CNSString retValue;
			retValue.format( "<h2>%s Control</h2>%s", spHttpDebugger->mName.getBuffer( ), e.mErrorDesc );
			NSHttp::returnUtf8( name, sessionID, retValue );
		}
		return;
	}

	void init( const CNSString& name, const CNSString& address )
	{
		spHttpDebugger = new CDebuggerServer( name, address );
		registerServer( spHttpDebugger, 1 );
		NSLog::log( _UTF8( "http调试接口[%s]成功打开" ), address.getBuffer( ) );
		NSLog::log( _UTF8( "\tbuffsize - %d" ), spHttpDebugger->mBufferSize );
		NSLog::log( _UTF8( "\tstreamsize - %d" ), spHttpDebugger->mStreamSize );
		NSLog::log( _UTF8( "\tinterval - %d" ), spHttpDebugger->mPollInterval );
	}

	void exit( )
	{
		if (spHttpDebugger != NULL)
			delete spHttpDebugger;
	}

	void setDebuggerHandler( FDebuggerProc proc )
	{
		gDebuggerHandler = proc;
	}

	CNSString getAddress( const CNSString& name )
	{
		if (sDebuggerLoaded == false)
		{
			sDebuggerLoaded = true;
			TiXmlDocument doc;
			if (doc.LoadFile( "control.xml" ) == false)
			{
				static CNSString errorDesc;
				errorDesc.format( _UTF8( "control.xml打开错误, 错误码[%s]" ), doc.ErrorDesc( ) );
				NSException( errorDesc );
			}

			TiXmlElement* ele = doc.FirstChildElement( "control" );
			for (; ele != NULL; ele = ele->NextSiblingElement( "control" ))
			{
				CNSString controlName = ele->Attribute( "name" );
				if (controlName.compareNoCase( name ) == true)
					return ele->Attribute( "address" );
			}
		}

		return "local:10001";
	}

	void SetHttpReturn( const CNSString& value )
	{
		gHttpReturn += value;
		gHttpReturn += "<BR>";
	}

	CNSString LuaHttpDebug( const CNSString& command, const CNSVector< CNSString >& paramList )
	{
		NSBase::CNSLuaStack& luaStack = NSBase::CNSLuaStack::getLuaStack( );
		// 得到某个全局变量值
		if (command.compareNoCase( "get" ) == true)
		{
			if (paramList.getCount( ) != 2)
				return "get param is error";

			CNSString varName = paramList[ 1 ];

			TiXmlDocument doc;
			//luaStack.getVarValue( varName, &doc );

			TiXmlPrinter printer;
			printer.SetIndent( "&bnsp" );
			printer.SetLineBreak( "<BR>" );
			static CNSString text;
			text = printer.CStr( );
			if ( text.getLength( ) == 0)
			{
				CNSString retValue;
				retValue.format( "var - %s is not exist", varName.getBuffer( ) );
				return retValue;
			}

			return text;
		}

		// 远程执行一段脚本
		if (command.compareNoCase( "run" ) == true)
		{
			if (paramList.getCount( ) != 2)
				return "run param is error";

			CNSString luaCode = paramList[ 1 ];
			gHttpReturn.clear( );
			luaStack.openScriptFromMemory( "webchunk", luaCode.getBuffer( ), luaCode.getLength( ) );
			if (gHttpReturn.isEmpty( ) == false)
			{
				CNSString tmpValue = gHttpReturn;
				gHttpReturn.clear( );
				return tmpValue;
			}

			return "success";
		}

		return "";
	}

	static int httpTrace( lua_State* lua );

	BEGIN_EXPORT( ControlLua )
		EXPORT_FUNC( httpTrace )
	END_EXPORT

	static int httpTrace( lua_State* lua )
	{
		DECLARE_BEGIN_PROTECTED
		static CNSString text;
		luaStack >> text;
		NSHttpDebugger::SetHttpReturn( text );
		DECLARE_END_PROTECTED
	}
}
