/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "osal_files.hpp"

#include <sys/stat.h>

osal_files_file_time osal_files_get_file_time(std::filesystem::path file)
{
    int ret;
    struct stat file_stat;

    ret = stat(file.string().c_str(), &file_stat);
    if (ret != 0)
    {
        return -1;
    }

    return file_stat.st_mtime;
}
