#pragma once

class CDialogBase : public CDialog
{
public:
	static CNSMap< HWND, CNSOctetsStream >	wndParams;
	static CProgressDlg*					progressDlg;

protected:
	CNSString mWindowProc;
	CNSString mPreWinProc;
	CNSOctetsStream mStream;
	bool	mIsModeless;
	bool	mIsFrameWnd;

public:
	CDialogBase( bool modeless, const CNSString& windowProc, const CNSString& preWinProc, const CNSOctetsStream& stream );

public:
	virtual BOOL OnInitDialog( );
	virtual void PostNcDestroy( );
	virtual void OnCancel( );
	virtual BOOL PreTranslateMessage( MSG* msg );
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
};

HWND lua_tohwnd( lua_State* lua, unsigned int index, const CString& classType );
CRect lua_torect( lua_State* lua, unsigned int index );

// 这个函数会填充text, 包括0结束也会被填充
void windowText( HWND wnd, TCHAR* text, int len );