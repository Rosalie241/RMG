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

#ifdef _WIN32
#ifdef _WIN32_WINNT
#  if _WIN32_WINNT < 0x0600 // we need APIs that didn't arrive until Windows Vista.
#    undef _WIN32_WINNT
#  endif
#endif
#ifndef _WIN32_WINNT
#  define _WIN32_WINNT 0x0600
#endif
#endif /* _WIN32 */

#include "SDL3_net/SDL_net.h"

#ifdef SDL_PLATFORM_WINDOWS
#define WIN32_LEAN_AND_MEAN 1
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
typedef SOCKET Socket;
typedef int SockLen;
typedef SOCKADDR_STORAGE AddressStorage;

static int write(SOCKET s, const void *buf, size_t count) {
    return send(s, (const char *)buf, (int) count, 0);
}

static int read(SOCKET s, char *buf, size_t count) {
    WSABUF wsabuf;
    wsabuf.buf = buf;
    wsabuf.len = (ULONG) count;
    DWORD count_received;
    DWORD flags = 0;
    const int res = WSARecv(s, &wsabuf, 1, &count_received, &flags, NULL, NULL);
    if (res != 0) {
        return -1;
    }
    return (int)count_received;
}

// WSAPoll doesn't exist on Windows before Vista, and isn't reliable before some version of Windows 10,
//  so for now we just fake it with select().
static int WindowsPoll(struct pollfd *fds, unsigned int nfds, int timeout)
{
    SDL_assert(fds != NULL);
    SDL_assert(nfds > 0);

    int nreadfds = 0, nwritefds = 0, nexceptfds = nfds;
    for (unsigned int i = 0; i < nfds; i++) {
        fds[i].revents = 0;
        if (fds[i].events & POLLIN) {
            nreadfds++;
        }
        if (fds[i].events & POLLOUT) {
            nwritefds++;
        }
    }

    // FD_SETSIZE is 64 on Windows, but they don't use bitsets, so you can
    //  just supply your own struct that uses any length.
    //
    //   https://devblogs.microsoft.com/oldnewthing/20221102-00/?p=107343
    typedef struct WindowsPoll_fd_set {
        Uint32 count;
        SOCKET sockets[]; // variable-length array
    } WindowsPoll_fd_set;

    Uint8 stackbuf_read[256];  // in case if we can do this without malloc.
    Uint8 stackbuf_write[256];  // in case if we can do this without malloc.
    Uint8 stackbuf_except[256];  // in case if we can do this without malloc.
    WindowsPoll_fd_set *readfds = NULL;
    WindowsPoll_fd_set *writefds = NULL;
    WindowsPoll_fd_set *exceptfds = NULL;

    bool failed = false;
    #define ALLOC_FDSET(typ) { \
        if (!failed && (n##typ##fds > 0)) { \
            const int len = sizeof (Uint32) + (n##typ##fds * sizeof (SOCKET)); \
            if (len < (int) sizeof (stackbuf_##typ)) { \
                typ##fds = (WindowsPoll_fd_set *) stackbuf_##typ; \
            } else { \
                typ##fds = (WindowsPoll_fd_set *) SDL_malloc(len); \
                if (!typ##fds) { \
                    failed = true; \
                } \
            } \
            if (typ##fds) { \
                SDL_memset(typ##fds, '\0', len); \
            } \
        } \
    }
    ALLOC_FDSET(read);
    ALLOC_FDSET(write);
    ALLOC_FDSET(except);
    #undef ALLOC_FDSET

    int retval = -1;
    if (!failed) {
        for (unsigned int i = 0; i < nfds; i++) {
            exceptfds->sockets[exceptfds->count++] = fds[i].fd;
            if (fds[i].events & POLLIN) {
                readfds->sockets[readfds->count++] = fds[i].fd;
            }
            if (fds[i].events & POLLOUT) {
                writefds->sockets[writefds->count++] = fds[i].fd;
            }
        }

        struct timeval tvtimeout;
        struct timeval *ptvtimeout = NULL;

        if (timeout >= 0) {
            tvtimeout.tv_sec = timeout / 1000;
            tvtimeout.tv_usec = (timeout % 1000) * 1000;
            ptvtimeout = &tvtimeout;
        }

        // WinSock's select() ignores the first parameter, since it doesn't use bitsets, and SOCKETs aren't small integers. Just specify zero here.
        retval = select(0, (fd_set *) readfds, (fd_set *) writefds, (fd_set *) exceptfds, ptvtimeout);
        if (retval > 0) {
            #define CHECKSET(typ, flag) { \
                if (typ##fds != NULL) { \
                    for (Uint32 i = 0; i < typ##fds->count; i++) { \
                        SOCKET sock = typ##fds->sockets[i]; \
                        for (unsigned int j = 0; j < nfds; j++) { \
                            if (fds[j].fd == sock) { \
                                fds[j].revents |= flag; \
                            } \
                        } \
                    } \
                } \
            }
            CHECKSET(read, POLLIN);
            CHECKSET(write, POLLOUT);
            CHECKSET(except, POLLERR);
            #undef CHECKSET
        }
    }

    if ((void *) readfds != (void *) stackbuf_read) { SDL_free(readfds); }
    if ((void *) writefds != (void *) stackbuf_write) { SDL_free(writefds); }
    if ((void *) exceptfds != (void *) stackbuf_except) { SDL_free(exceptfds); }

    return retval;
}

#define poll WindowsPoll

#else
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#ifndef SDL_PLATFORM_VITA
#include <net/if.h>
#endif
#include <netdb.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <poll.h>
#include <unistd.h>
#include <fcntl.h>

#if defined(SDL_PLATFORM_LINUX) || defined(SDL_PLATFORM_ANDROID)
#define USE_NETLINK 1
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#endif

#ifdef HAVE_GETIFADDRS
#include <ifaddrs.h>
#endif

#ifdef SDL_PLATFORM_VITA
#include <psp2/net/netctl.h>
#endif

#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
typedef int Socket;
typedef socklen_t SockLen;
typedef struct sockaddr_storage AddressStorage;
#endif

typedef enum NET_SocketType
{
    SOCKETTYPE_STREAM,
    SOCKETTYPE_DATAGRAM,
    SOCKETTYPE_SERVER
} NET_SocketType;


int NET_Version(void)
{
    return SDL_NET_VERSION;
}

typedef struct NetworkInterface NetworkInterface;

struct NET_Address
{
    char *hostname;
    char *human_readable;
    char *errstr;
    SDL_AtomicInt refcount;
    SDL_AtomicInt status;  // This is actually a NET_Status.
    struct addrinfo *ainfo;
    NET_Address *resolver_next;  // a linked list for the resolution job queue.
};

struct NetworkInterface
{
    char *name;
    Uint32 index;
    NET_Address *address;
    NET_Address *broadcast;
};


#define MIN_RESOLVER_THREADS 2
#define MAX_RESOLVER_THREADS 10

// Address resolver state...
static NET_Address *resolver_queue = NULL;
static SDL_Thread *resolver_threads[MAX_RESOLVER_THREADS];
static SDL_Mutex *resolver_lock = NULL;
static SDL_Condition *resolver_condition = NULL;
static SDL_AtomicInt resolver_shutdown;
static SDL_AtomicInt resolver_num_threads;
static SDL_AtomicInt resolver_num_requests;
static SDL_AtomicInt resolver_percent_loss;

// Network interface state...
static SDL_InitState interface_init;
static SDL_RWLock *interface_rwlock = NULL;
static int num_interfaces = 0;
static NetworkInterface *interfaces = NULL;
static SDL_AtomicInt interfaces_have_changed;

// Other stuff...
static NET_Address *ipv6_broadcast_addr = NULL;


// between lo and hi (inclusive; it can return lo or hi itself, too!).
static int RandomNumberBetween(const int lo, const int hi)
{
    return SDL_rand(((hi + 1) - lo)) + lo;
}

static bool ShouldSimulateLoss(const int percent_likely_to_lose)
{
    // these should be clamped when assigning them.
    SDL_assert(percent_likely_to_lose >= 0);
    SDL_assert(percent_likely_to_lose <= 100);
    return (percent_likely_to_lose > 0) ? (RandomNumberBetween(0, 100) < percent_likely_to_lose) : false;
}

static int CloseSocketHandle(Socket handle)
{
#ifdef SDL_PLATFORM_WINDOWS
    return closesocket(handle);
#else
    return close(handle);
#endif
}

static int LastSocketError(void)
{
#ifdef SDL_PLATFORM_WINDOWS
    return WSAGetLastError();
#else
    return errno;
#endif
}

static char *CreateSocketErrorString(int rc)
{
#ifdef SDL_PLATFORM_WINDOWS
    WCHAR msgbuf[256];
    const DWORD bw = FormatMessageW(
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        rc,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), /* Default language */
        msgbuf,
        SDL_arraysize(msgbuf),
        NULL 
    );
    if (bw == 0) {
        return SDL_strdup("Unknown error");
    }
    return SDL_iconv_string("UTF-8", "UTF-16LE", (const char *)msgbuf, (((size_t) bw)+1) * sizeof (WCHAR));
#else
    return SDL_strdup(strerror(rc));
#endif
}

static char *CreateGetAddrInfoErrorString(int rc)
{
#ifdef SDL_PLATFORM_WINDOWS
    return CreateSocketErrorString(rc);  // same error codes.
#else
    return SDL_strdup((rc == EAI_SYSTEM) ? strerror(errno) : gai_strerror(rc));
#endif
}

static int SetSocketError(const char *msg, int err)
{
    char *errmsg = CreateSocketErrorString(err);
    SDL_SetError("%s: %s", msg, errmsg);
    SDL_free(errmsg);
    return -1;
}

static bool SetSocketErrorBool(const char *msg, int err)
{
    SetSocketError(msg, err);
    return false;
}

static int SetLastSocketError(const char *msg)
{
    return SetSocketError(msg, LastSocketError());
}

static int SetGetAddrInfoError(const char *msg, int err)
{
    char *errmsg = CreateGetAddrInfoErrorString(err);
    SDL_SetError("%s: %s", msg, errmsg);
    SDL_free(errmsg);
    return -1;
}

static bool SetGetAddrInfoErrorBool(const char *msg, int err)
{
    SetGetAddrInfoError(msg, err);
    return false;
}

static int MakeSocketNonblocking(Socket handle)
{
    #ifdef SDL_PLATFORM_WINDOWS
    DWORD one = 1;
    return ioctlsocket(handle, FIONBIO, &one);
    #else
    return fcntl(handle, F_SETFL, fcntl(handle, F_GETFL, 0) | O_NONBLOCK);
    #endif
}

static bool WouldBlock(const int err)
{
    #ifdef SDL_PLATFORM_WINDOWS
    return (err == WSAEWOULDBLOCK);
    #else
    return ((err == EWOULDBLOCK) || (err == EAGAIN) || (err == EINPROGRESS));
    #endif
}

static NET_Address *CreateSDLNetAddrFromSockAddr(const struct sockaddr *saddr, SockLen saddrlen);



/* Network interface enumeration... */
static void FreeNetworkInterfaces(NetworkInterface *ifaces, int num_ifaces)
{
    for (int i = 0; i < num_ifaces; i++) {
        NET_UnrefAddress(ifaces[i].address);
        NET_UnrefAddress(ifaces[i].broadcast);
        SDL_free(ifaces[i].name);
    }
    SDL_free(ifaces);
}


#ifdef SDL_PLATFORM_WINDOWS
static HANDLE interface_change_notifications_handle;

static VOID WINAPI NetworkInterfaceChangedCallback(PVOID ctx, PMIB_IPINTERFACE_ROW row, MIB_NOTIFICATION_TYPE type)
{
    SDL_SetAtomicInt(&interfaces_have_changed, 1);
}

static bool InitInterfaceChangeNotifications(void)   // WINDOWS VERSION
{
    // this would need to use NotifyAddrChange for Windows XP, but it doesn't work with IPv6 before Vista.
    //return (NotifyAddrChange(&addr_change_notification_handle, &overlapped) == ERROR_IO_PENDING);

    // !!! FIXME: this function is available on Vista and later.
    return (NotifyIpInterfaceChange(AF_UNSPEC, NetworkInterfaceChangedCallback, NULL, FALSE, &interface_change_notifications_handle) == NO_ERROR);
}

static void QuitInterfaceChangeNotifications(void)  // WINDOWS VERSION
{
    CancelMibChangeNotify2(interface_change_notifications_handle);
    interface_change_notifications_handle = 0;
}

static void RefreshInterfaces(void)  // WINDOWS VERSION
{
    // MSDN docs say start with a 15K buffer, which usually works on the first
    //  try, instead of trying to query for size, allocate, and then retry,
    //  since this tends to be more expensive.
    ULONG buflen = 15 * 1024;
    IP_ADAPTER_ADDRESSES *addrs = NULL;
    ULONG rc;

    do {
        SDL_free(addrs);
        addrs = (IP_ADAPTER_ADDRESSES *) SDL_malloc(buflen);
        if (!addrs) {
            return;
        }

        const ULONG flags = GAA_FLAG_SKIP_DNS_SERVER | GAA_FLAG_SKIP_FRIENDLY_NAME;
        rc = GetAdaptersAddresses(AF_UNSPEC, flags, NULL, addrs, &buflen);
    } while (rc == ERROR_BUFFER_OVERFLOW);

    if (rc != NO_ERROR) {
        SDL_free(addrs);
        return;
    }

    int new_num_interfaces = 0;
    for (IP_ADAPTER_ADDRESSES *i = addrs; i != NULL; i = i->Next) {
        for (IP_ADAPTER_UNICAST_ADDRESS *j = i->FirstUnicastAddress; j != NULL; j = j->Next) {
            new_num_interfaces++;
        }
    }

    NetworkInterface *new_interfaces = (NetworkInterface *) SDL_calloc(new_num_interfaces + 1, sizeof (*new_interfaces));
    if (!new_interfaces) {
        SDL_free(addrs);
        return;
    }

    int count = 0;
    for (IP_ADAPTER_ADDRESSES *i = addrs; i != NULL; i = i->Next) {
        char *friendlyname = SDL_iconv_string("UTF-8", "UTF-16LE", (const char*)(i->FriendlyName), (SDL_wcslen(i->FriendlyName) + 1) * sizeof(WCHAR));
        for (IP_ADAPTER_UNICAST_ADDRESS *j = i->FirstUnicastAddress; j != NULL; j = j->Next) {
            NET_Address *addr = CreateSDLNetAddrFromSockAddr(j->Address.lpSockaddr, j->Address.iSockaddrLength);
            if (addr) {
                new_interfaces[count].name = friendlyname ? SDL_strdup(friendlyname) : NULL;
                new_interfaces[count].address = addr;
                if (j->Address.lpSockaddr->sa_family == AF_INET6) {
                    new_interfaces[count].index = (Uint32) i->Ipv6IfIndex;
                } else if (j->Address.lpSockaddr->sa_family == AF_INET) {  // if this is IPv4, calculate the broadcast address.
                    SOCKADDR_IN bcast;
                    SDL_assert(j->Address.iSockaddrLength == sizeof (bcast));
                    SDL_memcpy(&bcast, (const SOCKADDR_IN *) j->Address.lpSockaddr, sizeof (SOCKADDR_IN));
                    bcast.sin_addr.S_un.S_addr |= htonl((1 << (32 - j->OnLinkPrefixLength)) - 1);
                    new_interfaces[count].broadcast = CreateSDLNetAddrFromSockAddr((const struct sockaddr*)&bcast, sizeof(bcast));
                    new_interfaces[count].index = (Uint32) i->IfIndex;
                }
                count++;
            }
        }
        SDL_free(friendlyname);
    }

    new_num_interfaces = count;  // in case we dropped one somewhere.

    SDL_LockRWLockForWriting(interface_rwlock);
    NetworkInterface *old_interfaces = interfaces;
    const int old_num_interfaces = num_interfaces;
    interfaces = new_interfaces;
    num_interfaces = new_num_interfaces;
    SDL_UnlockRWLock(interface_rwlock);
    FreeNetworkInterfaces(old_interfaces, old_num_interfaces);

    SDL_free(addrs);
}

#elif defined(USE_NETLINK) || defined(HAVE_GETIFADDRS)

// AF_NETLINK covers Linux (and by extension Android). PF_ROUTE covers the BSDs (and by extension Apple platforms).
// This doesn't cover all Unix platforms that ever existed, but this hits just about everything that is still being maintained seriously.

#if defined(SDL_PLATFORM_HAIKU)  // haiku has its own thing for monitoring, but needs getifaddrs.
extern bool NET_HAIKU_InitInterfaceChangeNotifications(SDL_AtomicInt *changed);
extern void NET_HAIKU_QuitInterfaceChangeNotifications(void);
#elif defined(USE_NETLINK) || defined(PF_ROUTE)
#define USE_NETWORK_MONITOR 1
#else
#warning unknown network monitoring system for this platform - will not report network changes
#endif

#if defined(USE_NETWORK_MONITOR)
static SDL_Thread *interface_change_notifications_thread = NULL;
static SDL_AtomicInt interface_change_notifications_flag;  // !!! FIXME

static int SDLCALL LinuxInterfaceChangeNotificationThread(void *data)
{
    #ifdef USE_NETLINK
    const int fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
    #else
    const int fd = socket(PF_ROUTE, SOCK_RAW, 0);
    #endif
    if (fd == -1) {
        return 0;  //  oh well.
    }

    // !!! FIXME: don't make this non-blocking, find a more efficient way to terminate the thread.
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);

    #ifdef USE_NETLINK
    struct sockaddr_nl addr;
    SDL_zero(addr);
    addr.nl_family = AF_NETLINK;
    addr.nl_pid = 0;  // zero==let the kernel choose a unique number. THIS IS THE SOCKET PORT ID, NOT THE PROCESS ID!
    addr.nl_groups = RTMGRP_LINK|RTMGRP_IPV4_IFADDR|RTMGRP_IPV6_IFADDR;
    if (bind(fd, (struct sockaddr *) &addr, sizeof (addr)) == -1) {
        close(fd);
        return 0;  // oh well.
    }
    #elif defined(RO_MSGFILTER)
    const unsigned char filter[] = { RTM_IFANNOUNCE, RTM_IFINFO, RTM_NEWADDR, RTM_DELADDR, RTM_CHGADDR };
    setsockopt(fd, PF_ROUTE, RO_MSGFILTER, &filter, sizeof (filter));
    #endif

    while (SDL_GetAtomicInt(&interface_change_notifications_flag) == 0) {
        Uint8 buf[8192];
        struct iovec iov = { buf, sizeof (buf) };
        struct msghdr msg;
        SDL_zero(msg);
        msg.msg_iov = &iov;
        msg.msg_iovlen = 1;

        #ifdef USE_NETLINK
        struct sockaddr_nl sa;
        SDL_zero(sa);
        msg.msg_name = &sa;
        msg.msg_namelen = sizeof (sa);
        #endif

        const ssize_t br = recvmsg(fd, &msg, 0);
        if (br < 0) {
            if (WouldBlock(LastSocketError())) {
                SDL_Delay(300);
                continue;
            }
            // quit if there's a failure. Presumably this is because another thread close()'d the socket, to unblock the thread and request termination.
            break;
        }

        // we don't currently bother parsing for specifics, we just use this as a signal to reenumerate interfaces.
        //for (struct nlmsghdr *i = buf; NLMSG_OK(i, br); i = NLMSG_NEXT(i, br)) {}
        //SDL_Log("Network interfaces changed!");
        SDL_SetAtomicInt(&interfaces_have_changed, 1);
    }

    close(fd);

    return 0;
}
#endif

