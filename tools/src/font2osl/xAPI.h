extern int xLoadBitmap(char *fichier, unsigned long **data, int *width, int *height);
extern int xWriteBmpFile(HDC hdc, char *pszflname, int width, int height);
extern int xCreateDIB(HDC hdc, int cx, int cy, int ibitcount, BITMAPINFOHEADER *BIH, HBITMAP *hbm, void **bits);

#define XTA_BOLD 1,0,0,0,0
#define XTA_NORMAL 0,0,0,0,0
#define XTA_ITALIC 0,1,0,0,0
#define XTA_UNDERLINE 0,0,1,0,0
#define XTA_BOLDITALIC 1,1,0,0,0
extern void xSetTextFont(HDC hdcImage, const char *police, int taille);
extern void xSetTextAttributes(HDC hdcImage, int gras=0, int italique=0, int souligne=0, int barre=0, int angle=0);

