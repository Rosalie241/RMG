/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include "Thread/EmulationThread.hpp"
#include "Dialog/SettingsDialog.hpp"
#include "Dialog/CheatsDialog.hpp"
#include "Dialog/RomInfoDialog.hpp"
#include "Dialog/LogDialog.hpp"
#include "EventFilter.hpp"
#include "Widget/OGLWidget.hpp"
#include "Widget/VKWidget.hpp"
#include "Widget/RomBrowserWidget.hpp"
#include "Callbacks.hpp"

#include <QAction>
#include <QToolBar>
#include <QCloseEvent>
#include <QMainWindow>
#include <QSettings>
#include <QStackedWidget>
#include <QGuiApplication>

#ifdef UPDATER
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#endif // UPDATER

#include "ui_MainWindow.h"

namespace UserInterface
{
class MainWindow : public QMainWindow, private Ui::MainWindow
{
    Q_OBJECT

  public:
    MainWindow(void);
    ~MainWindow(void);

    bool Init(QApplication* app, bool showUI);
    void OpenROM(QString file, QString disk, bool fullscreen, bool quitAfterEmulation);

  private:
    Thread::EmulationThread *emulationThread = nullptr;

    CoreCallbacks* coreCallBacks = nullptr;

    QStackedWidget *ui_Widgets                     = nullptr;
    Widget::OGLWidget *ui_Widget_OpenGL            = nullptr;
    Widget::VKWidget  *ui_Widget_Vulkan            = nullptr;
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

    VidExtRenderMode ui_VidExtRenderMode;

    bool ui_ShowUI        = false;
    bool ui_ShowMenubar   = false;
    bool ui_ShowToolbar   = false;
    bool ui_ShowStatusbar = false;

    bool ui_ManuallyPaused = true;

    bool ui_ManuallySavedState  = false;
    bool ui_ManuallyLoadedState = false;

    QList<QAction*> ui_Actions;
    bool ui_AddedActions = false;

    QList<QAction*> ui_SlotActions;

    bool ui_SilentUpdateCheck = false;

    int ui_TimerId      = 0;
    int ui_TimerTimeout = 0;

    int ui_FullscreenTimerId = 0;
    int ui_GamesharkButtonTimerId = 0;
    int ui_UpdateSaveStateSlotTimerId = 0;

    QString ui_WindowTitle;

    Dialog::LogDialog logDialog;

    void closeEvent(QCloseEvent *) Q_DECL_OVERRIDE;

    void initializeUI();

    void configureUI(QApplication* app, bool showUI);
    void configureTheme(QApplication* app);

    void showErrorMessage(QString text, QString details = "");

    void updateUI(bool inEmulation, bool isPaused);

    void storeGeometry(void);
    void loadGeometry(void);

    void initializeEmulationThread(void);
    void connectEmulationThreadSignals(void);
    void launchEmulationThread(QString cartRom, QString diskRom = "");

    QString getSaveStateSlotDateTimeText(QAction* action);
    QString getSaveStateSlotText(QAction* action, int slot);

    int getToolbarSettingAreaFromArea(Qt::ToolBarArea area);
    Qt::ToolBarArea getToolbarAreaFromSettingArea(int value);

    void configureActions(void);
    void connectActionSignals(void);
    void updateActions(bool inEmulation, bool isPaused);
    void updateSaveStateSlotActions(bool inEmulation, bool isPaused);

    void addActions(void);
    void removeActions(void);

#ifdef UPDATER
    void checkForUpdates(bool silent, bool force);
#endif // UPDATER
  protected:
    void timerEvent(QTimerEvent *event) Q_DECL_OVERRIDE;

  private slots:
    void on_EventFilter_KeyPressed(QKeyEvent *event);
    void on_EventFilter_KeyReleased(QKeyEvent *event);

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

    void on_Action_Help_Github(void);
    void on_Action_Help_About(void);
    void on_Action_Help_Update(void);

    void on_Emulation_Started(void);
    void on_Emulation_Finished(bool ret);

    void on_RomBrowser_PlayGame(QString file);
    void on_RomBrowser_PlayGameWith(CoreRomType type, QString file);
    void on_RomBrowser_ChangeRomDirectory(void);
    void on_RomBrowser_RomInformation(QString file);
    void on_RomBrowser_EditGameSettings(QString file);
    void on_RomBrowser_EditGameInputSettings(QString file);
    void on_RomBrowser_Cheats(QString file);

  public slots:

    void on_VidExt_Init(VidExtRenderMode renderMode);
    void on_VidExt_SetupOGL(QSurfaceFormat format, QThread *thread);
    void on_VidExt_SetWindowedMode(int width, int height, int bps, int flags);
    void on_VidExt_SetFullscreenMode(int width, int height, int bps, int flags);
    void on_VidExt_ResizeWindow(int width, int height);
    void on_VidExt_ToggleFS(bool fullscreen);

    void on_Core_DebugCallback(CoreDebugMessageType type, QString context, QString message);
    void on_Core_StateCallback(CoreStateCallbackType type, int value);
};
} // namespace UserInterface

#endif // MAINWINDOW_HPP