static bool InitInterfaceChangeNotifications(void)  // LINUX/BSD VERSION
{
#ifdef USE_NETWORK_MONITOR
    interface_change_notifications_thread = SDL_CreateThread(LinuxInterfaceChangeNotificationThread, "SDLNetIfaceEnum", NULL);
    return (interface_change_notifications_thread != NULL);
#elif defined(SDL_PLATFORM_HAIKU)
    return NET_HAIKU_InitInterfaceChangeNotifications(&interfaces_have_changed);
#else
    return true;
#endif
}

static void QuitInterfaceChangeNotifications(void)  // LINUX/BSD VERSION
{
#ifdef USE_NETWORK_MONITOR
    if (interface_change_notifications_thread) {
        SDL_SetAtomicInt(&interface_change_notifications_flag, 1);
        SDL_WaitThread(interface_change_notifications_thread, NULL);
        SDL_SetAtomicInt(&interface_change_notifications_flag, 0);
    }
    interface_change_notifications_thread = NULL;
#elif defined(SDL_PLATFORM_HAIKU)
    NET_HAIKU_QuitInterfaceChangeNotifications();
#endif
}

static void RefreshInterfaces(void)  // LINUX/BSD VERSION
{
#ifdef USE_NETLINK
    // Android has getifaddrs() in Android 24, but we still target 21, so do it the hard way. Since this works on normal Linux, we'll just use it there, too.
    const int sock = socket(AF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE);
    if (sock < 0) {
        return;  // oh well.
    }

    fcntl(sock, F_SETFL, fcntl(sock, F_GETFL, 0) | O_NONBLOCK);

    // Ask for the address information.
    typedef struct reqstruct
    {
        struct nlmsghdr header;
        struct ifaddrmsg msg;
    } reqstruct;

    reqstruct req;
    SDL_zero(req);
    req.header.nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP;
    req.header.nlmsg_type = RTM_GETADDR;
    req.header.nlmsg_len = NLMSG_LENGTH(sizeof (req.msg));
    req.msg.ifa_family = AF_UNSPEC;

    if (send(sock, &req, req.header.nlmsg_len, 0) != (Sint32)req.header.nlmsg_len) {
        close(sock);
        return;  // oh well.
    }

    int new_num_interfaces = 0;
    NetworkInterface *new_interfaces = NULL;

    Uint8 buffer[64 * 1024];
    ssize_t br;
    while ((br = recv(sock, buffer, sizeof (buffer), 0)) > 0) {
        for (const struct nlmsghdr *header = (const struct nlmsghdr *) buffer; NLMSG_OK(header, (size_t) br); header = NLMSG_NEXT(header, br)) {
            if (header->nlmsg_type == NLMSG_DONE) {
                goto succeeded;  // we got it all.
            } else if (header->nlmsg_type == NLMSG_ERROR) {
                goto failed;  // uhoh.
            } else if (header->nlmsg_type == RTM_NEWADDR) {
                // strictly speaking, one interface can have multiple IP addresses, so you might have multiple addresses with the same interface index.
                void *ptr = SDL_realloc(new_interfaces, sizeof (*new_interfaces) * (new_num_interfaces + 1));
                if (!ptr) {
                    goto failed;
                }
                new_interfaces = (NetworkInterface *) ptr;
                NetworkInterface *iface = &new_interfaces[new_num_interfaces];
                SDL_zerop(iface);

                const struct ifaddrmsg *msg = (const struct ifaddrmsg *) (NLMSG_DATA(header));
                size_t payload_len = IFA_PAYLOAD(header);
                for (const struct rtattr *attr = IFA_RTA(msg); RTA_OK(attr, payload_len); attr = RTA_NEXT(attr, payload_len)) {
                    const bool isbroadcast = (attr->rta_type == IFA_BROADCAST);
                    if (isbroadcast || (attr->rta_type == IFA_ADDRESS)) {
                        // this gives us the raw bytes of an address, but not the actual sockaddr_* layout, so we have to go with known protocols.  :/
                        AddressStorage addrstorage;
                        SockLen addrlen = 0;
                        SDL_zero(addrstorage);
                        addrstorage.ss_family = msg->ifa_family;
                        if (addrstorage.ss_family == AF_INET) {
                            struct sockaddr_in *sa = (struct sockaddr_in *) &addrstorage;
                            SDL_memcpy(&sa->sin_addr, RTA_DATA(attr), RTA_PAYLOAD(attr));
                            addrlen = sizeof (*sa);
                        } else if (addrstorage.ss_family == AF_INET6) {
                            struct sockaddr_in6 *sa6 = (struct sockaddr_in6 *) &addrstorage;
                            SDL_memcpy(&sa6->sin6_addr, RTA_DATA(attr), RTA_PAYLOAD(attr));
                            addrlen = sizeof (*sa6);
                        } else {
                            continue;  // unknown protocol family.
                        }

                        NET_Address **paddr = isbroadcast ? &iface->broadcast : &iface->address;
                        SDL_assert(*paddr == NULL);  // shouldn't be two of these attributes on a single RTM_NEWADDR.
                        *paddr = CreateSDLNetAddrFromSockAddr((const struct sockaddr *) &addrstorage, addrlen);
                    }
                }

                if (iface->address) {
                    iface->index = (Uint32) msg->ifa_index;

                    char interface_name[IF_NAMESIZE];
                    if (if_indextoname((unsigned int) iface->index, interface_name) == NULL) {  // miraculously, this function is in Android 21; nothing else is, though! We could have done this with netlink too, but this is the easy button.
                        interface_name[0] = '\0';
                    }

                    iface->name = SDL_strdup(interface_name);
                    if (!iface->name) {
                        goto failed;
                    }

                    new_num_interfaces++;
                } else {
                    NET_UnrefAddress(iface->broadcast);  // just in case.
                }
            }
        }
    }

succeeded:
    close(sock);
    SDL_LockRWLockForWriting(interface_rwlock);
    NetworkInterface *old_interfaces = interfaces;
    const int old_num_interfaces = num_interfaces;
    interfaces = new_interfaces;
    num_interfaces = new_num_interfaces;
    SDL_UnlockRWLock(interface_rwlock);
    FreeNetworkInterfaces(old_interfaces, old_num_interfaces);
    return;

failed:
    close(sock);
    FreeNetworkInterfaces(new_interfaces, new_num_interfaces);
    // if this was a failure case, oh well, old (maybe incorrect) ones will have to stand.
#else
    struct ifaddrs *ifaddr = NULL;
    if (getifaddrs(&ifaddr) == -1) {
        return;  // oh well.
    }

    NetworkInterface *new_interfaces = NULL;
    int new_num_interfaces = 0;
    for (struct ifaddrs *i = ifaddr; i != NULL; i = i->ifa_next) {
        if (i->ifa_name && i->ifa_addr && (i->ifa_flags & IFF_UP)) {
            new_num_interfaces++;
        }
    }

    if (!new_num_interfaces) {
        goto succeeded;
    }

    new_interfaces = SDL_calloc(new_num_interfaces, sizeof (*new_interfaces));
    if (!new_interfaces) {
        goto failed;
    }

    NetworkInterface *iface = &new_interfaces[0];
    for (struct ifaddrs *i = ifaddr; i != NULL; i = i->ifa_next) {
        if (!i->ifa_name || !i->ifa_addr || ((i->ifa_flags & IFF_UP) == 0)) {
            continue;
        }

        // !!! FIXME: getifaddrs doesn't return the sockaddr length, so we have to go with known protocols.  :/
        if (i->ifa_addr->sa_family == AF_INET) {
            iface->address = CreateSDLNetAddrFromSockAddr(i->ifa_addr, sizeof (struct sockaddr_in));
            if (i->ifa_flags & IFF_BROADCAST) {
                iface->broadcast = CreateSDLNetAddrFromSockAddr(i->ifa_broadaddr, sizeof (struct sockaddr_in));
            }
        } else if (i->ifa_addr->sa_family == AF_INET6) {
            iface->address = CreateSDLNetAddrFromSockAddr(i->ifa_addr, sizeof (struct sockaddr_in6));
        } else {
            new_num_interfaces--;
            continue;
        }

        if (iface->address) {
            iface->index = (Uint32) if_nametoindex(i->ifa_name);
            iface->name = SDL_strdup(i->ifa_name);
            if (!iface->name) {
                goto failed;
            }

            char *ptr = SDL_strchr(iface->address->human_readable, '%');  // chop off interface name.
            if (ptr) {
                *ptr = '\0';
            }
        } else {
            NET_UnrefAddress(iface->broadcast);  // just in case.
            goto failed;
        }

        iface++;
    }

succeeded:
    if (ifaddr) {
        freeifaddrs(ifaddr);
    }

    SDL_LockRWLockForWriting(interface_rwlock);
    NetworkInterface *old_interfaces = interfaces;
    const int old_num_interfaces = num_interfaces;
    interfaces = new_interfaces;
    num_interfaces = new_num_interfaces;
    SDL_UnlockRWLock(interface_rwlock);
    FreeNetworkInterfaces(old_interfaces, old_num_interfaces);
    return;

failed:
    if (ifaddr) {
        freeifaddrs(ifaddr);
    }
    FreeNetworkInterfaces(new_interfaces, new_num_interfaces);
    // if this was a failure case, oh well, old (maybe incorrect) ones will have to stand.
#endif
}

