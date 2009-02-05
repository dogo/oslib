/*
 * libccc.h
 * Character Code Conversion Library
 * Version 0.30 by BenHur - http://www.psp-programming.com/benhur
 *
 * This work is licensed under the Creative Commons Attribution-Share Alike 3.0 License.
 * See LICENSE for more details.
 *
 */

#include <pspkernel.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>

#include "libccc.h"

static unsigned char cccInitialized = 0;
static cccUCS2 * __table_ptr__[CCC_N_CP];
static unsigned char __table_dyn__[CCC_N_CP];
static cccUCS2 __error_char_ucs2__ = 0x0000U;

void cccSetTable(cccUCS2 * code2ucs2, unsigned char cp, unsigned char dyn) {
	if (cp < CCC_N_CP) {
		__table_ptr__[cp] = code2ucs2;
		__table_dyn__[cp] = dyn;
	}	
}

/* In a future version codepage tables might be loaded from firmware files.
 * This is currently disabled, since there's no easy and compatible way to
 * access those files.
cccUCS2* cccLoadTable(const char *filename) {
    SceUID fd = sceIoOpen(filename, PSP_O_RDONLY, 0777);
    if(fd < 0) return NULL;

    unsigned int filesize = sceIoLseek(fd, 0, SEEK_END);
    sceIoLseek(fd, 0, SEEK_SET);
    cccUCS2* table = (cccUCS2*)malloc(filesize);
	if (!table) {
		sceIoClose(fd);
		return NULL;
	}
    if (sceIoRead(fd, table, filesize) != filesize) {
		sceIoClose(fd);
		free(table);
		return NULL;
	}
	sceIoClose(fd);
	return table;    
}	*/

void cccInit(void) {
	int cp;
	for (cp = 0; cp < CCC_N_CP; cp++) {
		__table_ptr__[cp] = NULL;
		__table_dyn__[cp] = 0;
	}

#ifndef LIBCCC_NO_CP437
    cccSetTable((cccUCS2*)__CP437toUCS2__, CCC_CP437, 0);
#endif
#ifndef LIBCCC_NO_CP850
	cccSetTable((cccUCS2*)__CP850toUCS2__, CCC_CP850, 0);
#endif
#ifndef LIBCCC_NO_CP866
	cccSetTable((cccUCS2*)__CP866toUCS2__, CCC_CP866, 0);    
#endif
#ifndef LIBCCC_NO_CP932
    cccSetTable((cccUCS2*)__CP932toUCS2__, CCC_CP932, 0);
#endif
#ifndef LIBCCC_NO_CP936
	cccSetTable((cccUCS2*)__CP936toUCS2__, CCC_CP936, 0);
#endif
#ifndef LIBCCC_NO_CP949
	cccSetTable((cccUCS2*)__CP949toUCS2__, CCC_CP949, 0);
#endif
#ifndef LIBCCC_NO_CP950
	cccSetTable((cccUCS2*)__CP950toUCS2__, CCC_CP950, 0);
#endif
#ifndef LIBCCC_NO_CP1251
	cccSetTable((cccUCS2*)__CP1251toUCS2__, CCC_CP1251, 0);
#endif
#ifndef LIBCCC_NO_CP1252
	cccSetTable((cccUCS2*)__CP1252toUCS2__, CCC_CP1252, 0);
#endif

	cccInitialized = 1;
}

void cccShutDown(void) {
	if (cccInitialized) {
		//free dynamically loaded tables
		int cp;
		for (cp = 0; cp < CCC_N_CP; cp++) {
			if (__table_dyn__[cp] && __table_ptr__[cp]) {
				free(__table_ptr__[cp]);
				__table_ptr__[cp] = NULL;
				__table_dyn__[cp] = 0;
			}
		}
		cccInitialized = 0;
	}
}

cccUCS2 cccSetErrorCharUCS2(cccUCS2 code) {
	cccUCS2 old = __error_char_ucs2__;
	__error_char_ucs2__ = code;
	return old;
}

int cccStrlen(cccCode const * str) {
	if (!str || *str == '\0') return 0;

	return strlen((char*)str);
}

