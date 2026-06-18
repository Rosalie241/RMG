/*
  SDL_net: A simple networking library for use with SDL
  Copyright (C) 1997-2026 Sam Lantinga <slouken@libsdl.org>

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

#include "SDL3_net/SDL_net.h"

// don't do anything. Sorry!
int NET_Version(void) { return SDL_NET_VERSION; }
bool NET_Init(void) { return SDL_Unsupported(); }
void NET_Quit(void) {}
NET_Address * NET_ResolveHostname(const char *host) { SDL_Unsupported(); return NULL; }
NET_Status NET_WaitUntilResolved(NET_Address *address, Sint32 timeout) { SDL_Unsupported(); return NET_FAILURE; }
NET_Status NET_GetAddressStatus(NET_Address *address) { SDL_Unsupported(); return NET_FAILURE; }
const char * NET_GetAddressString(NET_Address *address) { SDL_Unsupported(); return NULL; }
const void * NET_GetAddressBytes(NET_Address *address, int *num_bytes) { if (num_bytes) { *num_bytes = 0; } SDL_Unsupported(); return NULL; }
NET_Address *NET_RefAddress(NET_Address *address) { SDL_Unsupported(); return NULL; }
void NET_UnrefAddress(NET_Address *address) {}
void NET_SimulateAddressResolutionLoss(int percent_loss) {}
int NET_CompareAddresses(const NET_Address *a, const NET_Address *b) { return 0; }
NET_Address **NET_GetLocalAddresses(int *num_addresses) { SDL_Unsupported(); return NULL; }
void NET_FreeLocalAddresses(NET_Address **addresses) {}
NET_StreamSocket * NET_CreateClient(NET_Address *address, Uint16 port, SDL_PropertiesID props) { SDL_Unsupported(); return NULL; }
NET_Status NET_WaitUntilConnected(NET_StreamSocket *sock, Sint32 timeout) { SDL_Unsupported(); return NET_FAILURE; }
NET_Server * NET_CreateServer(NET_Address *addr, Uint16 port, SDL_PropertiesID props) { SDL_Unsupported(); return NULL; }
bool NET_AcceptClient(NET_Server *server, NET_StreamSocket **client_stream) { SDL_Unsupported(); return false; }
void NET_DestroyServer(NET_Server *server) {}
NET_Address * NET_GetStreamSocketAddress(NET_StreamSocket *sock) { SDL_Unsupported(); return NULL; }
NET_Status NET_GetConnectionStatus(NET_StreamSocket *sock) { SDL_Unsupported(); return NET_FAILURE; }
bool NET_WriteToStreamSocket(NET_StreamSocket *sock, const void *buf, int buflen) { SDL_Unsupported(); return false; }
int NET_GetStreamSocketPendingWrites(NET_StreamSocket *sock) { SDL_Unsupported(); return -1; }
int NET_WaitUntilStreamSocketDrained(NET_StreamSocket *sock, Sint32 timeout) { SDL_Unsupported(); return -1; }
int NET_ReadFromStreamSocket(NET_StreamSocket *sock, void *buf, int buflen) { SDL_Unsupported(); return -1; }
void NET_SimulateStreamPacketLoss(NET_StreamSocket *sock, int percent_loss) {}
void NET_DestroyStreamSocket(NET_StreamSocket *sock) {}
NET_DatagramSocket * NET_CreateDatagramSocket(NET_Address *addr, Uint16 port, SDL_PropertiesID props) { SDL_Unsupported(); return NULL; }
bool NET_SendDatagram(NET_DatagramSocket *sock, NET_Address *address, Uint16 port, const void *buf, int buflen) { SDL_Unsupported(); return false; }
bool NET_ReceiveDatagram(NET_DatagramSocket *sock, NET_Datagram **dgram) { SDL_Unsupported(); return false; }
void NET_DestroyDatagram(NET_Datagram *dgram) {}
void NET_SimulateDatagramPacketLoss(NET_DatagramSocket *sock, int percent_loss) {}
void NET_DestroyDatagramSocket(NET_DatagramSocket *sock) {}
int NET_WaitUntilInputAvailable(void **vsockets, int numsockets, Sint32 timeout) { SDL_Unsupported(); return -1; }

