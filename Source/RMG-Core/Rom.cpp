/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#define CORE_INTERNAL
#include "Rom.hpp"
#include "Directories.hpp"
#include "Error.hpp"
#include "MediaLoader.hpp"
#include "m64p/Api.hpp"
#include "RomSettings.hpp"
#include "Cheats.hpp"
#include "osal/osal_files.hpp"
#include "CachedRomHeaderAndSettings.hpp"
#include "Utils/File.hpp"

// lzma includes
#include "../3rdParty/lzma/7zTypes.h"
#include "../3rdParty/lzma/7z.h"
#include "../3rdParty/lzma/7zAlloc.h"
#include "../3rdParty/lzma/7zBuf.h"
#include "../3rdParty/lzma/7zCrc.h"
#include "../3rdParty/lzma/7zFile.h"
#include "../3rdParty/lzma/7zVersion.h"

#include <string>
#include <unzip.h>
#include <zlib.h>
#include <fstream>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <iterator>

//
// Local Defines
//

#define UNZIP_READ_SIZE 67108860 /* 64 MiB */

//
// Local Variables
//

static bool l_HasRomOpen       = false;
static bool l_HasDisk          = false;
static bool l_HasExtractedDisk = false;
static std::filesystem::path l_ExtractedDiskPath;
static std::filesystem::path l_RomPath;

//
// Local Functions
//

static std::string to_lower_str(std::string str)
{
    std::string resultString = str;

    std::transform(resultString.begin(), resultString.end(), resultString.begin(), 
        [](unsigned char c)
        { 
            return std::tolower(c); 
        }
    );

    return resultString;
}

static voidpf zlib_filefunc_open(voidpf opaque, const void* filename, int mode)
{
    std::filesystem::path path = *(std::filesystem::path*)filename;

    // we need a static filestream
    static std::ifstream fileStream;

    // attempt to open file
    fileStream.open(path, std::ios::binary);
    if (!fileStream.is_open())
    {
        return nullptr;
    }

    return (voidpf)&fileStream;
}

static uLong zlib_filefunc_read(voidpf opaque, voidpf stream, void* buf, uLong size)
{
    std::ifstream* fileStream = (std::ifstream*)stream;
    fileStream->read((char*)buf, size);
    return fileStream->fail() ? fileStream->gcount() : size;
}

static ZPOS64_T zlib_filefunc_tell(voidpf opaque, voidpf stream)
{
    std::ifstream* fileStream = (std::ifstream*)stream;
    return fileStream->tellg();
}

static long zlib_filefunc_seek(voidpf opaque, voidpf stream, ZPOS64_T offset, int origin)
{
    std::ifstream* fileStream = (std::ifstream*)stream;
    std::ios_base::seekdir seekOrigin;

    switch (origin)
    {
    default:
        return 0;
    case ZLIB_FILEFUNC_SEEK_CUR:
        seekOrigin = std::ios::cur;
        break;
    case ZLIB_FILEFUNC_SEEK_END:
        seekOrigin = std::ios::end;
        break;
    case ZLIB_FILEFUNC_SEEK_SET:
        seekOrigin = std::ios::beg;
        break;
    }

    fileStream->seekg(offset, seekOrigin);
    return fileStream->fail() ? -1 : 0;
}

static int zlib_filefunc_close(voidpf opaque, voidpf stream)
{
    std::ifstream* fileStream = (std::ifstream*)stream;
    fileStream->close();
    return fileStream->fail() ? -1 : 0;
}

static int zlib_filefunc_testerror(voidpf opaque, voidpf stream)
{
    return errno;
}

