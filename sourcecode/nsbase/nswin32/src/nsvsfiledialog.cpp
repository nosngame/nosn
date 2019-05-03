#include <nsbase.h>
#include <Shlwapi.h>

namespace NSWin32
{
	HBRUSH CVsFileDialog::backBrush = NULL;
	CVsFileDialog::CVsFileDialog( const CNSString& windowID, unsigned int style ) : CNSFrame( windowID ), mStyle( (EType) style )
	{
	}

	void CVsFileDialog::init( )
	{
		backBrush = CreateSolidBrush( RGB( 45, 45, 48 ) );
	}

	void CVsFileDialog::exit( )
	{
		if (backBrush != NULL)
			DeleteObject( backBrush );
	}

	void CVsFileDialog::regLuaLib( )
	{
		NSBase::CNSLuaStack& luaStack = NSBase::CNSLuaStack::getLuaStack( );
		luaStack.newTable( );
		luaStack.pushField( "DIRECTORY_FILEDIALOG", CVsFileDialog::DIRECTORY_FILEDIALOG );
		luaStack.pushField( "FILE_FILEDIALOG", CVsFileDialog::FILE_FILEDIALOG );
		luaStack.setGlobalTable( "fileDialogStyle" );
	}

	bool CVsFileDialog::onFileDialogPaint( CNSWindow* child, WPARAM wParam, LPARAM lParam, LRESULT* result )
	{
		PAINTSTRUCT ps;
		BeginPaint( child->getHWnd( ), &ps );

		RECT rc;
		GetClientRect( child->getHWnd( ), &rc );
		rc.top = rc.bottom - 70;
		FillRect( ps.hdc, &rc, backBrush );

		EndPaint( child->getHWnd( ), &ps );
		return true;
	}

	bool CVsFileDialog::onFilterSelectChanged( CNSWindow* child, WPARAM wParam, LPARAM lParam, LRESULT* result )
	{
		NSWin32::CComboBox* filterCombo = (NSWin32::CComboBox*) child;
		if ( filterCombo == NULL )
			return true;

		CVsFileDialog* fileDialog = (CVsFileDialog*) child->getParent( );
		if ( fileDialog == NULL )
			return true;

		NSWin32::CVsFileBrowser* browser = (NSWin32::CVsFileBrowser*) CNSWindow::getWindow( fileDialog->mWindowID + "_browser" );
		if ( browser == NULL )
			return true;

		int index = filterCombo->getCurSel( );
		CFilter* filter = fileDialog->mFilters.get( index );
		if ( filter == NULL )
			return true;

		browser->setFilter( CVsFileBrowser::EFilterType::FilterFile, filter->mFilter );
		browser->refresh( );
		return true;
	}

	bool CVsFileDialog::onClickButtonOk( CNSWindow* child, WPARAM wParam, LPARAM lParam, LRESULT* result )
	{
		CVsFileDialog* fileDialog = (CVsFileDialog*) child->getParent( );
		if ( fileDialog == NULL )
			return true;

		NSWin32::CVsFileBrowser* browser = (NSWin32::CVsFileBrowser*) CNSWindow::getWindow( fileDialog->mWindowID + "_browser" );
		if ( browser == NULL )
			return true;

		CVsFileDialog::CResult* dlgResult = (CVsFileDialog::CResult*) fileDialog->mResult;
		if ( dlgResult != NULL )
		{
			// 填充选着结果
			dlgResult->mCurPath	= browser->getCurPath( );
			browser->getSelectFile( dlgResult->mSelectFiles );

			// 如果是文件选择, 需要处理选中文件夹逻辑
			if ( fileDialog->mStyle == EType::FILE_FILEDIALOG )
			{
				if ( dlgResult->mSelectFiles.getCount( ) > 0 )
				{
					CNSString filePath = dlgResult->mCurPath + "\\" + dlgResult->mSelectFiles[ 0 ];
					DWORD attrib = GetFileAttributes( CNSString::toTChar( filePath ) );

					// 如果选中的是一个目录，那么不能触发窗口结束逻辑
					if ( attrib & FILE_ATTRIBUTE_DIRECTORY )
					{
						browser->setCurPath( filePath );
						return true;
					}
				}
				else
					// 如果没有选中任何文件，那么什么也不能做
					return true;
			}
		}

		fileDialog->closeFrame( CNSFrame::EResultCode::RESULT_OK );
		return true;
	}

