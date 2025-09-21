#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_net/SDL_net.h>

int main(int argc, char *argv[])
{
    if (!SDL_Init(0)) {
        SDL_Log("SDL_Init(0) failed: %s", SDL_GetError());
        return 1;
    }

    if (!NET_Init()) {
        SDL_Log("NET_Init() failed: %s", SDL_GetError());
    }

    NET_Quit();
    SDL_Quit();
    return 0;
}
