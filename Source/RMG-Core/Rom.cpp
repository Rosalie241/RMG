/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020-2025 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#define CORE_INTERNAL
#include "CachedRomHeaderAndSettings.hpp"
#include "MediaLoader.hpp"
#include "Directories.hpp"
#include "RomSettings.hpp"
#include "m64p/Api.hpp"
#include "Archive.hpp"
#include "Library.hpp"
#include "Cheats.hpp"
#include "String.hpp"
#include "Error.hpp"
#include "File.hpp"
#include "Rom.hpp"

#include <string>
#include <cstdlib>
#include <cstring>

//
// Local Variables
//

static bool l_HasRomOpen       = false;
static bool l_HasDisk          = false;
static bool l_HasExtractedDisk = false;
static std::filesystem::path l_ExtractedDiskPath;
static std::filesystem::path l_RomPath;

//
// Exported Functions
//

CORE_EXPORT bool CoreOpenRom(std::filesystem::path file)
{
    std::string error;
    std::error_code error_code;
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
    file_extension = CoreLowerString(file_extension);

    if (file_extension == ".zip" || 
        file_extension == ".7z")
    {
        std::filesystem::path extracted_file;
        bool                  is_disk = false;

        if (!CoreReadArchiveFile(file, extracted_file, is_disk, buf))
        {
            return false;
        }

        if (is_disk)
        {
            std::filesystem::path disk_file;
            disk_file = CoreGetUserCacheDirectory();
            disk_file += "/extracted_disks/";
            disk_file += extracted_file.filename();

            // attempt to create extraction directory
            if (!std::filesystem::exists(disk_file.parent_path(), error_code) &&
                !std::filesystem::create_directory(disk_file.parent_path(), error_code))
            {
                error = "CoreOpenRom Failed: ";
                error += "Failed to create \"";
                error += disk_file.parent_path().string();
                error += "\": ";
                error += error_code.message();
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

CORE_EXPORT bool CoreHasRomOpen(void)
{
    return l_HasRomOpen;
}

CORE_EXPORT bool CoreGetRomType(CoreRomType& type)
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

CORE_EXPORT bool CoreGetRomPath(std::filesystem::path& path)
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

CORE_EXPORT bool CoreCloseRom(void)
{
    std::string error;
    std::error_code errorCode;
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
        if (!std::filesystem::remove(l_ExtractedDiskPath, errorCode))
        {
            error = "CoreCloseRom: Failed to remove \"";
            error += l_ExtractedDiskPath.string();
            error += "\": ";
            error += errorCode.message();
            CoreSetError(error);
            return false;
        }
    }

    return true;
}
