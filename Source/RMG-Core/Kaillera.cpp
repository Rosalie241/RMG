/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 * Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3.
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#define CORE_INTERNAL
#include "Kaillera.hpp"
#include "Error.hpp"

#include <windows.h>
#include <cstring>

//
// Kaillera API Structures and Types
//

typedef struct {
    char *appName;
    char *gameList;
    int (WINAPI *gameCallback)(char *game, int player, int numplayers);
    void (WINAPI *chatReceivedCallback)(char *nick, char *text);
    void (WINAPI *clientDroppedCallback)(char *nick, int playernb);
    void (WINAPI *moreInfosCallback)(char *gamename);
} kailleraInfos;

// Kaillera function pointers
typedef void (WINAPI *kailleraGetVersion_t)(char *version);
typedef int (WINAPI *kailleraInit_t)(void);
typedef void (WINAPI *kailleraShutdown_t)(void);
typedef void (WINAPI *kailleraSetInfos_t)(kailleraInfos *infos);
typedef int (WINAPI *kailleraSelectServerDialog_t)(HWND parent);
typedef int (WINAPI *kailleraModifyPlayValues_t)(void *values, int size);
typedef void (WINAPI *kailleraChatSend_t)(char *text);
typedef void (WINAPI *kailleraEndGame_t)(void);

//
// Static Variables
//

static HMODULE s_KailleraDLL = nullptr;
static bool s_Initialized = false;
static bool s_GameActive = false;
static int s_PlayerNumber = 0; // 0 = not in netplay, 1-4 = player number
static int s_NumPlayers = 0;   // Total number of players in the game
static std::string s_AppName;
static std::string s_GameList;

// Kaillera function pointers
static kailleraGetVersion_t kailleraGetVersion = nullptr;
static kailleraInit_t kailleraInit = nullptr;
static kailleraShutdown_t kailleraShutdown = nullptr;
static kailleraSetInfos_t kailleraSetInfos = nullptr;
static kailleraSelectServerDialog_t kailleraSelectServerDialog = nullptr;
static kailleraModifyPlayValues_t kailleraModifyPlayValues = nullptr;
static kailleraChatSend_t kailleraChatSend = nullptr;
static kailleraEndGame_t kailleraEndGame = nullptr;

// Callback storage
static CoreKaillera::GameStartCallback s_GameStartCallback;
static CoreKaillera::ChatReceivedCallback s_ChatReceivedCallback;
static CoreKaillera::ClientDroppedCallback s_ClientDroppedCallback;
static CoreKaillera::MoreInfosCallback s_MoreInfosCallback;

//
// C Callback Bridges (called by Kaillera from its internal thread)
//

static int WINAPI GameCallbackBridge(char *game, int player, int numplayers)
{
    // Store player number and total player count
    s_PlayerNumber = player;
    s_NumPlayers = numplayers;

    if (s_GameStartCallback)
    {
        try
        {
            s_GameStartCallback(std::string(game), player, numplayers);
            s_GameActive = true;
            return 0; // Success
        }
        catch (...)
        {
            return -1; // Error
        }
    }
    return 0;
}

static void WINAPI ChatReceivedCallbackBridge(char *nick, char *text)
{
    if (s_ChatReceivedCallback)
    {
        try
        {
            s_ChatReceivedCallback(std::string(nick), std::string(text));
        }
        catch (...)
        {
            // Ignore errors in callback
        }
    }
}

static void WINAPI ClientDroppedCallbackBridge(char *nick, int playernb)
{
    if (s_ClientDroppedCallback)
    {
        try
        {
            s_ClientDroppedCallback(std::string(nick), playernb);
        }
        catch (...)
        {
            // Ignore errors in callback
        }
    }
}

static void WINAPI MoreInfosCallbackBridge(char *gamename)
{
    if (s_MoreInfosCallback)
    {
        try
        {
            s_MoreInfosCallback(std::string(gamename));
        }
        catch (...)
        {
            // Ignore errors in callback
        }
    }
}

