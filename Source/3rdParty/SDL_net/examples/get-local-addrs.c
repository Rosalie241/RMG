/*
 * This is just for demonstration purposes! This doesn't
 * do anything as complicated as, say, the `ifconfig` utility.
 *
 * All this to say: don't use this for anything serious!
 */

#include <stdio.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_net/SDL_net.h>

int main(int argc, char **argv)
{
    NET_Address **addrs = NULL;
    int num_addrs = 0;
    int i;

    (void)argc;
    (void)argv;

    if (!NET_Init()) {
        SDL_Log("NET_Init() failed: %s", SDL_GetError());
        return 1;
    }

    addrs = NET_GetLocalAddresses(&num_addrs);
    if (addrs == NULL) {
        SDL_Log("Failed to determine local addresses: %s", SDL_GetError());
        NET_Quit();
        return 1;
    }

    SDL_Log("We saw %d local addresses:", num_addrs);
    for (i = 0; i < num_addrs; i++) {
        SDL_Log("  - %s", NET_GetAddressString(addrs[i]));
    }

    NET_FreeLocalAddresses(addrs);
    NET_Quit();

    return 0;
}
