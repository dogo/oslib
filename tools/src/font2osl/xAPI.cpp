#include <stdlib.h>
#include <stdio.h>
#include <windows.h>

int xLoadBitmap(char *fichier, unsigned long **data, int *width, int *height)		{
	HBITMAP hbmConv;
	BITMAP bmConv;
	HDC hdcConv;
	HDC hdcEcran;
	int x,y;

	hdcEcran = GetDC(NULL);
	hbmConv = (HBITMAP) LoadImage(NULL, fichier, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE|LR_CREATEDIBSECTION);
	if (!hbmConv)	{
		return 0;
	}
	GetObject(hbmConv, sizeof(bmConv), &bmConv);
	*width = bmConv.bmWidth;
	*height = bmConv.bmHeight;
	*data = (unsigned long*)malloc((*width) * (*height) * sizeof(*data));

	hdcConv = CreateCompatibleDC(hdcEcran);
	SelectObject(hdcConv, hbmConv);
	for (y=0;y<*height;y++)
		for (x=0;x<*width;x++)
			(*data)[((*width)*(y))+(x)] = GetPixel(hdcConv, x, y);

	DeleteObject(hbmConv);
	DeleteDC(hdcConv);
	ReleaseDC(NULL, hdcEcran);
	return 1;
}

int xWriteBmpFile(HDC hdc, char *pszflname, int width, int height) 
{ 
	HDC memdc/*, hdc*/; 
	HANDLE hfl; 
	DWORD dwBytes, dwNumColors; 
	void *pBits; 
	HBITMAP hbmp; 
	BITMAPFILEHEADER fileheader; 
	RGBQUAD colors[256]; 
	BITMAPINFO bmpinfo; 
	HGDIOBJ hret; 
	RECT rct; 
	int i;

//	hdc = GetWindowDC(hwnd); 
	if(!hdc) return 0;
	rct.bottom = height;
	rct.right = width;
/*	GetWindowRect(hwnd, &rct); 
	rct.bottom -= rct.top; 
	rct.right -= rct.left; */
	rct.top = GetDeviceCaps(hdc, BITSPIXEL); 
	if(rct.top <= 8) dwNumColors = 256; 
	else dwNumColors = 0; 
	if(!(memdc = CreateCompatibleDC(hdc))) goto relHwndDc; 
	bmpinfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER); 
	bmpinfo.bmiHeader.biWidth = rct.right; 
	bmpinfo.bmiHeader.biHeight = rct.bottom; 
	bmpinfo.bmiHeader.biPlanes = 1; 
	bmpinfo.bmiHeader.biBitCount = (WORD) rct.top; 
	bmpinfo.bmiHeader.biCompression = BI_RGB; 
	bmpinfo.bmiHeader.biSizeImage = 0; 
	bmpinfo.bmiHeader.biXPelsPerMeter = 0; 
	bmpinfo.bmiHeader.biYPelsPerMeter = 0; 
	bmpinfo.bmiHeader.biClrUsed = dwNumColors; 
	bmpinfo.bmiHeader.biClrImportant = dwNumColors; 
	hbmp = CreateDIBSection(hdc, &bmpinfo, DIB_PAL_COLORS, &pBits, NULL, 0); 
	if(!hbmp) goto errato; 
	//ATTENTION!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	rct.right += rct.right & 1;
	//ARRONDI A 2 A CAUSE DE PROBLEMES DE SAUVEGARDE
	hret = SelectObject(memdc, hbmp); 
	if(!hret || (hret == HGDI_ERROR)) goto errato; 
	if(!BitBlt(memdc, 0, 0, rct.right, rct.bottom, hdc, 0, 0, SRCCOPY)) goto errato; 
	if(dwNumColors) dwNumColors = GetDIBColorTable(memdc, 0, dwNumColors, colors); 
	fileheader.bfType = 0x4D42; 
	rct.left = dwNumColors * sizeof(RGBQUAD); 
	fileheader.bfSize = ((rct.right * rct.bottom * rct.top) >> 3) + rct.left + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER); 
	fileheader.bfReserved1 = fileheader.bfReserved2 = 0; 
	fileheader.bfOffBits = rct.left + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER); 
	bmpinfo.bmiHeader.biClrImportant = 0; 
	bmpinfo.bmiHeader.biClrUsed = dwNumColors; 
	hfl = CreateFile(pszflname,GENERIC_WRITE,0,0,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,0); 
	if(hfl == INVALID_HANDLE_VALUE) {DeleteObject(hbmp); goto errato;} 
	WriteFile(hfl, &fileheader, sizeof(BITMAPFILEHEADER), &dwBytes, 0); 
	WriteFile(hfl, &bmpinfo.bmiHeader, sizeof(BITMAPINFOHEADER), &dwBytes, 0); 
	if(!dwNumColors) WriteFile(hfl, colors, rct.left, &dwBytes, 0); 
