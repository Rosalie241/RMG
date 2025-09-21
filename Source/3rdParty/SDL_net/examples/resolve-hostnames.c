/*
 * This is just for demonstration purposes! This doesn't
 * do anything as complicated as, say, the `dig` utility.
 *
 * All this to say: don't use this for anything serious!
 */

#include <stdio.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_net/SDL_net.h>

int main(int argc, char **argv)
{
    if (!NET_Init()) {
        SDL_Log("NET_Init() failed: %s", SDL_GetError());
        return 1;
    }

    //NET_SimulateAddressResolutionLoss(3000, 30);

    NET_Address **addrs = (NET_Address **) SDL_calloc(argc, sizeof (NET_Address *));
    for (int i = 1; i < argc; i++) {
        addrs[i] = NET_ResolveHostname(argv[i]);
    }

    for (int i = 1; i < argc; i++) {
        NET_WaitUntilResolved(addrs[i], -1);

        if (NET_GetAddressStatus(addrs[i]) == NET_FAILURE) {
            SDL_Log("%s: [FAILED TO RESOLVE: %s]", argv[i], SDL_GetError());
        } else {
            SDL_Log("%s: %s", argv[i], NET_GetAddressString(addrs[i]));
        }

        NET_UnrefAddress(addrs[i]);
    }

    NET_Quit();

    return 0;
}
