#include "MainWindow.hpp"
#include "../Util/Logger.hpp"
#include "../Config.hpp"

#include <QMenuBar>
#include <QStatusBar>
#include <QCoreApplication>
#include <QDesktopServices>
#include <QUrl>
#include <QMessageBox>
#include <QString>
#include <QSettings>

using namespace UserInterface;

MainWindow::MainWindow() : QMainWindow(nullptr)
{
}

MainWindow::~MainWindow()
{
    // Delete all UI elements
    /*if (this->menuBar)
        delete this->menuBar;

    if (this->ui_Widgets)
        delete this->ui_Widgets;

    if (this->ui_Settings)
        delete this->ui_Settings; */
}

bool MainWindow::Init(void)
{
    if (!g_Logger.Init())
    {
        this->ui_MessageBox("Error", "Logger::Init Failed", g_Logger.GetLastError());
        return false;
    }

    if (!g_MupenApi.Init(MUPEN_CORE_FILE))
    {
        this->ui_MessageBox("Error", "M64P::Wrapper::Api::Init Failed", g_MupenApi.GetLastError());
        return false;
    }

    g_MupenApi.Core.GetPlugins(M64P::Wrapper::PluginType::Gfx);
    g_MupenApi.Core.GetPlugins(M64P::Wrapper::PluginType::Rsp);
    g_MupenApi.Core.GetPlugins(M64P::Wrapper::PluginType::Audio);
    g_MupenApi.Core.GetPlugins(M64P::Wrapper::PluginType::Input);

    this->ui_Init();
    this->ui_Setup();

    this->menuBar_Init();
    this->menuBar_Setup(false);

    this->emulationThread_Init();
    this->emulationThread_Connect();
    
    return true;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    this->ui_Settings->setValue(APP_SETTINGS_GEOMETRY, this->saveGeometry());

    QMainWindow::closeEvent(event);
}

void MainWindow::ui_Init(void)
{
    this->ui_Widget_OpenGL = new QOpenGLWidget();
    this->ui_Widget_RomBrowser = new RomBrowserWidget();
    this->ui_Widgets = new QStackedWidget();
    this->ui_Settings = new QSettings(APP_SETTINGS_ORG, APP_SETTINGS_NAME);
}

void MainWindow::ui_Setup(void)
{
    this->ui_Stylesheet_Setup();

    this->setWindowTitle(WINDOW_TITLE);
    this->setCentralWidget(this->ui_Widgets);
    this->restoreGeometry(this->ui_Settings->value(APP_SETTINGS_GEOMETRY).toByteArray());
    this->statusBar()->showMessage("Core Library Hooked");

    this->ui_Widgets->setMinimumSize(WINDOW_WIDGET_SIZE_W, WINDOW_WIDGET_SIZE_H);

    this->ui_Widgets->addWidget(this->ui_Widget_OpenGL);
    this->ui_Widgets->addWidget(this->ui_Widget_RomBrowser);
    this->ui_Widgets->setCurrentIndex(1);
}

void MainWindow::ui_Stylesheet_Setup(void)
{
    QFile stylesheet(APP_STYLESHEET_FILE);

    if (!stylesheet.open(QIODevice::ReadOnly))
        return;

    this->setStyleSheet(stylesheet.readAll());
}

void MainWindow::ui_MessageBox(QString title, QString text, QString details = "")
{
    g_Logger.AddText("MainWindow::ui_MessageBox: " + title + ", " + text + ", " + details);

    QMessageBox msgBox;
    msgBox.setIcon(QMessageBox::Icon::Critical);
    msgBox.setWindowTitle(title);
    msgBox.setText(text);
    msgBox.setDetailedText(details);
    msgBox.addButton(QMessageBox::Ok);
    msgBox.show();
    msgBox.exec();
}

void MainWindow::ui_InEmulation(bool inEmulation)
{
    int index;

    if (inEmulation)
        index = 0;
    else
        index = 1;

    this->ui_Widgets->setCurrentIndex(index);

    this->menuBar_Setup(inEmulation);
}

void MainWindow::menuBar_Init(void)
{
    this->menuBar = new QMenuBar();
    this->menuBar_Actions_Init();
    this->menuBar_Actions_Connect();
}

