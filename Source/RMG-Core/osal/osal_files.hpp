/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef OSAL_FILES_HPP
#define OSAL_FILES_HPP

#include <string>

typedef uint64_t osal_files_file_time;

#ifdef _WIN32
#define OSAL_FILES_DIR_SEPERATOR_STR "\\"
#else // Unix
#define OSAL_FILES_DIR_SEPERATOR_STR "/"
#endif // _WIN32

// returns file time for given file,
// returns -1 on failure
osal_files_file_time osal_files_get_file_time(std::string file);

#endif // OSAL_FILES_HPP
