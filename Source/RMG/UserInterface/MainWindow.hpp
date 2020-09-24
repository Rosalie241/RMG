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

#include "../Thread/EmulationThread.hpp"
#include "Widget/RomBrowserWidget.hpp"
#include "Widget/OGLWidget.hpp"
#include "../Globals.hpp"

#include <QMainWindow>
#include <QOpenGLWidget>
#include <QAction>
#include <QCloseEvent>
#include <QSettings>
#include <QStackedWidget>

namespace UserInterface
{
    class MainWindow : public QMainWindow
    {
        Q_OBJECT

    public:
        MainWindow(void);
        ~MainWindow(void);

        bool Init(void);

    private:
        QIcon ui_Icon;

        Thread::EmulationThread *emulationThread;

        QStackedWidget *ui_Widgets;
        Widget::OGLWidget *ui_Widget_OpenGL;
        Widget::RomBrowserWidget *ui_Widget_RomBrowser;

        QMenuBar *menuBar;
        QMenu *menuBar_Menu;
        QAction *action_File_OpenRom;
        QAction *action_File_OpenCombo;
        QAction *action_File_EndEmulation;
        QAction *action_File_ChooseDirectory;
        QAction *action_File_RefreshRomList;
        QAction *action_File_Exit;
        QAction *action_System_SoftReset;
        QAction *action_System_HardReset;
        QAction *action_System_Pause;
        QAction *action_System_GenerateBitmap;
        QAction *action_System_LimitFPS;
        QAction *action_System_SwapDisk;
        QAction *action_System_SaveState;
        QAction *action_System_SaveAs;
        QAction *action_System_LoadState;
        QAction *action_System_Load;
        QAction *action_System_CurrentSaveState;
        QAction *action_System_Cheats;
        QAction *action_System_GSButton;
        QAction *action_Options_FullScreen;
        QAction *action_Options_ConfigGfx;
        QAction *action_Options_ConfigAudio;
        QAction *action_Options_ConfigRsp;
        QAction *action_Options_ConfigControl;
        QAction *action_Options_Settings;
        QAction *action_Help_Support;
        QAction *action_Help_HomePage;
        QAction *action_Help_About;

        QSettings *ui_Settings;

        void closeEvent(QCloseEvent *);

        void ui_Init();
        void ui_Setup();
        void ui_Stylesheet_Setup();
        void ui_MessageBox(QString, QString, QString);
        void ui_InEmulation(bool);

        void menuBar_Init(void);
        void menuBar_Setup(bool);

        void emulationThread_Init(void);
        void emulationThread_Connect(void);
        void emulationThread_Launch(QString);

        void menuBar_Actions_Init(void);
        void menuBar_Actions_Setup(bool);
        void menuBar_Actions_Connect(void);

    public slots:
        void on_Action_File_OpenRom(void);
        void on_Action_File_OpenCombo(void);
        void on_Action_File_EndEmulation(void);
        void on_Action_File_ChooseDirectory(void);
        void on_Action_File_RefreshRomList(void);
        void on_Action_File_Exit(void);
        void on_Action_System_SoftReset(void);
        void on_Action_System_HardReset(void);
        void on_Action_System_Pause(void);
        void on_Action_System_GenerateBitmap(void);
        void on_Action_System_LimitFPS(void);
        void on_Action_System_SwapDisk(void);
        void on_Action_System_SaveState(void);
        void on_Action_System_SaveAs(void);
        void on_Action_System_LoadState(void);
        void on_Action_System_Load(void);
        void on_Action_System_CurrentSaveState(void);
        void on_Action_System_Cheats(void);
        void on_Action_System_GSButton(void);
        void on_Action_Options_FullScreen(void);
        void on_Action_Options_ConfigGfx(void);
        void on_Action_Options_ConfigAudio(void);
        void on_Action_Options_ConfigRsp(void);
        void on_Action_Options_ConfigControl(void);
        void on_Action_Options_Settings(void);
        void on_Action_Help_Support(void);
        void on_Action_Help_HomePage(void);
        void on_Action_Help_About(void);

        void on_Emulation_Started(void);
        void on_Emulation_Finished(bool);

        void on_RomBrowser_Selected(QString);

        void on_VidExt_Init(void);
        void on_VidExt_SetupOGL(QSurfaceFormat, QThread*);
        void on_VidExt_SetMode(int, int, int, int, int);
        void on_VidExt_SetModeWithRate(int, int, int, int, int, int);
        void on_VidExt_ResizeWindow(int, int);
        void on_VidExt_SetCaption(QString);
        void on_VidExt_ToggleFS(void);
        void on_VidExt_Quit(void);
    };
} // namespace UserInterface

#endif // MAINWINDOW_HPP
