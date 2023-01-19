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

#include "UserInterface/Dialog/AboutDialog.hpp"
#ifdef UPDATER
#include "UserInterface/Dialog/UpdateDialog.hpp"
#include "UserInterface/Dialog/DownloadUpdateDialog.hpp"
#include "UserInterface/Dialog/InstallUpdateDialog.hpp"
#endif // UPDATER
#include "UserInterface/EventFilter.hpp"
#include "Utilities/QtKeyToSdl2Key.hpp"
#include "OnScreenDisplay.hpp"
#include "Callbacks.hpp"
#include "VidExt.hpp"

#include <RMG-Core/Core.hpp>

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
#include <QTimer>

using namespace UserInterface;

MainWindow::MainWindow() : QMainWindow(nullptr)
{
}

MainWindow::~MainWindow()
{
}

bool MainWindow::Init(QApplication* app)
{
    if (!CoreInit())
    {
        this->showErrorMessage("CoreInit() Failed", QString::fromStdString(CoreGetError()));
        return false;
    }

    if (!CoreApplyPluginSettings())
    {
        this->showErrorMessage("CoreApplyPluginSettings() Failed", QString::fromStdString(CoreGetError()));
    }

    this->configureTheme(app);

    this->initializeUI();
    this->configureUI(app);

    this->connectActionSignals();
    this->configureActions();
    this->updateActions(false, false);

#ifdef UPDATER
    this->checkForUpdates();
#endif // UPDATER

    this->initializeEmulationThread();
    this->connectEmulationThreadSignals();

    if (!SetupVidExt(this->emulationThread, this, this->ui_Widget_OpenGL))
    {
        this->showErrorMessage("SetupVidExt() Failed", QString::fromStdString(CoreGetError()));
        return false;
    }

    this->coreCallBacks = new CoreCallbacks(this);
    if (!this->coreCallBacks->Init())
    {
        this->showErrorMessage("CoreCallbacks::Init() Failed", QString::fromStdString(CoreGetError()));
        return false;
    }

    connect(coreCallBacks, &CoreCallbacks::OnCoreDebugCallback, this, &MainWindow::on_Core_DebugCallback);
    connect(coreCallBacks, &CoreCallbacks::OnCoreDebugCallback, &this->logDialog, &Dialog::LogDialog::AddLogLine);
    connect(app, &QGuiApplication::applicationStateChanged, this, &MainWindow::on_QGuiApplication_applicationStateChanged);

    return true;
}

void MainWindow::OpenROM(QString file, QString disk, bool fullscreen, bool quitAfterEmulation)
{
    this->ui_LaunchInFullscreen = fullscreen;
    this->ui_QuitAfterEmulation = quitAfterEmulation;
    this->launchEmulationThread(file, disk);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    // we have to make sure we save the geomtry
    // for the ROM browser when emulation
    // isn't running (or hasn't run at all)
    if (!this->ui_QuitAfterEmulation && 
        !this->emulationThread->isRunning())
    {
        this->storeGeometry();    
    }

    this->ui_NoSwitchToRomBrowser = true;
    this->on_Action_System_Shutdown();

    this->ui_Widget_RomBrowser->StopRefreshRomList();

    this->coreCallBacks->Stop();

    this->logDialog.close();

    while (this->emulationThread->isRunning())
    {
        QCoreApplication::processEvents();
    }

    CoreSettingsSave();
    CoreShutdown();

    QMainWindow::closeEvent(event);
}

void MainWindow::initializeUI(void)
{
    this->setupUi(this);

    this->ui_Widgets = new QStackedWidget(this);
    this->ui_Widget_RomBrowser = new Widget::RomBrowserWidget(this);
    this->ui_Widget_OpenGL = new Widget::OGLWidget(this);
    this->ui_EventFilter = new EventFilter(this);
    this->ui_StatusBar_Label = new QLabel(this);

    this->ui_Widget_RomBrowser->RefreshRomList();

    connect(this->ui_Widget_RomBrowser, &Widget::RomBrowserWidget::PlayGame, this,
            &MainWindow::on_RomBrowser_PlayGame);
    connect(this->ui_Widget_RomBrowser, &Widget::RomBrowserWidget::PlayGameWith, this,
            &MainWindow::on_RomBrowser_PlayGameWith);
    connect(this->ui_Widget_RomBrowser, &Widget::RomBrowserWidget::EditGameSettings, this,
            &MainWindow::on_RomBrowser_EditGameSettings);
    connect(this->ui_Widget_RomBrowser, &Widget::RomBrowserWidget::Cheats, this,
            &MainWindow::on_RomBrowser_Cheats);
    connect(this->ui_Widget_RomBrowser, &Widget::RomBrowserWidget::ChangeRomDirectory, this,
            &MainWindow::on_RomBrowser_ChangeRomDirectory);
    connect(this->ui_Widget_RomBrowser, &Widget::RomBrowserWidget::RomInformation, this,
            &MainWindow::on_RomBrowser_RomInformation);

    connect(this->ui_EventFilter, &EventFilter::on_EventFilter_KeyPressed, this,
            &MainWindow::on_EventFilter_KeyPressed);
    connect(this->ui_EventFilter, &EventFilter::on_EventFilter_KeyReleased, this,
            &MainWindow::on_EventFilter_KeyReleased);
}

void MainWindow::configureUI(QApplication* app)
{
    this->setCentralWidget(this->ui_Widgets);

    QString geometry;
    geometry = QString::fromStdString(CoreSettingsGetStringValue(SettingsID::RomBrowser_Geometry));
    if (!geometry.isEmpty())
    {
        this->restoreGeometry(QByteArray::fromBase64(geometry.toLocal8Bit()));
    }

    this->ui_ShowToolbar = CoreSettingsGetBoolValue(SettingsID::GUI_Toolbar);
    this->ui_ShowStatusbar = CoreSettingsGetBoolValue(SettingsID::GUI_StatusBar);

    this->toolBar->setVisible(this->ui_ShowToolbar);
    this->statusBar()->setVisible(this->ui_ShowStatusbar);
    this->statusBar()->addPermanentWidget(this->ui_StatusBar_Label, 1);

    this->ui_TimerTimeout = CoreSettingsGetIntValue(SettingsID::GUI_StatusbarMessageDuration);

    this->ui_Widgets->addWidget(this->ui_Widget_RomBrowser);
    this->ui_Widgets->addWidget(this->ui_Widget_OpenGL->GetWidget());

    this->ui_Widgets->setCurrentWidget(this->ui_Widget_RomBrowser);

    this->setFocusPolicy(Qt::FocusPolicy::StrongFocus);
    this->installEventFilter(this->ui_EventFilter);
    this->ui_Widget_OpenGL->installEventFilter(this->ui_EventFilter);

    this->ui_WindowTitle = "Rosalie's Mupen GUI (";
    this->ui_WindowTitle += QString::fromStdString(CoreGetVersion());
    this->ui_WindowTitle += ")";

    this->setWindowTitle(this->ui_WindowTitle);
}

