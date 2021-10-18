/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "MainWindow.hpp"
#include "../Utilities/QtKeyToSdl2Key.hpp"
#include "Config.hpp"
#include "Globals.hpp"
#include "UserInterface/EventFilter.hpp"

#include <QCoreApplication>
#include <QDesktopServices>
#include <QFileDialog>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QSettings>
#include <QStatusBar>
#include <QString>
#include <QUrl>
#include <QActionGroup> 

using namespace UserInterface;
using namespace M64P::Wrapper;

MainWindow::MainWindow() : QMainWindow(nullptr)
{
}

MainWindow::~MainWindow()
{
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
        this->ui_MessageBox("Error", "Api::Init Failed", g_MupenApi.GetLastError());
        return false;
    }

    g_Settings.LoadDefaults();

    QString dataDir = g_Settings.GetStringValue(SettingsID::Core_UserDataDirOverride);
    QString cacheDir = g_Settings.GetStringValue(SettingsID::Core_UserCacheDirOverride);
    if (g_Settings.GetBoolValue(SettingsID::Core_OverrideUserDirs))
    {
        g_MupenApi.Config.OverrideUserPaths(dataDir, cacheDir);
    }

    // create mupen64plus directories
    // when they don't exist
    const SettingsID directorySettings[] =
    {
        SettingsID::Core_ScreenshotPath,
        SettingsID::Core_UserDataDirOverride,
        SettingsID::Core_UserCacheDirOverride,
    };
    for (const SettingsID settingId : directorySettings)
    {
        QString directory = g_Settings.GetStringValue(settingId);
        if (!QDir().exists(directory))
        {
            if (!QDir().mkdir(directory))
            {
                QString error = "Failed to create the \"" + directory + "\" directory";
                this->ui_MessageBox("Error", "MainWindow::Init Failed", error);
                return false;
            }
        }
    }

    this->ui_Init();
    this->ui_Setup();

    this->ui_Actions_Init();
    this->ui_Actions_Connect();

    this->menuBar_Init();
    this->menuBar_Setup(false, false);

    this->emulationThread_Init();
    this->emulationThread_Connect();

    g_EmuThread = this->emulationThread;

    connect(&g_MupenApi.Core, &Core::on_Core_DebugCallback, this, &MainWindow::on_Core_DebugCallback);

    return true;
}

void MainWindow::OpenROM(QString file)
{
    this->emulationThread_Launch(file);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    this->on_Action_File_EndEmulation();

    while (g_EmuThread->isRunning())
    {
        QCoreApplication::processEvents();
    }

    g_Settings.SetValue(SettingsID::RomBrowser_Geometry,
                        QString(this->saveGeometry().toBase64().toStdString().c_str()));

    QMainWindow::closeEvent(event);
}

void MainWindow::ui_Init(void)
{
    this->ui_Icon = QIcon(":Resource/RMG.png");

    this->ui_Widgets = new QStackedWidget(this);
    this->ui_Widget_RomBrowser = new Widget::RomBrowserWidget(this);
    this->ui_Widget_OpenGL = new Widget::OGLWidget(this);
    this->ui_EventFilter = new EventFilter(this);
    this->ui_StatusBar_Label = new QLabel(this);

    QString dir;
    dir = g_Settings.GetStringValue(SettingsID::RomBrowser_Directory);

    this->ui_Widget_RomBrowser->SetDirectory(dir);
    this->ui_Widget_RomBrowser->RefreshRomList();

    connect(this->ui_Widget_RomBrowser, &Widget::RomBrowserWidget::on_RomBrowser_Select, this,
            &MainWindow::on_RomBrowser_Selected);
    connect(this->ui_Widget_RomBrowser, &Widget::RomBrowserWidget::on_RomBrowser_FileDropped, this,
            &MainWindow::on_EventFilter_FileDropped);

    connect(this->ui_EventFilter, &EventFilter::on_EventFilter_KeyPressed, this,
            &MainWindow::on_EventFilter_KeyPressed);
    connect(this->ui_EventFilter, &EventFilter::on_EventFilter_KeyReleased, this,
            &MainWindow::on_EventFilter_KeyReleased);
    connect(this->ui_EventFilter, &EventFilter::on_EventFilter_FileDropped, this,
            &MainWindow::on_EventFilter_FileDropped);
    /*
    connect(this->ui_EventFilter, &EventFilter::on_EventFilter_FocusIn, this, &MainWindow::on_EventFilter_FocusIn);
    connect(this->ui_EventFilter, &EventFilter::on_EventFilter_FocusOut, this, &MainWindow::on_EventFilter_FocusOut);
    */
}

void MainWindow::ui_Setup(void)
{
    this->ui_Stylesheet_Setup();

    this->setWindowIcon(this->ui_Icon);
    this->setWindowTitle(WINDOW_TITLE);
    this->setCentralWidget(this->ui_Widgets);

    QString geometry;
    geometry = g_Settings.GetStringValue(SettingsID::RomBrowser_Geometry);

    if (!geometry.isEmpty())
    {
        this->restoreGeometry(QByteArray::fromBase64(geometry.toLocal8Bit()));
    }
    else
    {
        this->resize(610, 540);
    }

    this->statusBar()->setHidden(false);
    this->statusBar()->addPermanentWidget(this->ui_StatusBar_Label, 1);
    this->ui_TimerTimeout = g_Settings.GetIntValue(SettingsID::GUI_StatusbarMessageDuration);

    this->ui_Widgets->addWidget(this->ui_Widget_RomBrowser);
    this->ui_Widgets->addWidget(this->ui_Widget_OpenGL->GetWidget());

    this->ui_Widgets->setCurrentIndex(0);

    this->setFocusPolicy(Qt::FocusPolicy::StrongFocus);
    this->installEventFilter(this->ui_EventFilter);
    this->ui_Widget_OpenGL->installEventFilter(this->ui_EventFilter);
}

