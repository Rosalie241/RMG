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
#include "Cheats.hpp"
#include "RomHeader.hpp"
#include "RomSettings.hpp"
#include "Directories.hpp"
#include "m64p/Api.hpp"
#include "osal/osal_files.hpp"
#include "Error.hpp"
#include "Settings/Settings.hpp"

#include <algorithm>
#include <sstream>
#include <filesystem>
#include <fstream>
#include <iostream>

//
// Local Structs
//

struct l_LoadedCheat
{
    CoreCheat cheat;
    CoreCheatOption cheatOption;
};

//
// Local Variables
//

static CoreCheatFile l_SharedCheatFile;
static CoreCheatFile l_UserCheatFile;
static std::vector<l_LoadedCheat> l_LoadedCheats;

//
// Local Functions
//

static bool read_file_lines(std::filesystem::path file, std::vector<std::string>& lines)
{
    std::string error;
    std::ifstream inputStream(file);
    std::string line;

    if (!inputStream.is_open())
    {
        error = "read_file_lines Failed:";
        error += "failed to open \"";
        error += file.string();
        error += "\"";
        CoreSetError(error);
        return false;
    }

    // read file line by line
    while (std::getline(inputStream, line))
    {
        lines.push_back(line);
    }

    inputStream.close();
    return true;
}

static std::filesystem::path get_cheat_file_name_from_header(CoreRomHeader romHeader)
{
    std::filesystem::path cheatFileName;
    std::stringstream stringStream;

    stringStream << std::uppercase << std::hex << std::setw(8) << std::setfill('0') << romHeader.CRC1 << "-";
    stringStream << std::uppercase << std::hex << std::setw(8) << std::setfill('0') << romHeader.CRC2 << "-";
    stringStream << std::uppercase << std::hex << std::setw(2) << (uint32_t)romHeader.CountryCode << ".cht";

    cheatFileName = stringStream.str();

    return cheatFileName;
}

static std::filesystem::path get_shared_cheat_file_path_from_header(CoreRomHeader romHeader)
{
    std::filesystem::path cheatFilePath;

    cheatFilePath = CoreGetSharedDataDirectory();
    cheatFilePath += OSAL_FILES_DIR_SEPERATOR_STR;
    cheatFilePath += "Cheats";
    cheatFilePath += OSAL_FILES_DIR_SEPERATOR_STR;
    cheatFilePath += get_cheat_file_name_from_header(romHeader);

    return cheatFilePath;
}

static std::filesystem::path get_user_cheat_file_path_from_header(CoreRomHeader romHeader)
{
    std::filesystem::path cheatFilePath;

    cheatFilePath = CoreGetUserDataDirectory();
    cheatFilePath += OSAL_FILES_DIR_SEPERATOR_STR;
    cheatFilePath += "Cheats-User";
    cheatFilePath += OSAL_FILES_DIR_SEPERATOR_STR;
    cheatFilePath += get_cheat_file_name_from_header(romHeader);

    // try to make the user cheats directory
    // if it doesn't exist yet
    try
    {
        if (!std::filesystem::is_directory(cheatFilePath.parent_path()))
        {
            std::filesystem::create_directory(cheatFilePath.parent_path());
        }
    }
    catch (...)
    {
        // we'll fail later...
    }

    return cheatFilePath;
}

static std::vector<std::string> split_string(std::string str, char delim)
{
    std::vector<std::string> splitString;
    std::stringstream stringStream(str);
    std::string element;

    while (std::getline(stringStream, element, delim))
    {
        splitString.push_back(element);
    }

    return splitString;
}

static std::string join_split_string(std::vector<std::string> splitStr, char seperator, int skip = 0)
{
    std::string joinedString;
    std::string element;
    int skippedElements = 0;
    for (int i = 0; i < splitStr.size(); i++)
    {
        // allow for skipping elements
        if (skippedElements++ < skip)
        {
            continue;
        }

        element = splitStr.at(i);

        joinedString += element;
        // when not at the last element,
        // insert seperator
        if (i != (splitStr.size() - 1))
        {
            joinedString += seperator;
        }
    }

    return joinedString;
}

