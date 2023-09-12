// color.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <algorithm>
#include <conio.h>
#include "graphics.h"
#include "FreeImage.h"

const char* src_gray_file = "E:\\Alg 2\\resource\\lena_gray3.jpg";
const char* src_color_file = "E:\\Alg 2\\resource\\lena_recolor.jpg";

class EasyXEnv
{
public:
	EasyXEnv(int width, int height, int flags = 0)
	{
		m_hWnd = initgraph(width, height, flags);
	}
	virtual ~EasyXEnv()
	{
		closegraph();
	}
	HWND GetGraphHwnd()
	{
		return m_hWnd;
	}
protected:
	HWND m_hWnd;
};

RGBQUAD GrayScaleToRGB(BYTE gray)
{
	RGBQUAD color;

	color.rgbReserved = 0;
	if ((0 <= gray) && (gray <= 63))
	{
		color.rgbRed = 0;
		color.rgbGreen = 254 - 4 * gray;
		color.rgbBlue = 255;
	}
	else if ((64 <= gray) && (gray <= 127))
	{
		color.rgbRed = 0;
		color.rgbGreen = 4 * gray - 254;
		color.rgbBlue = 510 - 4 * gray;
	}
	else if ((128 <= gray) && (gray <= 191))
	{
		color.rgbRed = 4 * gray - 510;
		color.rgbGreen = 255;
		color.rgbBlue = 0;
	}
	else
	{
		color.rgbRed = 255;
		color.rgbGreen = 1022 - 4 * gray;
		color.rgbBlue = 0;
	}

	return color;
}

bool ConvertColorToGray(FIBITMAP* gray_bmp, FIBITMAP* color_bmp, RGBQUAD(*calculator)(BYTE gray))
{
	int gray_width = FreeImage_GetWidth(gray_bmp);
	int gray_height = FreeImage_GetHeight(gray_bmp);
	int color_width = FreeImage_GetWidth(color_bmp);
	int color_height = FreeImage_GetHeight(color_bmp);

	if ((color_width != gray_width) || (color_height != gray_height))
		return false;

	if (FreeImage_GetBPP(color_bmp) <= 16)
		return false;

	for (int y = 0; y < gray_height; y++)
	{
		for (int x = 0; x < gray_width; x++)
		{
			BYTE cidx;
			FreeImage_GetPixelIndex(gray_bmp, x, y, &cidx);
			RGBQUAD color = calculator(cidx);
			FreeImage_SetPixelColor(color_bmp, x, y, &color);
		}
	}

	return true;
}

void SaveColorFile(FIBITMAP* graybmp, const char* filename, RGBQUAD(*calculator)(BYTE gray))
{
	int width = FreeImage_GetWidth(graybmp);
	int height = FreeImage_GetHeight(graybmp);

	FIBITMAP* colorbmp = FreeImage_Allocate(width, height, 24);
	ConvertColorToGray(graybmp, colorbmp, calculator);
	FreeImage_Save(FIF_JPEG, colorbmp, filename, 0);
	FreeImage_Unload(colorbmp);
}

int main()
{
	EasyXEnv init(1024, 512);

	FREE_IMAGE_FORMAT fif = FreeImage_GetFileType(src_gray_file);
	if (fif != FIF_UNKNOWN)
	{
		FIBITMAP *graybmp = FreeImage_Load(fif, src_gray_file, 0);

		SaveColorFile(graybmp, src_color_file, GrayScaleToRGB);

		FreeImage_Unload(graybmp);

		IMAGE exImgGray, exImgColor;
		loadimage(&exImgGray, src_gray_file);
		loadimage(&exImgColor, src_color_file);

		putimage(0, 0, &exImgGray);
		putimage(512, 0, &exImgColor);

		Sleep(10000);
	}

}