void MainWindow::ui_Stylesheet_Setup(void)
{
    QFile stylesheet(APP_STYLESHEET_FILE);

    if (!stylesheet.open(QIODevice::ReadOnly))
    {
        return;
    }

    this->setStyleSheet(stylesheet.readAll());
}

void MainWindow::ui_MessageBox(QString title, QString text, QString details = "")
{
    g_Logger.AddText("MainWindow::ui_MessageBox: " + title + ", " + text + ", " + details);

    QMessageBox msgBox(this);
    msgBox.setIcon(QMessageBox::Icon::Critical);
    msgBox.setWindowTitle(title);
    msgBox.setText(text);
    msgBox.setDetailedText(details);
    msgBox.addButton(QMessageBox::Ok);
    msgBox.exec();
}

void MainWindow::ui_InEmulation(bool inEmulation, bool isPaused)
{
    if (!this->ui_NoSwitchToRomBrowser)
    {
        this->menuBar_Setup(inEmulation, isPaused);
    }

    if (inEmulation)
    {
        RomInfo_t info = {0};
        g_MupenApi.Core.GetRomInfo(&info);

        if (!QString(info.Settings.goodname).isEmpty())
        {
            this->setWindowTitle(info.Settings.goodname + QString(" - ") + QString(WINDOW_TITLE));
        }

        this->ui_Widgets->setCurrentIndex(1);
        this->ui_SaveGeometry();
    }
    else if (!this->ui_NoSwitchToRomBrowser)
    {
        this->setWindowTitle(QString(WINDOW_TITLE));
        this->ui_Widgets->setCurrentIndex(0);
        this->ui_LoadGeometry();
    }
    else
    {
        this->ui_NoSwitchToRomBrowser = false;
    }

    // update timer timeout
    this->ui_TimerTimeout = g_Settings.GetIntValue(SettingsID::GUI_StatusbarMessageDuration);
}

void MainWindow::ui_SaveGeometry(void)
{
    if (this->ui_Geometry_Saved)
    {
        return;
    }

    this->ui_Geometry = this->saveGeometry();
    this->ui_Geometry_Saved = true;
}

void MainWindow::ui_LoadGeometry(void)
{
    if (!this->ui_Geometry_Saved)
    {
        return;
    }

    // restore from fullscreen aswell
    if (this->isFullScreen())
    {
        this->showNormal();
    }

    if (this->menuBar->isHidden())
    {
        this->menuBar->show();
    }

    if (this->statusBar()->isHidden())
    {
        this->statusBar()->show();
    }

    this->setMinimumSize(0, 0);
    this->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
    this->restoreGeometry(this->ui_Geometry);
    this->ui_Geometry_Saved = false;
}

void MainWindow::menuBar_Init(void)
{
    this->menuBar = new QMenuBar(this);
}