static bool read_zip_file(std::filesystem::path file, std::filesystem::path& extractedFileName, bool& isDisk, std::vector<char>& outBuffer)
{
    std::string  error;
    std::fstream fileStream;

    unzFile           zipFile;
    unz_global_info64 zipInfo;

    zlib_filefunc64_def filefuncs;
    filefuncs.zopen64_file = zlib_filefunc_open;
    filefuncs.zread_file   = zlib_filefunc_read;
    filefuncs.zwrite_file  = nullptr;
    filefuncs.ztell64_file = zlib_filefunc_tell;
    filefuncs.zseek64_file = zlib_filefunc_seek;
    filefuncs.zclose_file  = zlib_filefunc_close;
    filefuncs.zerror_file  = zlib_filefunc_testerror;
    filefuncs.opaque       = nullptr;

    zipFile = unzOpen2_64((const void*)&file, &filefuncs);
    if (zipFile == nullptr)
    {
        if (errno == 0)
        {
            error = "read_zip_file: unzOpen Failed!";
        }
        else
        {
            error = "read_zip_file: unzOpen Failed: ";
            error += "failed to open file: ";
            error += strerror(errno);
            error += " (";
            error += std::to_string(errno);
            error += ")";
        }
        CoreSetError(error);
        return false;
    }

    if (unzGetGlobalInfo64(zipFile, &zipInfo) != UNZ_OK)
    {
        error = "read_zip_file: unzGetGlobalInfo Failed!";
        CoreSetError(error);
        return false;
    }

    for (uint64_t i = 0; i < zipInfo.number_entry; i++)
    {
        unz_file_info fileInfo;
        char          fileName[PATH_MAX];

        // if we can't retrieve file info,
        // skip the file
        if (unzGetCurrentFileInfo(zipFile, &fileInfo, fileName, PATH_MAX, nullptr, 0, nullptr, 0) != UNZ_OK)
        {
            continue;
        }

        // make sure file has supported file format,
        // if it does, read it in memory
        std::filesystem::path fileNamePath;
        // Windows sometimes throws an exception when assigning a string to a path
        // due to being unable to convert the character sequence.
        // so we have to catch the exception and do nothing
        try
        {
            fileNamePath = fileName;
        }
        catch (...)
        {
            // ignore exception
        }
        std::string fileExtension = fileNamePath.has_extension() ? fileNamePath.extension().string() : "";
        fileExtension = to_lower_str(fileExtension);
        if (fileExtension == ".z64" ||
            fileExtension == ".v64" ||
            fileExtension == ".n64" ||
            fileExtension == ".ndd" ||
            fileExtension == ".d64")
        {
            std::vector<char> buffer(UNZIP_READ_SIZE);
            int bytes_read = 0;

            outBuffer.reserve(UNZIP_READ_SIZE);

            if (unzOpenCurrentFile(zipFile) != UNZ_OK)
            {
                error = "read_zip_file Failed: unzOpenCurrentFile Failed!";
                CoreSetError(error);
                return false;
            }

            do
            {
                bytes_read = unzReadCurrentFile(zipFile, buffer.data(), UNZIP_READ_SIZE);
                if (bytes_read < 0)
                {
                    unzCloseCurrentFile(zipFile);
                    unzClose(zipFile);
                    error = "read_zip_file Failed: unzReadCurrentFile Failed: ";
                    error += std::to_string(bytes_read);
                    CoreSetError(error);
                    return false;
                }
                else if (bytes_read > 0)
                {
                    outBuffer.insert(outBuffer.end(), buffer.begin(), std::next(buffer.begin(), bytes_read));
                }
            } while (bytes_read > 0);

            extractedFileName = fileNamePath;
            isDisk            = (fileExtension == ".ndd" || fileExtension == ".d64");
            unzCloseCurrentFile(zipFile);
            unzClose(zipFile);
            return true;
        }

        // break when we've iterated over all entries
        if ((i + 1) >= zipInfo.number_entry)
        {
            break;
        }

        // move to next file
        if (unzGoToNextFile(zipFile) != UNZ_OK)
        {
            unzClose(zipFile);
            error = "read_zip_file Failed: unzGoToNextFile Failed!";
            CoreSetError(error);
            return false;
        }
    }

    error = "read_zip_file Failed: no valid ROMs found in zip!";
    CoreSetError(error);
    unzClose(zipFile);
    return false;
}