static bool parse_cheat(std::vector<std::string> lines, int startIndex, CoreCheat& cheat, int& endIndex)
{
    std::string error;
    std::string line;
    for (int i = startIndex; i < lines.size(); i++)
    {
        line = lines.at(i);

        // Parse metadata
        // $Cheat Name
        // Author=Cheat Author
        // Note=Cheat Note
        if (line.starts_with("$"))
        {
            line.erase(0, 1);
            cheat.Name = line;
            continue;
        } else if (line.starts_with("Author="))
        {
            line.erase(0, 7);
            cheat.Author = line;
            continue;
        } else if (line.starts_with("Note="))
        {
            line.erase(0, 5);
            cheat.Note = line;
            continue;
        } 

        // newline = new cheat
        if (line.empty())
        {
            endIndex = i;
            break;
        }

        std::vector<std::string> splitLine = split_string(line, ' ');

        // skip invalid lines
        if (splitLine.size() < 2)
        {
            error = "parse_cheat Failed: ";
            error += "invalid line: \"";
            error += line;
            error += "\"";
            CoreSetError(error);
            return false;
        }

        std::string address = splitLine.at(0);
        std::string value   = splitLine.at(1);

        if (splitLine.size() == 2 && address.size() == 8 && (value.size() == 4 || value.size() == 9))
        { // cheat code
            CoreCheatCode cheatCode;
            cheatCode.Address = std::strtoll(address.c_str(), nullptr, 16);

            // we don't support 'value:old value',
            // so strip the old value
            if (value.size() == 9)
            {
                if (value.at(4) == ':')
                {
                    value.erase(4, 5);
                }
                else
                { // invalid line
                    error = "parse_cheat Failed: ";
                    error += "invalid line: \"";
                    error += line;
                    error += "\"";
                    CoreSetError(error);
                    return false;
                }
            }

            if (value.find("?") != std::string::npos)
            { // value uses options
                cheatCode.UseOptions  = true;
                cheatCode.OptionIndex = value.find('?');
                cheatCode.OptionSize  = std::count(value.begin(), value.end(), '?');
                std::replace(value.begin(), value.end(), '?', '0');
                if (value.empty())
                {
                    cheatCode.Value = 0;
                }
                else
                {
                    cheatCode.Value = std::strtoll(value.c_str(), nullptr, 16);
                }
            }
            else
            { // value doesn't use options
                cheatCode.UseOptions = false;
                cheatCode.Value      = std::strtoll(value.c_str(), nullptr, 16);
            }

            // add code to cheat
            cheat.CheatCodes.push_back(cheatCode);
        } else if (address.size() <= 4 && value.size() > 0)
        { // cheat option
            CoreCheatOption option;

            option.Name  = join_split_string(splitLine, ' ', 1);
            option.Value = std::strtoll(address.c_str(), nullptr, 16);
            option.Size  = address.size();

            // add option to cheat
            cheat.HasOptions = true;
            cheat.CheatOptions.push_back(option);
        } else
        { // invalid line?
            error = "parse_cheat Failed: ";
            error += "invalid line: \"";
            error += line;
            error += "\"";
            CoreSetError(error);
            return false;
        }

        // eof = stop reading
        if (i == (lines.size() - 1))
        {
            endIndex = i;
            break;
        }
    }

    return !cheat.Name.empty() && !cheat.CheatCodes.empty();
}

static bool parse_cheat_file(std::vector<std::string> lines, CoreCheatFile& cheatFile)
{
    int endIndex = -1;
    std::string line;
    std::string error;
    bool readHeader = false;
    bool readHeaderName = false;

    for (int index = 0; index < lines.size(); index++)
    {
        line = lines.at(index);

        if (!readHeader && line.starts_with("[") && line.ends_with("]"))
        {
            line.erase(0, 1);                 // strip '['
            line.erase((line.size() - 1), 1); // strip ']'

            // header = "635A2BFF-8B022326-C:45"
            // so validate it
            if (line.at(8) != '-' || line.at(17) != '-' || line.at(18) != 'C')
            {
                error = "parse_cheat_file Failed: ";
                error += "invalid header: \"";
                error += line;
                error += "\"";
                CoreSetError(error);
                return false;
            }

            std::string crc1 = split_string(line, '-').at(0);
            std::string crc2 = split_string(line, '-').at(1);
            std::string countryCode = split_string(line, ':').at(1);

            cheatFile.CRC1 = std::strtoll(crc1.c_str(), nullptr, 16);;
            cheatFile.CRC2 = std::strtoll(crc2.c_str(), nullptr, 16);;;
            cheatFile.CountryCode = std::strtoll(countryCode.c_str(), nullptr, 16);;;
            readHeader = true;
        } else if (readHeader && !readHeaderName && line.starts_with("Name="))
        {
            line.erase(0, 5);
            cheatFile.Name = line;
            readHeaderName = true;
        } else if (readHeader && readHeaderName && line.starts_with('$'))
        {
            CoreCheat cheat;
            endIndex = 0;

            if (!parse_cheat(lines, index, cheat, endIndex))
            {
                return false;
            }

            cheatFile.Cheats.push_back(cheat);
            index = endIndex;
        } else if (!line.empty())
        {
            error = "parse_cheat_file Failed: ";
            error += "unknown line: \"";
            error += line;
            error += "\"";
            CoreSetError(error);
            return false;
        }
    }

    return true;
}

