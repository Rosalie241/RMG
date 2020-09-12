#include "MainWindow.hpp"
#include "../Config.hpp"

#include <QMenuBar>
#include <QStatusBar>
#include <QCoreApplication>
#include <QDesktopServices>
#include <QUrl>
#include <QMessageBox>
#include <QString>
#include <QSettings>

MainWindow::MainWindow() : QMainWindow(nullptr)
{
}

MainWindow::~MainWindow()
{
    // Delete all UI elements
    if (!this->menuBar)
        return;
    delete this->menuBar;
    delete this->ui_Widget_OpenGL;
    delete this->ui_Widget_RomBrowser;
    delete this->ui_Settings;
}

bool MainWindow::Init()
{
    if (!this->logger.Init())
    {
        this->ui_MessageBox("Error", "Logger::Init Failed", this->logger.GetLastError());
        return false;
    }

    if (!this->m64p.Init())
    {
        this->ui_MessageBox("Error", "Mupen::Init Failed", this->m64p.GetLastError());
        return false;
    }

    if (!this->m64p.Setup())
    {
        this->ui_MessageBox("Error", "Mupen::Setup Failed", this->m64p.GetLastError());
        return false;
    }

    this->ui_Init();
    this->ui_Setup();

    this->menuBar_Init();
    this->menuBar_Setup(false);

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
    this->ui_Widget_RomBrowser = new RomBrowser(&this->m64p);
    this->ui_Settings = new QSettings(APP_SETTINGS_ORG, APP_SETTINGS_NAME);
}

void MainWindow::ui_Setup(void)
{
    this->setWindowTitle(WINDOW_TITLE);
    this->setMinimumSize(WINDOW_SIZE_W, WINDOW_SIZE_H);
    this->setCentralWidget(this->ui_Widget_RomBrowser);
    this->restoreGeometry(this->ui_Settings->value(APP_SETTINGS_GEOMETRY).toByteArray());
    this->statusBar()->showMessage("Core Library Hooked");
}

void MainWindow::ui_MessageBox(QString title, QString text, QString details = "")
{
    logger.AddText("MainWindow::ui_MessageBox: " + title + ", " + text + ", " + details + "\n");
    QMessageBox msgBox;
    msgBox.setWindowTitle(title);
    msgBox.setText("<p align='center'>" + text + "</p>");
    msgBox.setDetailedText(details);
    msgBox.addButton(QMessageBox::Ok);
    msgBox.show();
    msgBox.exec();
}

void MainWindow::menuBar_Init(void)
{
    this->menuBar = new QMenuBar();
    this->menuBar_Actions_Init();
}

void MainWindow::menuBar_Setup(bool inEmulation)
{
    this->menuBar_Actions_Setup(inEmulation);
    this->menuBar_Actions_Connect();

    this->menuBar_Menu = this->menuBar->addMenu("File");
    this->menuBar_Menu->addAction(this->action_File_OpenRom);
    this->menuBar_Menu->addAction(this->action_File_OpenCombo);
    this->menuBar_Menu->addAction(this->action_File_EndEmulation);
    this->menuBar_Menu->addSeparator();
    this->menuBar_Menu->addAction(this->action_File_ChooseDirectory);
    this->menuBar_Menu->addAction(this->action_File_RefreshRomList);
    this->menuBar_Menu->addSeparator();
    this->menuBar_Menu->addAction(this->action_File_Exit);

    this->menuBar_Menu = this->menuBar->addMenu("System");
    this->menuBar_Menu->setEnabled(inEmulation);
    this->menuBar_Menu->addAction(action_System_Reset);
    this->menuBar_Menu->addAction(action_System_Pause);

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
    this->action_Options_ConfigGfx->setEnabled(this->m64p.HasPluginConfig(PluginType::Gfx));
    this->action_Options_ConfigAudio->setText("Configure Audio Plugin...");
    this->action_Options_ConfigAudio->setEnabled(this->m64p.HasPluginConfig(PluginType::Audio));
    this->action_Options_ConfigRsp->setText("Configure RSP...");
    this->action_Options_ConfigRsp->setEnabled(this->m64p.HasPluginConfig(PluginType::Rsp));
    this->action_Options_ConfigControl->setText("Configure Controller Plugin...");
    this->action_Options_ConfigControl->setEnabled(this->m64p.HasPluginConfig(PluginType::Input));
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

void MainWindow::on_Action_File_OpenRom(void)
{
    if (!m64p.OpenRom("/home/rosalie/Downloads/Resident Evil 2 (USA) (Rev A).n64"))
    {
        this->ui_MessageBox("Error", "Mupen::OpenRom Failed", m64p.GetLastError());
    }
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
    this->m64p.OpenPluginConfig(PluginType::Gfx);
}

void MainWindow::on_Action_Options_ConfigAudio(void)
{
    this->m64p.OpenPluginConfig(PluginType::Audio);
}

void MainWindow::on_Action_Options_ConfigRsp(void)
{
    this->m64p.OpenPluginConfig(PluginType::Rsp);
}

void MainWindow::on_Action_Options_ConfigControl(void)
{
    this->m64p.OpenPluginConfig(PluginType::Input);
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