static bool read_7zip_file(std::filesystem::path file, std::filesystem::path& extractedFileName, bool& isDisk, std::vector<char>& outBuffer)
{
    std::string  error;

    const ISzAlloc alloc = { SzAlloc, SzFree };
    const size_t bufSize = ((size_t)1 << 18);

    ISzAlloc allocImp;
    ISzAlloc allocTempImp;

    CFileInStream archiveStream;
    CLookToRead2 lookStream;
    CSzArEx db;
    SRes res;

    // initialize allocator 
    allocImp     = alloc;
    allocTempImp = alloc;

    // try to open file
#ifdef _WIN32
    WRes wres = InFile_OpenW(&archiveStream.file, file.wstring().c_str());
#else
    WRes wres = InFile_Open(&archiveStream.file, file.string().c_str());
#endif // _WIN32
    if (wres != 0)
    {
        error = "read_7zip_file Failed: InFile_Open Failed: ";
        error += std::to_string(wres);
        CoreSetError(error);
        return false;
    }

    // create vtables for streams
    FileInStream_CreateVTable(&archiveStream);
    archiveStream.wres = 0;
    LookToRead2_CreateVTable(&lookStream, 0);
    lookStream.buf = nullptr;

    // allocate memory look reader
    lookStream.buf = (Byte*)ISzAlloc_Alloc(&allocImp, bufSize);
    if (lookStream.buf == nullptr)
    {
        error = "read_7zip_file Failed: ISzAlloc_Alloc Failed!";
        CoreSetError(error);
        return false;
    }

    // initialize look reader
    lookStream.bufSize = bufSize;
    lookStream.realStream = &archiveStream.vt;
    LookToRead2_INIT(&lookStream);

    // initialize CRC table
    CrcGenerateTable();

    // initialize archive
    SzArEx_Init(&db);

    // try to open file
    res = SzArEx_Open(&db, &lookStream.vt, &allocImp, &allocTempImp);
    if (res != SZ_OK)
    {
        error = "read_7zip_file Failed: SzArEx_Open Failed: ";
        error += std::to_string(res);
        CoreSetError(error);
        SzArEx_Free(&db, &allocImp);
        ISzAlloc_Free(&allocImp, lookStream.buf);
        return false;
    }

    for (uint32_t i = 0; i < db.NumFiles; i++)
    {
        size_t filename_size = 0;
        uint16_t fileName[PATH_MAX];

        // skip directories
        if (SzArEx_IsDir(&db, i))
        {
            continue;
        }

        // skip when filename size exceeds our buffer size
        filename_size = SzArEx_GetFileNameUtf16(&db, i, nullptr);
        if (filename_size > PATH_MAX)
        {
            continue;
        }

        SzArEx_GetFileNameUtf16(&db, i, fileName);

        std::filesystem::path fileNamePath;
        // Windows sometimes throws an exception when assigning a string to a path
        // due to being unable to convert the character sequence.
        // so we have to catch the exception and do nothing
        try
        {
            fileNamePath = (char16_t*)fileName;
        }
        catch (...)
        {
            // ignore exception
        }
        std::string fileExtension = fileNamePath.has_extension() ? fileNamePath.extension().string() : "";
        fileExtension = to_lower_str(fileExtension);
        if (fileExtension == ".z64" ||
            fileExtension == ".v64" ||
            fileExtension == ".n64" ||
            fileExtension == ".ndd" ||
            fileExtension == ".d64")
        {
            uint32_t blockIndex = 0xFFFFFFFF;
            uint8_t* readBuffer = nullptr;
            size_t   readBufferSize = 0;
            size_t   offset = 0;
            size_t   outSizeProcessed = 0;

            res = SzArEx_Extract(&db, &lookStream.vt, i,
                                    &blockIndex, &readBuffer, &readBufferSize,
                                    &offset, &outSizeProcessed,
                                    &allocImp, &allocTempImp);
            if (res != SZ_OK)
            {
                error = "read_7zip_file Failed: SzArEx_Extract Failed: ";
                error += std::to_string(res);
                CoreSetError(error);

                SzArEx_Free(&db, &allocImp);
                ISzAlloc_Free(&allocImp, lookStream.buf);
                File_Close(&archiveStream.file);
                ISzAlloc_Free(&allocImp, readBuffer);
                return false;
            }

            outBuffer.insert(outBuffer.begin(), readBuffer, readBuffer + outSizeProcessed);
            extractedFileName = fileNamePath;
            isDisk            = (fileExtension == ".ndd" || fileExtension == ".d64");

            SzArEx_Free(&db, &allocImp);
            ISzAlloc_Free(&allocImp, lookStream.buf);
            File_Close(&archiveStream.file);
            ISzAlloc_Free(&allocImp, readBuffer);
            return true;
        }
    }

    // cleanup
    SzArEx_Free(&db, &allocImp);
    ISzAlloc_Free(&allocImp, lookStream.buf);
    File_Close(&archiveStream.file);

    error = "read_7zip_file Failed: no valid ROMs found in 7zip!";
    CoreSetError(error);
    return false;
}

//
// Exported Functions
//