	bool CVsFileDialog::onClickButtonCancel( CNSWindow* child, WPARAM wParam, LPARAM lParam, LRESULT* result )
	{
		CVsFileDialog* dialog = (CVsFileDialog*) child->getParent( );
		if ( dialog == NULL )
			return true;

		dialog->closeFrame( CNSFrame::EResultCode::RESULT_CANCEL );
		return true;
	}

	bool CVsFileDialog::onFileBrowserSelectFile( CNSWindow* child, WPARAM wParam, LPARAM lParam, LRESULT* result )
	{
		CVsFileDialog* dialog = (CVsFileDialog*) child->getParent( );
		if ( dialog == NULL )
			return true;

		CComboBox* select = (CComboBox*) NSWin32::CNSWindow::getWindow( dialog->mWindowID + "_select" );
		if ( select == NULL )
			return true;

		NSWin32::CVsFileBrowser* browser = (NSWin32::CVsFileBrowser*) CNSWindow::getWindow( dialog->mWindowID + "_browser" );
		if ( browser == NULL )
			return true;

		CNSVector< CNSString > selectFiles;
		browser->getSelectFile( selectFiles );
		if ( selectFiles.getCount( ) > 0 )
			select->setText( selectFiles[ 0 ] );
		return true;
	}

	void CVsFileDialog::setFilter( int index )
	{
		NSWin32::CComboBox* filterCombo = (NSWin32::CComboBox*) CNSWindow::getWindow( mWindowID + "_filter" );
		if ( filterCombo == NULL )
			return;

		NSWin32::CVsFileBrowser* browser = (NSWin32::CVsFileBrowser*) CNSWindow::getWindow( mWindowID + "_browser" );
		if ( browser == NULL )
			return;

		filterCombo->setCurSel( index );
		CFilter* filter = mFilters.get( index );
		if ( filter == NULL )
			return;

		browser->setFilter( CVsFileBrowser::EFilterType::FilterFile, filter->mFilter );
		browser->refresh( );
	}

	void CVsFileDialog::addFileFilter( const CNSString& desc, const CNSString& filter )
	{
		NSWin32::CComboBox* filterCombo = (NSWin32::CComboBox*) CNSWindow::getWindow( mWindowID + "_filter" );
		if ( filterCombo == NULL )
			return;

		NSWin32::CVsFileBrowser* browser = (NSWin32::CVsFileBrowser*) CNSWindow::getWindow( mWindowID + "_browser" );
		if ( browser == NULL )
			return;

		int index = filterCombo->addItem( desc );
		mFilters.insert( index, CFilter( desc, filter ) );
	}

	CNSString& CVsFileDialog::getCurPath( ) const
	{
		static CNSString curPath;
		NSWin32::CVsFileBrowser* browser = (NSWin32::CVsFileBrowser*) CNSWindow::getWindow( mWindowID + "_browser" );
		if ( browser == NULL )
			return curPath;
		
		curPath = browser->getCurPath( );
		return curPath;
	}

	void CVsFileDialog::setCurPath( const CNSString& curPath )
	{
		NSWin32::CVsFileBrowser* browser = (NSWin32::CVsFileBrowser*) CNSWindow::getWindow( mWindowID + "_browser" );
		if ( browser == NULL )
			return;
		
		browser->setCurPath( curPath );
	}

	void CVsFileDialog::getSelectFile( CNSVector< CNSString >& result ) const
	{
		NSWin32::CVsFileBrowser* browser = (NSWin32::CVsFileBrowser*) CNSWindow::getWindow( mWindowID + "_browser" );
		if ( browser == NULL )
			return;

		browser->getSelectFile( result );
	}
	
