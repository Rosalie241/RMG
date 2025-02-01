/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "CachedRomHeaderAndSettings.hpp"
#include "Directories.hpp"
#include "RomSettings.hpp"
#include "RomHeader.hpp"

#include "osal/osal_files.hpp"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <vector>

#ifdef _WIN32
#include <Windows.h>
#else // Linux
#include <linux/limits.h>
#endif // _WIN32

//
// Local Defines
//

#ifdef _WIN32
#define MAX_FILENAME_LEN MAX_PATH
#else // Linux
#define MAX_FILENAME_LEN PATH_MAX
#endif // _WIN32
#define ROMHEADER_NAME_LEN 256
#define GOODNAME_LEN 256
#define MD5_LEN 33
#define GAMEID_LEN 5
#define REGION_LEN 18

#ifdef _WIN32
#define CACHE_FILE_MAGIC "RMGCoreHeaderAndSettingsCacheWindows_06"
#else // Linux
#define CACHE_FILE_MAGIC "RMGCoreHeaderAndSettingsCacheLinux_06"
#endif // _WIN32
#define CACHE_FILE_ITEMS_MAX 10000

//
// Local Structures
//

struct l_CacheEntry
{
    std::filesystem::path fileName;
    osal_files_file_time  fileTime;

    CoreRomType     type;
    CoreRomHeader   header;
    CoreRomSettings settings;
};

//
// Local Variables
//

static bool                      l_CacheEntriesChanged = false;
static std::vector<l_CacheEntry> l_CacheEntries;

//
// Internal Functions
//

static std::filesystem::path get_cache_file_name()
{
    std::filesystem::path file;

    file = CoreGetUserCacheDirectory();
    file += OSAL_FILES_DIR_SEPERATOR_STR;
    file += "RomHeaderAndSettingsCache.cache";

    return file;
}

static std::vector<l_CacheEntry>::iterator get_cache_entry_iter(std::filesystem::path file, bool checkFileTime = true)
{
    osal_files_file_time fileTime = osal_files_get_file_time(file);

    auto predicate = [file, fileTime, checkFileTime](const auto& entry)
    {
        return entry.fileName == file &&
                (!checkFileTime || entry.fileTime == fileTime);
    };

    return std::find_if(l_CacheEntries.begin(), l_CacheEntries.end(), predicate);
}

//
// Exported Functions
//

void CoreReadRomHeaderAndSettingsCache(void)
{
    std::ifstream inputStream;
    char magicBuf[sizeof(CACHE_FILE_MAGIC)];
    wchar_t fileNameBuf[MAX_FILENAME_LEN];
    char headerNameBuf[ROMHEADER_NAME_LEN];
    char gameIDBuf[GAMEID_LEN];
    char regionBuf[REGION_LEN];
    char goodNameBuf[GOODNAME_LEN];
    char md5Buf[MD5_LEN];
    uint32_t size;
    l_CacheEntry cacheEntry;

    inputStream.open(get_cache_file_name(), std::ios::binary);
    if (!inputStream.good())
    {
        return;
    }

    // when magic doesn't match, don't read cache file
    inputStream.read((char*)magicBuf, sizeof(CACHE_FILE_MAGIC));
    if (std::string(magicBuf) != std::string(CACHE_FILE_MAGIC))
    {
        inputStream.close();
        return;
    }

    // read all file entries
#define FREAD(x) inputStream.read((char*)&x, sizeof(x))
#define FREAD_STR(x, y) inputStream.read((char*)x, y)
    while (!inputStream.eof())
    {
        // reset buffers
        size = 0;
        memset(fileNameBuf, 0, sizeof(fileNameBuf));
        memset(headerNameBuf, 0, sizeof(headerNameBuf));
        memset(gameIDBuf, 0, sizeof(gameIDBuf));
        memset(regionBuf, 0, sizeof(regionBuf));
        memset(goodNameBuf, 0, sizeof(goodNameBuf));
        memset(md5Buf, 0, sizeof(md5Buf));

        // file info
        FREAD(size);
        FREAD_STR(fileNameBuf, size);
        cacheEntry.fileName = std::filesystem::path(fileNameBuf);
        FREAD(cacheEntry.fileTime);
        // type
        FREAD(cacheEntry.type);
        // header
        FREAD(size);
        FREAD_STR(headerNameBuf, size);
        FREAD(size);
        FREAD_STR(gameIDBuf, size);
        FREAD(size);
        FREAD_STR(regionBuf, size);
        cacheEntry.header.Name = std::string(headerNameBuf);
        cacheEntry.header.GameID = std::string(gameIDBuf);
        cacheEntry.header.Region = std::string(regionBuf);
        FREAD(cacheEntry.header.CRC1);
        FREAD(cacheEntry.header.CRC2);
        // (partial) settings
        FREAD(size);
        FREAD_STR(goodNameBuf, size);
        FREAD(size);
        FREAD_STR(md5Buf, size);
        cacheEntry.settings.GoodName = std::string(goodNameBuf);
        cacheEntry.settings.MD5 = std::string(md5Buf);

        // add to cached entries
        l_CacheEntries.push_back(cacheEntry);
    }
#undef FREAD
#undef FREAD_STR

    inputStream.close();
}

