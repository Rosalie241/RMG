/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "OnScreenDisplay.hpp"

#include <RMG-Core/Core.hpp>

#include <imgui.h>
#include <backends/imgui_impl_opengl3.h>
#include <chrono>

//
// Local Variables
//

static bool l_Initialized     = false;
static bool l_Enabled         = false;
static bool l_RenderingPaused = false;

static std::chrono::time_point<std::chrono::high_resolution_clock> l_MessageTime;
static std::string                                                 l_Message;
static int                                                         l_MessagePosition = 1;
static float                                                       l_MessagePaddingX = 20.0f;
static float                                                       l_MessagePaddingY = 20.0f;

//
// Exported Functions
//

bool OnScreenDisplayInit(void)
{
    if (l_Initialized)
    {
        return false;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    if (!ImGui_ImplOpenGL3_Init())
    {
        return false;
    }

    l_Initialized     = true;
    return true;
}

void OnScreenDisplayShutdown(void)
{
    if (!l_Initialized)
    {
        return;
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui::DestroyContext();
    l_Initialized     = false;
    l_RenderingPaused = false;
}

void OnScreenDisplayLoadSettings(void)
{
    l_Enabled         = CoreSettingsGetBoolValue(SettingsID::GUI_OnScreenDisplayEnabled);
    l_MessagePosition = CoreSettingsGetIntValue(SettingsID::GUI_OnScreenDisplayLocation);
    l_MessagePaddingX = CoreSettingsGetIntValue(SettingsID::GUI_OnScreenDisplayPaddingX);
    l_MessagePaddingY = CoreSettingsGetIntValue(SettingsID::GUI_OnScreenDisplayPaddingY);
}

bool OnScreenDisplaySetDisplaySize(int width, int height)
{
    if (!l_Initialized)
    {
        return false;
    }

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)width, (float)height);
    return true;
}

void OnScreenDisplaySetMessage(std::string message)
{
    if (!l_Initialized)
    {
        return;
    }

    l_Message     = message;
    l_MessageTime = std::chrono::high_resolution_clock::now();
}

void OnScreenDisplayRender(void)
{
    if (!l_Initialized || !l_Enabled || l_RenderingPaused || l_Message.empty())
    {
        return;
    }

    const auto currentTime = std::chrono::high_resolution_clock::now();
    const int secondsPassed = std::chrono::duration_cast<std::chrono::seconds>(currentTime - l_MessageTime).count();
    if (secondsPassed >= 3)
    {
        return;
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();
    
    ImGuiIO& io = ImGui::GetIO();

    ImGui::SetNextWindowBgAlpha(0.3f);

    // right bottom = ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - 20.0f, io.DisplaySize.y - 20.0f), ImGuiCond_Always, ImVec2(1.0f, 1.0f));
    // right top    = ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - 20.0f, 20.0f), ImGuiCond_Always, ImVec2(1.0f, 0));
    // left  bottom = ImGui::SetNextWindowPos(ImVec2(20.0f, io.DisplaySize.y - 20.0f), ImGuiCond_Always, ImVec2(0.0f, 1.0f));
    // left  top    = ImGui::SetNextWindowPos(ImVec2(20.0f, 20.0f), ImGuiCond_Always, ImVec2(0.0f, 0.0f));
    switch (l_MessagePosition)
    {
    default:
    case 0: // left bottom
        ImGui::SetNextWindowPos(ImVec2(l_MessagePaddingX, io.DisplaySize.y - l_MessagePaddingY), ImGuiCond_Always, ImVec2(0.0f, 1.0f));
        break;
    case 1: // left top
        ImGui::SetNextWindowPos(ImVec2(l_MessagePaddingX, l_MessagePaddingY), ImGuiCond_Always, ImVec2(0.0f, 0.0f));
        break;
    case 2: // right top
        ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - l_MessagePaddingX, l_MessagePaddingY), ImGuiCond_Always, ImVec2(1.0f, 0));
        break;
    case 3: // right bottom
        ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - l_MessagePaddingX, io.DisplaySize.y - l_MessagePaddingY), ImGuiCond_Always, ImVec2(1.0f, 1.0f));
        break;
    }

    ImGui::Begin("Message", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoFocusOnAppearing); 
    ImGui::Text("%s", l_Message.c_str());
    ImGui::End();

    ImGui::Render();

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void OnScreenDisplayPause(void)
{
    l_RenderingPaused = true;
}

void OnScreenDisplayResume(void)
{
    l_RenderingPaused = false;
}
