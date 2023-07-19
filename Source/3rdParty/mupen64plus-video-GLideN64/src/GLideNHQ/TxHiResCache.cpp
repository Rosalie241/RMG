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

/* 2007 Gonetz <gonetz(at)ngs.ru>
 * Added callback to display hires texture info. */

#ifdef __MSC__
#pragma warning(disable: 4786)
#endif

#include "TxHiResCache.h"
#include "TxDbg.h"
#include <osal_files.h>
#include <osal_keys.h>
#include <zlib.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define HIRES_DUMP_ENABLED (FILE_HIRESTEXCACHE|DUMP_HIRESTEXCACHE)

TxHiResCache::~TxHiResCache()
{
}

TxHiResCache::TxHiResCache(int maxwidth,
						   int maxheight,
						   int maxbpp,
						   int options,
						   const wchar_t *cachePath,
						   const wchar_t *texPackPath,
						   const wchar_t *ident,
						   dispInfoFuncExt callback)
							 : TxCache((options & ~(GZ_TEXCACHE | FILE_TEXCACHE)), 0, cachePath, ident, callback)
							 , TxHiResLoader(maxwidth, maxheight, maxbpp, options)
							 , _abortLoad(false)
							 , _cacheDumped(false)
{
	if (texPackPath)
		_texPackPath.assign(texPackPath);

	if (_cachePath.empty() || _ident.empty()) {
		setOptions(getOptions() & ~HIRES_DUMP_ENABLED);
		return;
	}

	/* read in hires texture cache */
	if (getOptions() & HIRES_DUMP_ENABLED) {
		/* find it on disk */
		_cacheDumped = TxCache::load(!_HiResTexPackPathExists());
	}

	/* read in hires textures */
	if (!_cacheDumped) {
		if (_load(0) && (getOptions() & HIRES_DUMP_ENABLED) != 0)
			_cacheDumped = TxCache::save();
	}
}

void TxHiResCache::dump()
{
	if ((getOptions() & HIRES_DUMP_ENABLED) && !_cacheDumped && !_abortLoad && !empty()) {
	  /* dump cache to disk */
	  _cacheDumped = TxCache::save();
	}
}

tx_wstring TxHiResCache::_getFileName() const
{
	tx_wstring filename = _ident + wst("_HIRESTEXTURES.");
	filename += ((getOptions() & FILE_HIRESTEXCACHE) == 0) ? TEXCACHE_EXT : TEXSTREAM_EXT;
	removeColon(filename);
	return filename;
}

int TxHiResCache::_getConfig() const
{
	return getOptions() &
		(HIRESTEXTURES_MASK |
		FORCE16BPP_HIRESTEX |
		GZ_HIRESTEXCACHE |
		FILE_HIRESTEXCACHE |
		LET_TEXARTISTS_FLY);
}

boolean TxHiResCache::_HiResTexPackPathExists() const
{
	tx_wstring dir_path(_texPackPath);
	dir_path += OSAL_DIR_SEPARATOR_STR;
	dir_path += _ident;
	return osal_path_existsW(dir_path.c_str());
}

bool TxHiResCache::_load(boolean replace) /* 0 : reload, 1 : replace partial */
{
	if (_texPackPath.empty() || _ident.empty())
		return false;

	if (!replace)
		TxCache::clear();

	tx_wstring dir_path(_texPackPath);

	switch (getOptions() & HIRESTEXTURES_MASK) {
	case RICE_HIRESTEXTURES:
		INFO(80, wst("-----\n"));
		INFO(80, wst("using Rice hires texture format...\n"));
		INFO(80, wst("  must be one of the following;\n"));
		INFO(80, wst("    1) *_rgb.png + *_a.png\n"));
		INFO(80, wst("    2) *_all.png\n"));
		INFO(80, wst("    3) *_ciByRGBA.png\n"));
		INFO(80, wst("    4) *_allciByRGBA.png\n"));
		INFO(80, wst("    5) *_ci.bmp\n"));
		INFO(80, wst("  usage of only 2) and 3) highly recommended!\n"));
		INFO(80, wst("  folder names must be in US-ASCII characters!\n"));

		dir_path += OSAL_DIR_SEPARATOR_STR;
		dir_path += _ident;

		const LoadResult res = _loadHiResTextures(dir_path.c_str(), replace);
		if (res == resError) {
			if (_callback) (*_callback)(wst("Texture pack load failed. Clear hiresolution texture cache.\n"));
			INFO(80, wst("Texture pack load failed. Clear hiresolution texture cache.\n"));
			clear();
		}
		return res == resOk;
	}
	return false;
}

bool TxHiResCache::reload()
{
	return _load(0) && !TxCache::empty() && TxCache::save();
}

