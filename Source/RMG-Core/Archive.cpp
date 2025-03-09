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
#include "Archive.hpp"

#include "Directories.hpp"
#include "Library.hpp"
#include "String.hpp"
#include "Error.hpp"

#include <cstring>
#include <fstream>

// lzma includes
#include <3rdParty/lzma/7zVersion.h>
#include <3rdParty/lzma/7zAlloc.h>
#include <3rdParty/lzma/7zTypes.h>
#include <3rdParty/lzma/7zFile.h>
#include <3rdParty/lzma/7zBuf.h>
#include <3rdParty/lzma/7zCrc.h>
#include <3rdParty/lzma/7z.h>

// minizip includes
#include <unzip.h>
#include <zlib.h>

//
// Local Defines
//

#define UNZIP_READ_SIZE 67108860 /* 64 MiB */

//
// Local Functions
//

static voidpf zlib_filefunc_open(voidpf opaque, const void* filename, int)
{
    const std::filesystem::path* path = static_cast<const std::filesystem::path*>(filename);
    std::ifstream* fileStream = static_cast<std::ifstream*>(opaque);

    // attempt to open file
    fileStream->open(*path, std::ios::binary);
    if (!fileStream->is_open())
    {
        return nullptr;
    }

    return static_cast<voidpf>(fileStream);
}

static uLong zlib_filefunc_read(voidpf, voidpf stream, void* buf, uLong size)
{
    std::ifstream* fileStream = static_cast<std::ifstream*>(stream);
    fileStream->read(static_cast<char*>(buf), size);
    return fileStream->fail() ? fileStream->gcount() : size;
}

static ZPOS64_T zlib_filefunc_tell(voidpf, voidpf stream)
{
    std::ifstream* fileStream = static_cast<std::ifstream*>(stream);
    return fileStream->tellg();
}