//
// Helper Functions
//

static bool LoadKailleraDLL(void)
{
    if (s_KailleraDLL != nullptr)
    {
        return true; // Already loaded
    }

    // Try to load kailleraclient.dll from current directory
    s_KailleraDLL = LoadLibraryA("kailleraclient.dll");
    if (s_KailleraDLL == nullptr)
    {
        CoreSetError("Failed to load kailleraclient.dll. Make sure it's in the same directory as RMG.exe");
        return false;
    }

    // Load all required functions
    kailleraGetVersion = (kailleraGetVersion_t)GetProcAddress(s_KailleraDLL, "kailleraGetVersion");
    kailleraInit = (kailleraInit_t)GetProcAddress(s_KailleraDLL, "kailleraInit");
    kailleraShutdown = (kailleraShutdown_t)GetProcAddress(s_KailleraDLL, "kailleraShutdown");
    kailleraSetInfos = (kailleraSetInfos_t)GetProcAddress(s_KailleraDLL, "kailleraSetInfos");
    kailleraSelectServerDialog = (kailleraSelectServerDialog_t)GetProcAddress(s_KailleraDLL, "kailleraSelectServerDialog");
    kailleraModifyPlayValues = (kailleraModifyPlayValues_t)GetProcAddress(s_KailleraDLL, "kailleraModifyPlayValues");
    kailleraChatSend = (kailleraChatSend_t)GetProcAddress(s_KailleraDLL, "kailleraChatSend");
    kailleraEndGame = (kailleraEndGame_t)GetProcAddress(s_KailleraDLL, "kailleraEndGame");

    // Verify all functions were loaded
    if (!kailleraGetVersion || !kailleraInit || !kailleraShutdown ||
        !kailleraSetInfos || !kailleraSelectServerDialog || !kailleraModifyPlayValues ||
        !kailleraChatSend || !kailleraEndGame)
    {
        CoreSetError("Invalid kailleraclient.dll - missing required functions");
        FreeLibrary(s_KailleraDLL);
        s_KailleraDLL = nullptr;
        return false;
    }

    return true;
}

//
// Exported Functions
//

CORE_EXPORT bool CoreInitKaillera(void)
{
    if (s_Initialized)
    {
        return true; // Already initialized
    }

    // Load DLL
    if (!LoadKailleraDLL())
    {
        return false;
    }

    // Check version
    char version[16];
    kailleraGetVersion(version);
    // Version should be "0.9" or compatible
    // We don't enforce a specific version for now

    // Initialize Kaillera
    int ret = kailleraInit();
    if (ret != 0)
    {
        CoreSetError("Kaillera initialization failed with error code: " + std::to_string(ret));
        return false;
    }

    s_Initialized = true;
    s_GameActive = false;

    return true;
}

CORE_EXPORT bool CoreShutdownKaillera(void)
{
    if (!s_Initialized)
    {
        return true; // Not initialized, nothing to do
    }

    // End game if active
    if (s_GameActive)
    {
        CoreEndKailleraGame();
    }

    // Shutdown Kaillera
    if (kailleraShutdown)
    {
        kailleraShutdown();
    }

    s_Initialized = false;
    s_GameActive = false;

    // Note: We don't FreeLibrary the DLL because Kaillera may have
    // background threads that need to clean up

    return true;
}

CORE_EXPORT bool CoreHasInitKaillera(void)
{
    return s_Initialized && s_GameActive;
}

CORE_EXPORT bool CoreShowKailleraServerDialog(void* parentHwnd)
{
    if (!s_Initialized)
    {
        CoreSetError("Kaillera not initialized. Call CoreInitKaillera() first");
        return false;
    }

    // Show Kaillera's server selection dialog
    int ret = kailleraSelectServerDialog((HWND)parentHwnd);
    if (ret != 0)
    {
        CoreSetError("Failed to connect to Kaillera server");
        return false;
    }

    return true;
}

