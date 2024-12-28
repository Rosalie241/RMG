/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef RMG_VIDEXT_HPP
#define RMG_VIDEXT_HPP

#include <UserInterface/Widget/Render/OGLWidget.hpp>
#include <UserInterface/Widget/Render/VKWidget.hpp>
#include <UserInterface/MainWindow.hpp>
#include <Thread/EmulationThread.hpp>

bool SetupVidExt(Thread::EmulationThread* emuThread, UserInterface::MainWindow* mainWindow, 
    UserInterface::Widget::OGLWidget** oglWidget, UserInterface::Widget::VKWidget** vulkanWidget);

#endif // RMG_VIDEXT_HPP