void MainWindow::configureTheme(QApplication* app)
{
    // set theme style
    QString fallbackStyleSheet = "QTableView { border: none; color: #0096d3; selection-color: #FFFFFF; selection-background-color: #0096d3; }";

#ifndef _WIN32
    this->setStyleSheet(fallbackStyleSheet);

    // set icon theme
    QString fallbackThemeName = QIcon::themeName();
    QPalette palette = app->palette();
    bool dark = palette.windowText().color().value() > palette.window().color().value();
    QIcon::setThemeName(dark ? "white" : "black");

    // fallback for icons we don't provide (i.e standard system icons)
    QIcon::setFallbackThemeName(fallbackThemeName);
#else // _WIN32
    
    // set icon theme
    QIcon::setThemeName(QString::fromStdString(CoreSettingsGetStringValue(SettingsID::GUI_IconTheme)));

    QString styleFilePath = QString::fromStdString(CoreSettingsGetStringValue(SettingsID::GUI_Style));
    QFile styleFile(styleFilePath);

    if (styleFilePath.isEmpty() || !styleFile.exists())
    {
        this->setStyleSheet(fallbackStyleSheet);
        return;
    }

    if (!styleFile.open(QIODevice::ReadOnly))
    {
        this->setStyleSheet(fallbackStyleSheet);
        return;
    }

    app->setStyleSheet(styleFile.readAll());
#endif // _WIN32
}

void MainWindow::showErrorMessage(QString text, QString details = "")
{
    QMessageBox msgBox(this);
    msgBox.setIcon(QMessageBox::Icon::Critical);
    msgBox.setWindowTitle("Error");
    msgBox.setText(text);
    msgBox.setDetailedText(details);
    msgBox.addButton(QMessageBox::Ok);
    msgBox.exec();
}

void MainWindow::updateUI(bool inEmulation, bool isPaused)
{
    if (!this->ui_NoSwitchToRomBrowser)
    {
        this->updateActions(inEmulation, isPaused);
    }

    if (inEmulation)
    {
        CoreRomSettings settings;
        CoreGetCurrentRomSettings(settings);

        if (!settings.GoodName.empty())
        {
            this->setWindowTitle(QString::fromStdString(settings.GoodName) + QString(" - ") + this->ui_WindowTitle);
        }

        this->ui_Widgets->setCurrentWidget(this->ui_Widget_OpenGL->GetWidget());
        this->storeGeometry();
    }
    else if (!this->ui_NoSwitchToRomBrowser)
    {
        this->setWindowTitle(this->ui_WindowTitle);
        this->ui_Widgets->setCurrentWidget(this->ui_Widget_RomBrowser);
        this->loadGeometry();
    }
    else
    {
        this->ui_NoSwitchToRomBrowser = false;
    }

    // update timer timeout
    this->ui_TimerTimeout = CoreSettingsGetIntValue(SettingsID::GUI_StatusbarMessageDuration);
}

void MainWindow::storeGeometry(void)
{
    if (this->ui_Geometry_Saved)
    {
        return;
    }

    this->ui_Geometry = this->saveGeometry();
    this->ui_Geometry_Saved = true;

    std::string geometryStr = this->ui_Geometry.toBase64().toStdString();
    CoreSettingsSetValue(SettingsID::RomBrowser_Geometry, geometryStr);
}

void MainWindow::loadGeometry(void)
{
    if (!this->ui_Geometry_Saved)
    {
        return;
    }

    this->setMinimumSize(0, 0);
    this->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
    this->restoreGeometry(this->ui_Geometry);

    if (this->isFullScreen())
    {
        this->showNormal();
    }

    if (this->menuBar()->isHidden())
    {
        this->menuBar()->show();
    }

    if (this->ui_ShowToolbar && this->toolBar->isHidden())
    {
        this->toolBar->show();
    }

    if (this->ui_ShowStatusbar && this->statusBar()->isHidden())
    {
        this->statusBar()->show();
    }

    this->ui_Geometry_Saved = false;
}

void MainWindow::initializeEmulationThread(void)
{
    this->emulationThread = new Thread::EmulationThread(this);
}

void MainWindow::connectEmulationThreadSignals(void)
{
    connect(this->emulationThread, &Thread::EmulationThread::on_Emulation_Finished, this,
            &MainWindow::on_Emulation_Finished);
    connect(this->emulationThread, &Thread::EmulationThread::on_Emulation_Started, this,
            &MainWindow::on_Emulation_Started);

    connect(this->emulationThread, &Thread::EmulationThread::on_VidExt_Init, this, &MainWindow::on_VidExt_Init,
            Qt::BlockingQueuedConnection);
    connect(this->emulationThread, &Thread::EmulationThread::on_VidExt_SetupOGL, this, &MainWindow::on_VidExt_SetupOGL,
            Qt::BlockingQueuedConnection);
    connect(this->emulationThread, &Thread::EmulationThread::on_VidExt_SetWindowedMode, this,
            &MainWindow::on_VidExt_SetWindowedMode, Qt::BlockingQueuedConnection);
    connect(this->emulationThread, &Thread::EmulationThread::on_VidExt_SetFullscreenMode, this,
            &MainWindow::on_VidExt_SetFullscreenMode, Qt::BlockingQueuedConnection);
    connect(this->emulationThread, &Thread::EmulationThread::on_VidExt_ResizeWindow, this,
            &MainWindow::on_VidExt_ResizeWindow, Qt::BlockingQueuedConnection);
    connect(this->emulationThread, &Thread::EmulationThread::on_VidExt_ToggleFS, this, &MainWindow::on_VidExt_ToggleFS,
            Qt::BlockingQueuedConnection);
}

void MainWindow::launchEmulationThread(QString cartRom, QString diskRom)
{
    CoreSettingsSave();

    if (this->emulationThread->isRunning())
    {
        this->on_Action_System_Shutdown();

        while (this->emulationThread->isRunning())
        {
            QCoreApplication::processEvents();
        }
    }

    this->ui_RefreshRomListAfterEmulation = this->ui_Widget_RomBrowser->IsRefreshingRomList();
    if (this->ui_RefreshRomListAfterEmulation)
    {
        this->ui_Widget_RomBrowser->StopRefreshRomList();
    }

    if (this->ui_LaunchInFullscreen || CoreSettingsGetBoolValue(SettingsID::GUI_AutomaticFullscreen))
    {
        this->ui_FullscreenTimerId = this->startTimer(100);
        this->ui_LaunchInFullscreen = false;
    }

    if (!CoreArePluginsReady())
    {
        this->showErrorMessage("CoreArePluginsReady() Failed", QString::fromStdString(CoreGetError()));
        return;
    }

    this->ui_AllowManualResizing = CoreSettingsGetBoolValue(SettingsID::GUI_AllowManualResizing);
    this->ui_HideCursorInEmulation = CoreSettingsGetBoolValue(SettingsID::GUI_HideCursorInEmulation);
    this->ui_HideCursorInFullscreenEmulation = CoreSettingsGetBoolValue(SettingsID::GUI_HideCursorInFullscreenEmulation);
    this->ui_ShowToolbar = CoreSettingsGetBoolValue(SettingsID::GUI_Toolbar);
    this->ui_ShowStatusbar = CoreSettingsGetBoolValue(SettingsID::GUI_StatusBar);

    this->ui_Widget_OpenGL->SetHideCursor(this->ui_HideCursorInEmulation);

    this->emulationThread->SetRomFile(cartRom);
    this->emulationThread->SetDiskFile(diskRom);
    this->emulationThread->start();
}

