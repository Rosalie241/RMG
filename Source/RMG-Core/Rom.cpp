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
#include "Error.hpp"
#include "m64p/Api.hpp"
#include "RomSettings.hpp"

#include <unzip.h>
#include <iostream>
#include <fstream>
#include <cstring>

//
// Local Defines
//

#define UNZIP_READ_SIZE 67108860 /* 64 MiB */

//
// Local Variables
//

static bool l_HasRomOpen = false;

//
// Local Functions
//

static bool read_zip_file(std::string file, char** buf, int* size)
{
    std::string  error;
    std::fstream fileStream;
    int          fileStreamLen;
    char*        fileStreamBuf;

    unzFile         zipFile;
    unz_global_info zipInfo;

    zipFile = unzOpen(file.c_str());
    if (zipFile == nullptr)
    {
        error = "read_zip_file: unzOpen Failed!";
        CoreSetError(error);
        return false;
    }

    if (unzGetGlobalInfo(zipFile, &zipInfo) != UNZ_OK)
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
        std::string fileNameStr(fileName);
        if (fileNameStr.ends_with(".z64") ||
            fileNameStr.ends_with(".v64") ||
            fileNameStr.ends_with(".n64"))
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

            *size = total_bytes_read;
            *buf = outBuffer;
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

static bool read_raw_file(std::string file, char** buf, int* size)
{
    std::string  error;
    std::fstream fileStream;
    int          fileStreamLen;
    char*        fileStreamBuf;

    // attempt to open file
    fileStream.open(file);
    if (!fileStream.is_open())
    {
        error = "read_raw_file Failed: ";
        error += "failed to open file!";
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
    *size = fileStream.gcount();

    fileStream.close();
    return true;
}

//
// Exported Functions
//

bool CoreOpenRom(std::string file)
{
    std::string error;
    m64p_error  ret;
    char*       buf;
    int         buf_size;

    if (CoreHasRomOpen())
    {
        error = "CoreOpenRom Failed: ";
        error += "cannot open ROM when another ROM is already open!";
        CoreSetError(error);
        return false;
    }

    if (file.ends_with(".zip"))
    {
        if (!read_zip_file(file, &buf, &buf_size))
        {
            return false;
        }
    }
    else
    {
        if (!read_raw_file(file, &buf, &buf_size))
        {
            return false;
        }
    }

    ret = m64p::Core.DoCommand(M64CMD_ROM_OPEN, buf_size, buf);
    if (ret != M64ERR_SUCCESS)
    {
        error = "CoreOpenRom: m64p::Core.DoCommand(M64CMD_ROM_OPEN) Failed: ";
        error += m64p::Core.ErrorMessage(ret);
        CoreSetError(error);
    }

    free(buf);
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

bool CoreCloseRom(void)
{
    std::string error;
    m64p_error ret;

    if (!CoreHasRomOpen())
    {
        error = "CoreCloseRom Failed: ";
        error += "cannot close rom when no rom is open!";
        CoreSetError(error);
        return false;
    }

    ret = m64p::Core.DoCommand(M64CMD_ROM_CLOSE, 0, nullptr);
    if (ret != M64ERR_SUCCESS)
    {
        error = "CoreCloseRom: m64p::Core.DoCommand(M64CMD_ROM_CLOSE) Failed: ";
        error += m64p::Core.ErrorMessage(ret);
        CoreSetError(error);
        return false;
    }

    // clear default ROM settings
    CoreClearCurrentDefaultRomSettings();

    l_HasRomOpen = false;
    return true;
}