#elif defined(SDL_PLATFORM_VITA)

static int interface_change_notifications_handle;
static SDL_Thread *interface_change_notifications_thread = NULL;
static SDL_AtomicInt interface_change_notifications_flag;

static int SDLCALL VitaInterfaceChangeNotificationThread(void *data)
{
    (void)data;

    while(SDL_GetAtomicInt(&interface_change_notifications_flag) == 0)
    {
        sceNetCtlCheckCallback();
        SDL_Delay(100);
    }
    return 0;
}

static void NetworkInterfaceChangedCallback(int event_type, void* arg)
{
    SDL_SetAtomicInt(&interfaces_have_changed, 1);
}

static bool InitInterfaceChangeNotifications(void)
{
    if (sceNetCtlInetRegisterCallback(NetworkInterfaceChangedCallback, NULL, &interface_change_notifications_handle) != 0)
        return false;

    interface_change_notifications_thread = SDL_CreateThread(VitaInterfaceChangeNotificationThread, "SDLNetIfaceEnum", NULL);
    return (interface_change_notifications_thread != NULL);
}

static void QuitInterfaceChangeNotifications(void)
{
    sceNetCtlInetUnregisterCallback(interface_change_notifications_handle);
    if (interface_change_notifications_thread) {
        SDL_SetAtomicInt(&interface_change_notifications_flag, 1);
        SDL_WaitThread(interface_change_notifications_thread, NULL);
        SDL_SetAtomicInt(&interface_change_notifications_flag, 0);
    }
    interface_change_notifications_thread = NULL;
}

