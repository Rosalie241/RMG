/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef CORE_CHEATS_HPP
#define CORE_CHEATS_HPP

#include <string>
#include <vector>
#include <cstdint>
#include <filesystem>

struct CoreCheatCode
{
    // Cheat Code Address
    uint32_t Address = 0;

    // Cheat Code Address Value
    int32_t  Value   = 0;

    bool UseOptions  = false;
    int  OptionIndex = 0;
    int  OptionSize  = 0;

    bool operator==(const CoreCheatCode& other) const
    {
        return Address == other.Address &&
                Value == other.Value &&
                UseOptions == other.UseOptions &&
                OptionIndex == other.OptionIndex &&
                OptionSize == other.OptionSize;
    }
};

struct CoreCheatOption
{
    // Cheat Option Name
    std::string Name;

    // Cheat Option Value
    uint32_t    Value = 0;

    // Cheat Option Value Size
    int32_t     Size  = 0;

    bool operator==(const CoreCheatOption& other) const
    {
        return Name == other.Name &&
                Value == other.Value &&
                Size == other.Size;
    }
};

struct CoreCheat
{
    // Cheat Name
    std::string Name;

    // Cheat Author
    std::string Author;

    // Cheat Note
    std::string Note;

    // Whether Cheat Has Options
    bool                         HasOptions = false;
    // Cheat Options
    std::vector<CoreCheatOption> CheatOptions;

    // Cheat Codes
    std::vector<CoreCheatCode> CheatCodes;

    bool operator==(const CoreCheat& other) const
    {
        return Name == other.Name &&
                Author == other.Author &&
                Note == other.Note &&
                HasOptions == other.HasOptions &&
                CheatOptions == other.CheatOptions &&
                CheatCodes == other.CheatCodes;
    }
};

#ifdef CORE_INTERNAL
struct CoreCheatFile
{
    uint32_t CRC1 = 0;
    uint32_t CRC2 = 0;
    uint32_t CountryCode = 0;

    std::string MD5;

    std::string Name;

    std::vector<CoreCheat> Cheats;
};
#endif // CORE_INTERNAL

// attempts to retrieve the cheats for the currently opened ROM
bool CoreGetCurrentCheats(std::filesystem::path file, std::vector<CoreCheat>& cheats);

// attempts to parse cheat from lines
bool CoreParseCheat(const std::vector<std::string>& lines, CoreCheat& cheat);

// attemps to convert the cheat into parsable code lines & option lines
bool CoreGetCheatLines(CoreCheat cheat, std::vector<std::string>& codeLines, std::vector<std::string>& optionLines);

// attempts to add the cheat
bool CoreAddCheat(std::filesystem::path file, CoreCheat cheat);

// attemps to update given cheat
bool CoreUpdateCheat(std::filesystem::path file, CoreCheat oldCheat, CoreCheat newCheat);

// returns whether you can remove the cheat
bool CoreCanRemoveCheat(CoreCheat cheat);

// attempts to remove the given cheat
bool CoreRemoveCheat(std::filesystem::path file, CoreCheat cheat);

// attempt to enable the cheat
bool CoreEnableCheat(std::filesystem::path file, CoreCheat cheat, bool enabled);

// returns whether cheat is enabled
bool CoreIsCheatEnabled(std::filesystem::path file, CoreCheat cheat);

// returns whether an option has been set for the given cheat
bool CoreHasCheatOptionSet(std::filesystem::path file, CoreCheat cheat);

// attempts to set the cheat's option
bool CoreSetCheatOption(std::filesystem::path file, CoreCheat cheat, CoreCheatOption option);

// attempts to retrieve the currently's set cheat's option
bool CoreGetCheatOption(std::filesystem::path file, CoreCheat cheat, CoreCheatOption& option);

// attempts to reset the cheat option
bool CoreResetCheatOption(std::filesystem::path file, CoreCheat cheat);

// attempts to apply the enabled cheats to the currently opened ROM
bool CoreApplyCheats(void);

// attempts to remove all cheats from the currently opened ROM
bool CoreClearCheats(void);

// attempts to set netplay cheats
bool CoreSetNetplayCheats(const std::vector<CoreCheat>& cheats);

// attempts to apply the enabled netplay cheats
bool CoreApplyNetplayCheats(void);

// attemps to press the gameshark button
bool CorePressGamesharkButton(bool enabled);

#endif // CORE_CHEATS_HPP
