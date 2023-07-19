#ifndef TXHIRESLOADER_H
#define TXHIRESLOADER_H

/* support hires textures
 *   0: disable
 *   1: enable
 */
#define HIRES_TEXTURE 1

#ifdef OS_WINDOWS
#define CORRECTFILENAME(str)  for (uint32 i = 0; i < strlen(str); i++) str[i] = tolower(str[i])
#else
#define CORRECTFILENAME(str)
#endif /* OS_WINDOWS */

#include "TxCache.h"
#include "TxQuantize.h"
#include "TxImage.h"
#include "TxReSample.h"

#ifdef _WIN32
#define FULLFNAME_CHARTYPE wchar_t
#else
#define FULLFNAME_CHARTYPE char
#endif

class TxHiResLoader
{
protected:
	bool checkFolderName(const wchar_t *folderName) const;
	uint32_t checkFileName(char* ident, char* fname, uint32_t* pChksum, uint32_t* pPalchksum, uint32_t* pFmt, uint32_t* pSiz) const;
	uint8_t* loadFileInfoTex(FULLFNAME_CHARTYPE* fullfname, char* fname, int siz, int* pWidth, int* pHeight, uint32_t fmt, ColorFormat* pFormat) const;

	std::unique_ptr<TxImage> _txImage;
	std::unique_ptr<TxQuantize> _txQuantize;
	std::unique_ptr<TxReSample> _txReSample;

	int _maxwidth;
	int _maxheight;
	int _maxbpp;
	int _options;

public:
	TxHiResLoader(int maxwidth,
			   int maxheight,
			   int maxbpp,
			   int options);
	virtual ~TxHiResLoader(){}

	virtual bool empty() const = 0;
	virtual bool add(Checksum checksum, GHQTexInfo *info, int dataSize = 0) = 0;
	virtual bool get(Checksum checksum, N64FormatSize n64FmtSz, GHQTexInfo *info) = 0;
	virtual bool reload() = 0;
	virtual void dump() = 0;
};

#endif /* TXHIRESLOADER_H */
