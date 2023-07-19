#include "TxHiResNoCache.h"

#include "TxDbg.h"
#include "TxFilterExport.h"
#include <osal_files.h>

TxHiResNoCache::TxHiResNoCache(int maxwidth,
			   int maxheight,
			   int maxbpp,
			   int options,
			   const wchar_t *cachePath,
			   const wchar_t *texPackPath,
			   const wchar_t *fullTexPath,
			   const wchar_t *ident,
			   dispInfoFuncExt callback)
	: TxHiResLoader(maxwidth, maxheight, maxbpp, options)
	, _fullTexPath(fullTexPath)
	, _ident(ident)
	, _callback(callback)
{
	/* store this for _createFileIndexInDir */
	wcstombs(_identc, _ident.c_str(), MAX_PATH);

	/* lowercase on windows */
	CORRECTFILENAME(_identc);

	_createFileIndex(false);
}

TxHiResNoCache::~TxHiResNoCache()
{
	_clear();
}

void TxHiResNoCache::_clear()
{
	/* free loaded textures */
	for (auto texMap : _loadedTex) {
		free(texMap.second.data);
	}

	/* clear all lists */
	_loadedTex.clear();
	_filesIndex.clear();
}

bool TxHiResNoCache::empty() const
{
	return _filesIndex.empty();
}

TxHiResNoCache::FileIndexMap::const_iterator TxHiResNoCache::findFile(Checksum checksum, N64FormatSize n64FmtSz) const
{
	auto range = _filesIndex.equal_range(checksum);
	for (auto it = range.first; it != range.second; ++it) {
		if (N64FormatSize(it->second.fmt, it->second.siz).formatsize() == n64FmtSz.formatsize())
			return it;
	}
	return _filesIndex.end();
}

bool TxHiResNoCache::get(Checksum checksum, N64FormatSize n64FmtSz, GHQTexInfo *info)
{
	if (!checksum)
		return false;

	uint32 chksum = checksum._texture;
	uint32 palchksum = checksum._palette;

	/* loop over each file from the index and try to match it with checksum */
	auto indexEntry = findFile(checksum, n64FmtSz);
	if (indexEntry == _filesIndex.cend()) {
		DBG_INFO(80, wst("TxNoCache::get: chksum:%08X %08X not found\n"), chksum, palchksum);
		return false;
	}

	auto entry = indexEntry->second;

	/* make sure to not load the same texture twice */
	auto findTex = [n64FmtSz, this](Checksum checksum)
	{
		auto range = _loadedTex.equal_range(checksum);
		for (auto it = range.first; it != range.second; ++it) {
			if (it->second.n64_format_size == n64FmtSz)
				return it;
		}
		return _loadedTex.end();
	};
	{
		auto loadedTexMap = findTex(checksum);
		if (loadedTexMap != _loadedTex.end()) {
			DBG_INFO(80, wst("TxNoCache::get: cached chksum:%08X %08X found\n"), chksum, palchksum);
			*info = loadedTexMap->second;
			return true;
		}
	}

	DBG_INFO(80, wst("TxNoCache::get: loading chksum:%08X %08X\n"), chksum, palchksum);

	/* load texture */
	int width = 0, height = 0;
	ColorFormat format;
	uint8_t* tex = TxHiResLoader::loadFileInfoTex(entry.fullfname, entry.fname, entry.siz, &width, &height, entry.fmt, &format);

	if (tex == nullptr) {
		/* failed to load texture, so return false */
		DBG_INFO(80, wst("TxNoCache::get: failed to load chksum:%08X %08X\n"), chksum, palchksum);
		return false;
	}

	DBG_INFO(80, wst("TxNoCache::get: loaded chksum:%08X %08X\n"), chksum, palchksum);

	info->data = tex;
	info->width = width;
	info->height = height;
	info->is_hires_tex = 1;
	info->n64_format_size = n64FmtSz;
	setTextureFormat(format, info);

	/* add to loaded textures */
	_loadedTex.insert(std::map<uint64, GHQTexInfo>::value_type(checksum, *info));
	return true;
}

bool TxHiResNoCache::reload()
{
	_clear();
	return _createFileIndex(true);
}

bool TxHiResNoCache::_createFileIndex(bool update)
{
	/* don't display anything during an update,
	*	it causes flicker on i.e an ssd
	*/
	if (!update && _callback) {
		_callback(L"CREATING FILE INDEX. PLEASE WAIT...");
	}

	_createFileIndexInDir(_fullTexPath, update);

	return true;
}

bool TxHiResNoCache::_createFileIndexInDir(tx_wstring directory, bool update)
{
	/* find it on disk */
	if (!osal_path_existsW(directory.c_str())) {
		return false;
	}

	void *dir = osal_search_dir_open(directory.c_str());
	const wchar_t *foundfilename;
	tx_wstring texturefilename;
	bool result = true;

	do {
		foundfilename = osal_search_dir_read_next(dir);
		if (foundfilename == nullptr) {
			/* no more files/directories */
			break;
		}

		if (!checkFolderName(foundfilename))
			continue;

		texturefilename.assign(directory);
		texturefilename += OSAL_DIR_SEPARATOR_STR;
		texturefilename += foundfilename;

		/* recursive read into sub-directory */
		if (osal_is_directory(texturefilename.c_str())) {
			result = _createFileIndexInDir(texturefilename.c_str(), update);
			if (result) {
				continue;
			} else {
				break;
			}
		}

		uint64 chksum64 = 0;
		uint32 chksum = 0, palchksum = 0, length = 0;
		FileIndexEntry entry;
		entry.fmt = entry.siz = 0;

#ifdef _WIN32
		wcscpy(entry.fullfname, texturefilename.c_str());
#else
		wcstombs(entry.fullfname, texturefilename.c_str(), MAX_PATH);
#endif
		wcstombs(entry.fname, foundfilename, MAX_PATH);

		/* lowercase on windows */
		CORRECTFILENAME(entry.fname);

		/* read in Rice's file naming convention */
		length = TxHiResLoader::checkFileName(_identc, entry.fname, &chksum, &palchksum, &entry.fmt, &entry.siz);
		if (length == 0) {
			/* invalid file name, skip it */
			continue;
		}

		chksum64 = (uint64)palchksum;
		if (chksum) {
			chksum64 <<= 32;
			chksum64 |= (uint64)chksum;
		}

		/* try to add entry to file index */
		if (findFile(chksum64, N64FormatSize(entry.fmt, entry.siz)) != _filesIndex.cend()) {
			/* technically we should probably fail here,
			 * however HTS & HTC both don't fail when there are duplicates,
			 * so to maintain backwards compatability, we won't either
			 */
			DBG_INFO(80, wst("TxNoCache::_createFileIndexInDir: failed to add cksum:%08X %08X file:%ls\n"), chksum, palchksum, texturefilename.c_str());
		} else {
			_filesIndex.insert(std::map<uint64, FileIndexEntry>::value_type(chksum64, entry));
			DBG_INFO(80, wst("TxNoCache::_createFileIndexInDir: added cksum:%08X %08X file:%ls\n"), chksum, palchksum, texturefilename.c_str());
		}

	} while (foundfilename != nullptr);

	osal_search_dir_close(dir);

	return result;
}

