#pragma once
namespace NSWin32
{
	class CNSVsBtn : public CNSWindow
	{
	public:
		enum EVsButtonEvent
		{
			EventClicked = BN_CLICKED,
		};

		enum EVsButtonState
		{
			STATE_NORMAL,
			STATE_HOVER,
			STATE_DOWN,
		};

		enum EVsButtonStyle
		{
			STYLE_TEXT = 0x00000001,
			STYLE_IMAGE = 0x00000002,
			STYLE_PUSHBUTTON = 0x00000004,
			STYLE_CHECK = 0x00000008,
		};

	public:
		static HBRUSH brushDisableBack;
		static HBRUSH brushBack;
		static HBRUSH brushSelect;
		static HBRUSH brushCheck;
		static HBRUSH brushCheckHover;
		static HPEN penDisableBorder;
		static HPEN penNormalBorder;
		static HPEN penHoverBorder;
		static HPEN penCheckBorder;

	public:
		static void init( );
		static void exit( );
		static bool onButtonRender( CNSWindow* child, WPARAM wParam, LPARAM lParam, LRESULT* result );
		static bool onButtonMouseMove( CNSWindow* child, WPARAM wParam, LPARAM lParam, LRESULT* result );
		static bool onButtonMouseHover( CNSWindow* child, WPARAM wParam, LPARAM lParam, LRESULT* result );
		static bool onButtonMouseLeave( CNSWindow* child, WPARAM wParam, LPARAM lParam, LRESULT* result );
		static bool onButtonLButtonDown( CNSWindow* child, WPARAM wParam, LPARAM lParam, LRESULT* result );
		static bool onButtonLButtonUp( CNSWindow* child, WPARAM wParam, LPARAM lParam, LRESULT* result );
		static bool onButtonNcDestroy( CNSWindow* child, WPARAM wParam, LPARAM lParam, LRESULT* result );

	public:
		EVsButtonState mState;
		EVsButtonStyle mStyle;
		bool mInTracking;

		// 仅仅被Check使用
		bool mIsChecked = false;
		HBITMAP	mImage = NULL;
		HBITMAP mOldImage = NULL;
		HDC mImageDC = NULL;
		CImage*	mImageData = NULL;

	public:
		CNSVsBtn( const CNSString& windowID, EVsButtonStyle style );

	public:
		void setCheck( bool check );
		bool getCheck( ) const;
		void setImage( CImage* image );

	public:
		virtual void onPostCreateWindow( CNSWindow* parent );

	public:
		static void pushUserData( CNSLuaStack& luaStack, CNSVsBtn* ref );
		static void popUserData( const CNSLuaStack& luaStack, CNSVsBtn*& ref );
		static void regLuaLib( );
	};
}