CORE_EXPORT int CoreModifyKailleraPlayValues(void* values, int size)
{
    if (!s_Initialized || !s_GameActive)
    {
        return -1; // Not in netplay mode
    }

    if (!kailleraModifyPlayValues)
    {
        return -1;
    }

    // Call Kaillera to synchronize input
    int ret = kailleraModifyPlayValues(values, size);
    return ret;
}

CORE_EXPORT bool CoreKailleraSendChat(std::string text)
{
    if (!s_Initialized || !s_GameActive)
    {
        return false;
    }

    if (!kailleraChatSend)
    {
        return false;
    }

    // Kaillera expects non-const char*
    char* textBuf = new char[text.length() + 1];
    strcpy(textBuf, text.c_str());

    kailleraChatSend(textBuf);

    delete[] textBuf;
    return true;
}

CORE_EXPORT bool CoreEndKailleraGame(void)
{
    if (!s_GameActive)
    {
        return true; // No game active
    }

    if (kailleraEndGame)
    {
        kailleraEndGame();
    }

    s_GameActive = false;
    return true;
}

CORE_EXPORT void CoreSetKailleraCallbacks(
    CoreKaillera::GameStartCallback gameStartCallback,
    CoreKaillera::ChatReceivedCallback chatReceivedCallback,
    CoreKaillera::ClientDroppedCallback clientDroppedCallback,
    CoreKaillera::MoreInfosCallback moreInfosCallback)
{
    s_GameStartCallback = gameStartCallback;
    s_ChatReceivedCallback = chatReceivedCallback;
    s_ClientDroppedCallback = clientDroppedCallback;
    s_MoreInfosCallback = moreInfosCallback;

    // Set up kailleraInfos structure
    if (s_Initialized && kailleraSetInfos)
    {
        static kailleraInfos infos;

        // Allocate app name (must persist)
        static char appNameBuf[128];
        strncpy(appNameBuf, s_AppName.c_str(), sizeof(appNameBuf) - 1);
        appNameBuf[sizeof(appNameBuf) - 1] = '\0';
        infos.appName = appNameBuf;

        // Allocate game list (must persist)
        static char* gameListBuf = nullptr;
        if (gameListBuf)
        {
            delete[] gameListBuf;
        }
        gameListBuf = new char[s_GameList.length() + 1];
        strcpy(gameListBuf, s_GameList.c_str());
        infos.gameList = gameListBuf;

        // Set callbacks
        infos.gameCallback = GameCallbackBridge;
        infos.chatReceivedCallback = s_ChatReceivedCallback ? ChatReceivedCallbackBridge : nullptr;
        infos.clientDroppedCallback = s_ClientDroppedCallback ? ClientDroppedCallbackBridge : nullptr;
        infos.moreInfosCallback = s_MoreInfosCallback ? MoreInfosCallbackBridge : nullptr;

        kailleraSetInfos(&infos);
    }
}

CORE_EXPORT bool CoreSetKailleraAppInfo(std::string appName, std::string gameList)
{
    s_AppName = appName;
    s_GameList = gameList;

    // Update kailleraInfos if already initialized
    if (s_Initialized && kailleraSetInfos)
    {
        // Trigger callback update which will also update app info
        CoreSetKailleraCallbacks(
            s_GameStartCallback,
            s_ChatReceivedCallback,
            s_ClientDroppedCallback,
            s_MoreInfosCallback
        );
    }

    return true;
}

CORE_EXPORT void CoreSetKailleraPlayerNumber(int playerNumber)
{
    s_PlayerNumber = playerNumber;
}

CORE_EXPORT int CoreGetKailleraPlayerNumber(void)
{
    return s_PlayerNumber;
}

CORE_EXPORT int CoreGetKailleraNumPlayers(void)
{
    return s_NumPlayers;
}
