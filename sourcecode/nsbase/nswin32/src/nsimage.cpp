#include <nsbase.h>
#include <png.h>
#define PNG_BYTES_TO_CHECK 4

namespace NSWin32
{
	CImage* CImage::newPngImage( const CNSString& imagePath, EImageType type )  
	{  
		FILE *fp;  
		png_structp png_ptr;  
		png_infop info_ptr;  
		png_bytep* row_pointers;  
		char buf[PNG_BYTES_TO_CHECK];  
		int x, y, temp, color_type;  

		fp = _tfopen( CNSString::toTChar( imagePath ), _T("rb") );
		if( fp == NULL )
		{
			int errorCode = errno;
			static CNSString errorDesc;
			errorDesc.format( _UTF8( "crt函数[fopen]调用失败，文件[%s]错误码: %d" ), imagePath.getBuffer( ), errorCode );
			NSException( errorDesc );
		}
		
		png_ptr = png_create_read_struct( PNG_LIBPNG_VER_STRING, 0, 0, 0 );  
		info_ptr = png_create_info_struct( png_ptr );  

		setjmp( png_jmpbuf(png_ptr) );   
		/* 读取PNG_BYTES_TO_CHECK个字节的数据 */  
		temp = fread( buf, 1, PNG_BYTES_TO_CHECK, fp );  
		/* 若读到的数据并没有PNG_BYTES_TO_CHECK个字节 */  
		if( temp < PNG_BYTES_TO_CHECK ) {  
			fclose(fp);  
			png_destroy_read_struct( &png_ptr, &info_ptr, 0);  
			static CNSString errorDesc;
			errorDesc.format( _UTF8( "文件 - %s, 不是有效png文件" ), imagePath.getBuffer( ) );
			NSException( errorDesc );
		}  

		/* 检测数据是否为PNG的签名 */  
		temp = png_sig_cmp( (png_bytep)buf, (png_size_t)0, PNG_BYTES_TO_CHECK );  
		/* 如果不是PNG的签名，则说明该文件不是PNG文件 */  
		if( temp != 0 ) {  
			fclose(fp);  
			png_destroy_read_struct( &png_ptr, &info_ptr, 0);  
			static CNSString errorDesc;
			errorDesc.format( _UTF8( "文件 - %s, 不是有效png文件" ), imagePath.getBuffer( ) );
			NSException( errorDesc );
		}  

		/* 复位文件指针 */  
		rewind( fp );  
		/* 开始读文件 */  
		png_init_io( png_ptr, fp );   
		/* 读取PNG图片信息和像素数据 */  
		png_read_png( png_ptr, info_ptr, PNG_TRANSFORM_EXPAND, 0 );  
		/* 获取图像的色彩类型 */  
		color_type = png_get_color_type( png_ptr, info_ptr );  
		/* 获取图像的宽高 */  
		int w = png_get_image_width( png_ptr, info_ptr );
		int h = png_get_image_height( png_ptr, info_ptr );
		/* 获取图像的所有行像素数据，row_pointers里边就是rgba数据 */  
		row_pointers = png_get_rows( png_ptr, info_ptr );  
		/* 根据不同的色彩类型进行相应处理 */
		unsigned char* buffer = NULL;
		int bufferSize = 0;
		int bitPerPixel = 0;
		switch( color_type ) 
		{  
			case PNG_COLOR_TYPE_RGB_ALPHA:
			{
				buffer = new unsigned char[ w * h * sizeof( unsigned char ) * 4 ];
				int index = 0;
				for( int y = h - 1; y >= 0; y -- )
				{
					for( int x = 0; x < w * 4; )
					{
						/* 以下是RGBA数据，需要自己补充代码，保存RGBA数据 */  
						int red = row_pointers[y][x++]; // red  
						int green = row_pointers[y][x++]; // green  
						int blue = row_pointers[y][x++]; // blue  
						int alpha = row_pointers[y][x++]; // alpha 
						float alphaValue = alpha / 255.0f;
						if ( type == BGRA )
						{
							buffer[index + 3] = alpha;
							buffer[index + 2] = (int)( red * alphaValue );
							buffer[index + 1] = (int)( green * alphaValue );
							buffer[index + 0] = (int)( blue * alphaValue );
						}
						else if ( type == RGBA )
						{
							buffer[index + 3] = alpha;
							buffer[index + 2] = (int)( blue * alphaValue );
							buffer[index + 1] = (int)( green * alphaValue );
							buffer[index + 0] = (int)( red * alphaValue );
						}
						index += 4;
					}  
				}
				bufferSize = index;
				bitPerPixel = 32;
				break;  
			}
			case PNG_COLOR_TYPE_RGB:  
			{
				buffer = new unsigned char[ w * h * sizeof( unsigned char ) * 3 ];
				int index = 0;
				for( int y = h - 1; y >= 0; y -- )
				{
					for( int x = 0; x < w * 3; )
					{
						if ( type == BGRA )
						{
							buffer[index + 2] = row_pointers[y][x++]; // red  
							buffer[index + 1] = row_pointers[y][x++]; // green  
							buffer[index + 0] = row_pointers[y][x++]; // blue
						}
						else if ( type == RGBA )
						{
							buffer[index + 0] = row_pointers[y][x++]; // red  
							buffer[index + 1] = row_pointers[y][x++]; // green  
							buffer[index + 2] = row_pointers[y][x++]; // blue
						}
						index += 3;
					}  
				}  
				bufferSize = index;
				bitPerPixel = 24;
				break;
			}
			/* 其它色彩类型的图像就不读了 */  
			default:  
			{
				fclose(fp);  
				png_destroy_read_struct( &png_ptr, &info_ptr, 0);  
				static CNSString errorDesc;
				errorDesc.format( _UTF8( "文件 - %s, 不是有效png文件" ), imagePath.getBuffer( ) );
				NSException( errorDesc );
			}
		}  
		png_destroy_read_struct( &png_ptr, &info_ptr, 0);

		CNSOctets imageOctets;
		imageOctets.insert( imageOctets.end( ), buffer, bufferSize );
		return new CImage( imagePath, imageOctets, w, h, bitPerPixel );
	}  