void MainWindow::launchEmulationThread(QString file)
{
    this->launchEmulationThread(file, "");
}

void MainWindow::updateActions(bool inEmulation, bool isPaused)
{
    QString keyBinding;

    keyBinding = QString::fromStdString(CoreSettingsGetStringValue(SettingsID::KeyBinding_StartROM));
    this->action_System_StartRom->setShortcut(QKeySequence(keyBinding));
    this->action_System_StartRom->setEnabled(!inEmulation);
    keyBinding = QString::fromStdString(CoreSettingsGetStringValue(SettingsID::KeyBinding_StartCombo));
    this->action_System_OpenCombo->setShortcut(QKeySequence(keyBinding));
    this->action_System_OpenCombo->setEnabled(!inEmulation);
    keyBinding = QString::fromStdString(CoreSettingsGetStringValue(SettingsID::KeyBinding_Shutdown));
    this->action_System_Shutdown->setShortcut(QKeySequence(keyBinding));
    this->action_System_Shutdown->setEnabled(inEmulation);
    this->menuReset->setEnabled(inEmulation);
    keyBinding = QString::fromStdString(CoreSettingsGetStringValue(SettingsID::KeyBinding_SoftReset));
    this->action_System_SoftReset->setEnabled(inEmulation);
    this->action_System_SoftReset->setShortcut(QKeySequence(keyBinding));
    keyBinding = QString::fromStdString(CoreSettingsGetStringValue(SettingsID::KeyBinding_HardReset));
    this->action_System_HardReset->setEnabled(inEmulation);
    this->action_System_HardReset->setShortcut(QKeySequence(keyBinding));
    keyBinding = QString::fromStdString(CoreSettingsGetStringValue(SettingsID::KeyBinding_Resume));
    this->action_System_Pause->setChecked(isPaused);
    this->action_System_Pause->setEnabled(inEmulation);
    this->action_System_Pause->setShortcut(QKeySequence(keyBinding));
    keyBinding = QString::fromStdString(CoreSettingsGetStringValue(SettingsID::KeyBinding_Screenshot));
    this->action_System_Screenshot->setEnabled(inEmulation);
    this->action_System_Screenshot->setShortcut(QKeySequence(keyBinding));
    keyBinding = QString::fromStdString(CoreSettingsGetStringValue(SettingsID::KeyBinding_LimitFPS));
    this->action_System_LimitFPS->setEnabled(inEmulation);
    this->action_System_LimitFPS->setShortcut(QKeySequence(keyBinding));
    this->action_System_LimitFPS->setChecked(CoreIsSpeedLimiterEnabled());
    keyBinding = QString::fromStdString(CoreSettingsGetStringValue(SettingsID::KeyBinding_SaveState));
    this->action_System_SaveState->setEnabled(inEmulation);
    this->action_System_SaveState->setShortcut(QKeySequence(keyBinding));
    keyBinding = QString::fromStdString(CoreSettingsGetStringValue(SettingsID::KeyBinding_SaveAs));
    this->action_System_SaveAs->setEnabled(inEmulation);
    this->action_System_SaveAs->setShortcut(QKeySequence(keyBinding));
    keyBinding = QString::fromStdString(CoreSettingsGetStringValue(SettingsID::KeyBinding_LoadState));
    this->action_System_LoadState->setEnabled(inEmulation);
    this->action_System_LoadState->setShortcut(QKeySequence(keyBinding));
    keyBinding = QString::fromStdString(CoreSettingsGetStringValue(SettingsID::KeyBinding_Load));
    this->action_System_Load->setEnabled(inEmulation);
    this->action_System_Load->setShortcut(QKeySequence(keyBinding));
    this->menuCurrent_Save_State->setEnabled(inEmulation);
    keyBinding = QString::fromStdString(CoreSettingsGetStringValue(SettingsID::KeyBinding_Cheats));
    this->action_System_Cheats->setEnabled(inEmulation);
    this->action_System_Cheats->setShortcut(QKeySequence(keyBinding));
    keyBinding = QString::fromStdString(CoreSettingsGetStringValue(SettingsID::KeyBinding_GSButton));
    this->action_System_GSButton->setEnabled(inEmulation);
    this->action_System_GSButton->setShortcut(QKeySequence(keyBinding));
    keyBinding = QString::fromStdString(CoreSettingsGetStringValue(SettingsID::KeyBinding_Exit));
    this->action_System_Exit->setShortcut(QKeySequence(keyBinding));

    // configure keybindings for save slots
    QAction* slotActions[] =
    {
        this->actionSlot_0, this->actionSlot_1, this->actionSlot_2,
        this->actionSlot_3, this->actionSlot_4, this->actionSlot_5,
        this->actionSlot_6, this->actionSlot_7, this->actionSlot_8,
        this->actionSlot_9
    };
    SettingsID slotKeybindSettingsId[] =
    {
        SettingsID::KeyBinding_SaveStateSlot0, SettingsID::KeyBinding_SaveStateSlot1,
        SettingsID::KeyBinding_SaveStateSlot2, SettingsID::KeyBinding_SaveStateSlot3,
        SettingsID::KeyBinding_SaveStateSlot4, SettingsID::KeyBinding_SaveStateSlot5,
        SettingsID::KeyBinding_SaveStateSlot6, SettingsID::KeyBinding_SaveStateSlot7,
        SettingsID::KeyBinding_SaveStateSlot8, SettingsID::KeyBinding_SaveStateSlot9
    };
    for (int i = 0; i < 10; i++)
    {
        keyBinding = QString::fromStdString(CoreSettingsGetStringValue(slotKeybindSettingsId[i]));
        slotActions[i]->setShortcut(QKeySequence(keyBinding));
    }

    this->action_Settings_Graphics->setEnabled(CorePluginsHasConfig(CorePluginType::Gfx));
    this->action_Settings_Audio->setEnabled(CorePluginsHasConfig(CorePluginType::Audio));
    this->action_Settings_Rsp->setEnabled(CorePluginsHasConfig(CorePluginType::Rsp));
    this->action_Settings_Input->setEnabled(CorePluginsHasConfig(CorePluginType::Input));
    keyBinding = QString::fromStdString(CoreSettingsGetStringValue(SettingsID::KeyBinding_Settings));
    this->action_Settings_Settings->setShortcut(QKeySequence(keyBinding));

    this->action_View_UniformSize->setEnabled(!inEmulation);
    keyBinding = QString::fromStdString(CoreSettingsGetStringValue(SettingsID::KeyBinding_Fullscreen));
    this->action_View_Fullscreen->setEnabled(inEmulation);
    this->action_View_Fullscreen->setShortcut(QKeySequence(keyBinding));
    keyBinding = QString::fromStdString(CoreSettingsGetStringValue(SettingsID::KeyBinding_RefreshROMList));
    this->action_View_RefreshRoms->setEnabled(!inEmulation);
    this->action_View_RefreshRoms->setShortcut(QKeySequence(keyBinding));
    this->action_View_ClearRomCache->setEnabled(!inEmulation);
}

