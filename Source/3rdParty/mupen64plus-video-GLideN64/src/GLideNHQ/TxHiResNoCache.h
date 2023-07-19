#ifndef TXHIRESNOCACHE_H
#define TXHIRESNOCACHE_H

#include "TxHiResLoader.h"

class TxHiResNoCache : public TxHiResLoader
{
	private:
		bool _createFileIndex(bool update);
		bool _createFileIndexInDir(tx_wstring directory, bool update);
		void _clear();

		struct FileIndexEntry
		{
			FULLFNAME_CHARTYPE fullfname[MAX_PATH];
			char fname[MAX_PATH];
			uint32 siz;
			uint32 fmt;
		};

		tx_wstring _fullTexPath;
		tx_wstring _ident;
		char _identc[MAX_PATH];
		using FileIndexMap = std::multimap<uint64, FileIndexEntry>;
		FileIndexMap _filesIndex;
		FileIndexMap::const_iterator findFile(Checksum checksum, N64FormatSize n64FmtSz) const;
		std::multimap<uint64, GHQTexInfo> _loadedTex;
		dispInfoFuncExt _callback;
	public:
		~TxHiResNoCache();
  		TxHiResNoCache(int maxwidth,
			   int maxheight,
			   int maxbpp,
			   int options,
			   const wchar_t *cachePath,
			   const wchar_t *texPackPath,
			   const wchar_t *fullTexPath,
			   const wchar_t *ident,
			   dispInfoFuncExt callback);
  		bool empty() const override;
  		bool add(Checksum checksum, GHQTexInfo *info, int dataSize = 0) override { return false; }
		bool get(Checksum checksum, N64FormatSize n64FmtSz, GHQTexInfo *info) override;
  		bool reload() override;
  		void dump() override { };
};

#endif /* TXHIRESNOCACHE_H */
