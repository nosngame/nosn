#pragma once
namespace NSWin32
{
	class CNSFrame : public CNSWindow
	{
		friend class CNSWindow;
	public:
		enum ESplitType
		{
			SPLIT_NONE,
			SPLIT_HORZ,
			SPLIT_VERT
		};

		enum EFrameType
		{
			// 完整的应用程序主窗口
			STYLE_FULL,
			// 应用程序内弹出式窗口
			STYLE_POPUP,
			// 应用程序内无标题弹出式窗口
			STYLE_MINIPOPUP,
			// 应用程序内弹出式VS风格窗口
			STYLE_VSPOPUP,
			// 子窗口
			STYLE_CHILD
		};

		// 作为对话框时的返回值
		enum EResultCode
		{
			RESULT_OK,
			RESULT_CANCEL
		};

		class CFrameGrabber
		{
		public:
			RECT mGrabbRect = {0, 0, 0, 0};
			CNSWindow* mpLeft = NULL;
			CNSWindow* mpRight = NULL;
		};

		class CDialogResult : public NSBase::CNSLuaMarshal
		{
		public:
			EResultCode	mResult;

		public:
			virtual CNSLuaStack& marshal( CNSLuaStack& luaStack ) const
			{
				luaStack.pushField( "mResult", mResult );
				return luaStack;
			}

			virtual const CNSLuaStack& unmarshal( const CNSLuaStack& luaStack )
			{
				luaStack.popField( "mResult", mResult );
				return luaStack;
			}
		};

	public:
		static LRESULT CALLBACK windowProc(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam);
		static bool onGrabRender(CNSWindow* child, WPARAM wParam, LPARAM lParam, LRESULT* result);

	public:
		static HBRUSH frameBack;
		static int grabWidth;

	public:
		static void init( );
		static void exit( );
		static void regLuaLib( );

	protected:
		CFrameGrabber	mGrabbers;
		CFrameGrabber*	mCurGrab			= NULL;
		bool			mIsHoverGrab		= false;
		POINT			mLastCursor			= {0, 0};
		POINT			mDownCursor			= {0, 0};
		ESplitType		mSplitType			= ESplitType::SPLIT_NONE;
		bool			mExitWhenDestroy	= false;
		bool			mIsModalDialog		= false;
		CDialogResult*	mResult				= NULL;
		HBRUSH			mBackBrush			= NULL;
		COLORREF		mBackColor			= RGB( 45, 45, 48 );
		EFrameType		mFrameType;

	public:
		CNSFrame( const CNSString& windowID, unsigned int type = 0 );

	protected:
		virtual bool onNcCalcSize( bool clientArea, NCCALCSIZE_PARAMS* calcSize );
		virtual void getTitleRect(RECT& rc);
		virtual void drawTitle(HDC dc);

	public:
		void unSplitLeft( );
		void unSplitRight( );
		void split(ESplitType type, float percent);
		void bindLeft(CNSWindow* left);
		void bindRight(CNSWindow* right);
		void exitWhenDestroy();
		void bindResult( CNSFrame::CDialogResult* result );
		void closeFrame( CNSFrame::EResultCode code );
		void setBkColor( COLORREF color );
		CNSFrame::CDialogResult* getResult( ) const
		{
			return mResult;
		}

	public:
		virtual void onPostCreateWindow( CNSWindow* parent );

	public:
		static void pushUserData( CNSLuaStack& luaStack, CNSFrame* ref );
		static void popUserData( const CNSLuaStack& luaStack, CNSFrame*& ref );
	};
}