/*
 * Texture Filtering
 * Version:  1.0
 *
 * Copyright (C) 2007  Hiroshi Morii   All Rights Reserved.
 * Email koolsmoky(at)users.sourceforge.net
 * Web   http://www.3dfxzone.it/koolsmoky
 *
 * this is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * this is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GNU Make; see the file COPYING.  If not, write to
 * the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __TXIMAGE_H__
#define __TXIMAGE_H__

#include <stdio.h>
#include <png.h>
#include "TxInternal.h"

#ifndef WIN32
typedef struct tagBITMAPFILEHEADER {
  unsigned short bfType;
  unsigned long  bfSize;
  unsigned short bfReserved1;
  unsigned short bfReserved2;
  unsigned long  bfOffBits;
} BITMAPFILEHEADER;

typedef struct tagBITMAPINFOHEADER {
  unsigned long  biSize;
  long           biWidth;
  long           biHeight;
  unsigned short biPlanes;
  unsigned short biBitCount;
  unsigned long  biCompression;
  unsigned long  biSizeImage;
  long           biXPelsPerMeter;
  long           biYPelsPerMeter;
  unsigned long  biClrUsed;
  unsigned long  biClrImportant;
} BITMAPINFOHEADER;
#else
typedef struct tagBITMAPFILEHEADER BITMAPFILEHEADER;
typedef struct tagBITMAPINFOHEADER BITMAPINFOHEADER;
#endif

class TxImage
{
private:
  boolean getPNGInfo(FILE *fp, png_structp *png_ptr, png_infop *info_ptr);
  boolean getBMPInfo(FILE *fp, BITMAPFILEHEADER *bmp_fhdr, BITMAPINFOHEADER *bmp_ihdr);
public:
  TxImage() {}
  ~TxImage() {}
  uint8* readPNG(FILE* fp, int* width, int* height, ColorFormat* format);
  boolean writePNG(uint8* src, FILE* fp, int width, int height, int rowStride, ColorFormat format/*, uint8 *palette*/);
  uint8* readBMP(FILE* fp, int* width, int* height, ColorFormat* format);
};

#endif /* __TXIMAGE_H__ */