bool CoreSaveRomHeaderAndSettingsCache(void)
{
    std::ofstream outputStream;
    wchar_t fileNameBuf[MAX_FILENAME_LEN];
    char headerNameBuf[ROMHEADER_NAME_LEN];
    char gameIDBuf[GAMEID_LEN];
    char regionBuf[REGION_LEN];
    char goodNameBuf[GOODNAME_LEN];
    char md5Buf[MD5_LEN];
    uint32_t size;
    l_CacheEntry cacheEntry;

    // only save cache when the entries have changed
    if (!l_CacheEntriesChanged)
    {
        return true;
    }

    outputStream.open(get_cache_file_name(), std::ios::binary);
    if (!outputStream.good())
    {
        return false;
    }

    // write magic header
    outputStream.write((char*)CACHE_FILE_MAGIC, sizeof(CACHE_FILE_MAGIC));

    // write each entry in the file
#define FWRITE(x) outputStream.write((char*)&x, sizeof(x))
#define FWRITE_STR(x, y) outputStream.write((char*)x, y)
    for (auto iter = l_CacheEntries.cbegin(); iter != l_CacheEntries.end(); iter++)
    {
        cacheEntry = (*iter);

        // reset buffers
        size = 0;
        memset(fileNameBuf, 0, sizeof(fileNameBuf));
        memset(headerNameBuf, 0, sizeof(headerNameBuf));
        memset(gameIDBuf, 0, sizeof(gameIDBuf));
        memset(regionBuf, 0, sizeof(regionBuf));
        memset(goodNameBuf, 0, sizeof(goodNameBuf));
        memset(md5Buf, 0, sizeof(md5Buf));

        // copy strings into buffers
        wcsncpy(fileNameBuf, cacheEntry.fileName.wstring().c_str(), MAX_FILENAME_LEN);
        strncpy(headerNameBuf, cacheEntry.header.Name.c_str(), sizeof(headerNameBuf));
        strncpy(gameIDBuf, cacheEntry.header.GameID.c_str(), sizeof(gameIDBuf));
        strncpy(regionBuf, cacheEntry.header.Region.c_str(), sizeof(regionBuf));
        strncpy(goodNameBuf, cacheEntry.settings.GoodName.c_str(), sizeof(goodNameBuf));
        strncpy(md5Buf, cacheEntry.settings.MD5.c_str(), sizeof(md5Buf));

        // file info
        size = cacheEntry.fileName.wstring().size() * sizeof(wchar_t);
        FWRITE(size);
        FWRITE_STR(fileNameBuf, size);
        FWRITE(cacheEntry.fileTime);
        // type
        FWRITE(cacheEntry.type);
        // header
        size = cacheEntry.header.Name.size();
        FWRITE(size);
        FWRITE_STR(headerNameBuf, size);
        size = cacheEntry.header.GameID.size();
        FWRITE(size);
        FWRITE_STR(gameIDBuf, size);
        size = cacheEntry.header.Region.size();
        FWRITE(size);
        FWRITE_STR(regionBuf, size);
        FWRITE(cacheEntry.header.CRC1);
        FWRITE(cacheEntry.header.CRC2);
        // (partial) settings
        size = cacheEntry.settings.GoodName.size();
        FWRITE(size);
        FWRITE_STR(goodNameBuf, size);
        size = cacheEntry.settings.MD5.size();
        FWRITE(size);
        FWRITE_STR(md5Buf, size);
    }
#undef FWRITE
#undef FWRITE_STR

    outputStream.close();
    return true;
}