	CNSMap< CNSString, CImage* > CImage::mImageCache;
	HBITMAP CImage::getHBitmap( HDC dc )
	{
		BITMAPINFO info = { 0 };

		info.bmiHeader.biBitCount = mBitPerPixel;
		info.bmiHeader.biClrImportant = 0;
		info.bmiHeader.biClrUsed = 0;
		info.bmiHeader.biCompression = BI_RGB;
		info.bmiHeader.biHeight = mHeight;
		info.bmiHeader.biPlanes = 1;
		info.bmiHeader.biSize = sizeof( BITMAPINFOHEADER );
		info.bmiHeader.biSizeImage = 0;
		info.bmiHeader.biWidth = mWidth;
		info.bmiHeader.biXPelsPerMeter = 0;
		info.bmiHeader.biYPelsPerMeter = 0;

		return CreateDIBitmap( dc, &info.bmiHeader, CBM_INIT, mImageData.begin( ), &info, DIB_RGB_COLORS );
	}

	CImage* CImage::loadPngImageBGRA( const CNSString& imagePath )
	{
		CImage** imageRef = mImageCache.get( imagePath );
		if ( imageRef == NULL )
		{
			CImage* image = newPngImage( imagePath, BGRA );
			imageRef = &mImageCache.insert( imagePath, image );
		}

		(*imageRef)->mRefCount ++;
		return *imageRef;
	}

	CImage* CImage::loadPngImageRGBA( const CNSString& imagePath )
	{
		CImage** imageRef = mImageCache.get( imagePath );
		if ( imageRef == NULL )
		{
			CImage* image = newPngImage( imagePath, RGBA );
			imageRef = &mImageCache.insert( imagePath, image );
		}

		(*imageRef)->mRefCount ++;
		return *imageRef;
	}

	void CImage::release( )
	{
		mRefCount --;
		if ( mRefCount == 0 )
		{
			CImage** imageRef = mImageCache.get( mImage );
			if ( imageRef == NULL )
				return;

			delete *imageRef;
			mImageCache.erase( mImage );
		}
	}

	void CImage::exit( )
	{
		HLISTINDEX beginIndex = mImageCache.getHead( );
		for ( ; beginIndex != NULL; mImageCache.getNext( beginIndex ) )
		{
			CImage* image = mImageCache.getValue( beginIndex );
			delete image;
		}

		mImageCache.clear( );
	}
}