int cccStrlenSJIS(cccCode const * str) {
	if (!str || *str == '\0') return 0;

	int i = 0, length = 0;
	while (str[i]) {
		length++;
		i += (str[i] <= 0x80 || (str[i] >= 0xA0 && str[i] <= 0xDF) || str[i] >= 0xFD) ? 1 : 2; //single or double byte
	}
	return length;
}

int cccStrlenGBK(cccCode const * str) {
	int i = 0, length = 0;
	while (str[i]) {
		length++;
		i += (str[i] <= 0x80 || str[i] == 0xFF) ? 1 : 2; //single or double byte
	}
	return length;
}

int cccStrlenKOR(cccCode const * str) {
	int i = 0, length = 0;
	while (str[i]) {
		length++;
		i += (str[i] <= 0x80 || str[i] == 0xFF) ? 1 : 2; //single or double byte
	}
	return length;
}

int cccStrlenBIG5(cccCode const * str) {
	int i = 0, length = 0;
	while (str[i]) {
		length++;
		i += (str[i] <= 0x80 || str[i] == 0xFF) ? 1 : 2; //single or double byte
	}
	return length;
}

int cccStrlenUTF8(cccCode const * str) {
	if (!str || *str == '\0') return 0;

	int i = 0, length = 0;
	while (str[i]) {
		if      (str[i] <= 0x7F) { i++;    length++; } //ASCII
		else if (str[i] <= 0xC1) { i++;              } //part of multi-byte or overlong encoding ->ignore
		else if (str[i] <= 0xDF) { i += 2; length++; } //2-byte
		else if (str[i] <= 0xEF) { i += 3; length++; } //3-byte
		else                     { i++;              } //4-byte, restricted or invalid range ->ignore
	}
	return length;
}

int cccStrlenCode(cccCode const * str, unsigned char cp) {
	if (!str || *str == '\0') return 0;
	
	int length = 0;
    switch (cp) {
		/* multi byte character sets */
		case CCC_CP932:  length = cccStrlenSJIS(str); break;
		case CCC_CP936:  length = cccStrlenGBK (str); break;
		case CCC_CP949:  length = cccStrlenKOR (str); break;
		case CCC_CP950:  length = cccStrlenBIG5(str); break;
		case CCC_CPUTF8: length = cccStrlenUTF8(str); break;
		/* single byte character sets */
		default:        length = cccStrlen(str); break;
	}
	return length;
}

int cccStrlenUCS2(cccUCS2 const * str) {
	if (!str || *str == '\0') return 0;

	int length = 0;
	while (str[length]) length++;
	return length;
}


