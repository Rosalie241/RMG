#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include "../Thread/EmulationThread.hpp"
#include "RomBrowserWidget.hpp"
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
        Thread::EmulationThread* emulationThread;
        
        QOpenGLWidget *ui_Widget_OpenGL;
        RomBrowserWidget *ui_Widget_RomBrowser;
        QStackedWidget *ui_Widgets;

        QMenuBar *menuBar;
        QMenu *menuBar_Menu;
        QAction *action_File_OpenRom;
        QAction *action_File_OpenCombo;
        QAction *action_File_EndEmulation;
        QAction *action_File_ChooseDirectory;
        QAction *action_File_RefreshRomList;
        QAction *action_File_Exit;
        QAction *action_System_Reset;
        QAction *action_System_Pause;
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

        void menuBar_Actions_Init(void);
        void menuBar_Actions_Setup(bool);
        void menuBar_Actions_Connect(void);

        void on_VidExt_Quit(void);

    public slots:
        void on_Action_File_OpenRom(void);
        void on_Action_File_OpenCombo(void);
        void on_Action_File_EndEmulation(void);
        void on_Action_File_ChooseDirectory(void);
        void on_Action_File_RefreshRomList(void);
        void on_Action_File_Exit(void);
        void on_Action_System_Reset(void);
        void on_Action_System_Pause(void);
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
    };
} // namespace UserInterface

#endif // MAINWINDOW_HPP