	void CVsFileDialog::onPostCreateWindow( CNSWindow* parent )
	{
		CNSFrame::onPostCreateWindow( parent );
		RECT rc;
		GetClientRect( getHWnd( ), &rc );

		RECT rcItem = rc;
		rcItem.top = rcItem.top + 2;
		rcItem.left = rcItem.left + 2;
		rcItem.right = rcItem.right - 2;
		rcItem.bottom = rcItem.bottom - 70;
		CVsFileBrowser* browser = CNSWindow::newFileBrowser( mWindowID + "_browser", 0, rcItem, this );
		browser->mLeftAnchor = CNSWindow::EAnchor::ANCHOR_LEFT;
		browser->mRightAnchor = CNSWindow::EAnchor::ANCHOR_RIGHT;
		browser->mTopAnchor = CNSWindow::EAnchor::ANCHOR_TOP;
		browser->mBottomAnchor = CNSWindow::EAnchor::ANCHOR_BOTTOM;
		browser->registerEvent( CVsFileBrowser::EVsFileBrowserEvent::EventSelect, onFileBrowserSelectFile );

		rcItem = rc;
		rcItem.top = rcItem.bottom - 58;
		rcItem.bottom = rcItem.top + 20;
		rcItem.left = 60;
		rcItem.right = rcItem.left + 57;
		CNSVsStatic* static1 = CNSWindow::newVsStatic( mWindowID + "_static1", 0, rcItem, this );
		static1->mLeftAnchor = CNSWindow::EAnchor::ANCHOR_LEFT;
		static1->mRightAnchor = CNSWindow::EAnchor::ANCHOR_LEFT;
		static1->mTopAnchor = CNSWindow::EAnchor::ANCHOR_BOTTOM;
		static1->mBottomAnchor = CNSWindow::EAnchor::ANCHOR_BOTTOM;
		static1->setText( _UTF8( "文件名(&N):" ) );
		static1->setBkColor( RGB( 45, 45, 48 ) );

		if ( mStyle == EType::FILE_FILEDIALOG )
		{
			rcItem = rc;
			rcItem.top = rcItem.bottom - 60;
			rcItem.bottom = rcItem.top + 300;
			rcItem.left = 120;
			rcItem.right = rcItem.right - 200;
			CComboBox* select = CNSWindow::newComboBox( mWindowID + "_select", CComboBox::EComboType::COMBO_DROPDOWN, rcItem, this );
			select->mLeftAnchor = CNSWindow::EAnchor::ANCHOR_LEFT;
			select->mRightAnchor = CNSWindow::EAnchor::ANCHOR_RIGHT;
			select->mTopAnchor = CNSWindow::EAnchor::ANCHOR_BOTTOM;
			select->mBottomAnchor = CNSWindow::EAnchor::ANCHOR_BOTTOM;

			rcItem = rc;
			rcItem.top = rcItem.bottom - 60;
			rcItem.bottom = rcItem.top + 300;
			rcItem.left = rcItem.right - 193;
			rcItem.right = rcItem.right - 5;
			CComboBox* filter = CNSWindow::newComboBox( mWindowID + "_filter", CComboBox::EComboType::COMBO_DROPLIST, rcItem, this );
			filter->mLeftAnchor = CNSWindow::EAnchor::ANCHOR_RIGHT;
			filter->mRightAnchor = CNSWindow::EAnchor::ANCHOR_RIGHT;
			filter->mTopAnchor = CNSWindow::EAnchor::ANCHOR_BOTTOM;
			filter->mBottomAnchor = CNSWindow::EAnchor::ANCHOR_BOTTOM;
			filter->registerEvent( CComboBox::EComboBoxEvent::EventChanged, onFilterSelectChanged );
			registerMessage( WM_PAINT, onFileDialogPaint );

			rcItem = rc;
			rcItem.top = rcItem.bottom - 30;
			rcItem.bottom = rcItem.top + 23;
			rcItem.left = rcItem.right - 193;
			rcItem.right = rcItem.right - 104;
			CNSVsBtn* btnOK = CNSWindow::newVsBtn( mWindowID + "_ok", CNSVsBtn::EVsButtonStyle::STYLE_PUSHBUTTON | CNSVsBtn::EVsButtonStyle::STYLE_TEXT, rcItem, this );
			btnOK->mLeftAnchor = CNSWindow::EAnchor::ANCHOR_RIGHT;
			btnOK->mRightAnchor = CNSWindow::EAnchor::ANCHOR_RIGHT;
			btnOK->mTopAnchor = CNSWindow::EAnchor::ANCHOR_BOTTOM;
			btnOK->mBottomAnchor = CNSWindow::EAnchor::ANCHOR_BOTTOM;
			btnOK->setText( _UTF8( "确定(&O)" ) );
			btnOK->registerEvent( CNSVsBtn::EVsButtonEvent::EventClicked, onClickButtonOk );

			rcItem = rc;
			rcItem.top = rcItem.bottom - 30;
			rcItem.bottom = rcItem.top + 23;
			rcItem.left = rcItem.right - 94;
			rcItem.right = rcItem.right - 5;
			CNSVsBtn* btnCancel = CNSWindow::newVsBtn( mWindowID + "_cancel", CNSVsBtn::EVsButtonStyle::STYLE_PUSHBUTTON | CNSVsBtn::EVsButtonStyle::STYLE_TEXT, rcItem, this );
			btnCancel->mLeftAnchor = CNSWindow::EAnchor::ANCHOR_RIGHT;
			btnCancel->mRightAnchor = CNSWindow::EAnchor::ANCHOR_RIGHT;
			btnCancel->mTopAnchor = CNSWindow::EAnchor::ANCHOR_BOTTOM;
			btnCancel->mBottomAnchor = CNSWindow::EAnchor::ANCHOR_BOTTOM;
			btnCancel->setText( _UTF8( "取消" ) );
			btnCancel->registerEvent( CNSVsBtn::EVsButtonEvent::EventClicked, onClickButtonCancel );
		}
		else
		{
			rcItem = rc;
			rcItem.top = rcItem.bottom - 60;
			rcItem.bottom = rcItem.top + 300;
			rcItem.left = 120;
			rcItem.right = rcItem.right - 5;
			CComboBox* select = CNSWindow::newComboBox( mWindowID + "_select", CComboBox::EComboType::COMBO_DROPDOWN, rcItem, this );
			select->mLeftAnchor = CNSWindow::EAnchor::ANCHOR_LEFT;
			select->mRightAnchor = CNSWindow::EAnchor::ANCHOR_RIGHT;
			select->mTopAnchor = CNSWindow::EAnchor::ANCHOR_BOTTOM;
			select->mBottomAnchor = CNSWindow::EAnchor::ANCHOR_BOTTOM;

			rcItem = rc;
			rcItem.top = rcItem.bottom - 30;
			rcItem.bottom = rcItem.top + 23;
			rcItem.left = rcItem.right - 210;
			rcItem.right = rcItem.right - 104;
			CNSVsBtn* btnOK = CNSWindow::newVsBtn( mWindowID + "_ok", CNSVsBtn::EVsButtonStyle::STYLE_PUSHBUTTON | CNSVsBtn::EVsButtonStyle::STYLE_TEXT, rcItem, this );
			btnOK->mLeftAnchor = CNSWindow::EAnchor::ANCHOR_RIGHT;
			btnOK->mRightAnchor = CNSWindow::EAnchor::ANCHOR_RIGHT;
			btnOK->mTopAnchor = CNSWindow::EAnchor::ANCHOR_BOTTOM;
			btnOK->mBottomAnchor = CNSWindow::EAnchor::ANCHOR_BOTTOM;
			btnOK->setText( _UTF8( "选择文件夹(&O)" ) );
			btnOK->registerEvent( CNSVsBtn::EVsButtonEvent::EventClicked, onClickButtonOk );

			rcItem = rc;
			rcItem.top = rcItem.bottom - 30;
			rcItem.bottom = rcItem.top + 23;
			rcItem.left = rcItem.right - 94;
			rcItem.right = rcItem.right - 5;
			CNSVsBtn* btnCancel = CNSWindow::newVsBtn( mWindowID + "_cancel", CNSVsBtn::EVsButtonStyle::STYLE_PUSHBUTTON | CNSVsBtn::EVsButtonStyle::STYLE_TEXT, rcItem, this );
			btnCancel->mLeftAnchor = CNSWindow::EAnchor::ANCHOR_RIGHT;
			btnCancel->mRightAnchor = CNSWindow::EAnchor::ANCHOR_RIGHT;
			btnCancel->mTopAnchor = CNSWindow::EAnchor::ANCHOR_BOTTOM;
			btnCancel->mBottomAnchor = CNSWindow::EAnchor::ANCHOR_BOTTOM;
			btnCancel->setText( _UTF8( "取消" ) );
			btnCancel->registerEvent( CNSVsBtn::EVsButtonEvent::EventClicked, onClickButtonCancel );
		}
	}
}