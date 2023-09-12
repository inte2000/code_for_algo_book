// diff.cpp : This file contains the 'main' function. Program execution begins and ends there.
//


#include <iostream>
#include <algorithm>
#include <conio.h>
#include "graphics.h"
#include "FreeImage.h"

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

bool InitGrayScalePalette(FIBITMAP* bmp)
{
	if (FreeImage_GetBPP(bmp) != 8)
		return false;

	RGBQUAD* pal = FreeImage_GetPalette(bmp);
	for (int i = 0; i < 256; i++)
	{
		pal[i].rgbRed = i;
		pal[i].rgbGreen = i;
		pal[i].rgbBlue = i;
	}

	return true;
}

BYTE RGB2GrayWeight(BYTE r, BYTE g, BYTE b)
{
	return BYTE(r * 0.3 + g * 0.59 + b * 0.11 + 0.5);
}

bool ConvertColorToGray(FIBITMAP* color_bmp, FIBITMAP* gray_bmp, BYTE(*calculator)(BYTE r, BYTE g, BYTE b))
{
	int color_width = FreeImage_GetWidth(color_bmp);
	int color_height = FreeImage_GetHeight(color_bmp);
	int gray_width = FreeImage_GetWidth(gray_bmp);
	int gray_height = FreeImage_GetHeight(gray_bmp);

	if ((color_width != gray_width) || (color_height != gray_height))
		return false;

	for (int y = 0; y < color_height; y++)
	{
		for (int x = 0; x < color_width; x++)
		{
			RGBQUAD color;
			FreeImage_GetPixelColor(color_bmp, x, y, &color);
			BYTE cidx = calculator(color.rgbRed, color.rgbGreen, color.rgbBlue);
			FreeImage_SetPixelIndex(gray_bmp, x, y, &cidx);
		}
	}
	return true;
}

void SaveGrayScaleFile(FIBITMAP* bmp, const char* filename, BYTE(*calculator)(BYTE r, BYTE g, BYTE b))
{
	int width = FreeImage_GetWidth(bmp);
	int height = FreeImage_GetHeight(bmp);

	FIBITMAP* graybmp = FreeImage_Allocate(width, height, 8);
	InitGrayScalePalette(graybmp);
	ConvertColorToGray(bmp, graybmp, calculator);
	FreeImage_Save(FIF_BMP, graybmp, filename, 0);
	FreeImage_Unload(graybmp);
}

FIBITMAP* LoadImageFile(const char* file_name)
{
	FREE_IMAGE_FORMAT fif = FreeImage_GetFileType(file_name);
	if (fif == FIF_UNKNOWN)
		return nullptr;

	return FreeImage_Load(fif, file_name, 0);
}

bool ConvertToGrayImage(const char* color_file, const char* gray_file)
{
	FIBITMAP *colorbmp = LoadImageFile(color_file);
	if (colorbmp == nullptr)
		return false;

	SaveGrayScaleFile(colorbmp, gray_file, RGB2GrayWeight);
	FreeImage_Unload(colorbmp);

	return true;
}

//diff1和diff2 使用相同的调色板
FIBITMAP* SubstractImage(FIBITMAP* diff1, FIBITMAP* diff2)
{
	int width = FreeImage_GetWidth(diff1);
	int height = FreeImage_GetHeight(diff1);

	FIBITMAP* result = FreeImage_Allocate(width, height, 8);
	if (result == nullptr)
		return nullptr;

	InitGrayScalePalette(result);
	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			BYTE vp1, vp2, dp;
			FreeImage_GetPixelIndex(diff1, x, y, &vp1);
			FreeImage_GetPixelIndex(diff2, x, y, &vp2);
			dp = std::abs(vp1 - vp2);
			FreeImage_SetPixelIndex(result, x, y, &dp);
		}
	}

	return result;
}

const char* src_diff1_file = "E:\\Alg 2\\resource\\diff1.bmp";
const char* src_diff2_file = "E:\\Alg 2\\resource\\diff2.bmp";
const char* src_diff1_gray_file = "E:\\Alg 2\\resource\\diff1_gray.bmp";
const char* src_diff2_gray_file = "E:\\Alg 2\\resource\\diff2_gray.bmp";
const char* src_diff_result_file = "E:\\Alg 2\\resource\\diff_result.bmp";

int main()
{
	EasyXEnv init(918, 397);

	ConvertToGrayImage(src_diff1_file, src_diff1_gray_file);
	ConvertToGrayImage(src_diff2_file, src_diff2_gray_file);

	FIBITMAP* diff1 = LoadImageFile(src_diff1_gray_file);
	FIBITMAP* diff2 = LoadImageFile(src_diff2_gray_file);
	FIBITMAP* result = SubstractImage(diff1, diff2);
	FreeImage_Save(FIF_BMP, result, src_diff_result_file, 0);

	FreeImage_Unload(diff1);
	FreeImage_Unload(diff2);
	FreeImage_Unload(result);

	IMAGE exImgDiff1, exImgDiff2, exImgResult;
	loadimage(&exImgDiff1, src_diff1_file);
	loadimage(&exImgDiff2, src_diff2_file);
	loadimage(&exImgResult, src_diff_result_file);

	putimage(0, 0, &exImgDiff1);
	putimage(306, 0, &exImgDiff2);
	putimage(612, 0, &exImgResult);

	Sleep(10000);
}