bool CoreOpenRom(std::filesystem::path file)
{
    std::string error;
    m64p_error  ret;
    std::vector<char> buf;
    std::string file_extension;

    if (!m64p::Core.IsHooked())
    {
        return false;
    }

    if (CoreHasRomOpen())
    {
        error = "CoreOpenRom Failed: ";
        error += "cannot open ROM when another ROM is already open!";
        CoreSetError(error);
        return false;
    }

    file_extension = file.has_extension() ? file.extension().string() : "";
    file_extension = to_lower_str(file_extension);

    if (file_extension == ".zip" || 
        file_extension == ".7z")
    {
        std::filesystem::path extracted_file;
        bool                  is_disk = false;

        if (file_extension == ".zip")
        {
            if (!read_zip_file(file, extracted_file, is_disk, buf))
            {
                return false;
            }
        }
        else
        {
            if (!read_7zip_file(file, extracted_file, is_disk, buf))
            {
                return false;
            }
        }

        if (is_disk)
        {
            std::filesystem::path disk_file;
            disk_file = CoreGetUserCacheDirectory();
            disk_file += "/extracted_disks/";
            disk_file += extracted_file.filename();

            // attempt to create extraction directory
            try
            {
                if (!std::filesystem::exists(disk_file.parent_path()) &&
                    !std::filesystem::create_directory(disk_file.parent_path()))
                {
                    throw std::exception();
                }
            }
            catch (...)
            {
                error = "CoreOpenRom Failed: ";
                error += "Failed to create \"";
                error += disk_file.parent_path().string();
                error += "\"!";
                CoreSetError(error);
                return false;
            }

            // attempt to write temporary file
            if (!CoreWriteFile(disk_file, buf))
            {
                return false;
            }

            CoreMediaLoaderSetDiskFile(disk_file);

            l_ExtractedDiskPath = disk_file;
        }

        l_HasDisk          = is_disk;
        l_HasExtractedDisk = is_disk;
    }
    else if (file_extension == ".d64" || 
             file_extension == ".ndd")
    {
        CoreMediaLoaderSetDiskFile(file);
        l_HasDisk          = true;
        l_HasExtractedDisk = false;
    }
    else
    {
        if (!CoreReadFile(file, buf))
        {
            return false;
        }

        l_HasDisk          = false;
        l_HasExtractedDisk = false;
    }

    if (l_HasDisk)
    {
        ret = m64p::Core.DoCommand(M64CMD_DISK_OPEN, 0, nullptr);
        error = "CoreOpenRom: m64p::Core.DoCommand(M64CMD_DISK_OPEN) Failed: ";
    }
    else
    {
        ret = m64p::Core.DoCommand(M64CMD_ROM_OPEN, buf.size(), buf.data());
        error = "CoreOpenRom: m64p::Core.DoCommand(M64CMD_ROM_OPEN) Failed: ";
    }

    if (ret != M64ERR_SUCCESS)
    {
        error += m64p::Core.ErrorMessage(ret);
        CoreSetError(error);
    }

    l_HasRomOpen = (ret == M64ERR_SUCCESS);

    if (l_HasRomOpen)
    {
        // store ROM path
        l_RomPath = file;
        // store default ROM settings
        CoreStoreCurrentDefaultRomSettings();
        // apply rom settings overlay
        CoreApplyRomSettingsOverlay();
        // update cached rom header and settings entry
        CoreUpdateCachedRomHeaderAndSettings(file);
    }

    return l_HasRomOpen;
}

bool CoreHasRomOpen(void)
{
    return l_HasRomOpen;
}

bool CoreGetRomType(CoreRomType& type)
{
    std::string error;

    if (!l_HasRomOpen)
    {
        error = "CoreGetRomType Failed: ";
        error += "cannot retrieve ROM type when no ROM has been opened!";
        CoreSetError(error);
        return false;
    }

    type = l_HasDisk ? CoreRomType::Disk : CoreRomType::Cartridge;
    return true;
}

bool CoreGetRomPath(std::filesystem::path& path)
{
    std::string error;

    if (!l_HasRomOpen)
    {
        error = "CoreGetRomPath Failed: ";
        error += "cannot retrieve ROM path when no ROM has been opened!";
        CoreSetError(error);
        return false;
    }

    path = l_RomPath;
    return true;
}

bool CoreCloseRom(void)
{
    std::string error;
    m64p_error ret;

    if (!m64p::Core.IsHooked())
    {
        return false;
    }

    if (!CoreHasRomOpen())
    {
        error = "CoreCloseRom Failed: ";
        error += "cannot close rom when no rom is open!";
        CoreSetError(error);
        return false;
    }

    if (!CoreClearCheats())
    {
        return false;
    }

    if (l_HasDisk)
    {
        ret = m64p::Core.DoCommand(M64CMD_DISK_CLOSE, 0, nullptr);
        error = "CoreCloseRom: m64p::Core.DoCommand(M64CMD_DISK_CLOSE) Failed: ";
    }
    else
    {
        ret = m64p::Core.DoCommand(M64CMD_ROM_CLOSE, 0, nullptr);
        error = "CoreCloseRom: m64p::Core.DoCommand(M64CMD_ROM_CLOSE) Failed: ";
    }

    if (ret != M64ERR_SUCCESS)
    {
        error += m64p::Core.ErrorMessage(ret);
        CoreSetError(error);
        return false;
    }

    // clear default ROM settings
    CoreClearCurrentDefaultRomSettings();

    // if removing the temporary extracted disk file
    // fails, then we shouldn't break the state of
    // whether we currently have an open ROM/disk,
    // so we can safely say the ROM/disk isn't open
    // anymore here
    l_HasRomOpen = false;

    // attempt to clean temporary extracted disk file
    if (l_HasExtractedDisk)
    {
        try
        {
            if (!std::filesystem::remove(l_ExtractedDiskPath))
            {
                throw std::exception();
            }
        }
        catch (...)
        {
            error = "CoreCloseRom: Failed to remove \"";
            error += l_ExtractedDiskPath.string();
            error += "\"!";
            CoreSetError(error);
            return false;
        }
    }

    return true;
}
