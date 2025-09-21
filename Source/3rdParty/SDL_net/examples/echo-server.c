#include <SDL3/SDL_main.h>
#include <SDL3/SDL.h>
#include <SDL3_net/SDL_net.h>

int main(int argc, char **argv)
{
    const char *interface = NULL;
    Uint16 server_port = 2382;
    int simulate_failure = 0;

    for (int i = 1; i < argc; i++) {
        const char *arg = argv[i];
        if ((SDL_strcmp(arg, "--port") == 0) && (i < (argc-1))) {
            server_port = (Uint16) SDL_atoi(argv[++i]);
        } else if ((SDL_strcmp(arg, "--simulate-failure") == 0) && (i < (argc-1))) {
            simulate_failure = (int) SDL_atoi(argv[++i]);
        } else {
            interface = arg;
        }
    }

    if (!NET_Init()) {
        SDL_Log("NET_Init failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    if (interface) {
        SDL_Log("Attempting to listen on interface '%s', port %d", interface, (int) server_port);
    } else {
        SDL_Log("Attempting to listen on all interfaces, port %d", (int) server_port);
    }

    simulate_failure = SDL_clamp(simulate_failure, 0, 100);
    if (simulate_failure) {
        SDL_Log("Simulating failure at %d percent", simulate_failure);
    }

    NET_Address *server_addr = NULL;
    if (interface) {
        server_addr = NET_ResolveHostname(interface);
        if (!server_addr || (NET_WaitUntilResolved(server_addr, -1) != NET_SUCCESS)) {
            SDL_Log("Failed to resolve interface for '%s': %s", interface, SDL_GetError());
            if (server_addr) {
                NET_UnrefAddress(server_addr);
            }
            NET_Quit();
            SDL_Quit();
            return 1;
        } else {
            SDL_Log("Interface '%s' resolves to '%s' ...", interface, NET_GetAddressString(server_addr));
        }
    }

    NET_Server *server = NET_CreateServer(server_addr, server_port);
    if (!server) {
        SDL_Log("Failed to create server: %s", SDL_GetError());
    } else {
        SDL_Log("Server is ready! Connect to port %d and send text!", (int) server_port);
        int num_vsockets = 1;
        void *vsockets[128];
        SDL_zeroa(vsockets);
        vsockets[0] = server;
        while (NET_WaitUntilInputAvailable(vsockets, num_vsockets, -1) >= 0) {
            NET_StreamSocket *streamsocket = NULL;
            if (!NET_AcceptClient(server, &streamsocket)) {
                SDL_Log("NET_AcceptClient failed: %s", SDL_GetError());
                break;
            } else if (streamsocket) { // new connection!
                SDL_Log("New connection from %s!", NET_GetAddressString(NET_GetStreamSocketAddress(streamsocket)));
                if (num_vsockets >= (int) (SDL_arraysize(vsockets) - 1)) {
                    SDL_Log("  (too many connections, though, so dropping immediately.)");
                    NET_DestroyStreamSocket(streamsocket);
                } else {
                    if (simulate_failure) {
                        NET_SimulateStreamPacketLoss(streamsocket, simulate_failure);
                    }
                    vsockets[num_vsockets++] = streamsocket;
                }
            }

            // see if anything has new stuff.
            char buffer[1024];
            for (int i = 1; i < num_vsockets; i++) {
                bool kill_socket = false;
                streamsocket = (NET_StreamSocket *) vsockets[i];
                const int br = NET_ReadFromStreamSocket(streamsocket, buffer, sizeof (buffer));
                if (br < 0) {  // uhoh, socket failed!
                    kill_socket = true;
                } else if (br > 0) {
                    const char *addrstr = NET_GetAddressString(NET_GetStreamSocketAddress(streamsocket));
                    SDL_Log("Got %d more bytes from '%s'", br, addrstr);
                    if (!NET_WriteToStreamSocket(streamsocket, buffer, br)) {
                        SDL_Log("Failed to echo data back to '%s': %s", addrstr, SDL_GetError());
                        kill_socket = true;
                    }
                }

                if (kill_socket) {
                    SDL_Log("Dropping connection to '%s'", NET_GetAddressString(NET_GetStreamSocketAddress(streamsocket)));
                    NET_DestroyStreamSocket(streamsocket);
                    vsockets[i] = NULL;
                    if (i < (num_vsockets - 1)) {
                        SDL_memmove(&vsockets[i], &vsockets[i+1], sizeof (vsockets[0]) * ((num_vsockets - i) - 1));
                    }
                    num_vsockets--;
                    i--;
                }
            }
        }

        SDL_Log("Destroying server...");
        NET_DestroyServer(server);
    }

    SDL_Log("Shutting down...");
    NET_Quit();
    SDL_Quit();
    return 0;
}