//	for (i=0;i<height;i++)
//		WriteFile(hfl, (unsigned char*)pBits + ((rct.right * rct.top * i) >> 3), (rct.right * rct.top) >> 3, &dwBytes, 0); 
	WriteFile(hfl, pBits, (rct.right * rct.bottom * rct.top) >> 3, &dwBytes, 0); 
	CloseHandle(hfl); 
	DeleteObject(hbmp); 
	DeleteDC(memdc); 
	return 1; 
errato: 
	DeleteDC(memdc); 
relHwndDc:
	return 0;
//	ReleaseDC(hwnd, hdc); return 0; 
}


int xCreateDIB(HDC hdc, int cx, int cy, int ibitcount, BITMAPINFOHEADER *BIH, HBITMAP *hbm, void **bits) 
{
   // Create a BITMAPINFOHEADER structure to describe the DIB.
   int iSize = sizeof(BITMAPINFOHEADER);
   HGDIOBJ hbmAncien;

   if (!((ibitcount == 8) || (ibitcount == 16) || (ibitcount == 24) || (ibitcount == 32)))
	   return 0;
   if (cx <= 0 || cy <= 0)
	   return 0;
   memset(BIH, 0, iSize);

   // Fill in the header info. 
   BIH->biSize = iSize;
   BIH->biWidth = cx;
   BIH->biHeight = cy+1;
   BIH->biPlanes = 1;
   BIH->biBitCount = ibitcount;
   BIH->biCompression = BI_RGB;

   // Create a new device context.

   // Create the DIB section.
   *hbm = CreateDIBSection(hdc,
               (BITMAPINFO*) BIH,
                             DIB_RGB_COLORS,
                             (void**)bits,
                             NULL,
                             0);

    // Select the new bitmap into the buffer DC.
    if (*hbm && *bits)
    {
        hbmAncien=(HGDIOBJ)SelectObject(hdc,*hbm);
		DeleteObject(hbmAncien);
		return 1;
    }
	else
		return 0;
}

LOGFONT xLogFCour;
HFONT xCurrentFont;
void xSetTextFont(HDC hdcImage, const char *police, int taille)		{
	HFONT hAncienne;

	xLogFCour.lfHeight=taille;
	xLogFCour.lfWidth=0;
	xLogFCour.lfEscapement=0;

	xLogFCour.lfCharSet=DEFAULT_CHARSET;
	xLogFCour.lfOutPrecision=0;
	xLogFCour.lfClipPrecision=0;
	xLogFCour.lfQuality=0;
	xLogFCour.lfPitchAndFamily=FF_DONTCARE;
	strcpy(xLogFCour.lfFaceName,police);

	xCurrentFont = CreateFontIndirect(&xLogFCour);
	hAncienne=(HFONT)SelectObject(hdcImage, xCurrentFont);
	DeleteObject(hAncienne);
}

void xSetTextAttributes(HDC hdcImage, int gras=0, int italique=0, int souligne=0, int barre=0, int angle=0)
{
	HFONT hAncienne;
	xLogFCour.lfEscapement=angle;
	xLogFCour.lfWeight=gras?FW_BOLD:FW_NORMAL;
	xLogFCour.lfItalic=italique;
	xLogFCour.lfUnderline=souligne;
	xLogFCour.lfStrikeOut=barre;
	xCurrentFont=CreateFontIndirect(&xLogFCour);
	hAncienne=(HFONT)SelectObject(hdcImage,xCurrentFont);
	DeleteObject(hAncienne);
}