void MainWindow::addFullscreenActions(void)
{
    this->addAction(this->action_System_StartRom);
    this->addAction(this->action_System_OpenCombo);
    this->addAction(this->action_System_Shutdown);
    this->addAction(this->action_System_SoftReset);
    this->addAction(this->action_System_HardReset);
    this->addAction(this->action_System_Pause);
    this->addAction(this->action_System_Screenshot);
    this->addAction(this->action_System_LimitFPS);
    this->addAction(this->action_System_SaveState);
    this->addAction(this->action_System_SaveAs);
    this->addAction(this->action_System_LoadState);
    this->addAction(this->action_System_Load);
    this->addAction(this->actionSlot_0);
    this->addAction(this->actionSlot_1);
    this->addAction(this->actionSlot_2);
    this->addAction(this->actionSlot_3);
    this->addAction(this->actionSlot_4);
    this->addAction(this->actionSlot_5);
    this->addAction(this->actionSlot_6);
    this->addAction(this->actionSlot_7);
    this->addAction(this->actionSlot_8);
    this->addAction(this->actionSlot_9);
    this->addAction(this->action_System_Cheats);
    this->addAction(this->action_System_GSButton);
    this->addAction(this->action_System_Exit);
    this->addAction(this->action_Settings_Graphics);
    this->addAction(this->action_Settings_Audio);
    this->addAction(this->action_Settings_Rsp);
    this->addAction(this->action_Settings_Input);
    this->addAction(this->action_Settings_Settings);
    this->addAction(this->action_View_Fullscreen);
    this->addAction(this->action_Help_Github);
    this->addAction(this->action_Help_About);
}

void MainWindow::removeFullscreenActions(void)
{
    this->removeAction(this->action_System_StartRom);
    this->removeAction(this->action_System_OpenCombo);
    this->removeAction(this->action_System_Shutdown);
    this->removeAction(this->action_System_SoftReset);
    this->removeAction(this->action_System_HardReset);
    this->removeAction(this->action_System_Pause);
    this->removeAction(this->action_System_Screenshot);
    this->removeAction(this->action_System_LimitFPS);
    this->removeAction(this->action_System_SaveState);
    this->removeAction(this->action_System_SaveAs);
    this->removeAction(this->action_System_LoadState);
    this->removeAction(this->action_System_Load);
    this->removeAction(this->actionSlot_0);
    this->removeAction(this->actionSlot_1);
    this->removeAction(this->actionSlot_2);
    this->removeAction(this->actionSlot_3);
    this->removeAction(this->actionSlot_4);
    this->removeAction(this->actionSlot_5);
    this->removeAction(this->actionSlot_6);
    this->removeAction(this->actionSlot_7);
    this->removeAction(this->actionSlot_8);
    this->removeAction(this->actionSlot_9);
    this->removeAction(this->action_System_Cheats);
    this->removeAction(this->action_System_GSButton);
    this->removeAction(this->action_System_Exit);
    this->removeAction(this->action_Settings_Graphics);
    this->removeAction(this->action_Settings_Audio);
    this->removeAction(this->action_Settings_Rsp);
    this->removeAction(this->action_Settings_Input);
    this->removeAction(this->action_Settings_Settings);
    this->removeAction(this->action_View_Fullscreen);
    this->removeAction(this->action_Help_Github);
    this->removeAction(this->action_Help_About);
}

void MainWindow::configureActions(void)
{
    // configure save slot actions
    QActionGroup* slotActionGroup = new QActionGroup(this);
    QAction* slotActions[] =
    {
        this->actionSlot_0, this->actionSlot_1, this->actionSlot_2,
        this->actionSlot_3, this->actionSlot_4, this->actionSlot_5,
        this->actionSlot_6, this->actionSlot_7, this->actionSlot_8,
        this->actionSlot_9
    };
    int currentSlot = CoreGetSaveStateSlot();
    for (int i = 0; i < 10; i++)
    {
        QAction* slotAction = slotActions[i];

        slotAction->setCheckable(true);
        slotAction->setChecked(i == currentSlot);
        slotAction->setActionGroup(slotActionGroup);

        // connect slot action here because we need to do
        // something special for them
        connect(slotAction, &QAction::triggered, [=](bool checked)
        {
            if (checked)
            {
                int slot = slotAction->text().split(" ").last().toInt();
                this->on_Action_System_CurrentSaveState(slot);
            }
        });
    }

    // configure toolbar & statusbar actions
    this->action_View_Toolbar->setChecked(CoreSettingsGetBoolValue(SettingsID::GUI_Toolbar));
    this->action_View_StatusBar->setChecked(CoreSettingsGetBoolValue(SettingsID::GUI_StatusBar));

    // configure ROM browser view actions
    QActionGroup* romBrowserViewActionGroup = new QActionGroup(this);
    int currentView = CoreSettingsGetIntValue(SettingsID::RomBrowser_ViewMode);
    QAction* romBrowserViewActions[] =
    {
        this->action_View_GameList,
        this->action_View_GameGrid
    };
    for (int i = 0; i < 2; i++)
    {
        QAction* action = romBrowserViewActions[i];

        action->setCheckable(true);
        action->setChecked(i == currentView);
        action->setActionGroup(romBrowserViewActionGroup);
    }

    // configure grid view options actions
    this->action_View_UniformSize->setChecked(CoreSettingsGetBoolValue(SettingsID::RomBrowser_GridViewUniformItemSizes));
}