void MainWindow::menuBar_Setup(bool inEmulation)
{
    this->menuBar_Actions_Setup(inEmulation);

    this->menuBar->clear();

    this->menuBar_Menu = this->menuBar->addMenu("File");
    this->menuBar_Menu->addAction(this->action_File_OpenRom);
    this->menuBar_Menu->addAction(this->action_File_OpenCombo);
    this->menuBar_Menu->addAction(this->action_File_EndEmulation);
    this->menuBar_Menu->addSeparator();
    this->menuBar_Menu->addAction(this->action_File_ChooseDirectory);
    this->menuBar_Menu->addAction(this->action_File_RefreshRomList);
    this->menuBar_Menu->addSeparator();
    this->menuBar_Menu->addAction(this->action_File_Exit);

    if (inEmulation)
    {
        this->menuBar_Menu = this->menuBar->addMenu("System");
        this->menuBar_Menu->setEnabled(inEmulation);
        this->menuBar_Menu->addAction(action_System_Reset);
        this->menuBar_Menu->addAction(action_System_Pause);
    }

    this->menuBar_Menu = this->menuBar->addMenu("Options");
    this->menuBar_Menu->addAction(this->action_Options_FullScreen);
    this->menuBar_Menu->addSeparator();
    this->menuBar_Menu->addAction(this->action_Options_ConfigGfx);
    this->menuBar_Menu->addAction(this->action_Options_ConfigAudio);
    this->menuBar_Menu->addAction(this->action_Options_ConfigRsp);
    this->menuBar_Menu->addAction(this->action_Options_ConfigControl);
    this->menuBar_Menu->addSeparator();
    this->menuBar_Menu->addAction(this->action_Options_Settings);

    this->menuBar_Menu = this->menuBar->addMenu("Help");
    this->menuBar_Menu->addAction(this->action_Help_Support);
    this->menuBar_Menu->addAction(this->action_Help_HomePage);
    this->menuBar_Menu->addSeparator();
    this->menuBar_Menu->addAction(this->action_Help_About);

    this->setMenuBar(menuBar);
}

void MainWindow::emulationThread_Init(void)
{
    this->emulationThread = new Thread::EmulationThread();
}

void MainWindow::emulationThread_Connect(void)
{
    connect(this->emulationThread, &Thread::EmulationThread::on_Emulation_Finished, this, &MainWindow::on_Emulation_Finished);
    connect(this->emulationThread, &Thread::EmulationThread::on_Emulation_Started, this, &MainWindow::on_Emulation_Started);
}

void MainWindow::menuBar_Actions_Init(void)
{
    this->action_File_OpenRom = new QAction();
    this->action_File_OpenCombo = new QAction();
    this->action_File_EndEmulation = new QAction();
    this->action_File_ChooseDirectory = new QAction();
    this->action_File_RefreshRomList = new QAction();
    this->action_File_Exit = new QAction();

    this->action_System_Reset = new QAction();
    this->action_System_Pause = new QAction();

    this->action_Options_FullScreen = new QAction();
    this->action_Options_ConfigGfx = new QAction();
    this->action_Options_ConfigAudio = new QAction();
    this->action_Options_ConfigRsp = new QAction();
    this->action_Options_ConfigControl = new QAction();
    this->action_Options_Settings = new QAction();

    this->action_Help_Support = new QAction();
    this->action_Help_HomePage = new QAction();
    this->action_Help_About = new QAction();
}

void MainWindow::menuBar_Actions_Setup(bool inEmulation)
{
    this->action_File_OpenRom->setText("Open ROM");
    this->action_File_OpenRom->setShortcut(QKeySequence("Ctrl+O"));
    this->action_File_OpenCombo->setText("Open Combo");
    this->action_File_OpenCombo->setShortcut(QKeySequence("Ctrl+Shift+O"));
    this->action_File_EndEmulation->setText("End Emulation");
    this->action_File_EndEmulation->setEnabled(inEmulation);
    this->action_File_ChooseDirectory->setText("Choose ROM Directory...");
    this->action_File_RefreshRomList->setText("Refresh ROM List");
    this->action_File_RefreshRomList->setShortcut(QKeySequence("F5"));
    this->action_File_Exit->setText("Exit");
    this->action_File_Exit->setShortcut(QKeySequence("Alt+F4"));

    this->action_System_Reset->setText("Reset");
    this->action_System_Reset->setShortcut(QKeySequence("F1"));
    this->action_System_Pause->setText("Pause");
    this->action_System_Pause->setShortcut(QKeySequence("F2"));

    this->action_Options_FullScreen->setText("Full Screen");
    this->action_Options_FullScreen->setEnabled(inEmulation);
    this->action_Options_FullScreen->setShortcut(QKeySequence("Alt+Return"));
    this->action_Options_ConfigGfx->setText("Configure Graphics Plugin...");
    this->action_Options_ConfigGfx->setEnabled(g_MupenApi.Core.HasPluginConfig(M64P::Wrapper::PluginType::Gfx));
    this->action_Options_ConfigAudio->setText("Configure Audio Plugin...");
    this->action_Options_ConfigAudio->setEnabled(g_MupenApi.Core.HasPluginConfig(M64P::Wrapper::PluginType::Audio));
    this->action_Options_ConfigRsp->setText("Configure RSP...");
    this->action_Options_ConfigRsp->setEnabled(g_MupenApi.Core.HasPluginConfig(M64P::Wrapper::PluginType::Rsp));
    this->action_Options_ConfigControl->setText("Configure Controller Plugin...");
    this->action_Options_ConfigControl->setEnabled(g_MupenApi.Core.HasPluginConfig(M64P::Wrapper::PluginType::Input));
    this->action_Options_Settings->setText("Settings...");
    this->action_Options_Settings->setShortcut(QKeySequence("Ctrl+T"));

    this->action_Help_Support->setText("Support Discord");
    this->action_Help_HomePage->setText("Homepage");
    this->action_Help_About->setText("About");
}

