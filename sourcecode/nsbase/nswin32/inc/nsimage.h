#pragma once

namespace NSWin32
{
	class CImage
	{
		enum EImageType
		{
			RGBA,
			BGRA
		};

	protected:
		static CNSMap< CNSString, CImage* > mImageCache;

	public:
		CNSOctets mImageData;
		int mWidth = 0;
		int mHeight = 0;
		int mBitPerPixel;
		int mRefCount = 0;
		CNSString mImage;

	protected:
		CImage( const CNSString& image, const CNSOctets imageData, int width, int height, int bitPerPixel ) 
			: mImage( image ), mImageData( imageData ), mWidth( width ), mHeight( height ), mBitPerPixel( bitPerPixel )
		{
		}
		static CImage* newPngImage( const CNSString& imagePath, EImageType type );

	public:
		HBITMAP getHBitmap( HDC dc );
		void release( );

	public:
		// ���ǿ������������������ ���Ҫ���������ֱ��ָ�븴�ƣ���������ָ�벻�ܱ�֤��ȫ
		static CImage* loadPngImageRGBA( const CNSString& image );
		static CImage* loadPngImageBGRA( const CNSString& image );
		static void exit( );
	};
}
