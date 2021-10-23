/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef M64P_API_HPP
#define M64P_API_HPP

#include "ConfigApi.hpp"
#include "CoreApi.hpp"

namespace m64p
{
extern m64p::CoreApi Core;
extern m64p::ConfigApi Config;
} // namespace m64p

#endif // M64P_API_HPP
