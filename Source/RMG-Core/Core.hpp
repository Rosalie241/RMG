/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef CORE_HPP
#define CORE_HPP

#include "CachedRomHeaderAndSettings.hpp"
#include "Settings/Settings.hpp"
#include "SpeedLimiter.hpp"
#include "RomSettings.hpp"
#include "Directories.hpp"
#include "MediaLoader.hpp"
#include "Screenshot.hpp"
#include "Emulation.hpp"
#include "SaveState.hpp"
#include "RomHeader.hpp"
#include "Callback.hpp"
#include "Plugins.hpp"
#include "Cheats.hpp"
#include "Error.hpp"
#include "Unzip.hpp"
#include "Video.hpp"
#include "Key.hpp"
#include "Rom.hpp"
#ifdef CORE_PLUGIN
#include "m64p/api/m64p_common.h"
#include "m64p/api/m64p_custom.h"
#include "m64p/api/m64p_types.h"
#include "m64p/api/m64p_config.h"
#include "m64p/api/m64p_plugin.h"
#include "m64p/api/version.h"
#endif // CORE_PLUGIN


// initializes the core library,
// returns false when failed
bool CoreInit(void);

#ifdef CORE_PLUGIN
// initializes the core library for plugins,
// returns false when failed
bool CoreInit(m64p_dynlib_handle handle);
#endif // CORE_PLUGIN

// shuts down the core library
void CoreShutdown(void);

#endif // CORE_HPP
