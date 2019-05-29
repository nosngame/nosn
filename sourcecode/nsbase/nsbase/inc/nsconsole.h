#pragma once
namespace NSConsole
{
	void setConsoleTitle( const CNSString& title );

	// 开启调试控制台
	void init( );

	// 结束控制台
	void exit( );

	// 绑定窗口
	void setHostWnd( HWND wnd );

	// 开启Lua调试, 这个接口是为了unity插件调试lua用的
	// 因为加载lua过程中如果处于调试状态, 会导致unity项目启动比较慢
	// 所以在启动过程中可以关闭调试, 启动完毕之后再开启调试
	void enableDebug( bool enable );

	// 显示控制台
	void showConsole( );

	// 添加lua文件数据
	void addLuaFile( const CNSString& fileName, const CNSString& buffer );

	// 强制断点
	void interrupt( );
	void consoleExceptionHandler( const NSBase::CNSString& exception );
	void consoleLogHandler( const NSBase::CNSString& exception );
}
