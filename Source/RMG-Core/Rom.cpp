/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "Rom.hpp"
#include "Directories.hpp"
#include "Error.hpp"
#include "MediaLoader.hpp"
#include "m64p/Api.hpp"
#include "RomSettings.hpp"
#include "Cheats.hpp"
#include "osal/osal_files.hpp"

#include <unzip.h>
#include <fstream>
#include <cstdlib>
#include <cstring>
#include <algorithm>

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
    return fileStream->fail() ? 0 : size;
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

static bool read_zip_file(std::filesystem::path file, std::filesystem::path* extractedFileName, bool* isDisk, char** buf, int* size)
{
    std::string  error;
    std::fstream fileStream;
    int          fileStreamLen;
    char*        fileStreamBuf;

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

    for (int i = 0; i < zipInfo.number_entry; i++)
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
        std::filesystem::path fileNamePath(fileName);
        std::string fileExtension = fileNamePath.has_extension() ? fileNamePath.extension().string() : "";
        fileExtension = to_lower_str(fileExtension);
        if (fileExtension == ".z64" ||
            fileExtension == ".v64" ||
            fileExtension == ".n64" ||
            fileExtension == ".ndd" ||
            fileExtension == ".d64")
        {
            char* buffer;
            char* outBuffer;
            int   dataSize = UNZIP_READ_SIZE;
            int   total_bytes_read = 0;
            int   bytes_read = 0;

            buffer = (char*)malloc(UNZIP_READ_SIZE);
            if (buffer == nullptr)
            {
                error = "read_zip_file Failed: malloc Failed!";
                CoreSetError(error);
                return false;
            }

            outBuffer = (char*)malloc(UNZIP_READ_SIZE);
            if (outBuffer == nullptr)
            {
                free(buffer);
                error = "read_zip_file Failed: malloc Failed!";
                CoreSetError(error);
                return false;
            }

            if (unzOpenCurrentFile(zipFile) != UNZ_OK)
            {
                free(buffer);
                free(outBuffer);
                error = "read_zip_file Failed: unzOpenCurrentFile Failed!";
                CoreSetError(error);
                return false;
            }

            do
            {
                bytes_read = unzReadCurrentFile(zipFile, buffer, UNZIP_READ_SIZE);
                if (bytes_read < 0)
                {
                    unzCloseCurrentFile(zipFile);
                    unzClose(zipFile);
                    free(buffer);
                    free(outBuffer);
                    error = "read_zip_file Failed: unzReadCurrentFile Failed: ";
                    error += std::to_string(bytes_read);
                    CoreSetError(error);
                    return false;
                }

                if (bytes_read > 0)
                {
                    if (total_bytes_read + bytes_read > dataSize)
                    {
                        outBuffer = (char*)realloc(outBuffer, total_bytes_read + bytes_read);
                        dataSize += bytes_read;
                        if (outBuffer == nullptr)
                        {
                            unzCloseCurrentFile(zipFile);
                            unzClose(zipFile);
                            free(buffer);
                            free(outBuffer);
                            error = "read_zip_file Failed: realloc Failed!";
                            CoreSetError(error);
                            return false;
                        }
                    }

                    memcpy((outBuffer + total_bytes_read), buffer, bytes_read);
                    total_bytes_read += bytes_read;
                }
            } while (bytes_read > 0);

            *size              = total_bytes_read;
            *buf               = outBuffer;
            *extractedFileName = fileNamePath;
            *isDisk            = (fileExtension == ".ndd" || fileExtension == ".d64");
            unzCloseCurrentFile(zipFile);
            unzClose(zipFile);
            free(buffer);
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

static bool read_raw_file(std::filesystem::path file, char** buf, int* size)
{
    std::string   error;
    std::ifstream fileStream;
    int           fileStreamLen;
    char*         fileStreamBuf;

    // attempt to open file
    fileStream.open(file, std::ios::binary);
    if (!fileStream.is_open())
    {
        error = "read_raw_file Failed: ";
        error += "failed to open file: ";
        error += strerror(errno);
        error += " (";
        error += std::to_string(errno);
        error += ")";
        CoreSetError(error);
        return false;
    }

    // attempt to retrieve file length
    fileStream.seekg(0, fileStream.end);
    fileStreamLen = fileStream.tellg();
    fileStream.seekg(0, fileStream.beg);

    // try to create buffer
    fileStreamBuf = (char*)malloc(fileStreamLen);
    if (fileStreamBuf == nullptr)
    {
        error = "read_raw_file Failed: ";
        error += "malloc failed!";
        CoreSetError(error);
        free(fileStreamBuf);
        fileStream.close();
        return false;
    }

    // read file
    fileStream.read(fileStreamBuf, fileStreamLen);

    *buf  = fileStreamBuf;
    *size = fileStreamLen;

    fileStream.close();
    return true;
}

static bool write_file(std::filesystem::path file, char* buf, int size)
{
    std::string   error;
    std::ofstream fileStream;

    // attempt to open file
    fileStream.open(file, std::ios::binary);
    if (!fileStream.is_open())
    {
        error = "write_file Failed: ";
        error += "failed to open file: ";
        error += strerror(errno);
        error += " (";
        error += std::to_string(errno);
        error += ")";
        CoreSetError(error);
        return false;
    }

    // write buffer to file
    fileStream.write(buf, size);
    
    fileStream.close();
    return true;
}

//
// Exported Functions
//

bool CoreOpenRom(std::filesystem::path file)
{
    std::string error;
    m64p_error  ret;
    char*       buf      = nullptr;
    int         buf_size = 0;
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

    if (file_extension == ".zip")
    {
        std::filesystem::path extracted_file;
        bool                  is_disk = false;

        if (!read_zip_file(file, &extracted_file, &is_disk, &buf, &buf_size))
        {
            return false;
        }

        if (is_disk)
        {
            std::filesystem::path disk_file;
            disk_file = CoreGetUserCacheDirectory();
            disk_file += OSAL_FILES_DIR_SEPERATOR_STR;
            disk_file += "extracted_disks";
            disk_file += OSAL_FILES_DIR_SEPERATOR_STR;
            disk_file += extracted_file;

            // attempt to create extraction directory
            try
            {
                if (!std::filesystem::exists(disk_file.parent_path()))
                {
                    if (!std::filesystem::create_directory(disk_file.parent_path()))
                    {
                        throw std::exception();
                    }
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
            if (!write_file(disk_file, buf, buf_size))
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
        if (!read_raw_file(file, &buf, &buf_size))
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
        ret = m64p::Core.DoCommand(M64CMD_ROM_OPEN, buf_size, buf);
        error = "CoreOpenRom: m64p::Core.DoCommand(M64CMD_ROM_OPEN) Failed: ";
        free(buf);
    }

    if (ret != M64ERR_SUCCESS)
    {
        error += m64p::Core.ErrorMessage(ret);
        CoreSetError(error);
    }

    l_HasRomOpen = (ret == M64ERR_SUCCESS);

    if (l_HasRomOpen)
    {
        // store default ROM settings
        CoreStoreCurrentDefaultRomSettings();
        // apply rom settings overlay
        CoreApplyRomSettingsOverlay();
    }

    return l_HasRomOpen;
}

bool CoreHasRomOpen(void)
{
    return l_HasRomOpen;
}

CoreRomType CoreGetRomType(void)
{
    return l_HasDisk ? CoreRomType::Disk : CoreRomType::Cartridge;
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
