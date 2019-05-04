#include <nsbase.h>
#include "PPExcelParser.h"
#include "FileConfig.h"
#include <iostream>
#include <fstream>
#include "tempeditor.h"

static int convert( lua_State* lua );

BEGIN_EXPORT( TempEditor )
	EXPORT_FUNC( convert )
END_EXPORT

int convert( lua_State* lua )
{
	DECLARE_BEGIN_PROTECTED
	static CNSMap< CNSString, CNSString > path;
	luaStack >> path;
	static CNSString inputPath;
	static CNSString serverPath;
	static CNSString clientPath;
	HMAPINDEX findIndex = path.findNode( "mInputPath" );
	if ( findIndex != NULL )
		inputPath = path.getValueEx( findIndex );

	findIndex = path.findNode( "mOServerPath" );
	if ( findIndex != NULL )
		serverPath = path.getValueEx( findIndex );

	findIndex = path.findNode( "mOClientPath" );
	if ( findIndex != NULL )
		clientPath = path.getValueEx( findIndex );

	FileConfig* fileConfig = FileConfig::GetSingleton( );
	fileConfig->LoadChildFiles( inputPath );

	PPExcelParser* excelParser = PPExcelParser::GetSingleton( );
	CNSVector< CNSString >& inputFiles = fileConfig->GetInputFiles( );
	NSLog::log( _UTF8( "�ļ��б�����" ) );
	for ( size_t i = 0; i < inputFiles.getCount( ); i ++ )
		NSLog::log( inputFiles[ i ] );

	NSLog::log( _UTF8( "�ļ��б���ʾ���" ) );
	if ( excelParser->ParseFiles( inputFiles ) == false )
	{
		NSException( _UTF8( "excel�ļ�����ʧ��" ) );
		return 0;
	}

	CNSOctetsStream server = excelParser->GetServerBuffer( );
	CNSOctets outputServer = server.mBuffer.compress( );

	CNSOctetsStream client = excelParser->GetClientBuffer( );
	CNSOctets outputClient = client.mBuffer.compress( );
	CNSString serverOPath = CNSString( serverPath ) + "/server.dat";
	CNSFile fileServer;
	fileServer.createNew( serverOPath );
	fileServer << outputServer;
	fileServer.close( );

	CNSString clientOPath = CNSString( clientPath ) + "/client.dat";
	CNSFile fileClient;
	fileClient.createNew( clientOPath );
	fileClient << outputClient;
	fileClient.close( );
	NSLog::log( _UTF8( "��ʽ���ļ����" ) );
	DECLARE_END_PROTECTED
}

void CTempApp::registerLua( )
{
	NSBase::CNSLuaStack& luaStack = NSBase::CNSLuaStack::getLuaStack( );
	luaStack.regLib( "TempEditor", TempEditor );
}

void CTempApp::onInitApp( )
{
	// ���ؽű�
	CNSBaseApp::onInitApp( );
	registerLua( );
	NSBase::CNSLuaStack& luaStack = NSBase::CNSLuaStack::getLuaStack( );
	luaStack.load( "assets/script" );

	luaStack.preCall( "onLaunch" );
	if ( luaStack.call( ) == false )
		NSException( _UTF8( "lua����[onLaunch]�����쳣" ) );
}

void CTempApp::onExitApp( )
{
}

int APIENTRY _tWinMain( _In_ HINSTANCE hInstance,
						_In_opt_ HINSTANCE hPrevInstance,
						_In_ LPTSTR    lpCmdLine,
						_In_ int       nCmdShow )
{
	try
	{
		CTempApp app( true );
		app.useIPName( false );
		app.useNSHttp( false );
		app.useNSLocal( false );
		app.useNSHttpDebugger( false );
		app.useNSMysql( false );
		app.run( );
	}
	catch ( CNSException& e )
	{
		static CNSString errorDesc;
		errorDesc.format( _UTF8( "NSLog exception:\n%s\nCRT main���������쳣\n��������: \n\t%s\nC++���ö�ջ:\n%s" ), NSLog::sExceptionText.getBuffer( ),
						  e.mErrorDesc, NSBase::NSFunction::getStackInfo( ).getBuffer( ) );
		MessageBox( NULL, CNSString::toTChar( errorDesc ), _T( "�쳣" ), MB_OK | MB_ICONERROR );
		return 1;
	}
	return 0;
}
