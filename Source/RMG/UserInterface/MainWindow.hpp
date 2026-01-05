/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020-2025 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include "Thread/EmulationThread.hpp"
#include "EventFilter.hpp"
#include "Callbacks.hpp"

#include "Widget/RomBrowser/RomBrowserWidget.hpp"
#include "Widget/Render/DummyWidget.hpp"
#include "Widget/Render/OGLWidget.hpp"
#include "Widget/Render/VKWidget.hpp"

#ifdef NETPLAY
#include "Dialog/Netplay/NetplaySessionDialog.hpp"
#include "KailleraSessionManager.hpp"
#endif // NETPLAY
#include "Dialog/LogDialog.hpp"

#ifdef UPDATER
#include <QNetworkReply>
#endif // UPDATER
#include <QGuiApplication>
#include <QStackedWidget>
#include <QCloseEvent>
#include <QMainWindow>
#include <QMessageBox>
#include <QAction>

#include "ui_MainWindow.h"

namespace UserInterface
{
class MainWindow : public QMainWindow, private Ui::MainWindow
{
    Q_OBJECT

  public:
    MainWindow(void);
    ~MainWindow(void);

    bool Init(QApplication* app, bool showUI, bool launchROM);
    void OpenROM(QString file, QString disk, bool fullscreen, bool quitAfterEmulation, int stateSlot);

  private:
    Thread::EmulationThread *emulationThread = nullptr;

    CoreCallbacks* coreCallBacks = nullptr;

    QStackedWidget *ui_Widgets                     = nullptr;
    Widget::DummyWidget *ui_Widget_Dummy           = nullptr;
    Widget::OGLWidget   *ui_Widget_OpenGL          = nullptr;
    Widget::VKWidget    *ui_Widget_Vulkan          = nullptr;
    Widget::RomBrowserWidget *ui_Widget_RomBrowser = nullptr;
    EventFilter *ui_EventFilter                    = nullptr;
    QLabel *ui_StatusBar_Label                     = nullptr;
    QLabel *ui_StatusBar_RenderModeLabel           = nullptr;

    QByteArray ui_Geometry;
    bool ui_Geometry_Maximized = false;
    bool ui_Geometry_Saved = false;

    bool ui_HideCursorInEmulation = false;
    bool ui_HideCursorInFullscreenEmulation = false;
    bool ui_NoSwitchToRomBrowser = false;
    bool ui_VidExtForceSetMode   = false;
    bool ui_LaunchInFullscreen   = false;
    bool ui_QuitAfterEmulation   = false;
    bool ui_RefreshRomListAfterEmulation = false;
    int  ui_LoadSaveStateSlot    = -1;

    VidExtRenderMode ui_VidExtRenderMode = VidExtRenderMode::Invalid;

    bool ui_ShowUI        = false;
    bool ui_ShowMenubar   = false;
    bool ui_ShowToolbar   = false;
    bool ui_ShowStatusbar = false;

    bool ui_ManuallyPaused = true;
    bool ui_ManuallySavedState  = false;
    bool ui_ManuallyLoadedState = false;

    bool ui_ForceClose = false;

    QList<QAction*> ui_Actions;
    bool ui_AddedActions = false;

    QList<QAction*> ui_SlotActions;

    QList<QMessageBox*> ui_MessageBoxList;
    QList<QString> ui_DebugCallbackErrors;

    // Hotkey exclusive QActions
    QAction* action_Audio_IncreaseVolume   = nullptr;
    QAction* action_Audio_DecreaseVolume   = nullptr;
    QAction* action_Audio_ToggleVolumeMute = nullptr;

    bool ui_SilentUpdateCheck = false;

    int ui_ResetStatusBarTimerId = 0;
    int ui_StatusBarTimerTimeout = 0;

    int ui_FullscreenTimerId = 0;
    int ui_GamesharkButtonTimerId = 0;
    int ui_UpdateSaveStateSlotTimerId = 0;
    int ui_CheckVideoSizeTimerId = 0;

    int ui_LoadSaveStateSlotCounter = 0;
    int ui_LoadSaveStateSlotTimerId = -1;

    QString ui_WindowTitle;

    Dialog::LogDialog logDialog;
#ifdef NETPLAY
    Dialog::NetplaySessionDialog* netplaySessionDialog = nullptr;
    KailleraSessionManager* kailleraSessionManager = nullptr;
#endif // NETPLAY

    void closeEvent(QCloseEvent *) Q_DECL_OVERRIDE;

    void initializeUI(bool launchROM);

    void configureUI(QApplication* app, bool showUI);
    void configureTheme(QApplication* app);

    QString getWindowTitle(void);

    void showErrorMessage(QString text, QString details = "", bool force = true);

    void updateUI(bool inEmulation, bool isPaused);

    void storeGeometry(void);
    void loadGeometry(void);

    void initializeEmulationThread(void);
    void connectEmulationThreadSignals(void);
    void launchEmulationThread(QString cartRom, QString address, int port, int player);
    void launchEmulationThread(QString cartRom, QString diskRom = "", bool refreshRomListAfterEmulation = false, int slot = -1, bool netplay = false, bool dragdrop = false);

