/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "Settings/Settings.hpp"
#include "DiscordRpc.hpp"
#include "RomSettings.hpp"
#include "RomHeader.hpp"

#include <../3rdParty/discord-rpc/include/discord_rpc.h>
#include <cstring>
#include <ctime>
#include <algorithm>

//
// Exported Functions
//

void CoreDiscordRpcInit(void)
{
    if (!CoreSettingsGetBoolValue(SettingsID::GUI_DiscordRpc))
    {
        return;
    }

    Discord_Initialize("801450412280119356", nullptr, 0, "");
}

void CoreDiscordRpcShutdown(void)
{
    Discord_Shutdown();
}

void CoreDiscordRpcUpdate(bool inGame)
{
    std::string smallImageKey;
    std::string largeImageKey;
    std::string details;

    if (!CoreSettingsGetBoolValue(SettingsID::GUI_DiscordRpc))
    {
        return;
    }

    if (inGame)
    {
        CoreRomSettings romSettings;
        CoreRomHeader romHeader;

        // make sure we retrieve the rom settings & header
        if (!CoreGetCurrentRomSettings(romSettings) ||
            !CoreGetCurrentRomHeader(romHeader))
        {
            return;
        }

        smallImageKey = "rmg-icon";
        largeImageKey = romHeader.Name;
        details = romSettings.GoodName;

        // replace ' ' with '_' and replace '&' with '_'
        // also tolower the entire string
        std::replace(largeImageKey.begin(),largeImageKey.end(),' ','_');
        std::replace(largeImageKey.begin(),largeImageKey.end(),'&','_');
        std::transform(largeImageKey.begin(), largeImageKey.end(), largeImageKey.begin(), ::tolower);
    }
    else
    {
        largeImageKey = "rmg-icon";
        details = "Not in-game";
    }

    DiscordRichPresence discordPresence;
    memset(&discordPresence, 0, sizeof(discordPresence));
    discordPresence.smallImageKey = smallImageKey.c_str();
    discordPresence.largeImageKey = largeImageKey.c_str();
    discordPresence.largeImageText = "Rosalie's Mupen GUI";
    discordPresence.details = details.c_str();
    discordPresence.startTimestamp = time(nullptr);

    Discord_UpdatePresence(&discordPresence);
}