int cccSJIStoUCS2(cccUCS2 * dst, size_t count, cccCode const * str) {
	if (!str || *str == '\0' || !dst) return 0;
	if (!cccInitialized) cccInit();

	int i = 0, length = 0;
	while (str[i] && length < count) {
        if (str[i] <= 0x7f) {
			dst[length] = (cccUCS2)str[i]; 
		} else if (str[i] <= 0x80) {
			dst[length] = __error_char_ucs2__;
		} else if (str[i] <= 0x84) {
			if ((str[i+1] >= 0x40) && (str[i+1] <= 0xfc) && (__table_ptr__[CCC_CP932])) {
				dst[length] = (__table_ptr__[CCC_CP932])[(str[i]-0x81)*0xbd+str[i+1]-0x40];
                if (!dst[length]) dst[length] = __error_char_ucs2__;
			} else {
				dst[length] = __error_char_ucs2__;
			}
        } else if (str[i] <= 0x86) {
			dst[length] = __error_char_ucs2__;
		} else if (str[i] <= 0x9f) {
			if ((str[i+1] >= 0x40) && (str[i+1] <= 0xfc) && (__table_ptr__[CCC_CP932])) {
				dst[length] = (__table_ptr__[CCC_CP932])[(str[i]-0x83)*0xbd+str[i+1]-0x40];
                if (!dst[length]) dst[length] = __error_char_ucs2__;
			} else {
				dst[length] = __error_char_ucs2__;
			}
        } else if (str[i] <= 0xa0) {
			dst[length] = __error_char_ucs2__;
        } else if (str[i] <= 0xdf) {
			dst[length] = 0xff60u + str[i] - 0xa0u;
		} else if (str[i] <= 0xea) {
			if ((str[i+1] >= 0x40) && (str[i+1] <= 0xfc) && (__table_ptr__[CCC_CP932])) {
				dst[length] = (__table_ptr__[CCC_CP932])[(str[i]-0xc3)*0xbd+str[i+1]-0x40];
                if (!dst[length]) dst[length] = __error_char_ucs2__;
			} else {
				dst[length] = __error_char_ucs2__;
			}
        } else if (str[i] <= 0xec) {
			dst[length] = __error_char_ucs2__;
		} else if (str[i] <= 0xee) {
			if ((str[i+1] >= 0x40) && (str[i+1] <= 0xfc) && (__table_ptr__[CCC_CP932])) {
				dst[length] = (__table_ptr__[CCC_CP932])[(str[i]-0xc5)*0xbd+str[i+1]-0x40];
                if (!dst[length]) dst[length] = __error_char_ucs2__;
			} else {
				dst[length] = __error_char_ucs2__;
			}
         } else if (str[i] <= 0xf9) {
			dst[length] = __error_char_ucs2__;
		} else if (str[i] <= 0xfc) {
			if ((str[i+1] >= 0x40) && (str[i+1] <= 0xfc) && (__table_ptr__[CCC_CP932])) {
				dst[length] = (__table_ptr__[CCC_CP932])[(str[i]-0xd0)*0xbd+str[i+1]-0x40];
                if (!dst[length]) dst[length] = __error_char_ucs2__;
			} else {
				dst[length] = __error_char_ucs2__;
			}
		} else {
			dst[length] = __error_char_ucs2__;
		}
		length++;
        i += (str[i] <= 0x80 || (str[i] >= 0xA0 && str[i] <= 0xDF) || str[i] >= 0xFD) ? 1 : 2; //single or double byte
	}
	return length;
}

int cccGBKtoUCS2(cccUCS2 * dst, size_t count, cccCode const * str) {
	if (!str || *str == '\0' || !dst) return 0;
	if (!cccInitialized) cccInit();
	//if (!__table_ptr__[cccCP936]) cccSetTable(cccLoadTable("flash0:/kd/resource/gbk_table.dat"), cccCP936, 1);

	int i = 0, length = 0;
	while (str[i] && length < count) {
		if (str[i] <= 0x7f) {
			dst[length] = (cccUCS2)str[i]; 
		} else if (str[i] <= 0x80) {
			dst[length] = 0x20ac;
        } else if (str[i] <= 0xfe) {
			if ((str[i+1] >= 0x40) && (str[i+1] <= 0xfe) && (__table_ptr__[CCC_CP936])) {
				dst[length] = (__table_ptr__[CCC_CP936])[(str[i]-0x81)*0xbf+str[i+1]-0x40];
                if (!dst[length]) dst[length] = __error_char_ucs2__;
			} else {
				dst[length] = __error_char_ucs2__;
			}
		} else {
			dst[length] = __error_char_ucs2__;
		}
        length++;
        i += (str[i] <= 0x80 || str[i] == 0xFF) ? 1 : 2; //single or double byte
	}
	return length;
}

int cccKORtoUCS2(cccUCS2 * dst, size_t count, cccCode const * str) {
	if (!str || *str == '\0' || !dst) return 0;
	if (!cccInitialized) cccInit();
	//if (!__table_ptr__[cccCP949]) cccSetTable(cccLoadTable("flash0:/kd/resource/cp949_table.dat"), cccCP949, 1);

	int i = 0, length = 0;

	while (str[i] && length < count) {
		if (str[i] <= 0x7f) {
			dst[length] = (cccUCS2)str[i]; 
		} else if (str[i] <= 0x80) {
			dst[length] = __error_char_ucs2__;
		} else if (str[i] <= 0xfd) {
			if ((str[i+1] >= 0x40) && (str[i+1] <= 0xfe) && (__table_ptr__[CCC_CP949])) {
				dst[length] = (__table_ptr__[CCC_CP949])[(str[i]-0x81)*0xbf+str[i+1]-0x40];
                if (!dst[length]) dst[length] = __error_char_ucs2__;
			} else {
				dst[length] = __error_char_ucs2__;
			}
        } else if (str[i] <= 0xfe) {
            dst[length] = __error_char_ucs2__;
		} else {
			dst[length] = __error_char_ucs2__;
		}
        length++;
        i += (str[i] <= 0x80 || str[i] == 0xFF) ? 1 : 2; //single or double byte
	}
	return length;
}

