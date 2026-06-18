/*
  SDL_net: A simple networking library for use with SDL
  Copyright (C) 1997-2025 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

// we use a little bit of C++ for Haiku support, since the system frameworks
//  require it. Specifically, we need to get network interface change
//  notifications through BNetworkRoster.

#include <AppKit.h>
#include <NetworkNotifications.h>

#include <SDL3/SDL.h>

extern "C" {
    extern bool NET_HAIKU_InitInterfaceChangeNotifications(SDL_AtomicInt *changed);
    extern void NET_HAIKU_QuitInterfaceChangeNotifications(void);
}

class NET_InterfaceLooper : public BLooper
{
public:
    NET_InterfaceLooper(SDL_AtomicInt *_changed);
    virtual void MessageReceived(BMessage *message);
    virtual ~NET_InterfaceLooper() {}

private:
    SDL_AtomicInt *changed;
};


NET_InterfaceLooper::NET_InterfaceLooper(SDL_AtomicInt *_changed)
    : BLooper("NET_InterfaceLooper")
    , changed(_changed)
{
}

void NET_InterfaceLooper::MessageReceived(BMessage *message)
{
    //SDL_Log("NET_InterfaceLooper::MessageReceived %d", (int) message->what);
    SDL_SetAtomicInt(changed, 1);
    BLooper::MessageReceived(message);
}


static NET_InterfaceLooper *looper = NULL;

bool NET_HAIKU_InitInterfaceChangeNotifications(SDL_AtomicInt *changed)
{
    SDL_assert(!looper);
    looper = new NET_InterfaceLooper(changed);
    looper->Run();
    return start_watching_network(
        B_NETWORK_INTERFACE_ADDED | B_NETWORK_INTERFACE_REMOVED |
        B_NETWORK_INTERFACE_CHANGED | B_NETWORK_DEVICE_LINK_CHANGED, looper
    ) == B_OK;
}

void NET_HAIKU_QuitInterfaceChangeNotifications(void)
{
    if (looper) {
        looper->PostMessage(B_QUIT_REQUESTED);
        looper = NULL;
    }
}

