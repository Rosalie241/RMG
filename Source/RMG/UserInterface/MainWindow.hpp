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

    bool Init(QApplication*, bool);
    void OpenROM(QString, QString, bool, bool);

  private:
    Thread::EmulationThread *emulationThread;

    CoreCallbacks* coreCallBacks;

    QStackedWidget *ui_Widgets;
    Widget::OGLWidget *ui_Widget_OpenGL;
    Widget::RomBrowserWidget *ui_Widget_RomBrowser;
    EventFilter *ui_EventFilter;
    QLabel *ui_StatusBar_Label;

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

    bool ui_ShowUI        = false;
    bool ui_ShowMenubar   = false;
    bool ui_ShowToolbar   = false;
    bool ui_ShowStatusbar = false;

    bool ui_ManuallyPaused = true;

    bool ui_AddedActions = false;

    int ui_TimerId      = 0;
    int ui_TimerTimeout = 0;

    int ui_FullscreenTimerId = 0;
    int ui_GamesharkButtonTimerId = 0;

    QString ui_WindowTitle;

    Dialog::LogDialog logDialog;

    void closeEvent(QCloseEvent *) Q_DECL_OVERRIDE;

    void initializeUI();
    
    void configureUI(QApplication*, bool);
    void configureTheme(QApplication*);

    void showErrorMessage(QString, QString);
    
    void updateUI(bool, bool);

    void storeGeometry(void);
    void loadGeometry(void);

    void initializeEmulationThread(void);
    void connectEmulationThreadSignals(void);
    void launchEmulationThread(QString, QString);
    void launchEmulationThread(QString);

    void configureActions(void);
    void connectActionSignals(void);
    void updateActions(bool, bool);

    void addActions(void);
    void removeActions(void);

#ifdef UPDATER
    void checkForUpdates(void);
#endif // UPDATER
  protected:
    void timerEvent(QTimerEvent *) Q_DECL_OVERRIDE;

  private slots:
    void on_EventFilter_KeyPressed(QKeyEvent *);
    void on_EventFilter_KeyReleased(QKeyEvent *);

    void on_QGuiApplication_applicationStateChanged(Qt::ApplicationState);
 
#ifdef UPDATER
    void on_networkAccessManager_Finished(QNetworkReply *);
#endif // UPDATER

    void on_Action_System_OpenRom(void);
    void on_Action_System_OpenCombo(void);
    void on_Action_System_Shutdown(void);
    void on_Action_System_SoftReset(void);
    void on_Action_System_HardReset(void);
    void on_Action_System_Pause(void);
    void on_Action_System_Screenshot(void);
    void on_Action_System_LimitFPS(void);
    void on_Action_System_SpeedFactor(int);
    void on_Action_System_SaveState(void);
    void on_Action_System_SaveAs(void);
    void on_Action_System_LoadState(void);
    void on_Action_System_Load(void);
    void on_Action_System_CurrentSaveState(int);
    void on_Action_System_Cheats(void);
    void on_Action_System_GSButton(void);
    void on_Action_System_Exit(void);

    void on_Action_Settings_Graphics(void);
    void on_Action_Settings_Audio(void);
    void on_Action_Settings_Rsp(void);
    void on_Action_Settings_Input(void);
    void on_Action_Settings_Settings(void);

    void on_Action_View_Toolbar(bool);
    void on_Action_View_StatusBar(bool);
    void on_Action_View_GameList(bool);
    void on_Action_View_GameGrid(bool);
    void on_Action_View_UniformSize(bool);
    void on_Action_View_Fullscreen(void);
    void on_Action_View_RefreshRoms(void);
    void on_Action_View_ClearRomCache(void);
    void on_Action_View_Log(void);

    void on_Action_Help_Github(void);
    void on_Action_Help_About(void);

    void on_Emulation_Started(void);
    void on_Emulation_Finished(bool);

    void on_RomBrowser_PlayGame(QString);
    void on_RomBrowser_PlayGameWith(CoreRomType, QString);
    void on_RomBrowser_ChangeRomDirectory(void);
    void on_RomBrowser_RomInformation(QString);
    void on_RomBrowser_EditGameSettings(QString);
    void on_RomBrowser_Cheats(QString);

  public slots:

    void on_VidExt_Init(void);
    void on_VidExt_SetupOGL(QSurfaceFormat, QThread *);
    void on_VidExt_SetWindowedMode(int, int, int, int);
    void on_VidExt_SetFullscreenMode(int, int, int, int);
    void on_VidExt_ResizeWindow(int, int);
    void on_VidExt_ToggleFS(bool);

    void on_Core_DebugCallback(CoreDebugMessageType, QString, QString);
};
} // namespace UserInterface

#endif // MAINWINDOW_HPP