TxHiResCache::LoadResult TxHiResCache::_loadHiResTextures(const wchar_t * dir_path, boolean replace)
{
	DBG_INFO(80, wst("-----\n"));
	DBG_INFO(80, wst("path: %ls\n"), dir_path);

	/* find it on disk */
	if (!osal_path_existsW(dir_path)) {
		INFO(80, wst("Error: path not found!\n"));
		return resNotFound;
	}

	LoadResult result = resOk;

	void *dir = osal_search_dir_open(dir_path);
	const wchar_t *foundfilename;
	// the path of the texture
	tx_wstring texturefilename;

	do {
		osal_keys_update_state();
		if (osal_is_key_pressed(KEY_Escape, 0x0001)) {
			_abortLoad = true;
			if (_callback) (*_callback)(wst("Aborted loading hiresolution texture!\n"));
			INFO(80, wst("Error: aborted loading hiresolution texture!\n"));
		}
		if (_abortLoad)
			break;

		foundfilename = osal_search_dir_read_next(dir);
		// The array is empty,  break the current operation
		if (foundfilename == nullptr)
			break;

		if (!checkFolderName(foundfilename))
			continue;

		texturefilename.assign(dir_path);
		texturefilename += OSAL_DIR_SEPARATOR_STR;
		texturefilename += foundfilename;

		/* recursive read into sub-directory */
		if (osal_is_directory(texturefilename.c_str())) {
			result = _loadHiResTextures(texturefilename.c_str(), replace);
			if (result == resOk)
				continue;
			else
				break;
		}

		DBG_INFO(80, wst("-----\n"));
		DBG_INFO(80, wst("file: %ls\n"), foundfilename);

		int width = 0, height = 0;
		ColorFormat format = graphics::internalcolorFormat::NOCOLOR;
		uint8 *tex = nullptr;

		/* Rice hi-res textures: begin
		 */
		uint32 chksum = 0, fmt = 0, siz = 0, palchksum = 0, length = 0;
		FULLFNAME_CHARTYPE fullfname[MAX_PATH];
		char fname[MAX_PATH];
		char ident[MAX_PATH];
		FILE *fp = nullptr;

#ifdef _WIN32
		wcscpy(fullfname, texturefilename.c_str());
#else
		wcstombs(fullfname, texturefilename.c_str(), MAX_PATH);
#endif
		wcstombs(ident, _ident.c_str(), MAX_PATH);
		wcstombs(fname, foundfilename, MAX_PATH);

		/* lowercase on windows */
		CORRECTFILENAME(ident);
		CORRECTFILENAME(fname);

		/* read in Rice's file naming convention */
		length = checkFileName(ident, fname, &chksum, &palchksum, &fmt, &siz);
		if (length == 0) {
			/* invalid file name, skip it */
			continue;
		}

		/* check if we already have it in hires texture cache */
		if (!replace) {
			uint64 chksum64 = (uint64)palchksum;
			if (chksum) {
				chksum64 <<= 32;
				chksum64 |= (uint64)chksum;
			}
			if (isCached(chksum64, N64FormatSize(fmt, siz))) {
#if !DEBUG
				INFO(80, wst("-----\n"));
				INFO(80, wst("file: %s\n"), fname);
#endif
				INFO(80, wst("Error: already cached! duplicate texture!\n"));
				continue;
			}
		}

		tex = loadFileInfoTex(fullfname, fname, siz, &width, &height, fmt, &format);
		if (tex == nullptr) {
			/* failed to load file into tex data, skip it */
			continue;
		}

		DBG_INFO(80, wst("rom: %ls chksum:%08X %08X fmt:%x size:%x\n"), _ident.c_str(), chksum, palchksum, fmt, siz);

		/* load it into hires texture cache. */
		uint64 chksum64 = (uint64)palchksum;
		if (chksum) {
			chksum64 <<= 32;
			chksum64 |= (uint64)chksum;
		}

		GHQTexInfo tmpInfo;
		tmpInfo.data = tex;
		tmpInfo.width = width;
		tmpInfo.height = height;
		tmpInfo.is_hires_tex = 1;
		tmpInfo.n64_format_size = N64FormatSize(fmt, siz);
		setTextureFormat(format, &tmpInfo);

		/* remove redundant in cache */
		if (replace && TxCache::del(chksum64)) {
			DBG_INFO(80, wst("removed duplicate old cache.\n"));
		}

		/* add to cache */
		const boolean added = TxCache::add(chksum64, &tmpInfo);
		free(tex);
		if (added) {
			/* Callback to display hires texture info.
			 * Gonetz <gonetz(at)ngs.ru> */
			if (_callback) {
				wchar_t tmpbuf[MAX_PATH];
				mbstowcs(tmpbuf, fname, MAX_PATH);
				(*_callback)(wst("[%d] total mem:%.2fmb - %ls\n"), int(size()), (totalSize() / 1024) / 1024.0f, tmpbuf);
			}
			DBG_INFO(80, wst("texture loaded!\n"));
		}
		else {
			result = resError;
			break;
		}

	} while (foundfilename != nullptr);

	osal_search_dir_close(dir);

	return result;
}

bool TxHiResCache::empty() const
{
	return TxCache::empty();
}

bool TxHiResCache::add(Checksum checksum, GHQTexInfo *info, int dataSize)
{
	return TxCache::add(checksum, info, dataSize);
}

bool TxHiResCache::get(Checksum checksum, N64FormatSize n64FmtSz, GHQTexInfo *info)
{
	return TxCache::get(checksum, n64FmtSz, info);
}