static bool write_cheat_file(CoreCheatFile cheatFile, std::filesystem::path path)
{
    std::stringstream stringStream;
    std::ofstream outputStream(path);
    std::string error;

    if (!outputStream.is_open())
    {
        error = "write_cheat_file Failed: ";
        error += "Failed to open \"";
        error += path.string();
        error += "\'";
        CoreSetError(error);
        return false;
    }

    stringStream << "[" << std::uppercase << std::hex << std::setw(8) << std::setfill('0') << cheatFile.CRC1 << "-" 
                        << std::uppercase << std::hex << std::setw(8) << std::setfill('0') << cheatFile.CRC2 << "-C:" 
                        << std::uppercase << std::hex << std::setw(2) << std::setfill('0') << cheatFile.CountryCode << "]" << std::endl;
    stringStream << "Name=" << cheatFile.Name << std::endl << std::endl;

    for (CoreCheat& cheat : cheatFile.Cheats)
    {
        stringStream << "$" << cheat.Name << std::endl;
    
        if (!cheat.Author.empty())
        {
            stringStream << "Author=" << cheat.Author << std::endl;
        }
    
        if (!cheat.Note.empty())
        {
            stringStream << "Note=" << cheat.Note << std::endl;
        }
    
        for (CoreCheatCode& code : cheat.CheatCodes)
        {
            if (code.UseOptions)
            {
                std::stringstream tmpStringStream;
                std::string str;
                tmpStringStream << std::uppercase << std::hex << std::setw(4) << std::setfill('0') << code.Value;
    
                str = tmpStringStream.str();
    
                // create string with required size
                std::string questionMarkString;
                for (int i = 0; i < code.OptionSize; i++)
                    questionMarkString += "?";
    
                str.replace(code.OptionIndex, code.OptionSize, questionMarkString);
    
                stringStream << std::uppercase << std::hex << std::setw(4) << std::setfill('0') << code.Address << " " << str << std::endl;
    
            }
            else
            {
                stringStream << std::uppercase << std::hex << std::setw(8) << std::setfill('0') << code.Address << " " 
                             << std::uppercase << std::hex << std::setw(4) << std::setfill('0') << code.Value << std::endl;
            }
        }
    
        if (cheat.HasOptions)
        {
            for (CoreCheatOption& option : cheat.CheatOptions)
            {
                stringStream << std::uppercase << std::hex << std::setw(option.Size) << std::setfill('0') << option.Value << " " << option.Name << std::endl;
            }
        }

        // extra newline
        stringStream << std::endl;
    }

    outputStream << stringStream.str();
    outputStream.close();
    return true;
}

bool combine_cheat_code_and_option(CoreCheatCode code, CoreCheatOption option, int32_t& combinedValue)
{
    std::stringstream codeValueStringStream;
    std::stringstream optionValueStringStream;
    std::string codeValueString;
    std::string optionValueString;

    // make sure the code needs an option
    if (!code.UseOptions)
    {
        return false;
    }

    // make sure the size is the same
    if (code.OptionSize != option.Size)
    {
        return false;
    }

    codeValueStringStream << std::hex << std::setw(4) << std::setfill('0') << code.Value;
    codeValueString = codeValueStringStream.str();

    optionValueStringStream << std::hex << std::setw(option.Size) << std::setfill('0') << option.Value;
    optionValueString = optionValueStringStream.str();

    // insert option
    codeValueString.replace(code.OptionIndex, code.OptionSize, optionValueString);

    // convert to int32_t
    combinedValue = std::strtoll(codeValueString.c_str(), nullptr, 16);
    return true;
}

static std::vector<CoreCheat>::iterator find_user_cheat_using_name(std::string name)
{
    auto predicate = [name](const CoreCheat& other)
    {
        return name == other.Name;
    };

    return std::find_if(l_UserCheatFile.Cheats.begin(), l_UserCheatFile.Cheats.end(), predicate);
}

