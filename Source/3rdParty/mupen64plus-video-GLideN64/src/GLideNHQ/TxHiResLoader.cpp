#include "TxHiResLoader.h"
#include "TxDbg.h"
#include "TxDbg.h"
#include "TxFilterExport.h"

#include <string.h>
#include <math.h>
#include <osal_files.h>

/* use power of 2 texture size
 * (0:disable, 1:enable, 2:3dfx) */
#define POW2_TEXTURES 0

/* use aggressive format assumption for quantization
 * (0:disable, 1:enable, 2:extreme) */
#define AGGRESSIVE_QUANTIZATION 1

TxHiResLoader::TxHiResLoader(int maxwidth,
			   int maxheight,
			   int maxbpp,
			   int options)
				: _txImage(new TxImage())
				, _txQuantize(new TxQuantize())
				, _txReSample(new TxReSample())
				, _maxwidth(maxwidth)
				, _maxheight(maxheight)
				, _maxbpp(maxbpp)
				, _options(options)
{

}

bool TxHiResLoader::checkFolderName(const wchar_t *folderName) const
{
	static tx_wstring skipFolderPrefix(wst("~!~"));
	tx_wstring folder(folderName);
	if (folder.compare(0, 1, wst(".")) == 0)
		/* skip hidden files */
		return false;

	return folder.compare(0, skipFolderPrefix.length(), skipFolderPrefix) != 0;
}

uint32_t TxHiResLoader::checkFileName(char* ident, char* filename,
	uint32_t* pChksum, uint32_t* pPalchksum,
	uint32_t* pFmt, uint32_t* pSiz) const
{
#define CRCFMTSIZ_LEN 13
#define CRCWILDCARD_LEN 15
#define PALCRC_LEN 9

	const char* strName;
	const char* pfilename;
	uint32_t length = 0, filename_type = 0;
	bool hasWildcard = false;
	static const char supported_ends[][20] = {
		"all.png",
#ifdef OS_WINDOWS
		"allcibyrgba.png",
		"cibyrgba.png",
#else
		"allciByRGBA.png",
		"ciByRGBA.png",
#endif
		"rgb.png",
		"rgb.bmp",
		"a.png",
		"a.bmp"
	};

	pfilename = filename + strlen(filename) - 4;

	if (strcmp(pfilename, ".png") &&
		strcmp(pfilename, ".bmp")) {
#if !DEBUG
		INFO(80, wst("-----\n"));
		INFO(80, wst("file: %s\n"), filename);
#endif
		INFO(80, wst("Error: not png or bmp!\n"));
		return 0;
	}

	/* make sure filename contains ident */
	pfilename = strstr(filename, ident);
	if (!pfilename) {
		return 0;
	}

	strName = pfilename + strlen(ident);

	/* wildcard support */
	if (strchr(strName, '$')) {
		if (sscanf(strName, "#%08X#%01X#%01X#$", pChksum, pFmt, pSiz) == 3) {
			filename_type = 1;
			length = CRCWILDCARD_LEN;
		} else if (sscanf(strName, "#$#%01X#%01X#%08X", pFmt, pSiz, pPalchksum) == 3) {
			filename_type = 2;
			length = CRCWILDCARD_LEN;
		}

		hasWildcard = (length != 0);
	} else {
		if (sscanf(strName, "#%08X#%01X#%01X#%08X", pChksum, pFmt, pSiz, pPalchksum) == 4) {
			filename_type = 3;
			length = CRCFMTSIZ_LEN + PALCRC_LEN;
		} else if (sscanf(strName, "#%08X#%01X#%01X", pChksum, pFmt, pSiz) == 3) {
			filename_type = 4;
			length = CRCFMTSIZ_LEN;
		}
	}

	/* try to re-create string and match it */
	bool supportedFilename = false;
	char test_filename[MAX_PATH];
	for (int i = 0; length && i < (sizeof(supported_ends) / sizeof(supported_ends[0])); i++) {
		char* end = (char*)supported_ends[i];

		switch (filename_type)
		{
			default:
			case 1:
				sprintf(test_filename, "%s#%08X#%01X#%01X#$_%s", ident, *pChksum, *pFmt, *pSiz, end);
				break;
			case 2:
				sprintf(test_filename, "%s#$#%01X#%01X#%08X_%s", ident, *pFmt, *pSiz, *pPalchksum, end);
				break;
			case 3:
				sprintf(test_filename, "%s#%08X#%01X#%01X#%08X_%s", ident, *pChksum, *pFmt, *pSiz, *pPalchksum, end);
				break;
			case 4:
				sprintf(test_filename, "%s#%08X#%01X#%01X_%s", ident, *pChksum, *pFmt, *pSiz, end);
				break;
		}

		/* lowercase on windows */
		CORRECTFILENAME(test_filename);

		/* when it matches, break */
		if (strcmp(test_filename, filename) == 0) {
			supportedFilename = true;
			break;
		}
	}

	if (!supportedFilename || !length) {
#if !DEBUG
		INFO(80, wst("-----\n"));
		INFO(80, wst("file: %s\n", filename));
#endif
		INFO(80, wst("Error: not Rice texture naming convention!\n"));
		return 0;
	}

	if (!*pChksum && !hasWildcard) {
#if !DEBUG
		INFO(80, wst("-----\n"));
		INFO(80, wst("file: %s\n"), filename);
#endif
		INFO(80, wst("Error: crc32 = 0!\n"));
		return 0;
	}

	return length;
}

