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

#ifndef __TXHIRESCACHE_H__
#define __TXHIRESCACHE_H__

#include "TxCache.h"
#include "TxQuantize.h"
#include "TxImage.h"
#include "TxReSample.h"
#include "TxHiResLoader.h"

class TxHiResCache : public TxCache, public TxHiResLoader
{
private:
	bool _abortLoad;
	bool _cacheDumped;

  tx_wstring _texPackPath;
  enum LoadResult {
	  resOk,
	  resNotFound,
	  resError
  };
  LoadResult _loadHiResTextures(const wchar_t * dir_path, boolean replace);
  boolean _HiResTexPackPathExists() const;
	tx_wstring _getFileName() const override;
	int _getConfig() const override;
  bool _load(boolean replace);

public:
  ~TxHiResCache();
  TxHiResCache(int maxwidth,
			   int maxheight,
			   int maxbpp,
			   int options,
			   const wchar_t *cachePath,
			   const wchar_t *texPackPath,
			   const wchar_t *ident,
			   dispInfoFuncExt callback);
  bool empty() const override;
  bool add(Checksum checksum, GHQTexInfo *info, int dataSize = 0) override;
  bool get(Checksum checksum, N64FormatSize n64FmtSz, GHQTexInfo *info) override;
  bool reload() override;
  void dump() override;
};

#endif /* __TXHIRESCACHE_H__ */
