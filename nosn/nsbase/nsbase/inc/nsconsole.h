#pragma once
namespace NSConsole
{
	void setConsoleTitle( const CNSString& title );

	// �������Կ���̨
	void init( );

	// ��������̨
	void exit( );

	// �󶨴���
	void setHostWnd( HWND wnd );

	// ����Lua����, ����ӿ���Ϊ��unity�������lua�õ�
	// ��Ϊ����lua������������ڵ���״̬, �ᵼ��unity��Ŀ�����Ƚ���
	// ���������������п��Թرյ���, �������֮���ٿ�������
	void enableDebug( bool enable );

	// ��ʾ����̨
	void showConsole( );

	// ���lua�ļ�����
	void addLuaFile( const CNSString& fileName, const CNSString& buffer );

	// ǿ�ƶϵ�
	void interrupt( );
	void consoleExceptionHandler( const NSBase::CNSString& exception );
	void consoleLogHandler( const NSBase::CNSString& exception );
}