uint8_t* TxHiResLoader::loadFileInfoTex(FULLFNAME_CHARTYPE* fullfname, char* fname,
	int siz, int* pWidth, int* pHeight,
	uint32_t fmt,
	ColorFormat* pFormat) const
{
	/* Deal with the wackiness some texture packs utilize Rice format.
	 * Read in the following order: _a.* + _rgb.*, _all.png _ciByRGBA.png,
	 * _allciByRGBA.png, and _ci.bmp. PNG are prefered over BMP.
	 *
	 * For some reason there are texture packs that include them all. Some
	 * even have RGB textures named as _all.* and ARGB textures named as
	 * _rgb.*... Someone pleeeez write a GOOD guideline for the texture
	 * designers!!!
	 *
	 * We allow hires textures to have higher bpp than the N64 originals.
	 */
	/* N64 formats
	 * Format: 0 - RGBA, 1 - YUV, 2 - CI, 3 - IA, 4 - I
	 * Size:   0 - 4bit, 1 - 8bit, 2 - 16bit, 3 - 32 bit
	 */

#ifdef _WIN32
#define FOPEN _wfopen
#define FOPEN_MODE L"rb"
#else
#define FOPEN fopen
#define FOPEN_MODE "rb"
#endif

	uint8_t* tex = nullptr;
	uint8_t* tmptex = nullptr;
	int tmpwidth = 0, tmpheight = 0;
	int width = 0, height = 0;
	FILE* fp = nullptr;

	ColorFormat tmpformat = graphics::internalcolorFormat::NOCOLOR;
	ColorFormat destformat = graphics::internalcolorFormat::NOCOLOR;
	ColorFormat format = graphics::internalcolorFormat::NOCOLOR;

	char* pfname;

	/*
	 * read in _rgb.* and _a.*
	 */
	if ((pfname = strstr(fname, "_rgb.")) ||
		(pfname = strstr(fname, "_a."))) {
		strcpy(pfname, "_rgb.png");
		if (!osal_path_existsA(fname)) {
			strcpy(pfname, "_rgb.bmp");
			if (!osal_path_existsA(fname)) {
#if !DEBUG
				INFO(80, wst("-----\n"));
				INFO(80, wst("file: %s\n"), fname);
#endif
				INFO(80, wst("Error: missing _rgb.*! _a.* must be paired with _rgb.*!\n"));
				return nullptr;;
			}
		}
		/* _a.png */
		strcpy(pfname, "_a.png");
		if ((fp = FOPEN(fullfname, FOPEN_MODE)) != nullptr) {
			tmptex = _txImage->readPNG(fp, &tmpwidth, &tmpheight, &tmpformat);
			fclose(fp);
		}
		if (!tmptex) {
			/* _a.bmp */
			strcpy(pfname, "_a.bmp");
			if ((fp = FOPEN(fullfname, FOPEN_MODE)) != nullptr) {
				tmptex = _txImage->readBMP(fp, &tmpwidth, &tmpheight, &tmpformat);
				fclose(fp);
			}
		}
		/* _rgb.png */
		strcpy(pfname, "_rgb.png");
		if ((fp = FOPEN(fullfname, FOPEN_MODE)) != nullptr) {
			tex = _txImage->readPNG(fp, &width, &height, &format);
			fclose(fp);
		}
		if (!tex) {
			/* _rgb.bmp */
			strcpy(pfname, "_rgb.bmp");
			if ((fp = FOPEN(fullfname, FOPEN_MODE)) != nullptr) {
				tex = _txImage->readBMP(fp, &width, &height, &format);
				fclose(fp);
			}
		}
		if (tmptex) {
			/* check if _rgb.* and _a.* have matching size and format. */
			if (!tex || width != tmpwidth || height != tmpheight ||
				format != graphics::internalcolorFormat::RGBA8 || tmpformat != graphics::internalcolorFormat::RGBA8) {
#if !DEBUG
				INFO(80, wst("-----\n"));
				INFO(80, wst("file: %s\n"), fname);
#endif
				if (!tex) {
					INFO(80, wst("Error: missing _rgb.*!\n"));
				}
				else if (width != tmpwidth || height != tmpheight) {
					INFO(80, wst("Error: _rgb.* and _a.* have mismatched width or height!\n"));
				}
				else if (format != graphics::internalcolorFormat::RGBA8 || tmpformat != graphics::internalcolorFormat::RGBA8) {
					INFO(80, wst("Error: _rgb.* or _a.* not in 32bit color!\n"));
				}
				if (tex) free(tex);
				free(tmptex);
				tex = nullptr;
				tmptex = nullptr;
				return nullptr;
			}
		}
		/* make adjustments */
		if (tex) {
			if (tmptex) {
				/* merge (A)RGB and A comp */
				DBG_INFO(80, wst("merge (A)RGB and A comp\n"));
				int i;
				for (i = 0; i < height * width; i++) {
#if 1
					/* use R comp for alpha. this is what Rice uses. sigh... */
					((uint32*)tex)[i] &= 0x00ffffff;
					((uint32*)tex)[i] |= ((((uint32*)tmptex)[i] & 0xff) << 24);
#endif
#if 0
					/* use libpng style grayscale conversion */
					uint32 texel = ((uint32*)tmptex)[i];
					uint32 acomp = (((texel >> 16) & 0xff) * 6969 +
						((texel >>  8) & 0xff) * 23434 +
						((texel      ) & 0xff) * 2365) / 32768;
					((uint32*)tex)[i] = (acomp << 24) | (((uint32*)tex)[i] & 0x00ffffff);
#endif
#if 0
					/* use the standard NTSC gray scale conversion */
					uint32 texel = ((uint32*)tmptex)[i];
					uint32 acomp = (((texel >> 16) & 0xff) * 299 +
						((texel >>  8) & 0xff) * 587 +
						((texel      ) & 0xff) * 114) / 1000;
					((uint32*)tex)[i] = (acomp << 24) | (((uint32*)tex)[i] & 0x00ffffff);
#endif
				}
				free(tmptex);
				tmptex = nullptr;
			}
			else {
				/* clobber A comp. never a question of alpha. only RGB used. */
#if !DEBUG
				INFO(80, wst("-----\n"));
				INFO(80, wst("file: %ls\n"), fname);
#endif
				INFO(80, wst("Warning: missing _a.*! only using _rgb.*. treat as opaque texture.\n"));
				int i;
				for (i = 0; i < height * width; i++) {
					((uint32*)tex)[i] |= 0xff000000;
				}
			}
		}
	}
	else
		/*
		 * read in _all.png, _allciByRGBA.png,
		 * _ciByRGBA.png, _ci.bmp
		 */
		if (strstr(fname, "_all.png") ||
#ifdef OS_WINDOWS
				strstr(fname, "_allcibyrgba.png") ||
				strstr(fname, "_cibyrgba.png") ||
#else
				strstr(fname, "_allciByRGBA.png") ||
				strstr(fname, "_ciByRGBA.png") ||
#endif
				strstr(fname, "_ci.bmp")) {

				if ((fp = FOPEN(fullfname, FOPEN_MODE)) != nullptr) {
					if (strstr(fname, ".png"))
						tex = _txImage->readPNG(fp, &width, &height, &format);
					else
						tex = _txImage->readBMP(fp, &width, &height, &format);

					fclose(fp);
				}
		 }

		/* if we do not have a texture at this point we are screwed */
		if (!tex) {
#if !DEBUG
			INFO(80, wst("-----\n"));
			INFO(80, wst("file: %s\n"), fname);
#endif
			INFO(80, wst("Error: load failed!\n"));
			return nullptr;
		}
		DBG_INFO(80, wst("read in as %d x %d gfmt:%x\n"), tmpwidth, tmpheight, tmpformat);

		/* check if size and format are OK */
		if (!(format == graphics::internalcolorFormat::RGBA8 || format == graphics::internalcolorFormat::COLOR_INDEX8) ||
			(width * height) < 4) { /* TxQuantize requirement: width * height must be 4 or larger. */
			free(tex);
			tex = nullptr;
#if !DEBUG
			INFO(80, wst("-----\n"));
			INFO(80, wst("file: %ls\n"), fname);
#endif
			INFO(80, wst("Error: not width * height > 4 or 8bit palette color or 32bpp or dxt1 or dxt3 or dxt5!\n"));
			return nullptr;
		}

		/* analyze and determine best format to quantize */
		if (format == graphics::internalcolorFormat::RGBA8) {
			int i;
			int alphabits = 0;
			int fullalpha = 0;
			boolean intensity = 1;

			if (!(_options & LET_TEXARTISTS_FLY)) {
				/* HACK ALERT! */
				/* Account for Rice's weirdness with fmt:0 siz:2 textures.
				 * Although the conditions are relaxed with other formats,
				 * the D3D RGBA5551 surface is used for this format in certain
				 * cases. See Nintemod's SuperMario64 life gauge and power
				 * meter. The same goes for fmt:2 textures. See Mollymutt's
				 * PaperMario text. */
				if ((fmt == 0 && siz == 2) || fmt == 2) {
					DBG_INFO(80, wst("Remove black, white, etc borders along the alpha edges.\n"));
					/* round A comp */
					for (i = 0; i < height * width; i++) {
						uint32 texel = ((uint32*)tex)[i];
						((uint32*)tex)[i] = ((texel & 0xff000000) == 0xff000000 ? 0xff000000 : 0) |
							(texel & 0x00ffffff);
					}
					/* Substitute texel color with the average of the surrounding
					 * opaque texels. This removes borders regardless of hardware
					 * texture filtering (bilinear, etc). */
					int j;
					for (i = 0; i < height; i++) {
						for (j = 0; j < width; j++) {
							uint32 texel = ((uint32*)tex)[i * width + j];
							if ((texel & 0xff000000) != 0xff000000) {
								uint32 tmptexel[8];
								uint32 k, numtexel, r, g, b;
								numtexel = r = g = b = 0;
								memset(&tmptexel, 0, sizeof(tmptexel));
								if (i > 0) {
									tmptexel[0] = ((uint32*)tex)[(i - 1) * width + j];                        /* north */
									if (j > 0)         tmptexel[1] = ((uint32*)tex)[(i - 1) * width + j - 1]; /* north-west */
									if (j < width - 1) tmptexel[2] = ((uint32*)tex)[(i - 1) * width + j + 1]; /* north-east */
								}
								if (i < height - 1) {
									tmptexel[3] = ((uint32*)tex)[(i + 1) * width + j];                        /* south */
									if (j > 0)         tmptexel[4] = ((uint32*)tex)[(i + 1) * width + j - 1]; /* south-west */
									if (j < width - 1) tmptexel[5] = ((uint32*)tex)[(i + 1) * width + j + 1]; /* south-east */
								}
								if (j > 0)         tmptexel[6] = ((uint32*)tex)[i * width + j - 1]; /* west */
								if (j < width - 1) tmptexel[7] = ((uint32*)tex)[i * width + j + 1]; /* east */
								for (k = 0; k < 8; k++) {
									if ((tmptexel[k] & 0xff000000) == 0xff000000) {
										b += ((tmptexel[k] & 0x00ff0000) >> 16);
										g += ((tmptexel[k] & 0x0000ff00) >> 8);
										r += ((tmptexel[k] & 0x000000ff));
										numtexel++;
									}
								}
								if (numtexel) {
									((uint32*)tex)[i * width + j] = ((b / numtexel) << 16) |
										((g / numtexel) << 8) |
										((r / numtexel));
								}
								else {
									((uint32*)tex)[i * width + j] = texel & 0x00ffffff;
								}
							}
						}
					}
				}
			}

			/* simple analysis of texture */
			for (i = 0; i < height * width; i++) {
				uint32 texel = ((uint32*)tex)[i];
				if (alphabits != 8) {
#if AGGRESSIVE_QUANTIZATION
					if ((texel & 0xff000000) < 0x00000003) {
						alphabits = 1;
						fullalpha++;
					} else if ((texel & 0xff000000) < 0xfe000000) {
						alphabits = 8;
					}
#else
					if ((texel & 0xff000000) == 0x00000000) {
						alphabits = 1;
						fullalpha++;
					} else if ((texel & 0xff000000) != 0xff000000) {
						alphabits = 8;
					}
#endif
				}
				if (intensity) {
					int rcomp = (texel >> 16) & 0xff;
					int gcomp = (texel >> 8) & 0xff;
					int bcomp = (texel)& 0xff;
#if AGGRESSIVE_QUANTIZATION
					if (abs(rcomp - gcomp) > 8 || abs(rcomp - bcomp) > 8 || abs(gcomp - bcomp) > 8)
						intensity = 0;
#else
					if (rcomp != gcomp || rcomp != bcomp || gcomp != bcomp) intensity = 0;
#endif
				}
				if (!intensity && alphabits == 8)
					break;
			}
			DBG_INFO(80, wst("required alpha bits:%d zero acomp texels:%d rgb as intensity:%d\n"), alphabits, fullalpha, intensity);

			/* preparations based on above analysis */
			if (_maxbpp < 32 || _options & FORCE16BPP_HIRESTEX) {
				if (alphabits == 0)
					destformat = graphics::internalcolorFormat::RGB8;
				else if (alphabits == 1)
					destformat = graphics::internalcolorFormat::RGB5_A1;
				else
					destformat = graphics::internalcolorFormat::RGBA8;
			}
			else {
				destformat = graphics::internalcolorFormat::RGBA8;
			}
			if (fmt == 4 && alphabits == 0) {
				destformat = graphics::internalcolorFormat::RGBA8;
				/* Rice I format; I = (R + G + B) / 3 */
				for (i = 0; i < height * width; i++) {
					uint32 texel = ((uint32*)tex)[i];
					uint32 icomp = (((texel >> 16) & 0xff) +
						((texel >> 8) & 0xff) +
						((texel)& 0xff)) / 3;
					((uint32*)tex)[i] = (icomp << 24) | (texel & 0x00ffffff);
				}
			}

			DBG_INFO(80, wst("best gfmt:%x\n"), u32(destformat));
		}
		/*
		 * Rice hi-res textures: end */


		/* XXX: only RGBA8888 for now. comeback to this later... */
		if (format == graphics::internalcolorFormat::RGBA8) {

			/* minification */
			if (width > _maxwidth || height > _maxheight) {
				int ratio = 1;
				if (width / _maxwidth > height / _maxheight) {
					ratio = (int)ceil((double)width / _maxwidth);
				}
				else {
					ratio = (int)ceil((double)height / _maxheight);
				}
				if (!_txReSample->minify(&tex, &width, &height, ratio)) {
					free(tex);
					tex = nullptr;
					DBG_INFO(80, wst("Error: minification failed!\n"));
					return nullptr;
				}
			}

#if POW2_TEXTURES
#if (POW2_TEXTURES == 2)
			/* 3dfx Glide3x aspect ratio (8:1 - 1:8) */
			if (!_txReSample->nextPow2(&tex, &width , &height, 32, 1)) {
#else
			/* normal pow2 expansion */
			if (!_txReSample->nextPow2(&tex, &width , &height, 32, 0)) {
#endif
				free(tex);
				tex = nullptr;
				DBG_INFO(80, wst("Error: aspect ratio adjustment failed!\n"));
				return nullptr;
			}
#endif

			/* quantize */
		{
			tmptex = (uint8 *)malloc(TxUtil::sizeofTx(width, height, destformat));
			if (tmptex == nullptr) {
				free(tex);
				tex = nullptr;
				return nullptr;
			}
			if (destformat == graphics::internalcolorFormat::RGBA8 ||
				destformat == graphics::internalcolorFormat::RGBA4) {
				if (_maxbpp < 32 || _options & FORCE16BPP_HIRESTEX)
					destformat = graphics::internalcolorFormat::RGBA4;
			}
			else if (destformat == graphics::internalcolorFormat::RGB5_A1) {
				if (_maxbpp < 32 || _options & FORCE16BPP_HIRESTEX)
					destformat = graphics::internalcolorFormat::RGB5_A1;
			}
			if (_txQuantize->quantize(tex, tmptex, width, height, graphics::internalcolorFormat::RGBA8, destformat, 0)) {
				format = destformat;
				free(tex);
				tex = tmptex;
			}
			else
				free(tmptex);
			tmptex = nullptr;
		}
	}

#undef FOPEN
#undef FOPEN_MODE

	/* last minute validations */
	if (!tex || !width || !height || format == graphics::internalcolorFormat::NOCOLOR || width > _maxwidth || height > _maxheight) {
#if !DEBUG
		INFO(80, wst("-----\n"));
		INFO(80, wst("file: %s\n"), fname);
#endif
		if (tex) {
			free(tex);
			tex = nullptr;
			INFO(80, wst("Error: bad format or size! %d x %d gfmt:%x\n"), width, height, u32(format));
		}
		else {
			INFO(80, wst("Error: load failed!!\n"));
		}
		return nullptr;
	}

	*pWidth = width;
	*pHeight = height;
	*pFormat = format;

	return tex;
}