bool CoreHasRomHeaderAndSettingsCached(std::filesystem::path file)
{
    return get_cache_entry_iter(file) != l_CacheEntries.end();
}

bool CoreGetCachedRomHeaderAndSettings(std::filesystem::path file, CoreRomType& type, CoreRomHeader& header, CoreRomSettings& settings)
{
    auto iter = get_cache_entry_iter(file);
    if (iter == l_CacheEntries.end())
    {
        return false;
    }

    type     = (*iter).type;
    header   = (*iter).header;
    settings = (*iter).settings;
    return true;
}

bool CoreAddCachedRomHeaderAndSettings(std::filesystem::path file, CoreRomType type, CoreRomHeader header, CoreRomSettings settings)
{
    l_CacheEntry cacheEntry;

    // try to find existing entry with same filename,
    // when found, remove it from the cache
    auto iter = get_cache_entry_iter(file, false);
    if (iter != l_CacheEntries.end())
    {
        l_CacheEntries.erase(iter);
    }
    else if (l_CacheEntries.size() >= CACHE_FILE_ITEMS_MAX)
    { // delete first item when we're over the item limit
        l_CacheEntries.erase(l_CacheEntries.begin());
    }

    cacheEntry.fileName = file;
    cacheEntry.fileTime = osal_files_get_file_time(file);
    cacheEntry.type     = type;
    cacheEntry.header   = header;
    cacheEntry.settings = settings;

    l_CacheEntries.push_back(cacheEntry);
    l_CacheEntriesChanged = true;
    return true;
}

bool CoreUpdateCachedRomHeaderAndSettings(std::filesystem::path file)
{
    l_CacheEntry cachedEntry;
    CoreRomType type;
    CoreRomHeader header;
    CoreRomSettings settings;

    // try to find existing entry with same filename,
    // when not found, do nothing
    auto iter = get_cache_entry_iter(file, false);
    if (iter == l_CacheEntries.end())
    {
        return true;
    }
    cachedEntry = (*iter);

    // attempt to retrieve required information
    if (!CoreGetRomType(type) ||
        !CoreGetCurrentRomHeader(header) ||
        !CoreGetCurrentDefaultRomSettings(settings))
    {
        return false;
    }

    // check if the cached entry needs to be updated,
    // if it does, then update the entry
    if (/* rom type */
        cachedEntry.type != type ||
        /* header */
        cachedEntry.header.Name   != header.Name   ||
        cachedEntry.header.Region != header.Region ||
        cachedEntry.header.CRC1   != header.CRC1   ||
        cachedEntry.header.CRC2   != header.CRC2   ||
        /* settings */
        cachedEntry.settings.MD5      != settings.MD5 ||
        cachedEntry.settings.GoodName != settings.GoodName)
    {
        (*iter).type              = type;
        (*iter).header            = header;
        (*iter).settings.MD5      = settings.MD5;
        (*iter).settings.GoodName = settings.GoodName;
        l_CacheEntriesChanged     = true;
    }

    return true;
}

bool CoreClearRomHeaderAndSettingsCache(void)
{
    l_CacheEntries.clear();
    l_CacheEntriesChanged = true;
    return true;
}
