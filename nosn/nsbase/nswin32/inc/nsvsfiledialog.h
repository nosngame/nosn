#pragma once
namespace NSWin32
{
	class CVsFileDialog : public CNSFrame
	{
	protected:
		static HBRUSH backBrush;

	public:
		enum EType
		{
			DIRECTORY_FILEDIALOG,
			FILE_FILEDIALOG
		};

		class CResult : public CNSFrame::CDialogResult
		{
		public:
			CNSString				mCurPath;
			CNSVector< CNSString >	mSelectFiles;

		public:
			virtual CNSLuaStack& marshal( CNSLuaStack& luaStack ) const
			{
				CDialogResult::marshal( luaStack );
				luaStack.pushField( "mCurPath", mCurPath );
				luaStack.pushField( "mSelectFiles", mSelectFiles );
				return luaStack;
			}

			virtual const CNSLuaStack& unmarshal( const CNSLuaStack& luaStack )
			{
				CDialogResult::unmarshal( luaStack );
				luaStack.popField( "mCurPath", mCurPath );
				luaStack.popField( "mSelectFiles", mSelectFiles );
				return luaStack;
			}

		};

	public:
		static void init( );
		static void exit( );
		static void regLuaLib( );

		class CFilter
		{
		public:
			CNSString mDesc;
			CNSString mFilter;

		public:
			CFilter( const CNSString& desc = "", const CNSString& filter = "" ) : mDesc( desc ), mFilter( filter )
			{
			}
		};

	protected:
		CNSMap< int, CFilter >	mFilters;
		EType					mStyle;

	public:
		CVsFileDialog( const CNSString& windowID, unsigned int style );

	public:
		void addFileFilter( const CNSString& desc, const CNSString& filter );
		void setFilter( int index );
		CNSString& getCurPath( ) const;
		void setCurPath( const CNSString& curPath );
		void getSelectFile( CNSVector< CNSString >& result ) const;

	public:
		static bool onFilterSelectChanged( CNSWindow* child, WPARAM wParam, LPARAM lParam, LRESULT* result );
		static bool onFileDialogPaint( CNSWindow* child, WPARAM wParam, LPARAM lParam, LRESULT* result );
		static bool onClickButtonOk( CNSWindow* child, WPARAM wParam, LPARAM lParam, LRESULT* result );
		static bool onClickButtonCancel( CNSWindow* child, WPARAM wParam, LPARAM lParam, LRESULT* result );
		static bool onFileBrowserSelectFile( CNSWindow* child, WPARAM wParam, LPARAM lParam, LRESULT* result );

	public:
		virtual void onPostCreateWindow( CNSWindow* parent );
	};
}