//
// Exported Functions
//

bool CoreGetCurrentCheats(std::vector<CoreCheat>& cheats)
{
    CoreRomHeader romHeader;
    CoreCheatFile sharedCheatFile;
    CoreCheatFile userCheatFile;
    std::filesystem::path sharedCheatFilePath;
    std::filesystem::path userCheatFilePath;
    std::vector<std::string> sharedCheatFilelines;
    std::vector<std::string> userCheatFileLines;
    bool ret1 = false;
    bool ret2 = false;

    if (!CoreGetCurrentRomHeader(romHeader))
    {
        return false;
    }

    sharedCheatFilePath = get_shared_cheat_file_path_from_header(romHeader);
    userCheatFilePath = get_user_cheat_file_path_from_header(romHeader);

    // do nothing if neither the shared or user cheat file exists
    ret1 = std::filesystem::is_regular_file(sharedCheatFilePath);
    ret2 = std::filesystem::is_regular_file(userCheatFilePath);
    if (!ret1 && !ret2)
    {
        return true;
    }

    // fail when we fail to read both the shared & user cheat file
    ret1 = read_file_lines(sharedCheatFilePath, sharedCheatFilelines);
    ret2 = read_file_lines(userCheatFilePath, userCheatFileLines);
    if (!ret1 && !ret2)
    {
        return false;
    }

    // fail when we fail to parse both the shared & user cheat file
    ret1 = parse_cheat_file(sharedCheatFilelines, sharedCheatFile);
    ret2 = parse_cheat_file(userCheatFileLines, userCheatFile);
    if (!ret1 && !ret2)
    {
        return false;
    }

    l_SharedCheatFile = sharedCheatFile;
    l_UserCheatFile = userCheatFile;

    // add shared & user cheats
    // add user cheats first
    for (CoreCheat& cheat : userCheatFile.Cheats)
    {
        cheats.push_back(cheat);
    }
    // add shared cheats
    // and check if any cheats with the same name
    // already exist, if it does, then just skip them
    for (CoreCheat& cheat : sharedCheatFile.Cheats)
    {
        auto iter = find_user_cheat_using_name(cheat.Name);

        // skip cheat with the same name
        if (iter != l_UserCheatFile.Cheats.end())
        {
            continue;
        }

        cheats.push_back(cheat);
    }

    return true;
}

bool CoreParseCheat(std::vector<std::string> lines, CoreCheat& cheat)
{
    int endIndex = 0;
    return parse_cheat(lines, 0, cheat, endIndex);
}

bool CoreGetCheatLines(CoreCheat cheat, std::vector<std::string>& codeLines, std::vector<std::string>& optionLines)
{
    std::stringstream stringStream;

    for (CoreCheatCode& code : cheat.CheatCodes)
    {
        if (code.UseOptions)
        {
            std::stringstream tmpStringStream;
            std::string str;
            tmpStringStream << std::uppercase << std::hex << std::setw(4) << std::setfill('0') << code.Value;

            str = tmpStringStream.str();

            // create string with required size
            std::string questionMarkString;
            for (int i = 0; i < code.OptionSize; i++)
                questionMarkString += "?";

            str.replace(code.OptionIndex, code.OptionSize, questionMarkString);

            stringStream << std::uppercase << std::hex << std::setw(4) << std::setfill('0') << code.Address << " " << str;
        }
        else
        {
            stringStream << std::uppercase << std::hex << std::setw(8) << std::setfill('0') << code.Address << " " 
                         << std::uppercase << std::hex << std::setw(4) << std::setfill('0') << code.Value;
        }

        codeLines.push_back(stringStream.str());
        // reset stringstream
        stringStream.str("");
    }

    if (cheat.HasOptions)
    {
        for (CoreCheatOption& option : cheat.CheatOptions)
        {
            stringStream << std::uppercase << std::hex << std::setw(option.Size) << std::setfill('0') << option.Value << " " << option.Name;
            optionLines.push_back(stringStream.str());
            // reset stringstream
            stringStream.str("");
        }
    }

    return true;
}