void MainWindow::connectActionSignals(void)
{
    connect(this->action_System_StartRom, &QAction::triggered, this, &MainWindow::on_Action_System_OpenRom);
    connect(this->action_System_OpenCombo, &QAction::triggered, this, &MainWindow::on_Action_System_OpenCombo);
    connect(this->action_System_Exit, &QAction::triggered, this, &MainWindow::on_Action_System_Exit);

    connect(this->action_System_Shutdown, &QAction::triggered, this, &MainWindow::on_Action_System_Shutdown);
    connect(this->action_System_SoftReset, &QAction::triggered, this, &MainWindow::on_Action_System_SoftReset);
    connect(this->action_System_HardReset, &QAction::triggered, this, &MainWindow::on_Action_System_HardReset);
    connect(this->action_System_Pause, &QAction::triggered, this, &MainWindow::on_Action_System_Pause);
    connect(this->action_System_Screenshot, &QAction::triggered, this,
            &MainWindow::on_Action_System_Screenshot);
    connect(this->action_System_LimitFPS, &QAction::triggered, this, &MainWindow::on_Action_System_LimitFPS);
    connect(this->action_System_SaveState, &QAction::triggered, this, &MainWindow::on_Action_System_SaveState);
    connect(this->action_System_SaveAs, &QAction::triggered, this, &MainWindow::on_Action_System_SaveAs);
    connect(this->action_System_LoadState, &QAction::triggered, this, &MainWindow::on_Action_System_LoadState);
    connect(this->action_System_Load, &QAction::triggered, this, &MainWindow::on_Action_System_Load);
    connect(this->action_System_Cheats, &QAction::triggered, this, &MainWindow::on_Action_System_Cheats);
    connect(this->action_System_GSButton, &QAction::triggered, this, &MainWindow::on_Action_System_GSButton);

    connect(this->action_Settings_Graphics, &QAction::triggered, this, &MainWindow::on_Action_Settings_Graphics);
    connect(this->action_Settings_Audio, &QAction::triggered, this, &MainWindow::on_Action_Settings_Audio);
    connect(this->action_Settings_Rsp, &QAction::triggered, this, &MainWindow::on_Action_Settings_Rsp);
    connect(this->action_Settings_Input, &QAction::triggered, this,
            &MainWindow::on_Action_Settings_Input);
    connect(this->action_Settings_Settings, &QAction::triggered, this, &MainWindow::on_Action_Settings_Settings);

    connect(this->action_View_Toolbar, &QAction::toggled, this, &MainWindow::on_Action_View_Toolbar);
    connect(this->action_View_StatusBar, &QAction::toggled, this, &MainWindow::on_Action_View_StatusBar);
    connect(this->action_View_GameList, &QAction::toggled, this, &MainWindow::on_Action_View_GameList);
    connect(this->action_View_GameGrid, &QAction::toggled, this, &MainWindow::on_Action_View_GameGrid);
    connect(this->action_View_UniformSize, &QAction::toggled, this, &MainWindow::on_Action_View_UniformSize);
    connect(this->action_View_Fullscreen, &QAction::triggered, this, &MainWindow::on_Action_View_Fullscreen);
    connect(this->action_View_RefreshRoms, &QAction::triggered, this, &MainWindow::on_Action_View_RefreshRoms);
    connect(this->action_View_ClearRomCache, &QAction::triggered, this, &MainWindow::on_Action_View_ClearRomCache);
    connect(this->action_View_Log, &QAction::triggered, this, &MainWindow::on_Action_View_Log);

    connect(this->action_Help_Github, &QAction::triggered, this, &MainWindow::on_Action_Help_Github);
    connect(this->action_Help_About, &QAction::triggered, this, &MainWindow::on_Action_Help_About);
}

#ifdef UPDATER
void MainWindow::checkForUpdates(void)
{
    if (!CoreSettingsGetBoolValue(SettingsID::GUI_CheckForUpdates))
    {
        return;
    }

    QNetworkAccessManager* networkAccessManager = new QNetworkAccessManager(this);
    connect(networkAccessManager, &QNetworkAccessManager::finished, this, &MainWindow::on_networkAccessManager_Finished);
    networkAccessManager->get(QNetworkRequest(QUrl("https://api.github.com/repos/Rosalie241/RMG/releases/latest")));
}
#endif // UPDATER

void MainWindow::timerEvent(QTimerEvent *event)
{
    int timerId = event->timerId();

    if (timerId == this->ui_TimerId)
    {
        this->ui_StatusBar_Label->clear();
    }
    else if (timerId == this->ui_FullscreenTimerId)
    {
        // only try to go to fullscreen
        // when emulation is running
        if (!CoreIsEmulationRunning())
        {
            return;
        }

        // we're finished when we're in fullscreen already,
        // or when switching to fullscreen succeeds
        if (this->isFullScreen() || CoreToggleFullscreen())
        {
            this->killTimer(timerId);
            this->ui_FullscreenTimerId = 0;
        }
    }
    else if (timerId == this->ui_GamesharkButtonTimerId)
    {
        if (!CoreIsEmulationRunning())
        {
            return;
        }

        if (CorePressGamesharkButton(false))
        {
            this->killTimer(timerId);
            this->ui_GamesharkButtonTimerId = 0;
        }
    }
}

void MainWindow::on_EventFilter_KeyPressed(QKeyEvent *event)
{
    if (!CoreIsEmulationRunning())
    {
        QMainWindow::keyPressEvent(event);
        return;
    }

    int key = Utilities::QtKeyToSdl2Key(event->key());
    int mod = Utilities::QtModKeyToSdl2ModKey(event->modifiers());

    CoreSetKeyDown(key, mod);
}

void MainWindow::on_EventFilter_KeyReleased(QKeyEvent *event)
{
    if (!CoreIsEmulationRunning())
    {
        QMainWindow::keyReleaseEvent(event);
        return;
    }

    int key = Utilities::QtKeyToSdl2Key(event->key());
    int mod = Utilities::QtModKeyToSdl2ModKey(event->modifiers());

    CoreSetKeyUp(key, mod);
}

void MainWindow::on_QGuiApplication_applicationStateChanged(Qt::ApplicationState state)
{
    bool isRunning = CoreIsEmulationRunning();
    bool isPaused = CoreIsEmulationPaused();

    bool pauseOnFocusLoss = CoreSettingsGetBoolValue(SettingsID::GUI_PauseEmulationOnFocusLoss);
    bool resumeOnFocus = CoreSettingsGetBoolValue(SettingsID::GUI_ResumeEmulationOnFocus);

    switch (state)
    {
        default:
            break;

        case Qt::ApplicationState::ApplicationInactive:
        {
            if (pauseOnFocusLoss && isRunning && !isPaused)
            {
                this->on_Action_System_Pause();
                this->ui_ManuallyPaused = false;
            }
        } break;

        case Qt::ApplicationState::ApplicationActive:
        {
            if (resumeOnFocus && isPaused && !this->ui_ManuallyPaused)
            {
                this->on_Action_System_Pause();
            }
        } break;
    }
}

#ifdef UPDATER
void MainWindow::on_networkAccessManager_Finished(QNetworkReply* reply)
{
    if (reply->error())
    {
        reply->deleteLater();
        return;
    }

    QJsonDocument jsonDocument = QJsonDocument::fromJson(reply->readAll());
    QJsonObject jsonObject = jsonDocument.object();

    QString currentVersion = QString::fromStdString(CoreGetVersion());
    QString latestVersion = jsonObject.value("tag_name").toString();

    reply->deleteLater();

    // make sure the current version is valid
    // and not a dev version
    if (currentVersion.size() != 6)
    {
        return;
    }

    // do nothing when versions match
    if (currentVersion == latestVersion)
    {
        return;
    }

    int ret = 0;

    Dialog::UpdateDialog updateDialog(this, jsonObject);
    ret = updateDialog.exec();
    if (ret != QDialog::Accepted)
    {
        return;
    }

    Dialog::DownloadUpdateDialog downloadUpdateDialog(this, updateDialog.GetUrl(), updateDialog.GetFileName());
    ret = downloadUpdateDialog.exec();
    if (ret != QDialog::Accepted)
    {
        return;
    }

    Dialog::InstallUpdateDialog installUpdateDialog(this, QCoreApplication::applicationDirPath(), downloadUpdateDialog.GetTempDirectory(), downloadUpdateDialog.GetFileName());
    ret = installUpdateDialog.exec();
    if (ret != QDialog::Accepted)
    {
        return;
    }
}
#endif // UPDATER