static NET_Address *CreateSDLNetAddrFromIp(char* hostbuf)
{
    NET_Address *addr = (NET_Address *) SDL_calloc(1, sizeof (NET_Address));
    if (!addr) {
        return NULL;
    }
    SDL_SetAtomicInt(&addr->status, (int) NET_SUCCESS);

    struct addrinfo hints;
    SDL_zero(hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = 0;
    hints.ai_flags = AI_NUMERICHOST;

    int gairc = getaddrinfo(hostbuf, NULL, &hints, &addr->ainfo);
    if (gairc != 0) {
        SDL_free(addr);
        SetGetAddrInfoError("Failed to determine address", gairc);
        return NULL;
    }

    addr->human_readable = SDL_strdup(hostbuf);
    if (!addr->human_readable) {
        freeaddrinfo(addr->ainfo);
        SDL_free(addr);
        return NULL;
    }

    return NET_RefAddress(addr);
}


static void RefreshInterfaces(void)
{
    NetworkInterface *new_interfaces = NULL;
    int new_num_interfaces = 0;
    SceNetCtlInfo info;

    int state;
    if (sceNetCtlInetGetState(&state) < 0 ) {
        goto failed;
    }

    if (state != SCE_NETCTL_STATE_CONNECTED) {
        goto succeeded;
    }

    if (state == SCE_NETCTL_STATE_CONNECTED) {
        new_num_interfaces = 1;

        new_interfaces = SDL_calloc(new_num_interfaces, sizeof (*new_interfaces));
        if (!new_interfaces) {
            goto failed;
        }

        NetworkInterface *iface = &new_interfaces[0];

        if (sceNetCtlInetGetInfo(SCE_NETCTL_INFO_GET_IP_ADDRESS, &info) < 0) {
            goto failed;
        }


        iface->address = CreateSDLNetAddrFromIp(info.ip_address);

        if (iface->address) {
            iface->index = 0;

            if (sceNetCtlInetGetInfo(SCE_NETCTL_INFO_GET_DEVICE, &info) < 0) {
                goto failed;
            }

            switch (info.device) {
                case 0:
                    iface->name = SDL_strdup("wlan0");
                    break;
                case 1:
                    iface->name = SDL_strdup("eth0");
                    break;
                case 2:
                    iface->name = SDL_strdup("phone0");
                    break;
            }

            if (!iface->name) {
                goto failed;
            }

            char *ptr = SDL_strchr(iface->address->human_readable, '%');  // chop off interface name.
            if (ptr) {
                *ptr = '\0';
            }
        } else {
            NET_UnrefAddress(iface->broadcast);  // just in case.
            goto failed;
        }
    }

succeeded:
    SDL_LockRWLockForWriting(interface_rwlock);
    NetworkInterface *old_interfaces = interfaces;
    const int old_num_interfaces = num_interfaces;
    interfaces = new_interfaces;
    num_interfaces = new_num_interfaces;
    SDL_UnlockRWLock(interface_rwlock);
    FreeNetworkInterfaces(old_interfaces, old_num_interfaces);
    return;

failed:
    FreeNetworkInterfaces(new_interfaces, new_num_interfaces);
}

#else
#warning implement me for your platform.
static bool InitInterfaceChangeNotifications(void)
{
    return true;
}

static void QuitInterfaceChangeNotifications(void)
{
}

static void RefreshInterfaces(void)
{
}
#endif


static bool InterfacesReady(void)
{
    if (SDL_ShouldInit(&interface_init)) {
        if (!InitInterfaceChangeNotifications()) {
            SDL_SetInitialized(&interface_init, false);
            return false;
        }
        SDL_SetAtomicInt(&interfaces_have_changed, 1);  // force refresh at start.
        SDL_SetInitialized(&interface_init, true);
    }

    // if there were changes, mark it as unchanged and then we'll enumerate. So if it changes again while enumerating, we'll pick that up next time.
    if (SDL_CompareAndSwapAtomicInt(&interfaces_have_changed, 1, 0)) {
        RefreshInterfaces();
        #if 0
        SDL_Log("NETWORK INTERFACE REFRESH");
        SDL_LockRWLockForReading(interface_rwlock);
        for (int i = 0; i < num_interfaces; i++) {
            const NetworkInterface *iface = &interfaces[i];
            SDL_Log("Interface %u ('%s')", (unsigned int) iface->index, iface->name);
            SDL_Log("  - address %s",  iface->address->human_readable);
            if (iface->broadcast) {
                SDL_Log("  - broadcast %s", iface->broadcast->human_readable);
            }
        }
        SDL_UnlockRWLock(interface_rwlock);
        #endif
    }

    return true;
}


// this blocks!
static NET_Status ResolveAddress(NET_Address *addr)
{
    SDL_assert(addr != NULL);  // we control all this, so this shouldn't happen.
    struct addrinfo *ainfo = NULL;
    int rc;

    //SDL_Log("getaddrinfo '%s'", addr->hostname);
    rc = getaddrinfo(addr->hostname, NULL, NULL, &ainfo);
    //SDL_Log("rc=%d", rc);
    if (rc != 0) {
        addr->errstr = CreateGetAddrInfoErrorString(rc);
        return NET_FAILURE;  // error
    } else if (ainfo == NULL) {
        addr->errstr = SDL_strdup("Unknown error (query succeeded but result was NULL!)");
        return NET_FAILURE;
    }

    char buf[128];
    rc = getnameinfo(ainfo->ai_addr, (socklen_t) ainfo->ai_addrlen, buf, sizeof (buf), NULL, 0, NI_NUMERICHOST);
    if (rc != 0) {
        addr->errstr = CreateGetAddrInfoErrorString(rc);
        freeaddrinfo(ainfo);
        return NET_FAILURE;  // error
    }

    addr->human_readable = SDL_strdup(buf);
    addr->ainfo = ainfo;

    return NET_SUCCESS;  // success (zero means "still in progress").
}

static int SDLCALL ResolverThread(void *data)
{
    const int threadnum = (int) ((intptr_t) data);
    //SDL_Log("ResolverThread #%d starting up!", threadnum);

    SDL_LockMutex(resolver_lock);

    while (!SDL_GetAtomicInt(&resolver_shutdown)) {
        NET_Address *addr = SDL_GetAtomicPointer((void **) &resolver_queue);
        if (!addr) {
            if (SDL_GetAtomicInt(&resolver_num_threads) > MIN_RESOLVER_THREADS) {  // nothing pending and too many threads waiting in reserve? Quit.
                SDL_DetachThread(resolver_threads[threadnum]);  // detach ourselves so no one has to wait on us.
                SDL_SetAtomicPointer((void **) &resolver_threads[threadnum], NULL);
                break;  // we quit. They'll spawn new threads if necessary.
            }

            // Block until there's something to do.
            SDL_WaitCondition(resolver_condition, resolver_lock);  // surrenders the lock, sleeps until alerted, then relocks.
            continue;  // check for shutdown and new work again!
        }

        SDL_SetAtomicPointer((void **) &resolver_queue, addr->resolver_next);   // take this task off the list, then release the lock so others can work.
        SDL_UnlockMutex(resolver_lock);

        //SDL_Log("ResolverThread #%d got new task ('%s')", threadnum, addr->hostname);

        const int simulated_loss = SDL_GetAtomicInt(&resolver_percent_loss);

        if (ShouldSimulateLoss(simulated_loss)) {
            // won the percent_loss lottery? Delay resolving this address between 250 and 7000 milliseconds
            SDL_Delay(RandomNumberBetween(250, 2000 + (50 * simulated_loss)));
        }

        NET_Status outcome;
        if (ShouldSimulateLoss(simulated_loss)) {
            outcome = NET_FAILURE;
            addr->errstr = SDL_strdup("simulated failure");
        } else {
            outcome = ResolveAddress(addr);
        }

        SDL_SetAtomicInt(&addr->status, (int) outcome);
        //SDL_Log("ResolverThread #%d finished current task (%s, '%s' => '%s')", threadnum, (outcome == NET_FAILURE) ? "failure" : "success", addr->hostname, (outcome < 0) ? addr->errstr : addr->human_readable);

        NET_UnrefAddress(addr);  // we're done with it, but others might still own it.

        SDL_AddAtomicInt(&resolver_num_requests, -1);

        // okay, we're done with this task, grab the lock so we can see what's next.
        SDL_LockMutex(resolver_lock);
        SDL_BroadcastCondition(resolver_condition);  // wake up anything waiting on results, and also give all resolver threads a chance to see if they are still needed.
    }

    SDL_AddAtomicInt(&resolver_num_threads, -1);
    SDL_UnlockMutex(resolver_lock);  // we're quitting, let go of the lock.

    //SDL_Log("ResolverThread #%d ending!", threadnum);
    return 0;
}

static SDL_Thread *SpinResolverThread(const int num)
{
    char name[16];
    SDL_snprintf(name, sizeof (name), "SDLNetRslv%d", num);
    SDL_assert(resolver_threads[num] == NULL);
    SDL_AddAtomicInt(&resolver_num_threads, 1);
    const SDL_PropertiesID props = SDL_CreateProperties();
    SDL_SetPointerProperty(props, SDL_PROP_THREAD_CREATE_ENTRY_FUNCTION_POINTER, (void *) ResolverThread);
    SDL_SetStringProperty(props, SDL_PROP_THREAD_CREATE_NAME_STRING, name);
    SDL_SetPointerProperty(props, SDL_PROP_THREAD_CREATE_USERDATA_POINTER, (void *) ((intptr_t) num));
    SDL_SetNumberProperty(props, SDL_PROP_THREAD_CREATE_STACKSIZE_NUMBER, 64 * 1024);
    resolver_threads[num] = SDL_CreateThreadWithProperties(props);
    SDL_DestroyProperties(props);
    if (!resolver_threads[num]) {
        SDL_AddAtomicInt(&resolver_num_threads, -1);
    }
    return resolver_threads[num];
}

static void DestroyAddress(NET_Address *addr)
{
    if (addr) {
        if (addr->ainfo) {
            freeaddrinfo(addr->ainfo);
        }
        SDL_free(addr->hostname);
        SDL_free(addr->human_readable);
        SDL_free(addr->errstr);
        SDL_free(addr);
    }
}

static NET_Address *CreateSDLNetAddrFromSockAddr(const struct sockaddr *saddr, SockLen saddrlen)
{
    // !!! FIXME: this all seems inefficient in the name of keeping addresses generic.
    char hostbuf[128];
    int gairc = getnameinfo(saddr, saddrlen, hostbuf, sizeof (hostbuf), NULL, 0, NI_NUMERICHOST);
    if (gairc != 0) {
        SetGetAddrInfoError("Failed to determine address", gairc);
        return NULL;
    }

    NET_Address *addr = (NET_Address *) SDL_calloc(1, sizeof (NET_Address));
    if (!addr) {
        return NULL;
    }
    SDL_SetAtomicInt(&addr->status, (int) NET_SUCCESS);

    struct addrinfo hints;
    SDL_zero(hints);
    hints.ai_family = saddr->sa_family;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = 0;
    hints.ai_flags = AI_NUMERICHOST;

    gairc = getaddrinfo(hostbuf, NULL, &hints, &addr->ainfo);
    if (gairc != 0) {
        SDL_free(addr);
        SetGetAddrInfoError("Failed to determine address", gairc);
        return NULL;
    }

    addr->human_readable = SDL_strdup(hostbuf);
    if (!addr->human_readable) {
        freeaddrinfo(addr->ainfo);
        SDL_free(addr);
        return NULL;
    }

    return NET_RefAddress(addr);
}

static SDL_AtomicInt initialize_count;

bool NET_Init(void)
{
    if (SDL_AddAtomicInt(&initialize_count, 1) > 0) {
        return true;  // already initialized, call it a success.
    }

    char *origerrstr = NULL;

    #ifdef SDL_PLATFORM_WINDOWS
    WSADATA data;
    if (WSAStartup(MAKEWORD(1, 1), &data) != 0) {
        return SetSocketErrorBool("WSAStartup() failed", LastSocketError());
    }
    #elif defined(SDL_PLATFORM_VITA)
    // trigger vita network stack startup
    gethostname(NULL,0);
    #else
    signal(SIGPIPE, SIG_IGN);
    #endif

    SDL_zeroa(resolver_threads);
    SDL_SetAtomicInt(&resolver_shutdown, 0);
    SDL_SetAtomicInt(&resolver_num_threads, 0);
    SDL_SetAtomicInt(&resolver_num_requests, 0);
    SDL_SetAtomicInt(&resolver_percent_loss, 0);
    resolver_queue = NULL;

    resolver_lock = SDL_CreateMutex();
    if (!resolver_lock) {
        goto failed;
    }

    resolver_condition = SDL_CreateCondition();
    if (!resolver_condition) {
        goto failed;
    }

    for (int i = 0; i < MIN_RESOLVER_THREADS; i++) {
        if (!SpinResolverThread(i)) {
            goto failed;
        }
    }

    struct sockaddr_in6 sa_in6;
    SDL_zero(sa_in6);
    if (inet_pton(AF_INET6, "ff02::1", &sa_in6.sin6_addr) == 1) {
        sa_in6.sin6_family = AF_INET6;
        ipv6_broadcast_addr = CreateSDLNetAddrFromSockAddr((const struct sockaddr *) &sa_in6, sizeof (sa_in6));
    }

    interface_rwlock = SDL_CreateRWLock();
    if (!interface_rwlock) {
        goto failed;
    }

    return true;  // good to go.

failed:
    origerrstr = SDL_strdup(SDL_GetError());

    NET_Quit();

    if (origerrstr) {
        SDL_SetError("%s", origerrstr);
        SDL_free(origerrstr);
    }

    return false;
}

void NET_Quit(void)
{
    const int prevcount = SDL_AddAtomicInt(&initialize_count, -1);
    if (prevcount <= 0) {
        SDL_AddAtomicInt(&initialize_count, 1);  // bump back up.
        return;  // we weren't initialized!
    } else if (prevcount > 1) {
        return;  // need to quit more, to match previous init calls.
    }

    if (resolver_lock && resolver_condition) {
        SDL_LockMutex(resolver_lock);
        SDL_SetAtomicInt(&resolver_shutdown, 1);
        for (int i = 0; i < ((int) SDL_arraysize(resolver_threads)); i++) {
            if (resolver_threads[i]) {
                SDL_BroadcastCondition(resolver_condition);
                SDL_UnlockMutex(resolver_lock);
                SDL_WaitThread(resolver_threads[i], NULL);
                SDL_LockMutex(resolver_lock);
                resolver_threads[i] = NULL;
            }
        }
        SDL_UnlockMutex(resolver_lock);
    }

    SDL_SetAtomicInt(&resolver_shutdown, 0);
    SDL_SetAtomicInt(&resolver_num_threads, 0);
    SDL_SetAtomicInt(&resolver_num_requests, 0);
    SDL_SetAtomicInt(&resolver_percent_loss, 0);

    if (resolver_condition) {
        SDL_DestroyCondition(resolver_condition);
        resolver_condition = NULL;
    }

    if (resolver_lock) {
        SDL_DestroyMutex(resolver_lock);
        resolver_lock = NULL;
    }

    resolver_queue = NULL;

    if (SDL_ShouldQuit(&interface_init)) {
        QuitInterfaceChangeNotifications();
        SDL_SetAtomicInt(&interfaces_have_changed, 0);
        if (interfaces) {
            FreeNetworkInterfaces(interfaces, num_interfaces);
        }
        interfaces = NULL;
        num_interfaces = 0;
        SDL_SetInitialized(&interface_init, false);
    }

    // asserts to catch that these shouldn't have ever been set if we never initialized interface_init...
    SDL_assert(!interfaces);
    SDL_assert(num_interfaces == 0);

    if (interface_rwlock) {
        SDL_DestroyRWLock(interface_rwlock);
        interface_rwlock = NULL;
    }

    NET_UnrefAddress(ipv6_broadcast_addr);
    ipv6_broadcast_addr = NULL;

    #ifdef SDL_PLATFORM_WINDOWS
    WSACleanup();
    #endif
}

static void trim_whitespace(char *str)
{
    char *ptr;
    for (ptr = str; *ptr; ptr++) {
        if (!SDL_isspace(*ptr)) {
            break;
        }
    }

    if (ptr > str) {
        SDL_memmove(str, ptr, SDL_strlen(ptr) + 1);
    }

    for (ptr = str + (SDL_strlen(str) - 1); ptr >= str; ptr--) {
        if (!SDL_isspace(*ptr)) {
            break;
        }
        *ptr = '\0';
    }
}

NET_Address *NET_ResolveHostname(const char *host)
{
    // If this isn't true, we'll spin up resolver threads without locking that will be orphaned in NET_Init()
    SDL_assert(SDL_GetAtomicInt(&initialize_count) > 0);

    NET_Address *addr = SDL_calloc(1, sizeof (NET_Address));
    if (!addr) {
        return NULL;
    }

    addr->hostname = SDL_strdup(host);
    if (!addr->hostname) {
        SDL_free(addr);
        return NULL;
    }

    // remove whitespace around name, just in case. https://github.com/libsdl-org/SDL_net/issues/148
    trim_whitespace(addr->hostname);

    SDL_SetAtomicInt(&addr->refcount, 2);  // one for creation, one for the resolver thread to unref when done.

    SDL_LockMutex(resolver_lock);

    // !!! FIXME: this should append to the list, not prepend; as is, new requests will make existing pending requests take longer to start processing.
    SDL_SetAtomicPointer((void **) &addr->resolver_next, SDL_GetAtomicPointer((void **) &resolver_queue));
    SDL_SetAtomicPointer((void **) &resolver_queue, addr);

    const int num_threads = SDL_GetAtomicInt(&resolver_num_threads);
    const int num_requests = SDL_AddAtomicInt(&resolver_num_requests, 1) + 1;
    //SDL_Log("num_threads=%d, num_requests=%d", num_threads, num_requests);
    if ((num_requests >= num_threads) && (num_threads < MAX_RESOLVER_THREADS)) {  // all threads are busy? Maybe spawn a new one.
        // if this didn't actually spin one up, it is what it is...the existing threads will eventually get there.
        for (int i = 0; i < ((int) SDL_arraysize(resolver_threads)); i++) {
            if (!resolver_threads[i]) {
                SpinResolverThread(i);
                break;
            }
        }
    }

    SDL_SignalCondition(resolver_condition);
    SDL_UnlockMutex(resolver_lock);

    return addr;
}

NET_Status NET_WaitUntilResolved(NET_Address *addr, Sint32 timeout)
{
    if (!addr) {
        return (NET_Status) SDL_InvalidParamError("address");  // obviously nothing to wait for.
    }

    // we _could_ use a different lock for this, but this is Good Enough.

    if (timeout) {
        SDL_LockMutex(resolver_lock);
        if (timeout < 0) {
            while (((NET_Status) SDL_GetAtomicInt(&addr->status)) == NET_WAITING) {
                SDL_WaitCondition(resolver_condition, resolver_lock);
            }
        } else {
            const Uint64 endtime = (SDL_GetTicks() + timeout);
            while (((NET_Status) SDL_GetAtomicInt(&addr->status)) == NET_WAITING) {
                const Uint64 now = SDL_GetTicks();
                if (now >= endtime) {
                    break;
                }
                SDL_WaitConditionTimeout(resolver_condition, resolver_lock, (Sint32) (endtime - now));
            }
        }
        SDL_UnlockMutex(resolver_lock);
    }

    return NET_GetAddressStatus(addr);  // so we set the error string if necessary.
}

NET_Status NET_GetAddressStatus(NET_Address *addr)
{
    if (!addr) {
        return (NET_Status) SDL_InvalidParamError("address");
    }
    const NET_Status retval = (NET_Status) SDL_GetAtomicInt(&addr->status);
    if (retval == NET_FAILURE) {
        SDL_SetError("%s", (const char *) SDL_GetAtomicPointer((void **) &addr->errstr));
    }
    return retval;
}

const char *NET_GetAddressString(NET_Address *addr)
{
    if (!addr) {
        SDL_InvalidParamError("address");
        return NULL;
    }

    const char *retval = (const char *) SDL_GetAtomicPointer((void **) &addr->human_readable);
    if (!retval) {
        const NET_Status rc = NET_GetAddressStatus(addr);
        if (rc != NET_FAILURE) {  // if NET_FAILURE, it'll set the error message.
            SDL_SetError("Address not yet resolved");  // if this resolved in a race condition, too bad, try again.
        }
    }
    return retval;
}

const void * NET_GetAddressBytes(NET_Address *addr, int *num_bytes)
{
    if (!num_bytes) {
        SDL_InvalidParamError("num_bytes");
        return NULL;
    }

    *num_bytes = 0;

    if (!addr) {
        SDL_InvalidParamError("address");
        return NULL;
    }

    const void *retval = NULL;
    const NET_Status rc = NET_GetAddressStatus(addr);
    if (rc == NET_SUCCESS) {
        const struct addrinfo *ainfo = addr->ainfo;
        SDL_assert(ainfo != NULL);
        retval = (const void *) ainfo->ai_addr;
        *num_bytes = (int) ainfo->ai_addrlen;
    } else if (rc != NET_FAILURE) {  // if NET_FAILURE, it'll set the error message.
        SDL_SetError("Address not yet resolved");
    }

    return retval;
}

int NET_CompareAddresses(const NET_Address *sdlneta, const NET_Address *sdlnetb)
{
    if (sdlneta == sdlnetb) {  // same pointer?
        return 0;
    } else if (sdlneta && !sdlnetb) {
        return -1;
    } else if (!sdlneta && sdlnetb) {
        return 1;
    }

    const struct addrinfo *a = sdlneta->ainfo;
    const struct addrinfo *b = sdlnetb->ainfo;
    if (a == b) {  // same pointer?
        return 0;
    } else if (a && !b) {
        return -1;
    } else if (!a && b) {
        return 1;
    } else if (a->ai_family < b->ai_family) {
        return -1;
    } else if (a->ai_family > b->ai_family) {
        return 1;
    } else if (a->ai_addrlen < b->ai_addrlen) {
        return -1;
    } else if (a->ai_addrlen > b->ai_addrlen) {
        return 1;
    }

    return SDL_memcmp(a->ai_addr, b->ai_addr, a->ai_addrlen);
}

NET_Address *NET_RefAddress(NET_Address *addr)
{
    if (addr) {
        SDL_AtomicIncRef(&addr->refcount);
    }
    return addr;
}

void NET_UnrefAddress(NET_Address *addr)
{
    if (addr && SDL_AtomicDecRef(&addr->refcount)) {
        DestroyAddress(addr);
    }
}

void NET_SimulateAddressResolutionLoss(int percent_loss)
{
    SDL_SetAtomicInt(&resolver_percent_loss, SDL_clamp(percent_loss, 0, 100));
}

NET_Address **NET_GetLocalAddresses(int *num_addresses)
{
    int dummy_addresses;
    if (!num_addresses) {
        num_addresses = &dummy_addresses;
    }

    *num_addresses = 0;

    NET_Address **retval = NULL;

    if (InterfacesReady()) {
        SDL_LockRWLockForReading(interface_rwlock);
        const int total = num_interfaces;
        retval = (NET_Address **) SDL_malloc((total + 1) * sizeof (NET_Address *));
        if (retval) {
            for (int i = 0; i < total; i++) {
                retval[i] = NET_RefAddress(interfaces[i].address);
            }
            retval[total] = NULL;
            *num_addresses = total;
        }
        SDL_UnlockRWLock(interface_rwlock);
    }

    return retval;
}

void NET_FreeLocalAddresses(NET_Address **addresses)
{
    if (addresses) {
        for (int i = 0; addresses[i] != NULL; i++) {
            NET_UnrefAddress(addresses[i]);
        }
        SDL_free(addresses);
    }
}

static struct addrinfo *MakeAddrInfoWithPort(const NET_Address *addr, const int socktype, const Uint16 port)
{
    const struct addrinfo *ainfo = addr ? addr->ainfo : NULL;
    SDL_assert(!addr || ainfo);

    // we need to set up a sockaddr with the port in it for connect(), etc, which is kind of a pain, since we
    // want to keep things generic and also not set up a port at resolve time.
    struct addrinfo hints;
    SDL_zero(hints);
    hints.ai_family = ainfo ? ainfo->ai_family : AF_UNSPEC;
    hints.ai_socktype = socktype;
    //hints.ai_protocol = ainfo ? ainfo->ai_protocol : 0;
    hints.ai_flags = AI_NUMERICHOST | AI_NUMERICSERV | (!ainfo ? AI_PASSIVE : 0);

    char service[16];
    SDL_snprintf(service, sizeof (service), "%d", (int) port);

    struct addrinfo *addrwithport = NULL;
    int rc = getaddrinfo(addr ? addr->human_readable : NULL, service, &hints, &addrwithport);
    if (rc != 0) {
        char *errstr = CreateGetAddrInfoErrorString(rc);
        SDL_SetError("Failed to prepare address with port: %s", errstr);
        SDL_free(errstr);
        return NULL;
    }

    return addrwithport;
}


struct NET_StreamSocket
{
    NET_SocketType socktype;
    NET_Address *addr;
    Uint16 port;
    Socket handle;
    NET_Status status;
    Uint8 *pending_output_buffer;
    int pending_output_len;
    int pending_output_allocation;
    int percent_loss;
    Uint64 simulated_failure_until;
};

NET_StreamSocket *NET_CreateClient(NET_Address *addr, Uint16 port, SDL_PropertiesID props)
{
    if (addr == NULL) {
        SDL_InvalidParamError("address");
        return NULL;
    } else if (((NET_Status) SDL_GetAtomicInt(&addr->status)) != NET_SUCCESS) {
        SDL_SetError("Address is not resolved");
        return NULL;
    }

    NET_StreamSocket *sock = (NET_StreamSocket *) SDL_calloc(1, sizeof (NET_StreamSocket));
    if (!sock) {
        return NULL;
    }

    sock->socktype = SOCKETTYPE_STREAM;
    sock->addr = addr;
    sock->port = port;

    // we need to set up a sockaddr with the port in it for connect(), which is kind of a pain, since we
    // want to keep things generic and also not set up a port at resolve time.
    struct addrinfo *addrwithport = MakeAddrInfoWithPort(addr, SOCK_STREAM, port);
    if (!addrwithport) {
        SDL_free(sock);
        return NULL;
    }

    sock->handle = socket(addrwithport->ai_family, addrwithport->ai_socktype, addrwithport->ai_protocol);
    if (sock->handle == INVALID_SOCKET) {
        SetLastSocketError("Failed to create socket");
        freeaddrinfo(addrwithport);
        SDL_free(sock);
        return NULL;
    }

    if (MakeSocketNonblocking(sock->handle) < 0) {
        CloseSocketHandle(sock->handle);
        freeaddrinfo(addrwithport);
        SDL_free(sock);
        SDL_SetError("Failed to make new socket non-blocking");
        return NULL;
    }

    const int rc = connect(sock->handle, addrwithport->ai_addr, (SockLen) addrwithport->ai_addrlen);

    freeaddrinfo(addrwithport);

    if (rc == SOCKET_ERROR) {
        const int err = LastSocketError();
        if (!WouldBlock(err)) {
            SetSocketError("Connection failed at startup", err);
            CloseSocketHandle(sock->handle);
            SDL_free(sock);
            return NULL;
        }
    }

    NET_RefAddress(addr);
    return sock;
}

static NET_Status CheckClientConnection(NET_StreamSocket *sock, int timeoutms)
{
    if (!sock) {
        return SDL_InvalidParamError("sock");
    } else if (sock->status == NET_WAITING) {  // still pending?
        /*!!! FIXME: add this later?
        if (sock->simulated_failure_ticks) {
            if (SDL_GetTicks() >= sock->simulated_failure_ticks) {
                sock->status = (NET_Status) SDL_SetError("simulated failure");
        } else */
        if (NET_WaitUntilInputAvailable((void **) &sock, 1, timeoutms) == NET_FAILURE) {
            sock->status = NET_FAILURE;  // just abandon the whole enterprise.
        }
    }
    return sock->status;
}

NET_Status NET_WaitUntilConnected(NET_StreamSocket *sock, Sint32 timeout)
{
    return CheckClientConnection(sock, (int) timeout);
}

NET_Status NET_GetConnectionStatus(NET_StreamSocket *sock)
{
    return CheckClientConnection(sock, 0);
}


struct NET_Server
{
    NET_SocketType socktype;
    NET_Address *addr;  // bound to this address (NULL for any).
    Uint16 port;
    int num_handles;   // for INADDR_ANY things, one handle per network family.
    Socket *handles;
    Socket handle_pool[4];
};

NET_Server *NET_CreateServer(NET_Address *addr, Uint16 port, SDL_PropertiesID props)
{
    if (addr && (((NET_Status) SDL_GetAtomicInt(&addr->status)) != NET_SUCCESS)) {
        SDL_SetError("Address is not resolved");  // strictly speaking, this should be a local interface, but a resolved address can fail later.
        return NULL;
    }

    struct addrinfo *addrwithport = MakeAddrInfoWithPort(addr, SOCK_STREAM, port);
    if (!addrwithport) {
        return NULL;
    }

    NET_Server *server = (NET_Server *) SDL_calloc(1, sizeof (NET_Server));
    if (!server) {
        freeaddrinfo(addrwithport);
        return NULL;
    }

    server->socktype = SOCKETTYPE_SERVER;
    server->addr = addr;
    server->port = port;

    int num_handles = 0;
    Socket *allocated_handles = NULL;
    if (addr != NULL) {
        num_handles = 1;
        server->handles = server->handle_pool;
    } else {  // bind to all interfaces.
        for (struct addrinfo *i = addrwithport; i != NULL; i = i->ai_next) {
            num_handles++;
        }
        if (num_handles <= (int) SDL_arraysize(server->handle_pool)) {
            server->handles = server->handle_pool;
        } else {
            allocated_handles = (Socket *) SDL_calloc(num_handles, sizeof (*server->handles));
            if (!allocated_handles) {
                SDL_free(server);
                freeaddrinfo(addrwithport);
                return NULL;
            }
            server->handles = allocated_handles;
        }
    }

    const int reuseaddr = SDL_GetBooleanProperty(props, NET_PROP_SERVER_REUSEADDR_BOOLEAN, true) ? 1 : 0;

    // Make sockets for all desired interfaces; if addr!=NULL, this is one socket on one interface,
    //  but if addr==NULL, it might be multiple sockets for IPv4, IPv6, etc, bound to their INADDR_ANY equivalent.
    struct addrinfo *ainfo = addrwithport;
    for (int i = 0; i < num_handles; i++, ainfo = ainfo->ai_next) {
        SDL_assert(ainfo != NULL);
        Socket handle = socket(ainfo->ai_family, ainfo->ai_socktype, ainfo->ai_protocol);
        if (handle == INVALID_SOCKET) {
            SetLastSocketError("Failed to create listen socket");
            goto failed;
        }

        server->handles[server->num_handles++] = handle;

        if (MakeSocketNonblocking(handle) < 0) {
            SDL_SetError("Failed to make new listen socket non-blocking");
            goto failed;
        }

        const int one = 1;
        if (ainfo->ai_family == AF_INET6) {
            setsockopt(handle, IPPROTO_IPV6, IPV6_V6ONLY, (const char *) &one, sizeof (one));  // if this fails, oh well.
        }

        setsockopt(handle, SOL_SOCKET, SO_REUSEADDR, (const char *) &reuseaddr, sizeof (reuseaddr));

        int rc = bind(handle, ainfo->ai_addr, (SockLen) ainfo->ai_addrlen);
        if (rc == SOCKET_ERROR) {
            const int err = LastSocketError();
            SDL_assert(!WouldBlock(err));  // binding shouldn't be a blocking operation.
            SetSocketError("Failed to bind listen socket", err);
            goto failed;
        }

        rc = listen(handle, 16);
        if (rc == SOCKET_ERROR) {
            const int err = LastSocketError();
            SDL_assert(!WouldBlock(err));  // listen shouldn't be a blocking operation.
            SetSocketError("Failed to listen on socket", err);
            goto failed;
        }
    }

    freeaddrinfo(addrwithport);

    NET_RefAddress(addr);
    return server;

failed:
    for (int i = 0; i < server->num_handles; i++) {
        CloseSocketHandle(server->handles[i]);
    }
    freeaddrinfo(addrwithport);
    SDL_free(allocated_handles);
    SDL_free(server);
    return NULL;
}

bool NET_AcceptClient(NET_Server *server, NET_StreamSocket **client_stream)
{
    if (!client_stream) {
        return SDL_InvalidParamError("client_stream");
    }

    *client_stream = NULL;

    if (!server) {
        return SDL_InvalidParamError("server");
    }

    for (int i = 0; i < server->num_handles; i++) {
        AddressStorage from;
        SockLen fromlen = sizeof (from);
        const Socket handle = accept(server->handles[i], (struct sockaddr *) &from, &fromlen);
        if (handle == INVALID_SOCKET) {
            const int err = LastSocketError();
            if (WouldBlock(err)) {
                continue;
            }
            return SetSocketErrorBool("Failed to accept new connection", err);
        }

        if (MakeSocketNonblocking(handle) < 0) {
            CloseSocketHandle(handle);
            return SDL_SetError("Failed to make incoming socket non-blocking");
        }

        char portbuf[16];
        const int gairc = getnameinfo((struct sockaddr *) &from, fromlen, NULL, 0, portbuf, sizeof (portbuf), NI_NUMERICSERV);
        if (gairc != 0) {
            CloseSocketHandle(handle);
            return SetGetAddrInfoErrorBool("Failed to determine port number", gairc);
        }

        NET_Address *fromaddr = CreateSDLNetAddrFromSockAddr((struct sockaddr *) &from, fromlen);
        if (!fromaddr) {
            CloseSocketHandle(handle);
            return false;  // error string was already set.
        }

        NET_StreamSocket *sock = (NET_StreamSocket *) SDL_calloc(1, sizeof (NET_StreamSocket));
        if (!sock) {
            NET_UnrefAddress(fromaddr);
            CloseSocketHandle(handle);
            return false;
        }

        sock->socktype = SOCKETTYPE_STREAM;
        sock->addr = fromaddr;
        sock->port = (Uint16) SDL_atoi(portbuf);
        sock->handle = handle;
        sock->status = NET_SUCCESS;  // connected

        *client_stream = sock;
        return true;  // we got one!
    }

    return true;  // nothing new.
}

void NET_DestroyServer(NET_Server *server)
{
    if (server) {
        for (int i = 0; i < server->num_handles; i++) {
            if (server->handles[i] != INVALID_SOCKET) {
                CloseSocketHandle(server->handles[i]);
            }
        }
        if (server->handles != server->handle_pool) {
            SDL_free(server->handles);
        }
        NET_UnrefAddress(server->addr);
        SDL_free(server);
    }
}

NET_Address *NET_GetStreamSocketAddress(NET_StreamSocket *sock)
{
    if (!sock) {
        SDL_InvalidParamError("sock");
        return NULL;
    }
    return NET_RefAddress(sock->addr);
}

static void UpdateStreamSocketSimulatedFailure(NET_StreamSocket *sock)
{
    if (ShouldSimulateLoss(sock->percent_loss)) {
        // won the percent_loss lottery? Refuse to move more data for between 250 and 7000 milliseconds.
        sock->simulated_failure_until = SDL_GetTicks() + (Uint64) (RandomNumberBetween(250, 2000 + (50 * sock->percent_loss)));
    } else {
        sock->simulated_failure_until = 0;
    }
}

// see if any pending data can finally be sent, etc
static bool PumpStreamSocket(NET_StreamSocket *sock)
{
    if (!sock) {
        return SDL_InvalidParamError("sock");
    } else if (sock->pending_output_len > 0) {
        // !!! FIXME: there should be some small chance of streams dropping connection to simulate failure.
        if (sock->simulated_failure_until && (SDL_GetTicks() < sock->simulated_failure_until)) {
            return true;  // streams are reliable, so instead of packet loss, we introduce lag.
        }

        const int bw = (int) write(sock->handle, sock->pending_output_buffer, sock->pending_output_len);
        if (bw < 0) {
            const int err = LastSocketError();
            return WouldBlock(err) ? true : SetSocketErrorBool("Failed to write to socket", err);
        } else if (bw < sock->pending_output_len) {
            SDL_memmove(sock->pending_output_buffer, sock->pending_output_buffer + bw, ((size_t) sock->pending_output_len) - bw);
        }
        sock->pending_output_len -= bw;

        UpdateStreamSocketSimulatedFailure(sock);
    }

    return true;
}

bool NET_WriteToStreamSocket(NET_StreamSocket *sock, const void *buf, int buflen)
{
    if (!PumpStreamSocket(sock)) {  // try to flush any queued data to the socket now, before we handle more.
        return false;
    } else if (buf == NULL) {
        return SDL_InvalidParamError("buf");
    } else if (buflen < 0) {
        return SDL_InvalidParamError("buflen");
    } else if (buflen == 0) {
        return true;  // nothing to do.
    }

    if (sock->pending_output_len == 0) {  // nothing queued? See if we can just send this without queueing.
        // don't ever try to send directly if simulating packet loss; we'll always queue and mess with it then.
        if (sock->percent_loss == 0) {
            const int bw = (int) write(sock->handle, buf, buflen);
            if (bw < 0) {
                const int err = LastSocketError();
                if (!WouldBlock(err)) {
                    return SetSocketErrorBool("Failed to write to socket", err);
                }
            } else if (bw == buflen) {  // sent the whole thing? We're good to go here.
                return true;
            } else /*if (bw < buflen)*/ {  // partial write? We'll queue the rest.
                buf = ((const Uint8 *) buf) + bw;
                buflen -= (int) bw;
            }
        }
    }

    // queue this up for sending later.
    const int min_alloc = sock->pending_output_len + buflen;
    if (min_alloc > sock->pending_output_allocation) {
        int newlen = SDL_max(1, sock->pending_output_allocation);
        while (newlen < min_alloc) {
            newlen *= 2;
            if (newlen < 0) {  // uhoh, overflowed! That's a lot of memory!!
                return SDL_OutOfMemory();
            }
        }
        void *ptr = SDL_realloc(sock->pending_output_buffer, newlen);
        if (!ptr) {
            return false;
        }
        sock->pending_output_buffer = (Uint8 *) ptr;
        sock->pending_output_allocation = newlen;
    }

    SDL_memcpy(sock->pending_output_buffer + sock->pending_output_len, buf, buflen);
    sock->pending_output_len += buflen;

    return true;
}

int NET_GetStreamSocketPendingWrites(NET_StreamSocket *sock)
{
    if (!PumpStreamSocket(sock)) {
        return -1;
    }
    return sock->pending_output_len;
}

int NET_WaitUntilStreamSocketDrained(NET_StreamSocket *sock, int timeoutms)
{
    if (!sock) {
        SDL_InvalidParamError("sock");
        return -1;
    }

    if (timeoutms != 0) {
        const Uint64 endtime = (timeoutms > 0) ? (SDL_GetTicks() + timeoutms) : 0;
        while (NET_GetStreamSocketPendingWrites(sock) > 0) {
            struct pollfd pfd;
            SDL_zero(pfd);
            pfd.fd = sock->handle;
            pfd.events = POLLOUT;
            const int rc = poll(&pfd, 1, timeoutms);
            if (rc == SOCKET_ERROR) {
                return SetLastSocketError("Socket poll failed");
            } else if (rc == 0) {
                break;  // timed out
            }

            if (timeoutms > 0) {   // We must have woken up for a pending write, etc. Figure out remaining wait time.
                const Uint64 now = SDL_GetTicks();
                if (now < endtime) {
                    timeoutms = (int) (endtime - now);
                } else {
                    break;  // time has expired, break out.
                }
            } // else timeout is meant to be infinite, but we woke up for a write, etc, so go back to an infinite poll until we fail or buffer is drained.
        }
    }

    return NET_GetStreamSocketPendingWrites(sock);
}

int NET_ReadFromStreamSocket(NET_StreamSocket *sock, void *buf, int buflen)
{
    if (!PumpStreamSocket(sock)) {  // try to flush any queued data to the socket now, before we go further.
        return -1;
    } else if (sock->simulated_failure_until && (SDL_GetTicks() < sock->simulated_failure_until)) {
        return 0;  // streams are reliable, so instead of packet loss, we introduce lag.
    }

    if (buf == NULL) {
        return SDL_InvalidParamError("buf");
    } else if (buflen < 0) {
        return SDL_InvalidParamError("buflen");
    } else if (buflen == 0) {
        return 0;  // nothing to do.
    }

    const int br = (int) read(sock->handle, buf, buflen);
    if (br == 0) {
        SDL_SetError("End of stream");
        return -1;
    } else if (br < 0) {
        const int err = LastSocketError();
        return WouldBlock(err) ? 0 : SetSocketError("Failed to read from socket", err);
    }

    UpdateStreamSocketSimulatedFailure(sock);

    return br;
}

void NET_SimulateStreamPacketLoss(NET_StreamSocket *sock, int percent_loss)
{
    if (!sock) {
        return;
    }

    PumpStreamSocket(sock);

    sock->percent_loss = SDL_clamp(percent_loss, 0, 100);

    UpdateStreamSocketSimulatedFailure(sock);
}

// !!! FIXME: docs should note that this will THROW AWAY pending writes in _our_ buffers (not the kernel-level buffers) if you didn't wait for them to finish.
void NET_DestroyStreamSocket(NET_StreamSocket *sock)
{
    if (sock) {
        PumpStreamSocket(sock);  // try one last time to send any last pending data.

        NET_UnrefAddress(sock->addr);
        if (sock->handle != INVALID_SOCKET) {
            CloseSocketHandle(sock->handle);  // !!! FIXME: what does this do with non-blocking sockets? Release the descriptor but the kernel continues sending queued buffers behind the scenes?
        }
        SDL_free(sock->pending_output_buffer);
        SDL_free(sock);
    }
}

typedef struct NET_DatagramSocketHandle
{
    Socket handle;
    int family;
    int protocol;
    NET_Address *broadcast;
} NET_DatagramSocketHandle;

struct NET_DatagramSocket
{
    NET_SocketType socktype;
    NET_Address *addr;  // bound to this address (NULL for any).
    Uint16 port;
    int percent_loss;
    Uint8 recv_buffer[64*1024];
    NET_Address *latest_recv_addrs[64];
    int latest_recv_addrs_idx;
    int num_handles;   // for INADDR_ANY things, one handle (etc) per network family.
    NET_DatagramSocketHandle *handles;
    NET_DatagramSocketHandle handle_pool[4];
    NET_Datagram **pending_output;
    int pending_output_len;
    int pending_output_allocation;
    bool allow_broadcast;
};

static NET_Address *FindBroadcastAddress(struct addrinfo *ainfo, Uint32 *interface_index)
{
    NET_Address *retval = NULL;

    NET_Address *iface = CreateSDLNetAddrFromSockAddr(ainfo->ai_addr, (SockLen) ainfo->ai_addrlen);
    if (!iface) {
        return NULL;
    }

    NetworkInterface *ni = NULL;
    if (!InterfacesReady()) {
        NET_UnrefAddress(iface);
        return NULL;
    }

    SDL_LockRWLockForReading(interface_rwlock);

    for (int i = 0; i < num_interfaces; i++) {
        if (NET_CompareAddresses(iface, interfaces[i].address) == 0) {
            ni = &interfaces[i];
            break;
        }
    }

    if (!ni) {
        SDL_SetError("Not a network interface address");
    } else {
        SDL_assert(ni->address != NULL);
        *interface_index = ni->index;
        if (ni->broadcast != NULL) {
            retval = NET_RefAddress(ni->broadcast);  // we calculated the broadcast address when discovering this interface. It's probably IPv4. We're good to go.
        } else if (ainfo->ai_family == AF_INET6) {   // we fake this for IPv6 on the all-nodes link-local multicast group.
            retval = NET_RefAddress(ipv6_broadcast_addr);
        } else {
            SDL_SetError("Can't determine broadcast address for this interface");
        }
    }

    SDL_UnlockRWLock(interface_rwlock);

    if (!retval && (ainfo->ai_family == AF_INET6)) {   // we fake this for IPv6 on the all-nodes link-local multicast group.
        retval = NET_RefAddress(ipv6_broadcast_addr);
    }

    return retval;
}

NET_DatagramSocket *NET_CreateDatagramSocket(NET_Address *addr, Uint16 port, SDL_PropertiesID props)
{
    if (addr && (((NET_Status) SDL_GetAtomicInt(&addr->status)) != NET_SUCCESS)) {
        SDL_SetError("Address is not resolved");  // strictly speaking, this should be a local interface, but a resolved address can fail later.
        return NULL;
    }

    struct addrinfo *addrwithport = MakeAddrInfoWithPort(addr, SOCK_DGRAM, port);
    if (!addrwithport) {
        return NULL;
    }

    NET_DatagramSocket *sock = (NET_DatagramSocket *) SDL_calloc(1, sizeof (NET_DatagramSocket));
    if (!sock) {
        freeaddrinfo(addrwithport);
        return NULL;
    }

    sock->socktype = SOCKETTYPE_DATAGRAM;
    sock->addr = addr;
    sock->port = port;

    const int reuseaddr = SDL_GetBooleanProperty(props, NET_PROP_DATAGRAM_SOCKET_REUSEADDR_BOOLEAN, true) ? 1 : 0;
    sock->allow_broadcast = SDL_GetBooleanProperty(props, NET_PROP_DATAGRAM_SOCKET_ALLOW_BROADCAST_BOOLEAN, false);

    const int bcast = sock->allow_broadcast ? 1 : 0;

    int num_handles = 0;
    NET_DatagramSocketHandle *allocated_handles = NULL;
    if (addr != NULL) {
        num_handles = 1;
        sock->handles = sock->handle_pool;
    } else {  // bind to all interfaces.
        for (struct addrinfo *i = addrwithport; i != NULL; i = i->ai_next) {
            num_handles++;
        }
        if (num_handles <= (int) SDL_arraysize(sock->handle_pool)) {
            sock->handles = sock->handle_pool;
        } else {
            allocated_handles = (NET_DatagramSocketHandle *) SDL_calloc(num_handles, sizeof (*sock->handles));
            if (!allocated_handles) {
                SDL_free(sock);
                freeaddrinfo(addrwithport);
                return NULL;
            }
            sock->handles = allocated_handles;
        }
    }

    #if 0
    for (struct addrinfo *i = addrwithport; i != NULL; i = i->ai_next) {
        SDL_Log("addr:");
        SDL_Log(" - ai_flags: %d", i->ai_flags);
        SDL_Log(" - ai_family: %d", i->ai_family);
        SDL_Log(" - ai_socktype: %d", i->ai_socktype);
        SDL_Log(" - ai_protocol: %d", i->ai_protocol);
        SDL_Log(" - ai_canonname: '%s'", i->ai_canonname);
    }
    #endif

    // Make sockets for all desired interfaces; if addr!=NULL, this is one socket on one interface,
    //  but if addr==NULL, it might be multiple sockets for IPv4, IPv6, etc, bound to their INADDR_ANY equivalent.
    struct addrinfo *ainfo = addrwithport;
    for (int i = 0; i < num_handles; i++, ainfo = ainfo->ai_next) {
        SDL_assert(ainfo != NULL);
        Socket handle = socket(ainfo->ai_family, ainfo->ai_socktype, ainfo->ai_protocol);

        if (handle == INVALID_SOCKET) {
            SetLastSocketError("Failed to create socket");
            goto failed;
        }

        NET_DatagramSocketHandle *socket_handle = &sock->handles[sock->num_handles++];

        SDL_zerop(socket_handle);
        socket_handle->handle = handle;
        socket_handle->family = ainfo->ai_family;
        socket_handle->protocol = ainfo->ai_protocol;

        if (MakeSocketNonblocking(handle) < 0) {
            SDL_SetError("Failed to make new socket non-blocking");
            goto failed;
        }

        setsockopt(handle, SOL_SOCKET, SO_REUSEADDR, (const char *) &reuseaddr, sizeof (reuseaddr));
        setsockopt(handle, SOL_SOCKET, SO_BROADCAST, (const char *) &bcast, sizeof (bcast));

        if (ainfo->ai_family == AF_INET6) {
            const int one = 1;
            setsockopt(handle, IPPROTO_IPV6, IPV6_V6ONLY, (const char *) &one, sizeof (one));  // if this fails, oh well.
        }

        const int rc = bind(handle, ainfo->ai_addr, (SockLen) ainfo->ai_addrlen);
        if (rc == SOCKET_ERROR) {
            const int err = LastSocketError();
            SDL_assert(!WouldBlock(err));  // binding shouldn't be a blocking operation.
            SetSocketError("Failed to bind socket", err);
            goto failed;
        }

        if (sock->allow_broadcast) {
            Uint32 interface_index = 0;  // this will stay zero for INADDR6_ANY, to pick a default interface.
            socket_handle->broadcast = FindBroadcastAddress(ainfo, &interface_index);
            // if failed but addr==NULL, this is IPv4 and we can't find an interface for INADDR_ANY. We'll broadcast to all interfaces when sending (because 255.255.255.255 doesn't work on Windows).
            // alternately, we're IPv6 and ipv6_broadcast_addr failed to init for some reason. Just panic and fail here if that happens, I guess.
            if (!socket_handle->broadcast && (addr || (ainfo->ai_family == AF_INET6))) {
                SDL_SetError("Failed to determine broadcast address for this interface");
                goto failed;
            } else if (ainfo->ai_family == AF_INET6) { // fake broadcast support via multicasting to all-nodes link-local group, ff02::1.
#ifdef SDL_PLATFORM_VITA
                SDL_SetError("No IPv6 support");
                goto failed;
#else
                SDL_assert(socket_handle->broadcast == ipv6_broadcast_addr);
                const struct addrinfo *bai = socket_handle->broadcast->ainfo;
                SDL_assert(bai != NULL);
                SDL_assert(bai->ai_addr != NULL);
                SDL_assert(bai->ai_addr->sa_family == AF_INET6);
                struct ipv6_mreq mreq6;
                SDL_zero(mreq6);
                SDL_copyp(&mreq6.ipv6mr_multiaddr, &((struct sockaddr_in6 *) bai->ai_addr)->sin6_addr);
                mreq6.ipv6mr_interface = interface_index;
                //SDL_Log("Add membership for %s, index=%d", socket_handle->broadcast->human_readable, (int) interface_index);
                if (setsockopt(handle, IPPROTO_IPV6, IPV6_JOIN_GROUP, (const char *) &mreq6, (SockLen) sizeof (mreq6)) < 0) {
                    SDL_SetError("Failed to join all-nodes link-local multicast group for broadcasting");
                    goto failed;
                }
                const int ifidx = (int) interface_index;
                setsockopt(handle, IPPROTO_IPV6, IPV6_MULTICAST_IF, (const char *) &ifidx, (SockLen) sizeof (ifidx));  // multicast sends go through the same interface.
#endif
            }
        }
    }

    freeaddrinfo(addrwithport);

    NET_RefAddress(addr);
    return sock;

failed:
    for (int i = 0; i < sock->num_handles; i++) {
        CloseSocketHandle(sock->handles[i].handle);
        NET_UnrefAddress(sock->handles[i].broadcast);
    }
    freeaddrinfo(addrwithport);
    SDL_free(allocated_handles);
    SDL_free(sock);
    return NULL;
}

static NET_Status SendOneDatagram(NET_DatagramSocket *sock, NET_Address *addr, Uint16 port, const void *buf, int buflen)
{
    if (addr) {  // unicast to a specific address.
        struct addrinfo *addrwithport = MakeAddrInfoWithPort(addr, SOCK_DGRAM, port);
        if (!addrwithport) {
            return NET_FAILURE;
        }

        const int family = addrwithport->ai_family;
        const int protocol = addrwithport->ai_protocol;
        for (int i = 0; i < sock->num_handles; i++) {
            const NET_DatagramSocketHandle *handle = &sock->handles[i];
            if ((handle->family == family) && (handle->protocol == protocol)) {  // !!! FIXME: strictly speaking, this _probably_ just needs to check `family`, right?
                const int rc = (int) sendto(handle->handle, buf, (size_t) buflen, 0, addrwithport->ai_addr, (SockLen) addrwithport->ai_addrlen);
                const int err = (rc == SOCKET_ERROR) ? LastSocketError() : 0;
                freeaddrinfo(addrwithport);
                if (err != 0) {
                    return WouldBlock(err) ? NET_WAITING : SetSocketError("Failed to send from socket", err);
                }
                SDL_assert(rc == buflen);
                return NET_SUCCESS;  // !!! FIXME: should we sent to _all_ interfaces in this family?
            }
        }
        SDL_SetError("Unsupported network family in destination address");
        return NET_FAILURE;

    }

    // broadcast (or fake with multicast) this packet.
    SDL_assert(sock->allow_broadcast);  // we should have checked this in NET_SendDatagram!

    bool all_wouldblock = false;
    NET_Status retval = NET_FAILURE;
    for (int i = 0; i < sock->num_handles; i++) {
        const NET_DatagramSocketHandle *handle = &sock->handles[i];

        if (handle->broadcast) {
            struct addrinfo *addrwithport = MakeAddrInfoWithPort(handle->broadcast, SOCK_DGRAM, port);
            if (!addrwithport) {
                continue;  // oh well, lost UDP packet, I guess.
            }

            //SDL_Log("Broadcasting on %s ...", handle->broadcast->human_readable);
            const int rc = (int) sendto(handle->handle, buf, (size_t) buflen, 0, addrwithport->ai_addr, (SockLen) addrwithport->ai_addrlen);
            const int err = (rc == SOCKET_ERROR) ? LastSocketError() : 0;
            freeaddrinfo(addrwithport);
            if (!err) {
                retval = NET_SUCCESS;  // it went to at least one interface's broadcast address, we'll call it success.
            } else {
                if (!WouldBlock(err)) {
                    all_wouldblock = false;
                    SetSocketError("Failed to send from socket", err);  // so there's a clear error message, but keep going, maybe something else works out.
                }
            }
        } else if (!addr && InterfacesReady()) {  // iterate all interfaces for this broadcast.
            SDL_LockRWLockForReading(interface_rwlock);
            for (int i = 0; i < num_interfaces; i++) {
                const NET_Address *bc = interfaces[i].broadcast;
                if (!bc) { continue; }
                const struct addrinfo *ainfo = bc->ainfo;
                SDL_assert(ainfo != NULL);
                if (ainfo->ai_family != handle->family) { continue; }

                struct addrinfo *addrwithport = MakeAddrInfoWithPort(bc, SOCK_DGRAM, port);
                if (!addrwithport) {
                    continue;  // oh well, lost UDP packet, I guess.
                }

                //SDL_Log("Broadcasting on %s ...", bc->human_readable);
                const int rc = (int) sendto(handle->handle, buf, (size_t) buflen, 0, addrwithport->ai_addr, (SockLen) addrwithport->ai_addrlen);
                const int err = (rc == SOCKET_ERROR) ? LastSocketError() : 0;
                freeaddrinfo(addrwithport);
                if (!err) {
                    retval = NET_SUCCESS;  // it went to at least one interface's broadcast address, we'll call it success.
                } else {
                    if (!WouldBlock(err)) {
                        all_wouldblock = false;
                        SetSocketError("Failed to send from socket", err);  // so there's a clear error message, but keep going, maybe something else works out.
                    }
                }
            }
            SDL_UnlockRWLock(interface_rwlock);
        }
    }

    if ((retval == NET_FAILURE) && all_wouldblock) {
        retval = NET_WAITING;
    }

    return retval;
}

// see if any pending data can finally be sent, etc
static bool PumpDatagramSocket(NET_DatagramSocket *sock)
{
    if (!sock) {
        SDL_InvalidParamError("sock");
        return false;
    }

    while (sock->pending_output_len > 0) {
        SDL_assert(sock->pending_output != NULL);
        NET_Datagram *dgram = sock->pending_output[0];
        const NET_Status rc = SendOneDatagram(sock, dgram->addr, dgram->port, dgram->buf, dgram->buflen);
        if (rc == NET_FAILURE) {
            return false;
        } else if (rc == NET_WAITING) {
            break;  // stop trying to send packets for now.
        }

        // else if (rc == NET_SUCCESS)
        NET_DestroyDatagram(dgram);
        sock->pending_output_len--;
        SDL_memmove(sock->pending_output, sock->pending_output + 1, sock->pending_output_len * sizeof (NET_Datagram *));
        sock->pending_output[sock->pending_output_len] = NULL;
    }

    return true;
}

bool NET_SendDatagram(NET_DatagramSocket *sock, NET_Address *addr, Uint16 port, const void *buf, int buflen)
{
    if (!PumpDatagramSocket(sock)) {  // try to flush any queued data to the socket now, before we handle more.
        return false;
    } else if (!addr && !sock->allow_broadcast) {
        return SDL_SetError("Datagram socket was not created with broadcast support");
    } else if (buf == NULL) {
        return SDL_InvalidParamError("buf");
    } else if (buflen < 0) {
        return SDL_InvalidParamError("buflen");
    } else if (buflen > (64*1024)) {
        return SDL_SetError("buffer is too large to send in a single datagram packet");
    } else if (buflen == 0) {
        return true;  // nothing to do.  (!!! FIXME: but strictly speaking, a UDP packet with no payload is legal.)
    } else if (ShouldSimulateLoss(sock->percent_loss)) {
        return true;  // you won the percent_loss lottery. Drop this packet as if you sent it and it never arrived.
    }

    if (sock->pending_output_len == 0) {  // nothing queued? See if we can just send this without queueing.
        const NET_Status rc = SendOneDatagram(sock, addr, port, buf, buflen);
        if (rc == NET_FAILURE) {
            return false;  // error string was already set in SendOneDatagram.
        } else if (rc == NET_SUCCESS) {
            return true;   // successfully sent.
        }
        // if rc==NET_WAITING, it wasn't sent, because we would have blocked. Queue it for later, below.
    }

    // queue this up for sending later.
    const int min_alloc = sock->pending_output_len + 1;
    if (min_alloc > sock->pending_output_allocation) {
        int newlen = SDL_max(1, sock->pending_output_allocation);
        while (newlen < min_alloc) {
            newlen *= 2;
            if (newlen < 0) {  // uhoh, overflowed! That's a lot of memory!!
                return SDL_OutOfMemory();
            }
        }
        void *ptr = SDL_realloc(sock->pending_output, newlen * sizeof (NET_Datagram *));
        if (!ptr) {
            return false;
        }
        sock->pending_output = (NET_Datagram **) ptr;
        sock->pending_output_allocation = newlen;
    }

    NET_Datagram *dgram = (NET_Datagram *) SDL_malloc(sizeof (NET_Datagram) + buflen);
    if (!dgram) {
        return false;
    }

    dgram->buf = (Uint8 *) (dgram+1);
    SDL_memcpy(dgram->buf, buf, buflen);

    dgram->addr = NET_RefAddress(addr);
    dgram->port = port;
    dgram->buflen = buflen;

    sock->pending_output[sock->pending_output_len++] = dgram;

    return true;
}


bool NET_ReceiveDatagram(NET_DatagramSocket *sock, NET_Datagram **dgram)
{
    if (!dgram) {
        return SDL_InvalidParamError("dgram");
    }

    *dgram = NULL;

    if (!PumpDatagramSocket(sock)) {  // try to flush any queued data to the socket now, before we go further.
        return false;
    }

    for (int i = 0; i < sock->num_handles; i++) {
        AddressStorage from;
        SockLen fromlen = sizeof (from);
        // WinSock's recvfrom wants a `char *` buffer instead of `void *`. The cast here is harmless on BSD Sockets.
        const int br = (int) recvfrom(sock->handles[i].handle, (char *) sock->recv_buffer, sizeof (sock->recv_buffer), 0, (struct sockaddr *) &from, &fromlen);
        if (br == SOCKET_ERROR) {
            const int err = LastSocketError();
            if (WouldBlock(err)) {
                continue;
            }
            return SetSocketErrorBool("Failed to receive datagrams", err);
        } else if (ShouldSimulateLoss(sock->percent_loss)) {
            // you won the percent_loss lottery. Drop this packet as if it never arrived.
            continue;
        }

        char hostbuf[128];
        char portbuf[16];
        const int rc = getnameinfo((struct sockaddr *) &from, fromlen, hostbuf, sizeof (hostbuf), portbuf, sizeof (portbuf), NI_NUMERICHOST | NI_NUMERICSERV);
        if (rc != 0) {
            return SetGetAddrInfoErrorBool("Failed to determine incoming packet's address", rc);
        }

        // Cache the last X addresses we saw; if we see it again, refcount it and reuse it.
        NET_Address *fromaddr = NULL;
        for (int i = sock->latest_recv_addrs_idx - 1; i >= 0; i--) {
            SDL_assert(sock->latest_recv_addrs != NULL);
            NET_Address *a = sock->latest_recv_addrs[i];
            SDL_assert(a != NULL);  // can't be NULL, we either set this before or wrapped around to set again, but it can't be NULL.
            if (SDL_strcmp(a->human_readable, hostbuf) == 0) {
                fromaddr = a;
                break;
            }
        }

        if (!fromaddr) {
            const int idx = sock->latest_recv_addrs_idx;
            for (int i = (int) SDL_arraysize(sock->latest_recv_addrs) - 1; i >= idx; i--) {
                NET_Address *a = sock->latest_recv_addrs[i];
                if (a == NULL) {
                    break;  // ran out of already-seen entries.
                }
                if (SDL_strcmp(a->human_readable, hostbuf) == 0) {
                    fromaddr = a;
                    break;
                }
            }
        }

        const bool create_fromaddr = (!fromaddr) ? true : false;
        if (create_fromaddr) {
            fromaddr = CreateSDLNetAddrFromSockAddr((struct sockaddr *) &from, fromlen);
            if (!fromaddr) {
                return false;  // already set the error string.
            }
        }

        NET_Datagram *dg = SDL_malloc(sizeof (NET_Datagram) + br);
        if (!dg) {
            if (create_fromaddr) {
                NET_UnrefAddress(fromaddr);
            }
            return false;
        }

        dg->buf = (Uint8 *) (dg+1);
        SDL_memcpy(dg->buf, sock->recv_buffer, br);
        dg->addr = create_fromaddr ? fromaddr : NET_RefAddress(fromaddr);
        dg->port = (Uint16) SDL_atoi(portbuf);
        dg->buflen = br;

        *dgram = dg;

        if (create_fromaddr) {
            // keep track of the last X addresses we saw.
            NET_UnrefAddress(sock->latest_recv_addrs[sock->latest_recv_addrs_idx]);  // okay if "oldest" address slot is still NULL.
            sock->latest_recv_addrs[sock->latest_recv_addrs_idx++] = NET_RefAddress(fromaddr);
            sock->latest_recv_addrs_idx %= SDL_arraysize(sock->latest_recv_addrs);
        }

        return true;  // we got one!
    }

    return true;  // nothing new.
}

void NET_DestroyDatagram(NET_Datagram *dgram)
{
    if (dgram) {
        NET_UnrefAddress(dgram->addr);
        SDL_free(dgram);  // the buffer is allocated in the same block as the main struct.
    }
}

void NET_SimulateDatagramPacketLoss(NET_DatagramSocket *sock, int percent_loss)
{
    if (sock) {
        PumpDatagramSocket(sock);
        sock->percent_loss = SDL_clamp(percent_loss, 0, 100);
    }
}

void NET_DestroyDatagramSocket(NET_DatagramSocket *sock)
{
    if (sock) {
        PumpDatagramSocket(sock);  // try one last time to send any last pending data.

        for (int i = 0; i < sock->num_handles; i++) {
            CloseSocketHandle(sock->handles[i].handle);  // !!! FIXME: what does this do with non-blocking sockets? Release the descriptor but the kernel continues sending queued buffers behind the scenes?
            NET_UnrefAddress(sock->handles[i].broadcast);
        }
        for (int i = 0; i < ((int) SDL_arraysize(sock->latest_recv_addrs)); i++) {
            NET_UnrefAddress(sock->latest_recv_addrs[i]);
        }
        for (int i = 0; i < sock->pending_output_len; i++) {
            NET_DestroyDatagram(sock->pending_output[i]);
        }
        SDL_free(sock->pending_output);
        if (sock->handles != sock->handle_pool) {
            SDL_free(sock->handles);
        }
        NET_UnrefAddress(sock->addr);
        SDL_free(sock);
    }
}

typedef union NET_GenericSocket
{
    NET_SocketType socktype;
    NET_StreamSocket stream;
    NET_DatagramSocket dgram;
    NET_Server server;
} NET_GenericSocket;


int NET_WaitUntilInputAvailable(void **vsockets, int numsockets, int timeoutms)
{
    NET_GenericSocket **sockets = (NET_GenericSocket **) vsockets;
    if (!sockets) {
        SDL_InvalidParamError("sockets");
        return -1;
    } else if (numsockets < 0) {
        SDL_InvalidParamError("numsockets");
        return -1;
    } else if (numsockets == 0) {
        return 0;
    }

    struct pollfd stack_pfds[64];
    struct pollfd *pfds = stack_pfds;
    struct pollfd *malloced_pfds = NULL;

    int numhandles = 0;
    for (int i = 0; i < numsockets; i++) {
        const NET_GenericSocket *sock = sockets[i];
        if (!sock) {
            SDL_InvalidParamError("sockets");
            return -1;
        }
        switch (sock->socktype) {
            case SOCKETTYPE_STREAM:
                numhandles++;
                break;
            case SOCKETTYPE_DATAGRAM:
                numhandles += sock->dgram.num_handles;
                break;
            case SOCKETTYPE_SERVER:
                numhandles += sock->server.num_handles;
                break;
        }
    }

    if (numhandles > ((int) SDL_arraysize(stack_pfds))) {  // allocate if there's a _ton_ of these.
        malloced_pfds = (struct pollfd *) SDL_malloc(numhandles * sizeof (*pfds));
        if (!malloced_pfds) {
            return -1;
        }
        pfds = malloced_pfds;
    }

    int retval = 0;
    const Uint64 endtime = (timeoutms > 0) ? (SDL_GetTicks() + timeoutms) : 0;

    while (true) {
        struct pollfd *pfd = &pfds[0];
        SDL_memset(pfds, '\0', sizeof (*pfds) * numhandles);

        for (int i = 0; i < numsockets; i++) {
            const NET_GenericSocket *sock = sockets[i];

            switch (sock->socktype) {
                case SOCKETTYPE_STREAM:
                    pfd->fd = sock->stream.handle;
                    if (sock->stream.status == NET_WAITING) {
                        pfd->events = POLLOUT;  // marked as writable when connection is complete.
                    } else if (sock->stream.pending_output_len > 0) {
                        pfd->events = POLLIN|POLLOUT;  // poll for input or when we can write more of the pending buffer.
                    } else {
                        pfd->events = POLLIN;  // poll for input or when we can write more of the pending buffer.
                    }
                    pfd++;
                    break;

                case SOCKETTYPE_DATAGRAM:
                    for (int j = 0; j < sock->dgram.num_handles; j++) {
                        pfd->fd = sock->dgram.handles[j].handle;
                        if (sock->dgram.pending_output_len > 0) {
                            pfd->events = POLLIN|POLLOUT;  // poll for input or when we can write more of the pending buffer.
                        } else {
                            pfd->events = POLLIN;  // poll for input or when we can write more of the pending buffer.
                        }
                        pfd++;
                    }
                    break;

                case SOCKETTYPE_SERVER:
                    for (int j = 0; j < sock->server.num_handles; j++) {
                        pfd->fd = sock->server.handles[j];
                        pfd->events = POLLIN;  // poll for new connections.
                        pfd++;
                    }
                    break;
            }
        }

        const int rc = poll(pfds, numhandles, timeoutms);

        if (rc == SOCKET_ERROR) {
            SDL_free(malloced_pfds);
            return SetLastSocketError("Socket poll failed");
        }

        // !!! FIXME: skip this loop if rc == 0.
        pfd = &pfds[0];
        for (int i = 0; i < numsockets; i++) {
            NET_GenericSocket *sock = sockets[i];
            bool count_it = false;

            switch (sock->socktype) {
                case SOCKETTYPE_STREAM: {
                    SDL_assert(pfd->fd == sock->stream.handle);
                    const bool failed = ((pfd->revents & (POLLERR|POLLHUP|POLLNVAL)) != 0) ? true : false;
                    const bool writable = (pfd->revents & POLLOUT) ? true : false;
                    const bool readable = (pfd->revents & POLLIN) ? true : false;

                    if (readable || failed) {
                        count_it = true;
                    }

                    if (sock->stream.status == NET_WAITING) {
                        if (failed) {
                            int err = 0;
                            SockLen errsize = sizeof (err);
                            getsockopt(pfd->fd, SOL_SOCKET, SO_ERROR, (char*)&err, &errsize);
                            sock->stream.status = (NET_Status) SetSocketError("Socket failed to connect", err);
                        } else if (writable) {
                            sock->stream.status = NET_SUCCESS;
                            count_it = true;
                        }
                    } else if (writable) {
                        PumpStreamSocket(&sock->stream);
                    }

                    pfd++;
                }
                break;

                case SOCKETTYPE_DATAGRAM: {
                    bool pump_socket = false;
                    for (int j = 0; j < sock->dgram.num_handles; j++) {
                        SDL_assert(pfd->fd == sock->dgram.handles[j].handle);
                        const bool failed = ((pfd->revents & (POLLERR|POLLHUP|POLLNVAL)) != 0) ? true : false;
                        const bool writable = (pfd->revents & POLLOUT) ? true : false;
                        const bool readable = (pfd->revents & POLLIN) ? true : false;

                        if (readable || failed) {
                            count_it = true;
                        }

                        if (writable) {
                            pump_socket = true;
                        }
                        pfd++;
                    }

                    if (pump_socket) {
                        PumpDatagramSocket(&sock->dgram);
                    }
                }
                break;

                case SOCKETTYPE_SERVER: {
                    for (int j = 0; j < sock->server.num_handles; j++) {
                        SDL_assert(pfd->fd == sock->server.handles[j]);
                        const bool failed = ((pfd->revents & (POLLERR|POLLHUP|POLLNVAL)) != 0) ? true : false;
                        const bool readable = (pfd->revents & POLLIN) ? true : false;
                        if (readable || failed) {
                            count_it = true;
                        }
                        pfd++;
                    }
                }
                break;
            }

            if (count_it) {
                retval++;
            }
        }

        if ((retval > 0) || (endtime == 0)) {
            break;  // something has input available, or we are doing a no-block poll.
        } else if (timeoutms > 0) {   // We must have woken up for a pending write, etc. Figure out remaining wait time.
            const Uint64 now = SDL_GetTicks();
            if (now < endtime) {
                timeoutms = (int) (endtime - now);
            } else {
                break;  // time has expired, break out.
            }
        } // else timeout is meant to be infinite, but we woke up for a write, etc, so go back to an infinite poll.
    }

    SDL_free(malloced_pfds);

    return retval;
}