bool CoreAddCheat(CoreCheat cheat)
{
    std::string error;
    CoreRomHeader romHeader;
    CoreRomSettings romSettings;
    std::filesystem::path cheatFilePath;

    if (!CoreGetCurrentRomHeader(romHeader) ||
        !CoreGetCurrentRomSettings(romSettings))
    {
        return false;
    }

    cheatFilePath = get_user_cheat_file_path_from_header(romHeader);

    // try to find another cheat with the same name,
    // if it exists, fail because we don't allow that
    auto iter = find_user_cheat_using_name(cheat.Name);
    if (iter != l_UserCheatFile.Cheats.end())
    {
        error = "CoreAddCheat Failed: ";
        error += "cheat with name already exists!";
        CoreSetError(error);
        return false;
    }

    // copy info to cheat file
    l_UserCheatFile.CRC1 = romHeader.CRC1;
    l_UserCheatFile.CRC2 = romHeader.CRC2;
    l_UserCheatFile.CountryCode = romHeader.CountryCode;
    l_UserCheatFile.Name = romSettings.GoodName;
    l_UserCheatFile.Cheats.push_back(cheat);

    return write_cheat_file(l_UserCheatFile, cheatFilePath);
}

bool CoreUpdateCheat(CoreCheat oldCheat, CoreCheat newCheat)
{
    CoreRomHeader romHeader;
    CoreRomSettings romSettings;
    std::filesystem::path cheatFilePath;

    if (!CoreGetCurrentRomHeader(romHeader) ||
        !CoreGetCurrentRomSettings(romSettings))
    {
        return false;
    }

    cheatFilePath = get_user_cheat_file_path_from_header(romHeader);

    // try to find old cheat in user cheats,
    // when it isnt found, it's most likely a system cheat
    auto iter = find_user_cheat_using_name(oldCheat.Name);
    if (iter != l_UserCheatFile.Cheats.end())
    { // when found, erase it
        l_UserCheatFile.Cheats.erase(iter);
    }

    // add cheat to user cheats
    l_UserCheatFile.Cheats.push_back(newCheat);
    return write_cheat_file(l_UserCheatFile, cheatFilePath);
}

bool CoreCanRemoveCheat(CoreCheat cheat)
{
    return std::find(l_UserCheatFile.Cheats.begin(), l_UserCheatFile.Cheats.end(), cheat) != l_UserCheatFile.Cheats.end();
}

bool CoreRemoveCheat(CoreCheat cheat)
{
    CoreRomHeader romHeader;
    CoreRomSettings romSettings;
    std::filesystem::path cheatFilePath;

    if (!CoreCanRemoveCheat(cheat))
    {
        return false;
    }

    if (!CoreGetCurrentRomHeader(romHeader))
    {
        return false;
    }

    cheatFilePath = get_user_cheat_file_path_from_header(romHeader);

    // try to find the cheat
    auto iter = std::find(l_UserCheatFile.Cheats.begin(), l_UserCheatFile.Cheats.end(), cheat);
    if (iter != l_UserCheatFile.Cheats.end())
    { 
        l_UserCheatFile.Cheats.erase(iter);
    }

    return write_cheat_file(l_UserCheatFile, cheatFilePath);
}

bool CoreEnableCheat(CoreCheat cheat, bool enabled)
{
    CoreRomSettings romSettings;
    std::string settingKey;

    if (!CoreGetCurrentRomSettings(romSettings))
    {
        return false;
    }

    settingKey = "Cheat \"" + cheat.Name + "\" Enabled";

    // if the cheat is disabled and the settings key doesn't exist, do nothing
    if (!enabled && !CoreSettingsKeyExists(romSettings.MD5, settingKey))
    {
        return true;
    }

    return CoreSettingsSetValue(romSettings.MD5, settingKey, enabled);
}

bool CoreIsCheatEnabled(CoreCheat cheat)
{
    CoreRomSettings romSettings;
    std::string settingKey;

    if (!CoreGetCurrentRomSettings(romSettings))
    {
        return false;
    }

    settingKey = "Cheat \"" + cheat.Name + "\" Enabled";

    return CoreSettingsGetBoolValue(romSettings.MD5, settingKey, false);
}

bool CoreHasCheatOptionSet(CoreCheat cheat)
{
    CoreRomSettings romSettings;
    std::string settingKey;

    if (!CoreGetCurrentRomSettings(romSettings))
    {
        return false;
    }

    settingKey = "Cheat \"" + cheat.Name + "\" Option";

    return CoreSettingsGetIntValue(romSettings.MD5, settingKey, -1) != -1;
}

