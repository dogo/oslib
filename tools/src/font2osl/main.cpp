#include <stdlib.h>
#include <stdio.h>
#include <windows.h>
#include "xAPI.h"

unsigned long *xSourceBitmap = NULL;
int xSourceWidth, xSourceHeight;
#define xSrc(x,y) (xSourceBitmap[(xSourceWidth*(y))+(x)])
HDC xHdcImage;
HBITMAP xHBitmap;
BITMAPINFOHEADER xBIH;
unsigned long *xBits;
#define xBmp(x,y) (*(xBits+(xSourceHeight-(y))*xSourceWidth+(x)))
#define MAX_COLORS 16

unsigned int atocolor(char *chn)	{
	int i;
	unsigned int n;
	int v;
	char *deb;
	while(*chn == ' ' || *chn == '	')
		chn++;
	if (*chn == '#')
		chn++;
	deb=chn;
	while((*chn>='0' && *chn<='9') || (*chn>='a' && *chn<='f') || (*chn>='A' && *chn<='F'))
		chn++;
	chn--;
	i=1;
	n=0;
	while(chn>=deb)	{
		v=*(chn)-48;
		if (v>=17+32)	v-=39;
		if (v>=17)		v-=7;
		n+=i*v;
		i*=16;
		chn--;
	}
	return n;
}

void DetermineLargeurCar(unsigned char *tailles, int c, int taille)			{
	int x, y, maxX=0;
	for (x=0;x<64;x++)		{
		for (y=taille*c; y<taille*(c+1);y++)		{
			int val = xBmp(x,y) & 0xffffff;
			if (val != 0xffffff && val != 0xffff00)			//Pas blanc?
				break;
		}
		if (y<taille*(c+1))				//Trouvé?
			maxX = x+1;
	}
	tailles[c] = maxX;
}

void DecodeFichierDimensions(char *fichier, unsigned char *tailles, int *width, int *height, int *variable, int *palCount, unsigned int *palColors, int *textCount, unsigned int *textColors, int *bitPlanes, int *addedSpace)		{
	FILE *f = fopen(fichier, "r");
	char str[100], *ptr, *lastegal;
	int c, valeur;
	if (!f)
		return;
	while (1)		{
		fgets(str, sizeof(str), f);
		if (feof(f))
			break;
		//Dernier =
		ptr = str;
		lastegal = ptr;
		while (*ptr)		{
			if (*ptr == '=')
				lastegal = ptr + 1;
			ptr++;
		}
		valeur = atoi(lastegal);
		c = atoi(str);
		if (!strncmp(str, "width", 5))
			*width = valeur;
		else if (!strncmp(str, "height", 6))
			*height = valeur;
		else if (!strncmp(str, "variable", 8))
			*variable = valeur;
		else if (!strncmp(str, "bitplanes", 9))
			*bitPlanes = valeur;
		else if (!strncmp(str, "addedspace", 10))
			*addedSpace = valeur;
		//Colors on the image itself
		else if (!strncmp(str, "imgpal[count]", 10))
			*palCount = valeur;
		else if (!strncmp(str, "imgpal[", 4))			{
			int i = atoi(str + 4);
			if (i >= 0 && i < MAX_COLORS)
				palColors[i] = atocolor(lastegal);
		}
		//Colors in the font file (will be used by the renderer). If not defined, the renderer will use the imgpal.
		else if (!strncmp(str, "textpal[count]", 10))
			*textCount = valeur;
		else if (!strncmp(str, "textpal[", 4))			{
			int i = atoi(str + 4);
			if (i >= 0 && i < MAX_COLORS)
				textColors[i] = atocolor(lastegal);
		}
		//DEPRECATED!!!!
		else if (!strncmp(str, "pal[count]", 10))
			*palCount = valeur;
		else if (!strncmp(str, "pal[", 4))			{
			int i = atoi(str + 4);
			if (i >= 0 && i < MAX_COLORS)
				palColors[i] = atocolor(lastegal);
		}
		//END DEPRECATED
		else if (c >= 0 && c <= 255)
			tailles[c] = valeur;
	}

}

void DisplayUsage()		{
	printf("================================== FONT2OSL ==================================\n");
	printf("Utility for converting a Windows font or creating your own for use with OSLib.\n\n======\n");
	printf("Usage:\n======\nfont2osl -convert \"YourWindowsFont\" fontSize \"YourFontName.bmp\" \"YourFontName.txt\"\n");
	printf("font2osl -create \"YourFontName.bmp\" \"YourFontName.txt\" \"YourFinalFontName.oft\"\n");
	printf("\n=========\nExamples:\n=========\n");
	printf("font2osl -convert \"Verdana\" 13 \"verdana.bmp\" \"verdana.txt\"\n");
	printf("font2osl -create \"verdana.bmp\" \"verdana.txt\" \"verdana.oft\"\n");
}