void MainWindow::menuBar_Setup(bool inEmulation, bool isPaused)
{
    this->ui_Actions_Setup(inEmulation, isPaused);

    this->menuBar->clear();

    this->menuBar_Menu = this->menuBar->addMenu("File");
    this->menuBar_Menu->addAction(this->action_File_OpenRom);
    this->menuBar_Menu->addAction(this->action_File_OpenCombo);
    this->menuBar_Menu->addAction(this->action_File_RomInfo);
    this->menuBar_Menu->addSeparator();
    this->menuBar_Menu->addAction(this->action_File_StartEmulation);
    this->menuBar_Menu->addAction(this->action_File_EndEmulation);
    this->menuBar_Menu->addSeparator();

    // TODO: Add language support
    QMenu *langMenu = new QMenu("Language", this);
    langMenu->addAction("English");
    this->menuBar_Menu->addMenu(langMenu);

    if (!inEmulation)
    {
        this->menuBar_Menu->addSeparator();
        this->menuBar_Menu->addAction(this->action_File_ChooseDirectory);
        this->menuBar_Menu->addAction(this->action_File_RefreshRomList);
        this->menuBar_Menu->addSeparator();

        // TODO: Actually implement this
        /*
        this->menuBar_Menu->addAction(this->action_File_RecentRom);
        this->menuBar_Menu->addAction(this->action_File_RecentRomDirectories);
        */
    }
    this->menuBar_Menu->addSeparator();
    this->menuBar_Menu->addAction(this->action_File_Exit);

    if (inEmulation)
    {
        QMenu *resetMenu = new QMenu("Reset", this);
        resetMenu->addAction(action_System_SoftReset);
        resetMenu->addAction(action_System_HardReset);

        this->menuBar_Menu = this->menuBar->addMenu("System");
        this->menuBar_Menu->addMenu(resetMenu);
        this->menuBar_Menu->addAction(this->action_System_Pause);
        this->menuBar_Menu->addAction(this->action_System_GenerateBitmap);
        this->menuBar_Menu->addSeparator();
        this->menuBar_Menu->addAction(this->action_System_LimitFPS);
        this->menuBar_Menu->addSeparator();
        this->menuBar_Menu->addAction(this->action_System_SwapDisk);
        this->menuBar_Menu->addSeparator();
        this->menuBar_Menu->addAction(this->action_System_SaveState);
        this->menuBar_Menu->addAction(this->action_System_SaveAs);
        this->menuBar_Menu->addAction(this->action_System_LoadState);
        this->menuBar_Menu->addAction(this->action_System_Load);
        this->menuBar_Menu->addSeparator();
        this->menuBar_Menu->addMenu(this->menu_System_CurrentSaveState);

        this->menu_System_CurrentSaveState->clear();

        QActionGroup *slotActionGroup = new QActionGroup(this);
        QList<QAction *> slotActions;
        QAction *slotAction;
        int currentSaveslot = g_MupenApi.Core.GetSaveSlot();
        for (int i = 0; i < 10; i++)
        {
            slotActions.append(new QAction(this));
            slotAction = slotActions.at(i);

            slotAction->setText("Slot " + QString::number(i));
            slotAction->setCheckable(true);
            slotAction->setChecked(i == currentSaveslot);
            slotAction->setActionGroup(slotActionGroup);

            connect(slotAction, &QAction::triggered, [=](bool checked)
            {
                if (checked)
                {
                    int slot = slotAction->text().split(" ").last().toInt();
                    this->on_Action_System_CurrentSaveState(slot);
                }
            });

            this->menu_System_CurrentSaveState->addAction(slotAction);
        }

        this->menuBar_Menu->addSeparator();
        this->menuBar_Menu->addAction(this->action_System_Cheats);
        this->menuBar_Menu->addAction(this->action_System_GSButton);
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
    this->menuBar_Menu->addAction(this->action_Help_HomePage);
    this->menuBar_Menu->addSeparator();
    this->menuBar_Menu->addAction(this->action_Help_About);

    this->setMenuBar(menuBar);
}

void MainWindow::emulationThread_Init(void)
{
    this->emulationThread = new Thread::EmulationThread(this);
}

void MainWindow::emulationThread_Connect(void)
{
    connect(this->emulationThread, &Thread::EmulationThread::on_Emulation_Finished, this,
            &MainWindow::on_Emulation_Finished);
    connect(this->emulationThread, &Thread::EmulationThread::on_Emulation_Started, this,
            &MainWindow::on_Emulation_Started);

    connect(this->emulationThread, &Thread::EmulationThread::on_VidExt_Init, this, &MainWindow::on_VidExt_Init,
            Qt::BlockingQueuedConnection);
    connect(this->emulationThread, &Thread::EmulationThread::on_VidExt_SetupOGL, this, &MainWindow::on_VidExt_SetupOGL,
            Qt::BlockingQueuedConnection);
    connect(this->emulationThread, &Thread::EmulationThread::on_VidExt_SetMode, this, &MainWindow::on_VidExt_SetMode,
            Qt::BlockingQueuedConnection);
    connect(this->emulationThread, &Thread::EmulationThread::on_VidExt_SetWindowedModeWithRate, this,
            &MainWindow::on_VidExt_SetWindowedModeWithRate, Qt::BlockingQueuedConnection);
    connect(this->emulationThread, &Thread::EmulationThread::on_VidExt_SetFullscreenModeWithRate, this,
            &MainWindow::on_VidExt_SetFullscreenModeWithRate, Qt::BlockingQueuedConnection);
    connect(this->emulationThread, &Thread::EmulationThread::on_VidExt_ResizeWindow, this,
            &MainWindow::on_VidExt_ResizeWindow, Qt::BlockingQueuedConnection);
    connect(this->emulationThread, &Thread::EmulationThread::on_VidExt_SetCaption, this,
            &MainWindow::on_VidExt_SetCaption, Qt::BlockingQueuedConnection);
    connect(this->emulationThread, &Thread::EmulationThread::on_VidExt_ToggleFS, this, &MainWindow::on_VidExt_ToggleFS,
            Qt::BlockingQueuedConnection);
    connect(this->emulationThread, &Thread::EmulationThread::on_VidExt_Quit, this, &MainWindow::on_VidExt_Quit,
            Qt::BlockingQueuedConnection);
}

void MainWindow::emulationThread_Launch(QString cartRom, QString diskRom)
{
    g_MupenApi.Config.Save();

    if (this->emulationThread->isRunning())
    {
        this->on_Action_File_EndEmulation();

        while (this->emulationThread->isRunning())
        {
            QCoreApplication::processEvents();
        }
    }

    ui_RefreshRomListAfterEmulation = this->ui_Widget_RomBrowser->IsRefreshingRomList();
    if (ui_RefreshRomListAfterEmulation)
    {
        this->ui_Widget_RomBrowser->StopRefreshRomList();
    }

    this->ui_AllowManualResizing = g_Settings.GetBoolValue(SettingsID::GUI_AllowManualResizing);
    this->ui_HideCursorInEmulation = g_Settings.GetBoolValue(SettingsID::GUI_HideCursorInEmulation);

    this->ui_Widget_OpenGL->SetAllowResizing(this->ui_AllowManualResizing);
    this->ui_Widget_OpenGL->SetHideCursor(this->ui_HideCursorInEmulation);

    this->emulationThread->SetRomFile(cartRom);
    this->emulationThread->SetDiskFile(diskRom);
    this->emulationThread->start();
}

void MainWindow::emulationThread_Launch(QString file)
{
    this->emulationThread_Launch(file, "");
}

void MainWindow::ui_Actions_Init(void)
{
    this->action_File_OpenRom = new QAction(this);
    this->action_File_OpenCombo = new QAction(this);
    this->action_File_RomInfo = new QAction(this);
    this->action_File_StartEmulation = new QAction(this);
    this->action_File_EndEmulation = new QAction(this);
    this->action_File_Language = new QAction(this);
    this->action_File_ChooseDirectory = new QAction(this);
    this->action_File_RefreshRomList = new QAction(this);
    this->action_File_RecentRom = new QAction(this);
    this->action_File_RecentRomDirectories = new QAction(this);
    this->action_File_Exit = new QAction(this);

    this->action_System_SoftReset = new QAction(this);
    this->action_System_HardReset = new QAction(this);
    this->action_System_Pause = new QAction(this);
    this->action_System_GenerateBitmap = new QAction(this);
    this->action_System_LimitFPS = new QAction(this);
    this->action_System_SwapDisk = new QAction(this);
    this->action_System_SaveState = new QAction(this);
    this->action_System_SaveAs = new QAction(this);
    this->action_System_LoadState = new QAction(this);
    this->action_System_Load = new QAction(this);
    this->menu_System_CurrentSaveState = new QMenu(this);
    this->action_System_Cheats = new QAction(this);
    this->action_System_GSButton = new QAction(this);

    this->action_Options_FullScreen = new QAction(this);
    this->action_Options_ConfigGfx = new QAction(this);
    this->action_Options_ConfigAudio = new QAction(this);
    this->action_Options_ConfigRsp = new QAction(this);
    this->action_Options_ConfigControl = new QAction(this);
    this->action_Options_Settings = new QAction(this);

    this->action_Help_HomePage = new QAction(this);
    this->action_Help_About = new QAction(this);
}

void MainWindow::ui_Actions_Setup(bool inEmulation, bool isPaused)
{
    QString keyBinding;
    keyBinding = g_Settings.GetStringValue(SettingsID::KeyBinding_OpenROM);
    this->action_File_OpenRom->setText("Open ROM");
    this->action_File_OpenRom->setShortcut(QKeySequence(keyBinding));
    keyBinding = g_Settings.GetStringValue(SettingsID::KeyBinding_OpenCombo);
    this->action_File_OpenCombo->setText("Open Combo");
    this->action_File_OpenCombo->setShortcut(QKeySequence(keyBinding));
    this->action_File_RomInfo->setText("ROM Info....");
    keyBinding = g_Settings.GetStringValue(SettingsID::KeyBinding_StartEmulation);
    this->action_File_StartEmulation->setText("Start Emulation");
    this->action_File_StartEmulation->setShortcut(QKeySequence(keyBinding));
    this->action_File_StartEmulation->setEnabled(!inEmulation);
    keyBinding = g_Settings.GetStringValue(SettingsID::KeyBinding_EndEmulation);
    this->action_File_EndEmulation->setText("End Emulation");
    this->action_File_EndEmulation->setShortcut(QKeySequence(keyBinding));
    this->action_File_EndEmulation->setEnabled(inEmulation);
    this->action_File_Language->setText("Language");
    this->action_File_ChooseDirectory->setText("Choose ROM Directory...");
    keyBinding = g_Settings.GetStringValue(SettingsID::KeyBinding_RefreshROMList);
    this->action_File_RefreshRomList->setText("Refresh ROM List");
    this->action_File_RefreshRomList->setShortcut(QKeySequence(keyBinding));
    this->action_File_RecentRom->setText("Recent ROM");
    this->action_File_RecentRomDirectories->setText("Recent ROM Directories");
    keyBinding = g_Settings.GetStringValue(SettingsID::KeyBinding_Exit);
    this->action_File_Exit->setText("Exit");
    this->action_File_Exit->setShortcut(QKeySequence(keyBinding));

    keyBinding = g_Settings.GetStringValue(SettingsID::KeyBinding_SoftReset);
    this->action_System_SoftReset->setText("Soft Reset");
    this->action_System_SoftReset->setShortcut(QKeySequence(keyBinding));
    keyBinding = g_Settings.GetStringValue(SettingsID::KeyBinding_HardReset);
    this->action_System_HardReset->setText("Hard Reset");
    this->action_System_HardReset->setShortcut(QKeySequence(keyBinding));
    keyBinding = g_Settings.GetStringValue(SettingsID::KeyBinding_Resume);
    this->action_System_Pause->setText(isPaused ? "Resume" : "Pause");
    this->action_System_Pause->setShortcut(QKeySequence(keyBinding));
    keyBinding = g_Settings.GetStringValue(SettingsID::KeyBinding_GenerateBitmap);
    this->action_System_GenerateBitmap->setText("Generate Bitmap");
    this->action_System_GenerateBitmap->setShortcut(QKeySequence(keyBinding));
    keyBinding = g_Settings.GetStringValue(SettingsID::KeyBinding_LimitFPS);
    this->action_System_LimitFPS->setText("Limit FPS");
    this->action_System_LimitFPS->setShortcut(QKeySequence(keyBinding));
    this->action_System_LimitFPS->setCheckable(true);
    this->action_System_LimitFPS->setChecked(g_MupenApi.Core.GetSpeedLimiterState());
    keyBinding = g_Settings.GetStringValue(SettingsID::KeyBinding_SwapDisk);
    this->action_System_SwapDisk->setText("Swap Disk");
    this->action_System_SwapDisk->setShortcut(QKeySequence(keyBinding));
    this->action_System_SwapDisk->setEnabled(false);
    keyBinding = g_Settings.GetStringValue(SettingsID::KeyBinding_SaveState);
    this->action_System_SaveState->setText("Save State");
    this->action_System_SaveState->setShortcut(QKeySequence(keyBinding));
    keyBinding = g_Settings.GetStringValue(SettingsID::KeyBinding_SaveAs);
    this->action_System_SaveAs->setText("Save As...");
    this->action_System_SaveAs->setShortcut(QKeySequence(keyBinding));
    keyBinding = g_Settings.GetStringValue(SettingsID::KeyBinding_LoadState);
    this->action_System_LoadState->setText("Load State");
    this->action_System_LoadState->setShortcut(QKeySequence(keyBinding));
    keyBinding = g_Settings.GetStringValue(SettingsID::KeyBinding_Load);
    this->action_System_Load->setText("Load...");
    this->action_System_Load->setShortcut(QKeySequence(keyBinding));
    this->menu_System_CurrentSaveState->setTitle("Current Save State");
    keyBinding = g_Settings.GetStringValue(SettingsID::KeyBinding_Cheats);
    this->action_System_Cheats->setText("Cheats...");
    this->action_System_Cheats->setShortcut(QKeySequence(keyBinding));
    keyBinding = g_Settings.GetStringValue(SettingsID::KeyBinding_GSButton);
    this->action_System_GSButton->setText("GS Button");
    this->action_System_GSButton->setShortcut(QKeySequence(keyBinding));

    keyBinding = g_Settings.GetStringValue(SettingsID::KeyBinding_Fullscreen);
    this->action_Options_FullScreen->setText("Fullscreen");
    this->action_Options_FullScreen->setEnabled(inEmulation);
    this->action_Options_FullScreen->setShortcut(QKeySequence(keyBinding));
    this->action_Options_ConfigGfx->setText("Graphics Settings");
    this->action_Options_ConfigGfx->setEnabled(g_MupenApi.Core.HasPluginConfig(M64P::Wrapper::PluginType::Gfx));
    this->action_Options_ConfigAudio->setText("Audio Settings");
    this->action_Options_ConfigAudio->setEnabled(g_MupenApi.Core.HasPluginConfig(M64P::Wrapper::PluginType::Audio));
    this->action_Options_ConfigRsp->setText("RSP Settings");
    this->action_Options_ConfigRsp->setEnabled(g_MupenApi.Core.HasPluginConfig(M64P::Wrapper::PluginType::Rsp));
    this->action_Options_ConfigControl->setText("Input Settings");
    this->action_Options_ConfigControl->setEnabled(g_MupenApi.Core.HasPluginConfig(M64P::Wrapper::PluginType::Input));
    keyBinding = g_Settings.GetStringValue(SettingsID::KeyBinding_Settings);
    this->action_Options_Settings->setText("Settings");
    this->action_Options_Settings->setShortcut(QKeySequence(keyBinding));

    this->action_Help_HomePage->setText("Website");
    this->action_Help_About->setText("About RMG");
}

void MainWindow::ui_Actions_Add(void)
{
    this->addAction(this->action_File_OpenRom);
    this->addAction(this->action_File_OpenCombo);
    this->addAction(this->action_File_EndEmulation);
    this->addAction(this->action_File_ChooseDirectory);
    // this->addAction(this->action_File_RefreshRomList);
    this->addAction(this->action_File_Exit);
    this->addAction(this->action_System_SoftReset);
    this->addAction(this->action_System_HardReset);
    this->addAction(this->action_System_Pause);
    this->addAction(this->action_System_GenerateBitmap);
    this->addAction(this->action_System_LimitFPS);
    this->addAction(this->action_System_SwapDisk);
    this->addAction(this->action_System_SaveState);
    this->addAction(this->action_System_SaveAs);
    this->addAction(this->action_System_LoadState);
    this->addAction(this->action_System_Load);
    this->addAction(this->action_System_Cheats);
    this->addAction(this->action_System_GSButton);
    this->addAction(this->action_Options_FullScreen);
    this->addAction(this->action_Options_ConfigGfx);
    this->addAction(this->action_Options_ConfigAudio);
    this->addAction(this->action_Options_ConfigRsp);
    this->addAction(this->action_Options_ConfigControl);
    this->addAction(this->action_Options_Settings);
    this->addAction(this->action_Help_HomePage);
    this->addAction(this->action_Help_About);
}

void MainWindow::ui_Actions_Remove(void)
{
    this->removeAction(this->action_File_OpenRom);
    this->removeAction(this->action_File_OpenCombo);
    this->removeAction(this->action_File_EndEmulation);
    this->removeAction(this->action_File_ChooseDirectory);
    // this->removeAction(this->action_File_RefreshRomList);
    this->removeAction(this->action_File_Exit);
    this->removeAction(this->action_System_SoftReset);
    this->removeAction(this->action_System_HardReset);
    this->removeAction(this->action_System_Pause);
    this->removeAction(this->action_System_GenerateBitmap);
    this->removeAction(this->action_System_LimitFPS);
    this->removeAction(this->action_System_SwapDisk);
    this->removeAction(this->action_System_SaveState);
    this->removeAction(this->action_System_SaveAs);
    this->removeAction(this->action_System_LoadState);
    this->removeAction(this->action_System_Load);
    this->removeAction(this->action_System_Cheats);
    this->removeAction(this->action_System_GSButton);
    this->removeAction(this->action_Options_FullScreen);
    this->removeAction(this->action_Options_ConfigGfx);
    this->removeAction(this->action_Options_ConfigAudio);
    this->removeAction(this->action_Options_ConfigRsp);
    this->removeAction(this->action_Options_ConfigControl);
    this->removeAction(this->action_Options_Settings);
    this->removeAction(this->action_Help_HomePage);
    this->removeAction(this->action_Help_About);
}

void MainWindow::ui_Actions_Connect(void)
{
    connect(this->action_File_OpenRom, &QAction::triggered, this, &MainWindow::on_Action_File_OpenRom);
    connect(this->action_File_OpenCombo, &QAction::triggered, this, &MainWindow::on_Action_File_OpenCombo);
    connect(this->action_File_EndEmulation, &QAction::triggered, this, &MainWindow::on_Action_File_EndEmulation);
    connect(this->action_File_ChooseDirectory, &QAction::triggered, this, &MainWindow::on_Action_File_ChooseDirectory);
    connect(this->action_File_RefreshRomList, &QAction::triggered, this, &MainWindow::on_Action_File_RefreshRomList);
    connect(this->action_File_Exit, &QAction::triggered, this, &MainWindow::on_Action_File_Exit);

    connect(this->action_System_SoftReset, &QAction::triggered, this, &MainWindow::on_Action_System_SoftReset);
    connect(this->action_System_HardReset, &QAction::triggered, this, &MainWindow::on_Action_System_HardReset);
    connect(this->action_System_Pause, &QAction::triggered, this, &MainWindow::on_Action_System_Pause);
    connect(this->action_System_GenerateBitmap, &QAction::triggered, this,
            &MainWindow::on_Action_System_GenerateBitmap);
    connect(this->action_System_LimitFPS, &QAction::triggered, this, &MainWindow::on_Action_System_LimitFPS);
    connect(this->action_System_SwapDisk, &QAction::triggered, this, &MainWindow::on_Action_System_SwapDisk);
    connect(this->action_System_SaveState, &QAction::triggered, this, &MainWindow::on_Action_System_SaveState);
    connect(this->action_System_SaveAs, &QAction::triggered, this, &MainWindow::on_Action_System_SaveAs);
    connect(this->action_System_LoadState, &QAction::triggered, this, &MainWindow::on_Action_System_LoadState);
    connect(this->action_System_Load, &QAction::triggered, this, &MainWindow::on_Action_System_Load);
    connect(this->action_System_Cheats, &QAction::triggered, this, &MainWindow::on_Action_System_Cheats);
    connect(this->action_System_GSButton, &QAction::triggered, this, &MainWindow::on_Action_System_GSButton);

    connect(this->action_Options_FullScreen, &QAction::triggered, this, &MainWindow::on_Action_Options_FullScreen);
    connect(this->action_Options_ConfigGfx, &QAction::triggered, this, &MainWindow::on_Action_Options_ConfigGfx);
    connect(this->action_Options_ConfigAudio, &QAction::triggered, this, &MainWindow::on_Action_Options_ConfigAudio);
    connect(this->action_Options_ConfigRsp, &QAction::triggered, this, &MainWindow::on_Action_Options_ConfigRsp);
    connect(this->action_Options_ConfigControl, &QAction::triggered, this,
            &MainWindow::on_Action_Options_ConfigControl);
    connect(this->action_Options_Settings, &QAction::triggered, this, &MainWindow::on_Action_Options_Settings);

    connect(this->action_Help_HomePage, &QAction::triggered, this, &MainWindow::on_Action_Help_HomePage);
    connect(this->action_Help_About, &QAction::triggered, this, &MainWindow::on_Action_Help_About);
}

void MainWindow::timerEvent(QTimerEvent *event)
{
    this->ui_StatusBar_Label->clear();
}

void MainWindow::on_EventFilter_KeyPressed(QKeyEvent *event)
{
    if (!g_MupenApi.Core.IsEmulationRunning())
    {
        QMainWindow::keyPressEvent(event);
        return;
    }

    int key = Utilities::QtKeyToSdl2Key(event->key());
    int mod = Utilities::QtModKeyToSdl2ModKey(event->modifiers());

    g_MupenApi.Core.SetKeyDown(key, mod);
}

void MainWindow::on_EventFilter_KeyReleased(QKeyEvent *event)
{
    if (!g_MupenApi.Core.IsEmulationRunning())
    {
        QMainWindow::keyReleaseEvent(event);
        return;
    }

    int key = Utilities::QtKeyToSdl2Key(event->key());
    int mod = Utilities::QtModKeyToSdl2ModKey(event->modifiers());

    g_MupenApi.Core.SetKeyUp(key, mod);
}

void MainWindow::on_EventFilter_FileDropped(QDropEvent *event)
{
    QString file;
    const QMimeData *mimeData = event->mimeData();

    if (!mimeData->hasUrls())
    {
        return;
    }

    file = mimeData->urls().first().toLocalFile();

    if (this->ui_Widgets->currentIndex() != 0)
    {
        this->ui_NoSwitchToRomBrowser = true;
    }

    this->emulationThread_Launch(file);
}

/* TODO for some day
void MainWindow::on_EventFilter_FocusIn(QFocusEvent *event)
{
    std::cout << "MainWindow::on_EventFilter_FocusIn" << std::endl;
    if (g_Settings.GetBoolValue(SettingsID::GUI_ResumeEmulationOnFocus))
    {
        if (g_MupenApi.Core.isEmulationPaused())
            this->on_Action_System_Pause();
    }

    QMainWindow::focusInEvent(event);
}

void MainWindow::on_EventFilter_FocusOut(QFocusEvent *event)
{
    std::cout << "MainWindow::on_EventFilter_FocusOut" << std::endl;
    if (g_Settings.GetBoolValue(SettingsID::GUI_PauseEmulationOnFocusLoss))
    {
        if (g_MupenApi.Core.IsEmulationRunning())
            this->on_Action_System_Pause();
    }

    QMainWindow::focusOutEvent(event);
}
*/

void MainWindow::on_Action_File_OpenRom(void)
{
    bool isRunning = g_MupenApi.Core.IsEmulationRunning();
    bool isPaused = g_MupenApi.Core.isEmulationPaused();

    if (isRunning && !isPaused)
    {
        this->on_Action_System_Pause();
    }

    QFileDialog dialog(this);
    int ret;
    QString dir;

    dialog.setFileMode(QFileDialog::FileMode::ExistingFile);
    dialog.setNameFilter("N64 ROMs & Disks (*.n64 *.z64 *.v64 *.ndd *.d64 *.zip)");

    ret = dialog.exec();
    if (!ret)
    {
        if (isRunning && !isPaused)
        {
            this->on_Action_System_Pause();
        }
        return;
    }

    if (this->ui_Widgets->currentIndex() != 0)
    {
        this->ui_NoSwitchToRomBrowser = true;
    }

    this->emulationThread_Launch(dialog.selectedFiles().first());
}

void MainWindow::on_Action_File_OpenCombo(void)
{
    bool isRunning = g_MupenApi.Core.IsEmulationRunning();
    bool isPaused = g_MupenApi.Core.isEmulationPaused();

    if (isRunning && !isPaused)
    {
        this->on_Action_System_Pause();
    }

    QFileDialog dialog(this);
    int ret;
    QString dir, cartRom, diskRom;

    dialog.setFileMode(QFileDialog::FileMode::ExistingFile);
    dialog.setNameFilter("N64 ROMs (*.n64 *.z64 *.v64 *.zip)");

    ret = dialog.exec();
    if (!ret)
    {
        if (isRunning && !isPaused)
        {
            this->on_Action_System_Pause();
        }
        return;
    }

    cartRom = dialog.selectedFiles().first();

    dialog.setNameFilter("N64DD Disk Image (*.ndd *.d64)");
    ret = dialog.exec();
    if (!ret)
    {
        if (isRunning && !isPaused)
        {
            this->on_Action_System_Pause();
        }
        return;
    }

    diskRom = dialog.selectedFiles().first();

    if (this->ui_Widgets->currentIndex() != 0)
    {
        this->ui_NoSwitchToRomBrowser = true;
    }

    this->emulationThread_Launch(cartRom, diskRom);
}

void MainWindow::on_Action_File_EndEmulation(void)
{
    if (g_MupenApi.Core.isEmulationPaused())
    {
        this->on_Action_System_Pause();
    }

    if (!g_MupenApi.Core.StopEmulation())
    {
        this->ui_MessageBox("Error", "Api::Core::StopEmulation Failed!", g_MupenApi.Core.GetLastError());
    }
}

void MainWindow::on_Action_File_ChooseDirectory(void)
{
    QFileDialog dialog;
    QString dir;
    int ret;

    dialog.setFileMode(QFileDialog::Directory);
    dialog.setOption(QFileDialog::ShowDirsOnly, true);
    dialog.setWindowIcon(this->ui_Icon);

    ret = dialog.exec();
    if (ret)
    {
        dir = dialog.selectedFiles().first();
        g_Settings.SetValue(SettingsID::RomBrowser_Directory, dir);
        this->ui_Widget_RomBrowser->SetDirectory(dir);
        this->ui_Widget_RomBrowser->RefreshRomList();
    }
}

void MainWindow::on_Action_File_RefreshRomList(void)
{
    this->ui_Widget_RomBrowser->RefreshRomList();
}

void MainWindow::on_Action_File_Exit(void)
{
    this->close();
}

void MainWindow::on_Action_System_SoftReset(void)
{
    if (!g_MupenApi.Core.ResetEmulation(false))
    {
        this->ui_MessageBox("Error", "Api::Core::ResetEmulation Failed!", g_MupenApi.Core.GetLastError());
    }
}

void MainWindow::on_Action_System_HardReset(void)
{
    if (!g_MupenApi.Core.ResetEmulation(true))
    {
        this->ui_MessageBox("Error", "Api::Core::ResetEmulation Failed!", g_MupenApi.Core.GetLastError());
    }
}

void MainWindow::on_Action_System_Pause(void)
{
    static bool paused = false;

    bool ret;
    QString error = "Api::Core::";

    if (!paused)
    {
        ret = g_MupenApi.Core.PauseEmulation();
        error += "PauseEmulation";
    }
    else
    {
        ret = g_MupenApi.Core.ResumeEmulation();
        error += "ResumeEmulation";
    }

    error += " Failed!";

    paused = !paused;

    if (!ret)
    {
        this->ui_MessageBox("Error", error, g_MupenApi.Core.GetLastError());
        return;
    }

    this->menuBar_Setup(true, paused);
}

void MainWindow::on_Action_System_GenerateBitmap(void)
{
    if (!g_MupenApi.Core.TakeScreenshot())
    {
        this->ui_MessageBox("Error", "Api::Core::TakeScreenshot Failed!", g_MupenApi.Core.GetLastError());
    }
}

void MainWindow::on_Action_System_LimitFPS(void)
{
    bool enabled, ret;

    enabled = this->action_System_LimitFPS->isChecked();

    ret = g_MupenApi.Core.SetSpeedLimiter(enabled);

    if (!ret)
    {
        this->ui_MessageBox("Error", "Api::Core::SetSpeedLimiter Failed!", g_MupenApi.Core.GetLastError());
    }
}

void MainWindow::on_Action_System_SwapDisk(void)
{
}

void MainWindow::on_Action_System_SaveState(void)
{
    if (!g_MupenApi.Core.SaveState())
    {
        this->ui_MessageBox("Error", "Api::Core::SaveState Failed", g_MupenApi.Core.GetLastError());
    }
}

void MainWindow::on_Action_System_SaveAs(void)
{
    bool isPaused = g_MupenApi.Core.isEmulationPaused();

    if (!isPaused)
    {
        this->on_Action_System_Pause();
    }

    QString fileName = QFileDialog::getSaveFileName(this, tr("Save State"), "", tr("SaveState (*.dat);;All Files (*)"));

    if (!g_MupenApi.Core.SaveStateAsFile(fileName))
    {
        this->ui_MessageBox("Error", "Api::Core::SaveStateAsFile Failed", g_MupenApi.Core.GetLastError());
    }

    if (!isPaused)
    {
        this->on_Action_System_Pause();
    }
}

void MainWindow::on_Action_System_LoadState(void)
{
    if (!g_MupenApi.Core.LoadState())
    {
        this->ui_MessageBox("Error", "Api::Core::LoadState Failed", g_MupenApi.Core.GetLastError());
    }
}

void MainWindow::on_Action_System_Load(void)
{
    bool isPaused = g_MupenApi.Core.isEmulationPaused();

    if (!isPaused)
    {
        this->on_Action_System_Pause();
    }

    QString fileName =
        QFileDialog::getOpenFileName(this, tr("Open Save State"), "", tr("SaveState (*.dat);;All Files (*)"));

    if (!g_MupenApi.Core.LoadStateFromFile(fileName))
    {
        this->ui_MessageBox("Error", "Api::Core::LoadStateFromFile Failed", g_MupenApi.Core.GetLastError());
    }

    if (!isPaused)
    {
        this->on_Action_System_Pause();
    }
}

void MainWindow::on_Action_System_CurrentSaveState(int slot)
{
    if (!g_MupenApi.Core.SetSaveSlot(slot))
    {
        this->ui_MessageBox("Error", "Api::Core::SetSaveSlot Failed", g_MupenApi.Core.GetLastError());
    }
}

void MainWindow::on_Action_System_Cheats(void)
{
}

void MainWindow::on_Action_System_GSButton(void)
{
    if (!g_MupenApi.Core.PressGameSharkButton())
    {
        this->ui_MessageBox("Error", "Api::Core::PressGameSharkButton Failed", g_MupenApi.Core.GetLastError());
    }
}

void MainWindow::on_Action_Options_FullScreen(void)
{
    if (!g_MupenApi.Core.ToggleFullscreen())
    {
        this->ui_MessageBox("Error", "Api::Core::ToggleFullscreen Failed", g_MupenApi.Core.GetLastError());
    }
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
    bool isRunning = g_MupenApi.Core.IsEmulationRunning();
    bool isPaused = g_MupenApi.Core.isEmulationPaused();

    if (isRunning && !isPaused)
    {
        this->on_Action_System_Pause();
    }

    Dialog::SettingsDialog dialog(this);
    dialog.exec();

    // reload UI,
    // because we need to keep Options -> Configure {type} Plugin...
    // up-to-date
    this->ui_InEmulation(emulationThread->isRunning(), isPaused);

    if (isRunning && !isPaused)
    {
        this->on_Action_System_Pause();
    }
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
}

void MainWindow::on_Emulation_Finished(bool ret)
{
    if (!ret)
    {
        this->ui_MessageBox("Error", "EmulationThread::run Failed", this->emulationThread->GetLastError());
        // whatever we do on failure,
        // always return to the rombrowser
        this->ui_NoSwitchToRomBrowser = false;
        this->ui_InEmulation(false, false);
    }

    if (this->ui_RefreshRomListAfterEmulation)
    {
        this->ui_Widget_RomBrowser->RefreshRomList();
        this->ui_RefreshRomListAfterEmulation = false;
    }
}

void MainWindow::on_RomBrowser_Selected(QString file)
{
    this->emulationThread_Launch(file);
}

void MainWindow::on_VidExt_Init(void)
{
    this->ui_VidExt_Geometry_Saved = false;
    this->ui_VidExtForceSetMode = true;

    this->ui_InEmulation(true, false);
}

void MainWindow::on_VidExt_SetupOGL(QSurfaceFormat format, QThread *thread)
{
    this->ui_Widget_OpenGL->MoveToThread(thread);

    g_OGLWidget = this->ui_Widget_OpenGL;

    this->ui_Widget_OpenGL->setFormat(format);
}

void MainWindow::on_VidExt_SetMode(int width, int height, int bps, int mode, int flags)
{
    this->on_VidExt_ResizeWindow(width, height);
}

void MainWindow::on_VidExt_SetWindowedModeWithRate(int width, int height, int refresh, int bps, int flags)
{
    // load window geometry
    if (this->ui_VidExt_Geometry_Saved)
    {
        this->restoreGeometry(this->ui_VidExt_Geometry);
        this->ui_VidExt_Geometry_Saved = false;

        // force 'refresh' the video plugin
        g_MupenApi.Core.SetVideoSize(width, height, true);
    }

    if (this->isFullScreen())
        this->showNormal();

    if (this->menuBar->isHidden())
        this->menuBar->show();

    if (this->statusBar()->isHidden())
        this->statusBar()->show();

    this->ui_Actions_Remove();
    this->on_VidExt_ResizeWindow(width, height);
}

void MainWindow::on_VidExt_SetFullscreenModeWithRate(int width, int height, int refresh, int bps, int flags)
{
    // save window geometry
    if (!this->ui_VidExt_Geometry_Saved)
    {
        this->ui_VidExt_Geometry = this->saveGeometry();
        this->ui_VidExt_Geometry_Saved = true;

        // force 'refresh' the video plugin
        g_MupenApi.Core.SetVideoSize(width, height, true);
    }

    if (!this->isFullScreen())
    {
        this->showFullScreen();
    }

    if (!this->menuBar->isHidden())
    {
        this->menuBar->hide();
    }

    if (!this->statusBar()->isHidden())
    {
        this->statusBar()->hide();
    }

    this->ui_Actions_Add();
    this->on_VidExt_ResizeWindow(width, height);
}

void MainWindow::on_VidExt_ResizeWindow(int width, int height)
{
    // account for HiDPI scaling
    // see https://github.com/Rosalie241/RMG/issues/2
    height /= this->devicePixelRatioF();
    width /= this->devicePixelRatioF();

    if (!this->menuBar->isHidden())
    {
        height += this->menuBar->height();
    }

    if (!this->statusBar()->isHidden())
    {
        height += this->statusBar()->height();
    }

    if (!this->ui_VidExtForceSetMode)
    {
        if (this->size() == QSize(width, height))
            return;
    }

    if (this->isMaximized() || this->isMinimized())
    {
        this->showNormal();
    }

    if (this->ui_AllowManualResizing)
    {
        this->resize(width, height);
    }
    else
    {
        this->setFixedSize(width, height);
    }

    // we've force set the size once,
    // we can safely disable it now
    this->ui_VidExtForceSetMode = false;
}

void MainWindow::on_VidExt_SetCaption(QString title)
{
}

void MainWindow::on_VidExt_ToggleFS(void)
{
}

void MainWindow::on_VidExt_Quit(void)
{
    this->ui_InEmulation(false, false);
}

void MainWindow::on_Core_DebugCallback(MessageType type, QString message)
{
    // only display in statusbar when emulation is running
    if (!this->emulationThread->isRunning())
        return;

    // drop verbose messages
    if (type == MessageType::Verbose)
        return;

    if (type == MessageType::Error)
    {
        this->ui_MessageBox("Error", "Core Error", message);
        return;
    }

    this->ui_StatusBar_Label->setText(message);

    // reset label deletion timer
    if (this->ui_TimerId != 0)
    {
        this->killTimer(this->ui_TimerId);
    }
    this->ui_TimerId = this->startTimer(this->ui_TimerTimeout * 1000);
}