void MainWindow::on_Action_System_OpenRom(void)
{
    bool isRunning = CoreIsEmulationRunning();
    bool isPaused = CoreIsEmulationPaused();

    if (isRunning && !isPaused)
    {
        this->on_Action_System_Pause();
    }

    QString romFile;

    romFile = QFileDialog::getOpenFileName(this, "", "", "N64 ROMs & Disks (*.n64 *.z64 *.v64 *.ndd *.d64 *.zip)");

    if (romFile.isEmpty())
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

    this->launchEmulationThread(romFile);
}

void MainWindow::on_Action_System_OpenCombo(void)
{
    bool isRunning = CoreIsEmulationRunning();
    bool isPaused = CoreIsEmulationPaused();

    if (isRunning && !isPaused)
    {
        this->on_Action_System_Pause();
    }

    QString cartRom, diskRom;

    cartRom = QFileDialog::getOpenFileName(this, "", "", "N64 ROMs (*.n64 *.z64 *.v64 *.zip)");
    
    if (cartRom.isEmpty())
    {
        if (isRunning && !isPaused)
        {
            this->on_Action_System_Pause();
        }
        return;
    }


    diskRom = QFileDialog::getOpenFileName(this, "", "", "N64DD Disk Image (*.ndd *.d64)");

    if (diskRom.isEmpty())
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

    this->launchEmulationThread(cartRom, diskRom);
}

void MainWindow::on_Action_System_Shutdown(void)
{
    if (CoreIsEmulationPaused())
    {
        this->on_Action_System_Pause();
    }

    if (!CoreIsEmulationRunning())
    {
        return;
    }

    if (!CoreStopEmulation())
    {
        this->showErrorMessage("CoreStopEmulation() Failed!", QString::fromStdString(CoreGetError()));
    }
}

void MainWindow::on_Action_System_Exit(void)
{
    this->close();
}

void MainWindow::on_Action_System_SoftReset(void)
{
    if (!CoreResetEmulation(false))
    {
        this->showErrorMessage("CoreResetEmulation() Failed!", QString::fromStdString(CoreGetError()));
    }
}

void MainWindow::on_Action_System_HardReset(void)
{
    if (!CoreResetEmulation(true))
    {
        this->showErrorMessage("CoreResetEmulation() Failed!", QString::fromStdString(CoreGetError()));
    }
}
void MainWindow::on_Action_System_Pause(void)
{
    bool isRunning = CoreIsEmulationRunning();
    bool isPaused = CoreIsEmulationPaused();

    bool ret;
    QString error;

    if (!isPaused)
    {
        if (isRunning)
        {
            OnScreenDisplayPause();
        }
        ret = CorePauseEmulation();
        error = "CorePauseEmulation() Failed!";
    }
    else
    {
        if (isPaused)
        {
            OnScreenDisplayResume();
        }
        ret = CoreResumeEmulation();
        error = "CoreResumeEmulation() Failed!";
    }

    if (!ret)
    {
        this->showErrorMessage(error, QString::fromStdString(CoreGetError()));
    }

    this->updateUI(true, (!isPaused && ret));
    this->ui_ManuallyPaused = true;
}

void MainWindow::on_Action_System_Screenshot(void)
{
    if (!CoreTakeScreenshot())
    {
        this->showErrorMessage("CoreTakeScreenshot() Failed!", QString::fromStdString(CoreGetError()));
    }
    else
    {
        OnScreenDisplaySetMessage("Captured screenshot.");
    }
}

void MainWindow::on_Action_System_LimitFPS(void)
{
    bool enabled, ret;

    enabled = this->action_System_LimitFPS->isChecked();

    ret = CoreSetSpeedLimiterState(enabled);

    if (!ret)
    {
        this->showErrorMessage("CoreSetSpeedLimiterState() Failed!", QString::fromStdString(CoreGetError()));
    }
}

void MainWindow::on_Action_System_SaveState(void)
{
    if (!CoreSaveState())
    {
        this->showErrorMessage("CoreSaveState() Failed", QString::fromStdString(CoreGetError()));
    }
    else
    {
        OnScreenDisplaySetMessage("Saved state to slot: " + std::to_string(CoreGetSaveStateSlot()));
    }
}

void MainWindow::on_Action_System_SaveAs(void)
{
    bool isRunning = CoreIsEmulationRunning();
    bool isPaused = CoreIsEmulationPaused();

    if (isRunning && !isPaused)
    {
        this->on_Action_System_Pause();
    }

    QString fileName = QFileDialog::getSaveFileName(this, tr("Save State"), "", tr("Save State (*.state);;All Files (*)"));

    if (!fileName.isEmpty())
    {
        if (!CoreSaveState(fileName.toStdU32String()))
        {
            this->showErrorMessage("CoreSaveState() Failed", QString::fromStdString(CoreGetError()));
        }
        else
        {
            OnScreenDisplaySetMessage("Saved state to: " + fileName.toStdString());
        }
    }

    if (isRunning && !isPaused)
    {
        this->on_Action_System_Pause();
    }
}

void MainWindow::on_Action_System_LoadState(void)
{
    if (!CoreLoadSaveState())
    {
        this->showErrorMessage("CoreLoadSaveState() Failed", QString::fromStdString(CoreGetError()));
    }
    else
    {
        OnScreenDisplaySetMessage("State loaded from slot: " + std::to_string(CoreGetSaveStateSlot()));
    }
}

void MainWindow::on_Action_System_Load(void)
{
    bool isRunning = CoreIsEmulationRunning();
    bool isPaused = CoreIsEmulationPaused();

    if (isRunning && !isPaused)
    {
        this->on_Action_System_Pause();
    }

    QString fileName =
        QFileDialog::getOpenFileName(this, tr("Open Save State"), "", tr("Save State (*.dat *.state);;All Files (*)"));

    if (!fileName.isEmpty())
    {
        if (!CoreLoadSaveState(fileName.toStdU32String()))
        {
            this->showErrorMessage("CoreLoadSaveState() Failed", QString::fromStdString(CoreGetError()));
        }
        else
        {
            OnScreenDisplaySetMessage("State loaded from: " + fileName.toStdString());
        }
    }

    if (isRunning && !isPaused)
    {
        this->on_Action_System_Pause();
    }
}

void MainWindow::on_Action_System_CurrentSaveState(int slot)
{
    if (!CoreSetSaveStateSlot(slot))
    {
        this->showErrorMessage("CoreSetSaveStateSlot() Failed", QString::fromStdString(CoreGetError()));
    }
    else
    {
        OnScreenDisplaySetMessage("Selected save slot: " + std::to_string(slot));
    }
}

void MainWindow::on_Action_System_Cheats(void)
{
    bool isRunning = CoreIsEmulationRunning();
    bool isPaused = CoreIsEmulationPaused();

    if (isRunning && !isPaused)
    {
        this->on_Action_System_Pause();
    }

    Dialog::CheatsDialog dialog(this);
    if (!dialog.HasFailed())
    {
        dialog.exec();
    }

    if (isRunning && !isPaused)
    {
        this->on_Action_System_Pause();
    }
}

