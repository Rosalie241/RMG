/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef RMG_ONSCREENDISPLAY_HPP
#define RMG_ONSCREENDISPLAY_HPP

#include <QString>

// attempts to initialize the OSD
bool OnScreenDisplayInit(void);

// shuts down the OSD when initialized
void OnScreenDisplayShutdown(void);

// loads settings for the OSD
void OnScreenDisplayLoadSettings(void);

// sets the display size for the OSD
bool OnScreenDisplaySetDisplaySize(int width, int height);

// sets the current message to the OSD
void OnScreenDisplaySetMessage(QString message);

// renders the OSD
void OnScreenDisplayRender(void);

// pauses OSD rendering
void OnScreenDisplayPause(void);

// resumes OSD rendering
void OnScreenDisplayResume(void);

#endif // RMG_ONSCREENDISPLAY_HPP