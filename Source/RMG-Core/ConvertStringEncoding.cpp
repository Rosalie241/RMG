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
#include "ConvertStringEncoding.hpp"
#include "Error.hpp"

#include <string.h>
#include <iconv.h>
#include <string>

//
// Exported Functions
//

std::string CoreConvertStringEncoding(std::string str, CoreStringEncoding encoding)
{
    std::string error;
    std::string encodingString;

    iconv_t cd;

    char outputBuf[256] = {0};

    char* inputBufPtr = (char*)str.c_str();
    char* outputBufPtr = outputBuf;

    size_t inputBufSize = str.size();
    size_t outputBufSize = sizeof(outputBuf);

    if (encoding == CoreStringEncoding::EUC_JP)
    {
        encodingString = "EUC-JP";
    }
    else
    {
        encodingString = "Shift_JIS";
    }

    cd = iconv_open("UTF-8//TRANSLIT//IGNORE", encodingString.c_str());
    if (cd == (iconv_t)-1)
    {
        error = "CoreConvertStringEncoding Failed: ";
        error += "iconv_open Failed: ";
        error += errno;
        CoreSetError(error);
        return str;
    }

    size_t ret = iconv(cd, &inputBufPtr, &inputBufSize, &outputBufPtr, &outputBufSize);
    if (ret == (size_t)-1)
    {
        error = "CoreConvertStringEncoding Failed: ";
        error += "iconv Failed: ";
        error += errno;
        CoreSetError(error);

        // reset iconv
        iconv(cd, nullptr, nullptr, nullptr, nullptr);
        iconv_close(cd);

        return str;
    }

    // reset iconv
    iconv(cd, nullptr, nullptr, nullptr, nullptr);
    iconv_close(cd);

    return std::string(outputBuf);
}