void MainWindow::menuBar_Actions_Connect(void)
{
    connect(this->action_File_OpenRom, &QAction::triggered, this, &MainWindow::on_Action_File_OpenRom);
    connect(this->action_File_OpenCombo, &QAction::triggered, this, &MainWindow::on_Action_File_OpenCombo);
    connect(this->action_File_EndEmulation, &QAction::triggered, this, &MainWindow::on_Action_File_EndEmulation);
    connect(this->action_File_ChooseDirectory, &QAction::triggered, this, &MainWindow::on_Action_File_ChooseDirectory);
    connect(this->action_File_RefreshRomList, &QAction::triggered, this, &MainWindow::on_Action_File_RefreshRomList);
    connect(this->action_File_Exit, &QAction::triggered, this, &MainWindow::on_Action_File_Exit);

    connect(this->action_System_Reset, &QAction::triggered, this, &MainWindow::on_Action_System_Reset);
    connect(this->action_System_Pause, &QAction::triggered, this, &MainWindow::on_Action_System_Pause);

    connect(this->action_Options_ConfigGfx, &QAction::triggered, this, &MainWindow::on_Action_Options_ConfigGfx);
    connect(this->action_Options_ConfigAudio, &QAction::triggered, this, &MainWindow::on_Action_Options_ConfigAudio);
    connect(this->action_Options_ConfigRsp, &QAction::triggered, this, &MainWindow::on_Action_Options_ConfigRsp);
    connect(this->action_Options_ConfigControl, &QAction::triggered, this, &MainWindow::on_Action_Options_ConfigControl);
    connect(this->action_Options_Settings, &QAction::triggered, this, &MainWindow::on_Action_Options_Settings);

    connect(this->action_Help_Support, &QAction::triggered, this, &MainWindow::on_Action_Help_Support);
    connect(this->action_Help_HomePage, &QAction::triggered, this, &MainWindow::on_Action_Help_HomePage);
    connect(this->action_Help_About, &QAction::triggered, this, &MainWindow::on_Action_Help_About);
}

#include <iostream>
void MainWindow::on_Action_File_OpenRom(void)
{
    if (this->emulationThread->isRunning())
        return;
        
    this->emulationThread->SetRomFile("/home/rosalie/Downloads/n64_roms/Super Mario 64 (U) [!].z64");
    this->emulationThread->start();
}

void MainWindow::on_Action_File_OpenCombo(void)
{
}

void MainWindow::on_Action_File_EndEmulation(void)
{
}

void MainWindow::on_Action_File_ChooseDirectory(void)
{
}

void MainWindow::on_Action_File_RefreshRomList(void)
{
    this->ui_Widget_RomBrowser->RefreshRomList();
}

void MainWindow::on_Action_File_Exit(void)
{
    this->close();
}

void MainWindow::on_Action_System_Reset(void)
{
}

void MainWindow::on_Action_System_Pause(void)
{
}

void MainWindow::on_Action_Options_FullScreen(void)
{
}

void MainWindow::on_Action_Options_ConfigGfx(void)
{
    g_MupenApi.Core.OpenPluginConfig(M64P::Wrapper::PluginType::Gfx);
}

void MainWindow::on_Action_Options_ConfigAudio(void)
{
    g_MupenApi.Core.OpenPluginConfig(M64P::Wrapper::PluginType::Audio);
}

void MainWindow::on_Action_Options_ConfigRsp(void)
{
    g_MupenApi.Core.OpenPluginConfig(M64P::Wrapper::PluginType::Rsp);
}

void MainWindow::on_Action_Options_ConfigControl(void)
{
    g_MupenApi.Core.OpenPluginConfig(M64P::Wrapper::PluginType::Input);
}

void MainWindow::on_Action_Options_Settings(void)
{
}

void MainWindow::on_Action_Help_Support(void)
{
    QDesktopServices::openUrl(QUrl(APP_URL_SUPPORT));
}

void MainWindow::on_Action_Help_HomePage(void)
{
    QDesktopServices::openUrl(QUrl(APP_URL_HOMEPAGE));
}

void MainWindow::on_Action_Help_About(void)
{
}

void MainWindow::on_Emulation_Started(void)
{
    this->ui_InEmulation(true);
}

void MainWindow::on_Emulation_Finished(bool ret)
{
    if (!ret)
        this->ui_MessageBox("Error", "EmulationThread::run Failed", this->emulationThread->GetLastError());

    this->ui_InEmulation(false);
}