void MainWindow::on_Action_System_GSButton(void)
{
    if (!CorePressGamesharkButton(true))
    {
        this->showErrorMessage("CorePressGamesharkButton() Failed", QString::fromStdString(CoreGetError()));
    }
    else
    {
        // only hold the gameshark button for 1 second
        this->ui_GamesharkButtonTimerId = this->startTimer(1000);
    }
}

void MainWindow::on_Action_Settings_Graphics(void)
{
    CorePluginsOpenConfig(CorePluginType::Gfx);
}

void MainWindow::on_Action_Settings_Audio(void)
{
    CorePluginsOpenConfig(CorePluginType::Audio);
}

void MainWindow::on_Action_Settings_Rsp(void)
{
    CorePluginsOpenConfig(CorePluginType::Rsp);
}

void MainWindow::on_Action_Settings_Input(void)
{
    CorePluginsOpenConfig(CorePluginType::Input);
}

void MainWindow::on_Action_Settings_Settings(void)
{
    bool isRunning = CoreIsEmulationRunning();
    bool isPaused = CoreIsEmulationPaused();

    if (isRunning && !isPaused)
    {
        this->on_Action_System_Pause();
    }

    Dialog::SettingsDialog dialog(this);
    dialog.exec();

    // reload UI,
    // because we need to keep Options -> Configure {type} Plugin...
    // up-to-date
    this->updateActions(emulationThread->isRunning(), isPaused);

    // update core callbacks settings
    this->coreCallBacks->LoadSettings();

    if (isRunning && !isPaused)
    {
        this->on_Action_System_Pause();
    }
}

void MainWindow::on_Action_View_Toolbar(bool checked)
{
    CoreSettingsSetValue(SettingsID::GUI_Toolbar, checked);
    this->toolBar->setVisible(checked);
    this->ui_ShowToolbar = checked;
}

void MainWindow::on_Action_View_StatusBar(bool checked)
{
    CoreSettingsSetValue(SettingsID::GUI_StatusBar, checked);
    this->statusBar()->setVisible(checked);
    this->ui_ShowStatusbar = checked;
}

void MainWindow::on_Action_View_GameList(bool checked)
{
    if (checked)
    {
        this->ui_Widget_RomBrowser->ShowList();
        CoreSettingsSetValue(SettingsID::RomBrowser_ViewMode, 0);
    }
}

void MainWindow::on_Action_View_GameGrid(bool checked)
{
    if (checked)
    {
        this->ui_Widget_RomBrowser->ShowGrid();
        CoreSettingsSetValue(SettingsID::RomBrowser_ViewMode, 1);
    }
}

void MainWindow::on_Action_View_UniformSize(bool checked)
{
    this->ui_Widget_RomBrowser->SetGridViewUniformSizes(checked);
    CoreSettingsSetValue(SettingsID::RomBrowser_GridViewUniformItemSizes, checked);
}

void MainWindow::on_Action_View_Fullscreen(void)
{
    if (!CoreToggleFullscreen())
    {
        this->showErrorMessage("CoreToggleFullscreen() Failed", QString::fromStdString(CoreGetError()));
    }
}

void MainWindow::on_Action_View_RefreshRoms(void)
{
    if (!this->ui_Widget_RomBrowser->IsRefreshingRomList())
    {
        this->ui_Widget_RomBrowser->RefreshRomList();
    }
}

void MainWindow::on_Action_View_ClearRomCache(void)
{
    if (!CoreClearRomHeaderAndSettingsCache())
    {
        this->showErrorMessage("CoreClearRomHeaderAndSettingsCache() Failed", QString::fromStdString(CoreGetError()));
    }
}

void MainWindow::on_Action_View_Log(void)
{
    this->logDialog.show();
}

void MainWindow::on_Action_Help_Github(void)
{
    QDesktopServices::openUrl(QUrl("https://github.com/Rosalie241/RMG"));
}

void MainWindow::on_Action_Help_About(void)
{
    Dialog::AboutDialog dialog(this);
    dialog.exec();
}

void MainWindow::on_Emulation_Started(void)
{
    this->logDialog.Clear();
}

void MainWindow::on_Emulation_Finished(bool ret)
{
    if (!ret)
    {
        this->showErrorMessage("EmulationThread::run Failed", this->emulationThread->GetLastError());
        // whatever we do on failure,
        // always return to the rombrowser
        this->ui_NoSwitchToRomBrowser = false;
    }

    if (this->ui_RefreshRomListAfterEmulation)
    {
        this->ui_Widget_RomBrowser->RefreshRomList();
        this->ui_RefreshRomListAfterEmulation = false;
    }

    if (this->ui_FullscreenTimerId != 0)
    {
        this->killTimer(this->ui_FullscreenTimerId);
        this->ui_FullscreenTimerId = 0;
    }

    if (this->ui_QuitAfterEmulation)
    {
        this->close();
        return;
    }

    // always refresh UI
    this->updateUI(false, false);
}

void MainWindow::on_RomBrowser_PlayGame(QString file)
{
    this->launchEmulationThread(file);
}

void MainWindow::on_RomBrowser_PlayGameWith(CoreRomType type, QString file)
{
    QString mainRom;
    QString otherRom;

    if (type == CoreRomType::Cartridge)
    { // cartridge
        mainRom = file;
        otherRom = QFileDialog::getOpenFileName(this, "", "", "N64DD Disk Image (*.ndd *.d64)");
    }
    else
    { // disk
        mainRom = QFileDialog::getOpenFileName(this, "", "", "N64 ROMs (*.n64 *.z64 *.v64 *.zip)");
        otherRom = file;
    }

    if (mainRom.isEmpty() || otherRom.isEmpty())
    {
        return;
    }

    this->launchEmulationThread(mainRom, otherRom);
}

void MainWindow::on_RomBrowser_ChangeRomDirectory(void)
{
    QString dir;

    dir = QFileDialog::getExistingDirectory(this);

    if (!dir.isEmpty())
    {
        CoreSettingsSetValue(SettingsID::RomBrowser_Directory, dir.toStdString());
        this->ui_Widget_RomBrowser->RefreshRomList();
    }
}

void MainWindow::on_RomBrowser_RomInformation(QString file)
{
    bool isRefreshingRomList = this->ui_Widget_RomBrowser->IsRefreshingRomList();

    if (isRefreshingRomList)
    {
        this->ui_Widget_RomBrowser->StopRefreshRomList();
    }

    CoreRomHeader romHeader;
    CoreRomSettings romSettings;

    if (!CoreOpenRom(file.toStdU32String()))
    {
        this->showErrorMessage("CoreOpenRom() Failed", QString::fromStdString(CoreGetError()));
        return;
    }

    if (!CoreGetCurrentRomHeader(romHeader))
    {
        this->showErrorMessage("CoreGetCurrentRomHeader() Failed", QString::fromStdString(CoreGetError()));
        return;
    }

    if (!CoreGetCurrentRomSettings(romSettings))
    {
        this->showErrorMessage("CoreGetCurrentRomSettings() Failed", QString::fromStdString(CoreGetError()));
        return;
    }

    if (!CoreCloseRom())
    {
        this->showErrorMessage("CoreCloseRom() Failed", QString::fromStdString(CoreGetError()));
        return;
    }

    Dialog::RomInfoDialog dialog(file, romHeader, romSettings, this);
    dialog.exec();

    if (isRefreshingRomList)
    {
        this->ui_Widget_RomBrowser->RefreshRomList();
    }
}