bool CoreSetCheatOption(CoreCheat cheat, CoreCheatOption option)
{   
    CoreRomSettings romSettings;
    std::string settingKey;

    if (!CoreGetCurrentRomSettings(romSettings))
    {
        return false;
    }

    settingKey = "Cheat \"" + cheat.Name + "\" Option";

    return CoreSettingsSetValue(romSettings.MD5, settingKey, (int)option.Value);
}

bool CoreGetCheatOption(CoreCheat cheat, CoreCheatOption& option)
{
    CoreRomSettings romSettings;
    std::string settingKey;
    int value = 0;

    if (!cheat.HasOptions)
    {
        return false;
    }

    if (!CoreGetCurrentRomSettings(romSettings))
    {
        return false;
    }

    settingKey = "Cheat \"" + cheat.Name + "\" Option";

    value = CoreSettingsGetIntValue(romSettings.MD5, settingKey, -1);
    if (value == -1)
    {
        return false;
    }

    for (CoreCheatOption& cheatOption : cheat.CheatOptions)
    {
        if (cheatOption.Value == value)
        {
            option = cheatOption;
            return true;
        }
    }

    // nothing found
    return false;
}

bool CoreApplyCheats(void)
{
    std::string error;
    m64p_error ret;
    std::vector<m64p_cheat_code> m64p_cheatCodes;
    std::vector<CoreCheat> cheats;
    CoreCheatOption cheatOption;
    bool skipCheat = false;
    int32_t combinedValue;

    // we shouldn't fail emulation
    // just because parsing cheats failed
    if (!CoreGetCurrentCheats(cheats))
    {
        return true;
    }

    // TODO: does this matter at all?
    // we apply cheats when emulation is paused
    CoreClearCheats();

    for (CoreCheat& cheat : cheats)
    {
        skipCheat = false;

        if (!CoreIsCheatEnabled(cheat))
        {
            continue;
        }

        for (CoreCheatCode& code : cheat.CheatCodes)
        {
            if (code.UseOptions)
            {
                // make sure an option has been set
                if (!CoreHasCheatOptionSet(cheat))
                {
                    skipCheat = true;
                    break;
                }

                // make sure retrieving it succeeds
                if (!CoreGetCheatOption(cheat, cheatOption))
                {
                    skipCheat = true;
                    break;
                }

                // make sure combining the cheat code & option succeeds
                if (!combine_cheat_code_and_option(code, cheatOption, combinedValue))
                {
                    skipCheat = true;
                    break;
                }

                m64p_cheatCodes.push_back({code.Address, combinedValue});
            }
            else
            {
                m64p_cheatCodes.push_back({code.Address, code.Value});
            }
        }

        if (skipCheat)
        {
            continue;
        }

        ret = m64p::Core.AddCheat(cheat.Name.c_str(), m64p_cheatCodes.data(), m64p_cheatCodes.size());
        if (ret != M64ERR_SUCCESS)
        {
            error = "CoreApplyCheats m64p::Core.AddCheat(";
            error += cheat.Name.c_str();
            error += ") Failed:";
            error += m64p::Core.ErrorMessage(ret);
            CoreSetError(error);
            return false;
        }

        // add cheat to loaded cheats
        l_LoadedCheats.push_back({cheat, cheatOption});
    }

    return true;
}

bool CoreClearCheats(void)
{
    std::string error;
    m64p_error ret;

    for (l_LoadedCheat& loadedCheat : l_LoadedCheats)
    {
        ret = m64p::Core.CheatEnabled(loadedCheat.cheat.Name.c_str(), 0);
        if (ret != M64ERR_SUCCESS)
        {
            error = "CoreClearCheats m64p::Core.CheatEnabled(";
            error += loadedCheat.cheat.Name.c_str();
            error += ") Failed:";
            error += m64p::Core.ErrorMessage(ret);
            CoreSetError(error);
            return false;
        }
    }

    l_SharedCheatFile = {};
    l_UserCheatFile = {};
    l_LoadedCheats.clear();
    return true;
}

bool CorePressGamesharkButton(bool enabled)
{
    std::string error;
    m64p_error ret;
    int tmpValue = enabled ? 1 : 0;

    ret = m64p::Core.DoCommand(M64CMD_CORE_STATE_SET, M64CORE_INPUT_GAMESHARK, &tmpValue);
    if (ret != M64ERR_SUCCESS)
    {
        error = "CorePressGamesharkButton m64p::Core.DoCommand(M64CMD_CORE_STATE_SET) Failed: ";
        error += m64p::Core.ErrorMessage(ret);
        CoreSetError(error);
        return false;
    }

    return true;
}
