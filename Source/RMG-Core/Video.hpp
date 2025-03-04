/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef CORE_VIDEO_HPP
#define CORE_VIDEO_HPP

enum CoreVideoMode
{
	None       = 1,
	Windowed   = 2,
	Fullscreen = 3
};

// attempts to retrieve the video size
bool CoreGetVideoSize(int& width, int& height);

// sets video size with given width & height
bool CoreSetVideoSize(int width, int height);

// toggles fullscreen
bool CoreToggleFullscreen(void);

// retrieves current video mode
bool CoreGetVideoMode(CoreVideoMode& mode);

#endif // CORE_VIDEO_HPP