void MainWindow::on_RomBrowser_EditGameSettings(QString file)
{
    bool isRefreshingRomList = this->ui_Widget_RomBrowser->IsRefreshingRomList();
    if (isRefreshingRomList)
    {
        this->ui_Widget_RomBrowser->StopRefreshRomList();
    }

    if (!CoreOpenRom(file.toStdU32String()))
    {
        this->showErrorMessage("CoreOpenRom() Failed", QString::fromStdString(CoreGetError()));
        return;
    }

    Dialog::SettingsDialog dialog(this);
    dialog.ShowGameTab();
    dialog.exec();

    this->updateActions(false, false);
    this->coreCallBacks->LoadSettings();

    if (!CoreCloseRom())
    {
        this->showErrorMessage("CoreCloseRom() Failed", QString::fromStdString(CoreGetError()));
        return;
    }

    if (isRefreshingRomList)
    {
        this->ui_Widget_RomBrowser->RefreshRomList();
    }
}

void MainWindow::on_RomBrowser_Cheats(QString file)
{
    bool isRefreshingRomList = this->ui_Widget_RomBrowser->IsRefreshingRomList();
    if (isRefreshingRomList)
    {
        this->ui_Widget_RomBrowser->StopRefreshRomList();
    }

    if (!CoreOpenRom(file.toStdU32String()))
    {
        this->showErrorMessage("CoreOpenRom() Failed", QString::fromStdString(CoreGetError()));
        return;
    }

    Dialog::CheatsDialog dialog(this);
    if (!dialog.HasFailed())
    {
        dialog.exec();
    }

    if (!CoreCloseRom())
    {
        this->showErrorMessage("CoreCloseRom() Failed", QString::fromStdString(CoreGetError()));
        return;
    }

    if (isRefreshingRomList)
    {
        this->ui_Widget_RomBrowser->RefreshRomList();
    }
}

void MainWindow::on_VidExt_Init(void)
{
    this->ui_VidExtForceSetMode = true;
    this->updateUI(true, false);
}

void MainWindow::on_VidExt_SetupOGL(QSurfaceFormat format, QThread* thread)
{
    this->ui_Widget_OpenGL->MoveContextToThread(thread);
    this->ui_Widget_OpenGL->setFormat(format);
}

void MainWindow::on_VidExt_SetWindowedMode(int width, int height, int bps, int flags)
{
    bool returnedFromFullscreen = false;

    if (this->isFullScreen())
    {
        returnedFromFullscreen = true;
        this->showNormal();
    }

    if (this->menuBar()->isHidden())
    {
        this->menuBar()->show();
    }

    if (this->ui_ShowToolbar && this->toolBar->isHidden())
    {
        this->toolBar->show();
    }

    if (this->ui_ShowStatusbar && this->statusBar()->isHidden())
    {
        this->statusBar()->show();
    }

    if (!this->ui_HideCursorInEmulation && this->ui_HideCursorInFullscreenEmulation)
    {
        this->ui_Widget_OpenGL->SetHideCursor(false);
    }

    this->removeFullscreenActions();

    // only resize window when we're
    // not returning from fullscreen
    if (!returnedFromFullscreen)
    {
        this->on_VidExt_ResizeWindow(width, height);
    }
}

void MainWindow::on_VidExt_SetFullscreenMode(int width, int height, int bps, int flags)
{
    if (!this->isFullScreen())
    {
        this->showFullScreen();
    }

    if (!this->menuBar()->isHidden())
    {
        this->menuBar()->hide();
    }

    if (this->ui_ShowToolbar && !this->toolBar->isHidden())
    {
        this->toolBar->hide();
    }

    if (this->ui_ShowStatusbar && !this->statusBar()->isHidden())
    {
        this->statusBar()->hide();
    }

    if (!this->ui_HideCursorInEmulation && this->ui_HideCursorInFullscreenEmulation)
    {
        this->ui_Widget_OpenGL->SetHideCursor(true);
    }

    this->addFullscreenActions();
}

void MainWindow::on_VidExt_ResizeWindow(int width, int height)
{
    // account for HiDPI scaling
    // see https://github.com/Rosalie241/RMG/issues/2
    height /= this->devicePixelRatioF();
    width /= this->devicePixelRatioF();

    if (!this->menuBar()->isHidden())
    {
        height += this->menuBar()->height();
    }

    if (!this->toolBar->isHidden())
    {
        height += this->toolBar->height();
    }

    if (!this->statusBar()->isHidden())
    {
        height += this->statusBar()->height();
    }

    if (!this->ui_VidExtForceSetMode)
    {
        if (this->size() == QSize(width, height))
        {
            return;
        }
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

void MainWindow::on_VidExt_ToggleFS(bool fullscreen)
{
    if (fullscreen)
    {
        if (!this->isFullScreen())
        {
            this->showFullScreen();
        }

        if (!this->menuBar()->isHidden())
        {
            this->menuBar()->hide();
        }

        if (this->ui_ShowToolbar && !this->toolBar->isHidden())
        {
            this->toolBar->hide();
        }

        if (this->ui_ShowStatusbar && !this->statusBar()->isHidden())
        {
            this->statusBar()->hide();
        }

        if (!this->ui_HideCursorInEmulation && this->ui_HideCursorInFullscreenEmulation)
        {
            this->ui_Widget_OpenGL->SetHideCursor(true);
        }

        this->addFullscreenActions();
    }
    else
    {
        if (this->isFullScreen())
        {
            this->showNormal();
        }

        if (this->menuBar()->isHidden())
        {
            this->menuBar()->show();
        }
        
        if (this->ui_ShowToolbar && this->toolBar->isHidden())
        {
            this->toolBar->show();
        }

        if (this->ui_ShowStatusbar && this->statusBar()->isHidden())
        {
            this->statusBar()->show();
        }

        if (!this->ui_HideCursorInEmulation && this->ui_HideCursorInFullscreenEmulation)
        {
            this->ui_Widget_OpenGL->SetHideCursor(false);
        }

        this->removeFullscreenActions();
    }
}

void MainWindow::on_Core_DebugCallback(CoreDebugMessageType type, QString context, QString message)
{
    // only display in statusbar when emulation is running
    if (!this->emulationThread->isRunning())
    {
        return;
    }

    if (!context.startsWith("[CORE]"))
    {
        return;
    }

    // drop verbose messages
    if (type == CoreDebugMessageType::Verbose)
    {
        return;
    }

    // drop IS64 messages
    if (message.startsWith("IS64:"))
    {
        return;
    }

    if (type == CoreDebugMessageType::Error)
    {
        this->showErrorMessage("Core Error", message);
        return;
    }

    if (!this->ui_ShowStatusbar)
    {
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