static long zlib_filefunc_seek(voidpf, voidpf stream, ZPOS64_T offset, int origin)
{
    std::ifstream* fileStream = static_cast<std::ifstream*>(stream);
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

static int zlib_filefunc_close(voidpf, voidpf stream)
{
    std::ifstream* fileStream = static_cast<std::ifstream*>(stream);
    fileStream->close();
    return fileStream->fail() ? -1 : 0;
}

static int zlib_filefunc_testerror(voidpf, voidpf)
{
    return errno;
}

//
// Exported Functions
//

CORE_EXPORT bool CoreReadZipFile(std::filesystem::path file, std::filesystem::path& extractedFileName, bool& isDisk, std::vector<char>& outBuffer)
{
    std::string  error;
    std::ifstream fileStream;

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
    filefuncs.opaque       = &fileStream;

    zipFile = unzOpen2_64(&file, &filefuncs);
    if (zipFile == nullptr)
    {
        if (errno == 0)
        {
            error = "CoreReadZipFile: unzOpen Failed!";
        }
        else
        {
            error = "CoreReadZipFile: unzOpen Failed: ";
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
        error = "CoreReadZipFile: unzGetGlobalInfo Failed!";
        CoreSetError(error);
        return false;
    }

    for (uint64_t i = 0; i < zipInfo.number_entry; i++)
    {
        unz_file_info fileInfo;
        char          fileName[CORE_DIR_MAX_LEN];

        // if we can't retrieve file info,
        // skip the file
        if (unzGetCurrentFileInfo(zipFile, &fileInfo, fileName, CORE_DIR_MAX_LEN, nullptr, 0, nullptr, 0) != UNZ_OK)
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
        fileExtension = CoreLowerString(fileExtension);
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
                error = "CoreReadZipFile Failed: unzOpenCurrentFile Failed!";
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
                    error = "CoreReadZipFile Failed: unzReadCurrentFile Failed: ";
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
            error = "CoreReadZipFile Failed: unzGoToNextFile Failed!";
            CoreSetError(error);
            return false;
        }
    }

    error = "CoreReadZipFile Failed: no valid ROMs found in zip!";
    CoreSetError(error);
    unzClose(zipFile);
    return false;
}

CORE_EXPORT bool CoreRead7zipFile(std::filesystem::path file, std::filesystem::path& extractedFileName, bool& isDisk, std::vector<char>& outBuffer)
{
    std::string  error;

    const ISzAlloc alloc = { SzAlloc, SzFree };
    const size_t bufSize = (static_cast<size_t>(1) << 18);

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
        error = "CoreRead7zipFile Failed: InFile_Open Failed: ";
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
    lookStream.buf = static_cast<Byte*>(ISzAlloc_Alloc(&allocImp, bufSize));
    if (lookStream.buf == nullptr)
    {
        error = "CoreRead7zipFile Failed: ISzAlloc_Alloc Failed!";
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
        error = "CoreRead7zipFile Failed: SzArEx_Open Failed: ";
        error += std::to_string(res);
        CoreSetError(error);
        SzArEx_Free(&db, &allocImp);
        ISzAlloc_Free(&allocImp, lookStream.buf);
        return false;
    }

    for (uint32_t i = 0; i < db.NumFiles; i++)
    {
        size_t filename_size = 0;
        uint16_t fileName[CORE_DIR_MAX_LEN];

        // skip directories
        if (SzArEx_IsDir(&db, i))
        {
            continue;
        }

        // skip when filename size exceeds our buffer size
        filename_size = SzArEx_GetFileNameUtf16(&db, i, nullptr);
        if (filename_size > CORE_DIR_MAX_LEN)
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
        fileExtension = CoreLowerString(fileExtension);
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
                error = "CoreRead7zipFile Failed: SzArEx_Extract Failed: ";
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

    error = "CoreRead7zipFile Failed: no valid ROMs found in 7zip!";
    CoreSetError(error);
    return false;
}

CORE_EXPORT bool CoreReadArchiveFile(std::filesystem::path file, std::filesystem::path& extractedFileName, bool& isDisk, std::vector<char>& outBuffer)
{
	std::string file_extension;

	file_extension = file.has_extension() ? file.extension().string() : "";
    file_extension = CoreLowerString(file_extension);

    if (file_extension != ".zip" && file_extension != ".7z")
    {
        return false;
    }

    if (file_extension == ".zip")
    {
    	if (!CoreReadZipFile(file, extractedFileName, isDisk, outBuffer))
        {
            return false;
        }
    }
    else
    {
        if (!CoreRead7zipFile(file, extractedFileName, isDisk, outBuffer))
        {
            return false;
        }
    }

    return true;
}

CORE_EXPORT bool CoreUnzip(std::filesystem::path file, std::filesystem::path path)
{
    std::string error;
    std::filesystem::path targetPath;
    std::filesystem::path fileNamePath;
    std::error_code errorCode;

    unzFile           zipFile;
    unz_global_info   zipInfo;
    std::ifstream     fileStream;
    std::ofstream     outputStream;
    std::vector<char> readBuffer(UNZIP_READ_SIZE);
    int bytes_read = 0;

    zlib_filefunc64_def filefuncs;
    filefuncs.zopen64_file = zlib_filefunc_open;
    filefuncs.zread_file   = zlib_filefunc_read;
    filefuncs.zwrite_file  = nullptr;
    filefuncs.ztell64_file = zlib_filefunc_tell;
    filefuncs.zseek64_file = zlib_filefunc_seek;
    filefuncs.zclose_file  = zlib_filefunc_close;
    filefuncs.zerror_file  = zlib_filefunc_testerror;
    filefuncs.opaque       = &fileStream;

    zipFile = unzOpen2_64(&file, &filefuncs);
    if (zipFile == nullptr)
    {
        error = "CoreUnzip: unzOpen Failed!";
        CoreSetError(error);
        return false;
    }

    if (unzGetGlobalInfo(zipFile, &zipInfo) != UNZ_OK)
    {
        error = "CoreUnzip: unzGetGlobalInfo Failed!";
        CoreSetError(error);
        return false;
    }


    for (uint64_t i = 0; i < zipInfo.number_entry; i++)
    {
        unz_file_info fileInfo;
        char          fileName[CORE_DIR_MAX_LEN];

        // ensure we can retrieve the current file info
        if (unzGetCurrentFileInfo(zipFile, &fileInfo, fileName, CORE_DIR_MAX_LEN, nullptr, 0, nullptr, 0) != UNZ_OK)
        {
            unzClose(zipFile);
            error = "CoreUnzip: unzGetCurrentFileInfo Failed!";
            CoreSetError(error);
            return false;
        }

        fileNamePath = fileName;
        targetPath   = path;
        targetPath   += "/";
        targetPath   += fileName;

        if (fileNamePath.has_filename() &&
            fileNamePath.has_parent_path())
        { // create parent directories
            if (!std::filesystem::is_directory(targetPath.parent_path(), errorCode) && 
                !std::filesystem::create_directories(targetPath.parent_path(), errorCode))
            {
                unzClose(zipFile);
                error = "CoreUnzip: std::filesystem::create_directory(";
                error += targetPath.string();
                error += ") Failed: ";
                error += errorCode.message();
                CoreSetError(error);
                return false;
            }
        }

        if (fileNamePath.has_filename())
        { // extract file
            if (unzOpenCurrentFile(zipFile) != UNZ_OK)
            {
                unzClose(zipFile);
                error = "CoreUnzip: unzOpenCurrentFile Failed!";
                CoreSetError(error);
                return false;
            }

            outputStream.open(targetPath, std::ios::trunc | std::ios::binary);
            if (!outputStream.is_open())
            {
                unzCloseCurrentFile(zipFile);
                unzClose(zipFile);
                error = "CoreUnzip: failed to open file: ";
                error += targetPath.string();
                CoreSetError(error);
                return false;
            }

            do
            {
                bytes_read = unzReadCurrentFile(zipFile, readBuffer.data(), UNZIP_READ_SIZE);
                if (bytes_read < 0)
                {
                    unzCloseCurrentFile(zipFile);
                    unzClose(zipFile);
                    error = "CoreUnzip: unzReadCurrentFile Failed!";
                    CoreSetError(error);
                    return false;
                }
                else if (bytes_read > 0)
                { // write data to file
                    outputStream.write(readBuffer.data(), bytes_read);
                }
            } while (bytes_read > 0);

            outputStream.close();
            unzCloseCurrentFile(zipFile);
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
            error = "CoreUnzip: unzGoToNextFile Failed!";
            CoreSetError(error);
            return false;
        }
    }

    unzClose(zipFile);
    return true;
}