int cccBIG5toUCS2(cccUCS2 * dst, size_t count, cccCode const * str) {
	if (!str || *str == '\0' || !dst) return 0;
	if (!cccInitialized) cccInit();
	//if (!__table_ptr__[cccCP950]) cccSetTable(cccLoadTable("flash0:/kd/resource/big5_table.dat"), cccCP950, 1);

	int i = 0, length = 0;
	while (str[i] && length < count) {
		if (str[i] <= 0x7f) {
			dst[length] = (cccUCS2)str[i]; 
		} else if (str[i] <= 0x80) {
			dst[length] = __error_char_ucs2__;
        } else if (str[i] <= 0xa0) {
			dst[length] = __error_char_ucs2__;
		} else if (str[i] <= 0xf9) {
			if ((((str[i+1] >= 0x40) && (str[i+1] <= 0x7e)) || ((str[i+1] >= 0xa1) && (str[i+1] <= 0xfe))) && (__table_ptr__[CCC_CP950])) {
				dst[length] = (__table_ptr__[CCC_CP950])[(str[i]-0xa1)*0x9d+str[i+1]-((str[i+1] <= 0x7e) ? 0x40 : 0x62)];
                if (!dst[length]) dst[length] = __error_char_ucs2__;
			} else {
				dst[length] = __error_char_ucs2__;
			}
        } else if (str[i] <= 0xfe) {
			dst[length] = __error_char_ucs2__;
		} else {
			dst[length] = __error_char_ucs2__;
		}
        length++;
        i += (str[i] <= 0x80 || str[i] == 0xFF) ? 1 : 2; //single or double byte
	}
	return length;
}

int cccUTF8toUCS2(cccUCS2 * dst, size_t count, cccCode const * str) {
	if (!str || *str == '\0' || !dst) return 0;

    int i = 0, length = 0;
    while (str[i] && length < count) {
		if  (str[i] <= 0x7FU) {       //ASCII
			dst[length] = (cccUCS2)str[i]; 
			i++;    length++; 
		} else if (str[i] <= 0xC1U) { //part of multi-byte or overlong encoding ->ignore
			i++;          
		} else if (str[i] <= 0xDFU) { //2-byte
			dst[length] = ((str[i]&0x001fu)<<6) | (str[i+1]&0x003fu); 
			i += 2; length++; 
		} else if (str[i] <= 0xEFU) { //3-byte
			dst[length] = ((str[i]&0x001fu)<<12) | ((str[i+1]&0x003fu)<<6) | (str[i+2]&0x003fu); 
			i += 3; length++; 
		} else i++;                    //4-byte, restricted or invalid range ->ignore
	}
    return length;
}

int cccCodetoUCS2(cccUCS2 * dst, size_t count, cccCode const * str, unsigned char cp) {
	int length = 0;
	switch (cp) {
		//multi-byte character sets
		case CCC_CP932: length = cccSJIStoUCS2(dst, count, str); break;
		case CCC_CP936: length = cccGBKtoUCS2(dst, count, str); break;
		case CCC_CP949: length = cccKORtoUCS2(dst, count, str); break;
		case CCC_CP950: length = cccBIG5toUCS2(dst, count, str); break;
		case CCC_CPUTF8: length = cccUTF8toUCS2(dst, count, str); break;
		//single-byte character sets
		default: 
			if (cp < CCC_N_CP) { //codepage in range?
				if (!cccInitialized) cccInit();
				while (str[length] && length < count) { //conversion: ASCII (if ASCII) or LUT-value (if LUT exists) or error_char (if LUT doesn't exist)
					if (str[length] < 0x80) {
						dst[length] = (cccUCS2)str[length];
					} else {
						dst[length] =  ((__table_ptr__[cp]) ? (__table_ptr__[cp])[str[length]-0x80] : __error_char_ucs2__); 
						if (!dst[length]) dst[length] = __error_char_ucs2__;
					}
					length++; 
				}
			}
			break;
	} 	
	return length;
}