typedef struct		{
	char strVersion[12];			// "OSLFont v01"
	unsigned char pixelFormat;		// 1 = 1 bit
	unsigned char variableWidth;	// Si oui les 256 premiers octets de data spécifient la largeur.
	int charWidth, charHeight;		// Tailles moyennes des caractères
	int lineWidth;					// Nombre d'octets par ligne
	unsigned char addedSpace;		// Space added between the characters on the texture (allows to make characters graphically bigger than what indicated by charWidths)
	unsigned short paletteCount;	// Default palette - stored after character data, 4 bytes per palette entry (RGBA, with A being not obligatorily taken in account)
	unsigned char reserved[29];		// Nul
} OSL_FONT_FORMAT_HEADER;

//-convert "Verdana" 13 "verdana.bmp" "verdana.txt"
//-create "verdana.bmp" "verdana.txt" "verdana.oft"
//-view "verdana.oft"
int main(int argc, char *argv[])		{
	char *action = argv[1];
	unsigned char tailles_car[256];
	int i;

	if (argc < 2)		{
		DisplayUsage();
		return 0;
	}

	memset(tailles_car, 0, sizeof(tailles_car));

	//Crée une fonte
	if (!strcmp(action, "-convert"))		{
		char *fonte = argv[2];
		int taille;
		char str[100];
		int grandeurMax;
		FILE *fTailles;
		HBRUSH hbr = CreateSolidBrush(RGB(255, 255, 0));
		RECT rect;

		if (argc < 5)		{
			DisplayUsage();
			return 0;
		}
		taille = atoi(argv[3]);
		//On va dessiner le charset sur xHdcImage
		xHdcImage=CreateCompatibleDC(GetDC(NULL));
		xSourceWidth = 64;						//Max 64 pixels de large, à modifier éventuellement
		xSourceHeight = taille * 256;
		xCreateDIB(xHdcImage, 64, taille * 256, 32, &xBIH, &xHBitmap, (void**)&xBits);
		//Remplit de blanc
//		PatBlt(xHdcImage, 0,0, 64,taille*256, WHITENESS);
		rect.top = 0;
		rect.left = 0;
		rect.right = 64;
		rect.bottom = taille*256;
		FillRect(xHdcImage, &rect, hbr);

		//Paramètre le texte
		xSetTextFont(xHdcImage, fonte, taille);
		xSetTextAttributes(xHdcImage, XTA_NORMAL);
		SetBkColor(xHdcImage, RGB(255,255,255));
		SetTextColor(xHdcImage, RGB(0,0,0));
		//Calcule les tailles des caractères
		for (i=0;i<256;i++)			{
			ABC charWidth;
			int currentCharWidth;
			sprintf(str, "%c", i);
			TextOut(xHdcImage, 0, i*taille, str, strlen(str));
			GetCharABCWidths(xHdcImage, i, i, &charWidth);
			//Si le caractère est plus grand que ça, il y a eu une erreur.
			if (charWidth.abcA < 0x1000000)			{
				tailles_car[i] = charWidth.abcA + charWidth.abcB + charWidth.abcC;
			}
			else		{
				GetCharWidth32(xHdcImage, i, i, &currentCharWidth);
				tailles_car[i] = currentCharWidth;
			}
//			DetermineLargeurCar(tailles_car, i, taille);
		}
		//Petite modif pour le caractère nul
		tailles_car[0] = tailles_car[' '];
		//Ecrit les tailles
		fTailles = fopen(argv[5], "w");
		if (fTailles)			{
			grandeurMax = 0;
			fprintf(fTailles, "width = %i\nheight = %i\nvariable = 1\n", tailles_car['E'], taille);
			fprintf(fTailles, "bitplanes = 1\npal[count] = 2\npal[0] = #ffffff\npal[1] = #000000\n");
			for (i=0;i<256;i++)		{
				fprintf(fTailles, "%03i (%c) = %i\n", i, i>32?i:32, tailles_car[i]);
				if (tailles_car[i] > grandeurMax)
					grandeurMax = tailles_car[i];
			}
			fclose(fTailles);
			//Ecrit la bitmap
			if (grandeurMax % 4)
				grandeurMax = (grandeurMax & ~3) + 4;
			xWriteBmpFile(xHdcImage, argv[4], grandeurMax, taille * 256);
		}
	}
	else if (!strcmp(action, "-create"))		{
		FILE *f;
		int charWidth, charHeight, isVariable=1, w;
		int colorCount = -1, textColorCount = -1, bitPlanes = 1, addedSpace = 0;
		unsigned int colors[MAX_COLORS], textColors[MAX_COLORS];
		OSL_FONT_FORMAT_HEADER ft;
		int x, xx, xxx, y, k;
		unsigned char c;

		if (argc<4)			{
			DisplayUsage();
			return 0;
		}
		//Remplit l'en-tête
		DecodeFichierDimensions(argv[3], tailles_car, &charWidth, &charHeight, &isVariable,
			&colorCount, colors, &textColorCount, textColors, &bitPlanes, &addedSpace);
		if (bitPlanes < 1 || bitPlanes > 4)			{
			printf("Invalid bitplane parameter.\n");
			return 0;
		}
		w = 0;
		for (i=0;i<256;i++)		{
			if (tailles_car[i] > w)
				w = tailles_car[i];
		}

		if (w <= 0)
			w = charWidth;

		memset(&ft, 0, sizeof(ft));
		strcpy(ft.strVersion, "OSLFont v01");
		//Espace ajouté à la droite des caractères (permet de définir des caractères dont l'image est large que ceux-ci)
		ft.addedSpace = addedSpace;
		ft.pixelFormat = bitPlanes;
		ft.variableWidth = isVariable;
		ft.charWidth = charWidth;
		ft.charHeight = charHeight;

		int decal = 0;

		if (bitPlanes == 1)
			decal = 3;
		else if (bitPlanes == 2)
			decal = 2;
		else if (bitPlanes == 4)
			decal = 1;

		//Aligner la largeur à l'octet près (en fonction du bitplane) - Pas sûr de ce code, à tester!
		int val = 1 << decal;
		if (w & (val - 1))
			w = (w & ~ (val - 1)) + val;

		ft.lineWidth = w >> decal;

		//Utilise les couleurs par défaut pour le texte si rien n'a été spécifié - sauf pour le 1 bit
		if (textColorCount == -1 && colorCount > 0 && bitPlanes != 1)		{
//			memcpy(textColors, colors, colorCount);
			//On va utiliser des niveaux de gris
			for (i=0;i<colorCount;i++)			{
				//The first color is transparent
				if (i == 0)
					textColors[i] = 0;
				else		{
					y = ((i - 1) * 255) / (colorCount - 2);
					textColors[i] = y | y << 8 | y << 16 | 0xff << 24;
				}
			}
			textColorCount = colorCount;
		}

		//No color found? Use the default palette.
		if (colorCount == -1)		{
			if (bitPlanes != 1)		{
				printf("Color palette missing in the .txt file\n");
				return 0;
			}
			else	{
				colorCount = 2;
				colors[0] = 0xffffffff;
				colors[1] = 0xff000000;
			}
		}


		//Au moins 0 couleur...
		textColorCount = max(textColorCount, 0);
		//Et pas plus de couleurs que possible dans le bitplane courant
		textColorCount = min(textColorCount, 1 << bitPlanes);
		ft.paletteCount = textColorCount;

		memset(ft.reserved, 0, sizeof(ft.reserved));
		//Ecrit le fichier
		f = fopen(argv[4], "wb");
		//Charge la bitmap des caractères
		if (f && xLoadBitmap(argv[2], &xSourceBitmap, &xSourceWidth, &xSourceHeight))		{
			//Ecrit l'en-tête
			fwrite(&ft, sizeof(OSL_FONT_FORMAT_HEADER), 1, f);
			//Si c'est une fonte variable, on écrit les 256 tailles de caractère
			if (isVariable)
				fwrite(tailles_car, sizeof(tailles_car), 1, f);
			//Maintenant on peut écrire le charset...
			for (i=0;i<256;i++)		{
				for (y=charHeight*i;y<charHeight*(i+1);y++)			{
					for (xx=0;xx<w;)		{
						c = 0;
						xxx = xx;
						for (x=0;x<8;)			{
							if (x+xxx < xSourceWidth)			{
								for (k=0;k<colorCount;k++)			{
									if ((xSrc(xx,y)&0xffffff) == (colors[k] & 0xffffff))
										break;
								}
								if (k < colorCount)
									c |= k<<x;
							}
							x += bitPlanes;
							xx++;
						}
						fwrite(&c, sizeof(c), 1, f);
					}
				}
			}

			//Maintenant il reste à écrire la palette
			if (textColorCount > 0)		{
				//Make sure alpha is currently set to the maximum
//				for (i=0;i<textColorCount;i++)
//					textColors[i] |= 0xff000000;
				fwrite(textColors, sizeof(textColors[0]), textColorCount, f);
			}
			//Terminé :)
			fclose(f);
		}
	}
	else	{
		DisplayUsage();
		return 0;
	}
/*	else if (!strcmp(action, "-view"))		{
		FILE *f;
		OSL_FONT_FORMAT_HEADER ft;
		int x, xx, y, v;
		f = fopen(argv[2], "rb");
		if (f)		{
			fread(&ft, sizeof(ft), 1, f);
			if (ft.variableWidth)
				fread(&tailles_car, sizeof(unsigned char), 256, f);
			for (i=0;i<53;i++)		{
				for (y=0;y<ft.charHeight;y++)		{
					for (xx=0;xx<ft.lineWidth;xx++)		{
						v = fgetc(f);
						for (x=0;x<8;x++)		{
							if (v&1)
								SetPixel(GetDC(GetForegroundWindow()), x+(xx<<3), y+(i-32)*ft.charHeight, RGB(0,255,0));
							v>>=1;
						}
					}
				}
			}
		}
		scanf("%s", NULL);
	}*/

	return 1;
}