    QString getSaveStateSlotDateTimeText(QAction* action);
    QString getSaveStateSlotText(QAction* action, int slot);

    int getToolbarSettingAreaFromArea(Qt::ToolBarArea area);
    Qt::ToolBarArea getToolbarAreaFromSettingArea(int value);


    void initializeActions(void);
    void configureActions(void);
    void connectActionSignals(void);
    void updateActions(bool inEmulation, bool isPaused);
    void updateSaveStateSlotActions(bool inEmulation, bool isPaused);

    void addActions(void);
    void removeActions(void);

#ifdef UPDATER
    void checkForUpdates(bool silent, bool force);
#endif // UPDATER

#ifdef NETPLAY
    void showNetplaySessionDialog(QWebSocket* webSocket, QJsonObject json, QString sessionFile);
    QString findRomByName(QString gameName);
#endif // NETPLAY
  protected:
    void timerEvent(QTimerEvent *event) Q_DECL_OVERRIDE;

  private slots:
    void on_EventFilter_KeyPressed(QKeyEvent *event);
    void on_EventFilter_KeyReleased(QKeyEvent *event);
    void on_EventFilter_FileDropped(QDropEvent *event);

    void on_QGuiApplication_applicationStateChanged(Qt::ApplicationState state);
 
#ifdef UPDATER
    void on_networkAccessManager_Finished(QNetworkReply *reply);
#endif // UPDATER

    void on_Action_System_OpenRom(void);
    void on_Action_System_OpenCombo(void);
    void on_Action_System_Shutdown(void);
    void on_Action_System_SoftReset(void);
    void on_Action_System_HardReset(void);
    void on_Action_System_Pause(void);
    void on_Action_System_Screenshot(void);
    void on_Action_System_LimitFPS(void);
    void on_Action_System_SpeedFactor(int factor);
    void on_Action_System_SaveState(void);
    void on_Action_System_SaveAs(void);
    void on_Action_System_LoadState(void);
    void on_Action_System_Load(void);
    void on_Action_System_CurrentSaveState(int slot);
    void on_Action_System_Cheats(void);
    void on_Action_System_GSButton(void);
    void on_Action_System_Exit(void);

    void on_Action_Settings_Graphics(void);
    void on_Action_Settings_Audio(void);
    void on_Action_Settings_Rsp(void);
    void on_Action_Settings_Input(void);
    void on_Action_Settings_Settings(void);

    void on_Action_View_Toolbar(bool checked);
    void on_Action_View_StatusBar(bool checked);
    void on_Action_View_GameList(bool checked);
    void on_Action_View_GameGrid(bool checked);
    void on_Action_View_UniformSize(bool checked);
    void on_Action_View_Fullscreen(void);
    void on_Action_View_RefreshRoms(void);
    void on_Action_View_ClearRomCache(void);
    void on_Action_View_Log(void);
    void on_Action_View_Search(void);

    void on_Action_Netplay_CreateSession(void);
    void on_Action_Netplay_BrowseSessions(void);
    void on_Action_Netplay_ViewSession(void);

    void on_Kaillera_GameStarted(QString gameName, int playerNum, int totalPlayers);
    void on_Kaillera_ChatReceived(QString nickname, QString message);
    void on_Kaillera_PlayerDropped(QString nickname, int playerNum);
    void on_Kaillera_GameEnded(void);

    void on_Action_Help_Github(void);
    void on_Action_Help_About(void);
    void on_Action_Help_Update(void);

    void on_Action_Audio_IncreaseVolume(void);
    void on_Action_Audio_DecreaseVolume(void);
    void on_Action_Audio_ToggleVolumeMute(void);

    void on_Emulation_Started(void);
    void on_Emulation_Finished(bool ret, QString error);

    void on_RomBrowser_PlayGame(QString file);
    void on_RomBrowser_PlayGameWith(CoreRomType type, QString file);
    void on_RomBrowser_PlayGameWithDisk(QString cartridge, QString disk);
    void on_RomBrowser_PlayGameWithSlot(QString file, int slot);
    void on_RomBrowser_ChangeRomDirectory(void);
    void on_RomBrowser_RomInformation(QString file);
    void on_RomBrowser_EditGameSettings(QString file);
    void on_RomBrowser_EditGameInputSettings(QString file);
    void on_RomBrowser_Cheats(QString file);

    void on_Netplay_PlayGame(QString file, QString address, int port, int player);
    void on_NetplaySessionDialog_rejected(void);

  public slots:

    void on_VidExt_Init(VidExtRenderMode renderMode);
    void on_VidExt_SetupOGL(QSurfaceFormat format, QThread *thread);
    void on_VidExt_SetWindowedMode(int width, int height, int bps, int flags);
    void on_VidExt_SetFullscreenMode(int width, int height, int bps, int flags);
    void on_VidExt_ResizeWindow(int width, int height);
    void on_VidExt_ToggleFS(bool fullscreen);
    void on_VidExt_Quit(void);

    void on_Core_DebugCallback(QList<CoreCallbackMessage> messages);
    void on_Core_StateCallback(CoreStateCallbackType type, int value);
};
} // namespace UserInterface

#endif // MAINWINDOW_HPP
