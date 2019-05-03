
// dbtools.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"		// ������
#include "ConsoleDlg.h"
#include <nsbase.h>
#include "common.h"

// CdbtoolsApp:
// �йش����ʵ�֣������ dbtools.cpp
//
class CGroupData
{
public:
	unsigned int	mIndex;
	unsigned int	mRetryCount;
	CNSString		mGroupName;
	CNSString		mGroupID;

public:
	CGroupData( unsigned int index = 0, const CNSString& groupName = "", const CNSString& groupID = "" ) : mIndex( index ), mRetryCount( 0 ), mGroupName( groupName ), mGroupID( groupID )
	{
	}
};

class CMenuCallback
{
public:
	CNSString			mCallback;
	CNSOctetsStream		mStream;
	UINT				mMenuID;
};

class CModule
{
public:
	CNSString		mName;
	CNSString		mDesc;
};

class CdbtoolsApp : public CWinApp
{
public:
	CdbtoolsApp();

	HACCEL				mAccel;  
	CString				mCurLog;
	CConsoleDlg*		console;
	HMODULE				resModule;
	CNSMap< CNSString, HGLOBAL >			mResCache;
	CNSMap< CNSString, HIMAGELIST >			mImageListCache;
	CNSMap< CNSString, HBITMAP >			mBitmapCache;
	CNSMap< CNSString, HICON >				mIconCache;
	CNSMap< CNSString, HCURSOR >			mCursorCache;
	CNSMap< CNSString, CGroupData >			mGroupData;
	CNSMap< unsigned int, CMenuCallback >	mMenuCallback;
	CNSVector< CModule >					mModuleList;
	CNSMap< unsigned int, CNSString >		mLangMenuID;

	HGLOBAL		GetResource( const CNSString& resName, LPTSTR resType );
	HIMAGELIST	GetImageList( const CNSString& resName );
	HBITMAP		GetBitmap( const CNSString& resName );
	HICON		GetIcon( const CNSString& resName );
	HCURSOR		GetCursor( const CNSString& resName );

public:
	static CString& getUnicodeLangText( int textID )
	{
		CNSString	text	= Common::getLangText( textID );
		text.Replace( "\\r", "\r" );
		text.Replace( "\\n", "\n" );
		text.Replace( "\\t", "\t" );
		CNSOctets&	uniText = CNSString::Utf8ToUnicode( text );

		static CString	unicode;
		unicode.Empty( );
		unicode = (wchar_t*) uniText.Begin( );
		return unicode;
	}

// ��д
public:
	virtual BOOL InitInstance( );
	virtual BOOL ExitInstance( );
	afx_msg void OnShowConsole( );
	virtual BOOL PreTranslateMessage( MSG* msg );

// ʵ��
	DECLARE_MESSAGE_MAP()
};

extern CdbtoolsApp theApp;