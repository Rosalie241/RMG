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
#include <QStyleFactory>
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
#include <cmath>

using namespace UserInterface;

MainWindow::MainWindow() : QMainWindow(nullptr)
{
}

MainWindow::~MainWindow()
{
}

bool MainWindow::Init(QApplication* app, bool showUI)
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
    this->configureUI(app, showUI);

    this->connectActionSignals();
    this->configureActions();
    this->updateActions(false, false);

#ifdef UPDATER
    this->checkForUpdates(true, false);
#else
    this->action_Help_Update->setVisible(false);
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
    connect(coreCallBacks, &CoreCallbacks::OnCoreStateCallback, this, &MainWindow::on_Core_StateCallback);
    connect(app, &QGuiApplication::applicationStateChanged, this, &MainWindow::on_QGuiApplication_applicationStateChanged);

    return true;
}

void MainWindow::OpenROM(QString file, QString disk, bool fullscreen, bool quitAfterEmulation)
{
    this->ui_LaunchInFullscreen = fullscreen;
    this->ui_QuitAfterEmulation = quitAfterEmulation;

    // ensure we don't switch to the ROM browser
    // because it can cause a slight flicker,
    // if we just ensure the UI is in an emulation
    // state, then the transition will be smoother
    this->updateUI(true, false);
    
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

void MainWindow::configureUI(QApplication* app, bool showUI)
{
    this->setCentralWidget(this->ui_Widgets);

    QString geometry;
    bool maximized;

    geometry = QString::fromStdString(CoreSettingsGetStringValue(SettingsID::RomBrowser_Geometry));
    maximized = CoreSettingsGetBoolValue(SettingsID::RomBrowser_Maximized);
    if (maximized)
    {
        this->showMaximized();
    }
    else if (!geometry.isEmpty())
    {
        this->restoreGeometry(QByteArray::fromBase64(geometry.toLocal8Bit()));
    }

    this->ui_ShowUI = showUI;

    if (this->ui_ShowUI)
    {
        this->ui_ShowMenubar = true;
        this->ui_ShowToolbar = CoreSettingsGetBoolValue(SettingsID::GUI_Toolbar);
        this->ui_ShowStatusbar = CoreSettingsGetBoolValue(SettingsID::GUI_StatusBar);
    }
    else
    {
        this->ui_ShowMenubar = false;
        this->ui_ShowToolbar = false;
        this->ui_ShowStatusbar = false;
    }

    this->menuBar()->setVisible(this->ui_ShowMenubar);
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
    // we have to retrieve the fallback icon theme
    // before applying the app theme
    QString fallbackThemeName = QIcon::themeName();

    // set theme style
    QString fallbackStyleSheet = "QTableView { border: none; color: #0096d3; selection-color: #FFFFFF; selection-background-color: #0096d3; }";
    this->setStyleSheet(fallbackStyleSheet);

    // set application theme
    QString theme = QString::fromStdString(CoreSettingsGetStringValue(SettingsID::GUI_Theme));
    if (theme == "Native")
    {
        // do nothing
    }
    else if (theme == "Fusion")
    {
        app->setPalette(QApplication::style()->standardPalette());
        app->setStyleSheet(QString());
        app->setStyle(QStyleFactory::create("Fusion"));
    }
    else if (theme == "Fusion Dark")
    {
        // adapted from https://gist.github.com/QuantumCD/6245215
        app->setStyle(QStyleFactory::create("Fusion"));

        const QColor lighterGray(75, 75, 75);
        const QColor darkGray(53, 53, 53);
        const QColor gray(128, 128, 128);
        const QColor black(25, 25, 25);
        const QColor blue(198, 238, 255);

        QPalette darkPalette;
        darkPalette.setColor(QPalette::Window, darkGray);
        darkPalette.setColor(QPalette::WindowText, Qt::white);
        darkPalette.setColor(QPalette::Base, black);
        darkPalette.setColor(QPalette::AlternateBase, darkGray);
        darkPalette.setColor(QPalette::ToolTipBase, darkGray);
        darkPalette.setColor(QPalette::ToolTipText, Qt::white);
        darkPalette.setColor(QPalette::Text, Qt::white);
        darkPalette.setColor(QPalette::Button, darkGray);
        darkPalette.setColor(QPalette::ButtonText, Qt::white);
        darkPalette.setColor(QPalette::Link, blue);
        darkPalette.setColor(QPalette::Highlight, lighterGray);
        darkPalette.setColor(QPalette::HighlightedText, Qt::white);
        darkPalette.setColor(QPalette::PlaceholderText, QColor(Qt::white).darker());

        darkPalette.setColor(QPalette::Active, QPalette::Button, darkGray);
        darkPalette.setColor(QPalette::Disabled, QPalette::ButtonText, gray);
        darkPalette.setColor(QPalette::Disabled, QPalette::WindowText, gray);
        darkPalette.setColor(QPalette::Disabled, QPalette::Text, gray);
        darkPalette.setColor(QPalette::Disabled, QPalette::Light, darkGray);

        app->setPalette(darkPalette);

        app->setStyleSheet("QToolTip { color: #ffffff; background-color: #2a82da; border: 1px solid white; }");
    }
    else if (theme.endsWith(".qss"))
    {
        QString themePath;
        themePath = QString::fromStdString(CoreGetSharedDataDirectory().string());
        themePath += "/Styles/";
        themePath += theme;

        // use Fusion as a base for the stylesheet
        app->setPalette(QApplication::style()->standardPalette());
        app->setStyleSheet(QString());
        app->setStyle(QStyleFactory::create("Fusion"));

        // set the stylesheet theme,
        // if the file exists and can be opened
        QFile themeFile(themePath);
        if (themeFile.exists() && 
            themeFile.open(QIODevice::ReadOnly))
        {
            app->setStyleSheet(themeFile.readAll());
        }
    }

    // set application icon theme
    QString iconTheme = QString::fromStdString(CoreSettingsGetStringValue(SettingsID::GUI_IconTheme));
    if (iconTheme == "White" || iconTheme == "Black")
    {
        QIcon::setThemeName(iconTheme.toLower());
    }
    else
    { // fallback to automatic
        QPalette palette = app->palette();
        bool dark = palette.windowText().color().value() > palette.window().color().value();
        QIcon::setThemeName(dark ? "white" : "black");
    }

    // fallback for icons we don't provide (i.e standard system icons)
    QIcon::setFallbackThemeName(fallbackThemeName);
}

void MainWindow::showErrorMessage(QString text, QString details)
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
    this->ui_Geometry_Maximized = this->isMaximized();
    this->ui_Geometry_Saved = true;

    std::string geometryStr = this->ui_Geometry.toBase64().toStdString();
    CoreSettingsSetValue(SettingsID::RomBrowser_Geometry, geometryStr);
    CoreSettingsSetValue(SettingsID::RomBrowser_Maximized, this->ui_Geometry_Maximized);
}

void MainWindow::loadGeometry(void)
{
    if (!this->ui_Geometry_Saved)
    {
        return;
    }

    if (this->ui_Geometry_Maximized)
    {
        this->showMaximized();
    }
    else
    {
        this->restoreGeometry(this->ui_Geometry);
    }

    if (this->isFullScreen())
    {
        this->showNormal();
    }

    if (this->ui_ShowMenubar && this->menuBar()->isHidden())
    {
        this->menuBar()->show();
    }
    else if (!this->ui_ShowMenubar && !this->menuBar()->isHidden())
    {
        this->menuBar()->hide();
    }

    if (this->ui_ShowToolbar && this->toolBar->isHidden())
    {
        this->toolBar->show();
    }
    else if (!this->ui_ShowToolbar && !this->toolBar->isHidden())
    {
        this->toolBar->hide();
    }

    if (this->ui_ShowStatusbar && this->statusBar()->isHidden())
    {
        this->statusBar()->show();
    }
    else if (!this->ui_ShowStatusbar && !this->statusBar()->isHidden())
    {
        this->statusBar()->hide();
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
        // always go back to ROM Browser
        this->ui_NoSwitchToRomBrowser = false;
        this->updateUI(false, false);

        this->showErrorMessage("CoreArePluginsReady() Failed", QString::fromStdString(CoreGetError()));
        return;
    }

    this->ui_HideCursorInEmulation = CoreSettingsGetBoolValue(SettingsID::GUI_HideCursorInEmulation);
    this->ui_HideCursorInFullscreenEmulation = CoreSettingsGetBoolValue(SettingsID::GUI_HideCursorInFullscreenEmulation);

    if (this->ui_ShowUI)
    {
        this->ui_ShowToolbar = CoreSettingsGetBoolValue(SettingsID::GUI_Toolbar);
        this->ui_ShowStatusbar = CoreSettingsGetBoolValue(SettingsID::GUI_StatusBar);
    }

    this->ui_Widget_OpenGL->SetHideCursor(this->ui_HideCursorInEmulation);

    this->emulationThread->SetRomFile(cartRom);
    this->emulationThread->SetDiskFile(diskRom);
    this->emulationThread->start();
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
    this->menuSpeedFactor->setEnabled(inEmulation);
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

    // configure keybindings for speed factor
    QAction* speedActions[] =
    {
        this->actionSpeed25, this->actionSpeed50, this->actionSpeed75,
        this->actionSpeed100, this->actionSpeed125, this->actionSpeed150,
        this->actionSpeed175, this->actionSpeed200, this->actionSpeed225,
        this->actionSpeed250, this->actionSpeed275, this->actionSpeed300
    };
    SettingsID speedKeybindSettingsId[] =
    {
        SettingsID::KeyBinding_SpeedFactor25, SettingsID::KeyBinding_SpeedFactor50,
        SettingsID::KeyBinding_SpeedFactor75, SettingsID::KeyBinding_SpeedFactor100,
        SettingsID::KeyBinding_SpeedFactor125, SettingsID::KeyBinding_SpeedFactor150,
        SettingsID::KeyBinding_SpeedFactor175, SettingsID::KeyBinding_SpeedFactor200,
        SettingsID::KeyBinding_SpeedFactor225, SettingsID::KeyBinding_SpeedFactor250,
        SettingsID::KeyBinding_SpeedFactor275, SettingsID::KeyBinding_SpeedFactor300
    };
    for (int i = 0; i < 12; i++)
    {
        keyBinding = QString::fromStdString(CoreSettingsGetStringValue(speedKeybindSettingsId[i]));
        speedActions[i]->setShortcut(QKeySequence(keyBinding));
    }

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
    int currentSlot = CoreGetSaveStateSlot();
    for (int i = 0; i < 10; i++)
    {
        keyBinding = QString::fromStdString(CoreSettingsGetStringValue(slotKeybindSettingsId[i]));
        slotActions[i]->setShortcut(QKeySequence(keyBinding));
        slotActions[i]->setChecked(i == currentSlot);
    }

    // configure text and filename data for save slots
    this->updateSaveStateSlotActions(inEmulation, isPaused);

    keyBinding = QString::fromStdString(CoreSettingsGetStringValue(SettingsID::KeyBinding_GraphicsSettings));
    this->action_Settings_Graphics->setEnabled(CorePluginsHasConfig(CorePluginType::Gfx));
    this->action_Settings_Graphics->setShortcut(QKeySequence(keyBinding));
    keyBinding = QString::fromStdString(CoreSettingsGetStringValue(SettingsID::KeyBinding_AudioSettings));
    this->action_Settings_Audio->setEnabled(CorePluginsHasConfig(CorePluginType::Audio));
    this->action_Settings_Audio->setShortcut(QKeySequence(keyBinding));
    keyBinding = QString::fromStdString(CoreSettingsGetStringValue(SettingsID::KeyBinding_RspSettings));
    this->action_Settings_Rsp->setEnabled(CorePluginsHasConfig(CorePluginType::Rsp));
    this->action_Settings_Rsp->setShortcut(QKeySequence(keyBinding));
    keyBinding = QString::fromStdString(CoreSettingsGetStringValue(SettingsID::KeyBinding_InputSettings));
    this->action_Settings_Input->setEnabled(CorePluginsHasConfig(CorePluginType::Input));
    this->action_Settings_Input->setShortcut(QKeySequence(keyBinding));
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

void MainWindow::updateSaveStateSlotActions(bool inEmulation, bool isPaused)
{
    std::filesystem::path saveStatePath;

    // do nothing when paused
    if (isPaused)
    {
        return;
    }

    QAction* slotActions[] =
    {
        this->actionSlot_0, this->actionSlot_1, this->actionSlot_2,
        this->actionSlot_3, this->actionSlot_4, this->actionSlot_5,
        this->actionSlot_6, this->actionSlot_7, this->actionSlot_8,
        this->actionSlot_9
    };
    for (int i = 0; i < 10; i++)
    {
        if (inEmulation &&
            CoreGetSaveStatePath(i, saveStatePath))
        {
            slotActions[i]->setData(QString::fromStdString(saveStatePath.string()));
            slotActions[i]->setText(this->getSaveStateSlotText(slotActions[i], i));
        }
        else
        {
            slotActions[i]->setData("");
            slotActions[i]->setText(this->getSaveStateSlotText(slotActions[i], i));
        }
    }
}

void MainWindow::addActions(void)
{
    if (this->ui_AddedActions)
    {
        return;
    }

    for (QAction* action : this->ui_Actions)
    {
        this->addAction(action);
    }

    this->ui_AddedActions = true;
}

void MainWindow::removeActions(void)
{
    if (!this->ui_AddedActions)
    {
        return;
    }

    for (QAction* action : this->ui_Actions)
    {
        this->removeAction(action);
    }

    this->ui_AddedActions = false;
}

QString MainWindow::getSaveStateSlotText(QAction* action, int slot)
{
    QString saveStateSlotText;
    QString filePath;

    // base text
    saveStateSlotText = "Slot " + QString::number(slot);

    // retrieve file name
    filePath = action->data().toString();

    // check if file exists, if it does,
    // return a string with the datetime
    QFileInfo saveStateFileInfo(filePath);
    if (!filePath.isEmpty() && saveStateFileInfo.exists())
    {
        saveStateSlotText += " - ";
        saveStateSlotText += saveStateFileInfo.lastModified().toString("dd/MM/yyyy hh:mm");
    }

    return saveStateSlotText;
}

void MainWindow::configureActions(void)
{
    // configure actions list
    this->ui_Actions.append(
    {
        // System actions
        this->action_System_StartRom, this->action_System_OpenCombo,
        this->action_System_Shutdown, this->action_System_SoftReset,
        this->action_System_HardReset, this->action_System_Pause,
        this->action_System_Screenshot, this->action_System_LimitFPS,
        this->actionSpeed25, this->actionSpeed50, this->actionSpeed75,
        this->actionSpeed100, this->actionSpeed125, this->actionSpeed150,
        this->actionSpeed175, this->actionSpeed200, this->actionSpeed225,
        this->actionSpeed250, this->actionSpeed275, this->actionSpeed300,
        this->action_System_SaveState, this->action_System_SaveAs,
        this->action_System_LoadState, this->action_System_Load,
        this->actionSlot_0, this->actionSlot_1, this->actionSlot_2,
        this->actionSlot_3, this->actionSlot_4, this->actionSlot_5,
        this->actionSlot_6, this->actionSlot_7, this->actionSlot_8,
        this->actionSlot_9, this->action_System_Cheats,
        this->action_System_GSButton, this->action_System_Exit,
        // Settings actions
        this->action_Settings_Graphics, this->action_Settings_Audio,
        this->action_Settings_Rsp, this->action_Settings_Input,
        this->action_Settings_Settings,
        // View actions
        this->action_View_Fullscreen, this->action_Help_Github,
        // Help actions
        this->action_Help_About,
    });

    // configure emulation speed actions
    QActionGroup* speedActionGroup = new QActionGroup(this);
    QAction* speedActions[] =
    {
        this->actionSpeed25, this->actionSpeed50, this->actionSpeed75,
        this->actionSpeed100, this->actionSpeed125, this->actionSpeed150,
        this->actionSpeed175, this->actionSpeed200, this->actionSpeed225,
        this->actionSpeed250, this->actionSpeed275, this->actionSpeed300
    };
    int speedActionNumbers[] =
    {
        25, 50, 75, 100, 
        125, 150, 175, 200,
        225, 250, 275, 300
    };
    int currentSpeedFactor = CoreGetSpeedFactor();
    for (int i = 0; i < 12; i++)
    {
        QAction* speedAction = speedActions[i];

        speedAction->setCheckable(true);
        speedAction->setChecked(currentSpeedFactor == speedActionNumbers[i]);
        speedAction->setActionGroup(speedActionGroup);

        // connect emulation speed action here because we need to do
        // something special for them
        connect(speedAction, &QAction::triggered, [=](bool checked)
        {
            if (checked)
            {
                int factor = speedAction->text().split("%").first().toInt();
                this->on_Action_System_SpeedFactor(factor);
            }
        });
    }

    // configure save slot actions
    QActionGroup* slotActionGroup = new QActionGroup(this);
    QAction* slotActions[] =
    {
        this->actionSlot_0, this->actionSlot_1, this->actionSlot_2,
        this->actionSlot_3, this->actionSlot_4, this->actionSlot_5,
        this->actionSlot_6, this->actionSlot_7, this->actionSlot_8,
        this->actionSlot_9
    };
    for (int i = 0; i < 10; i++)
    {
        QAction* slotAction = slotActions[i];

        slotAction->setCheckable(true);
        slotAction->setChecked(i == 0);
        slotAction->setActionGroup(slotActionGroup);

        // connect slot action here because we need to do
        // something special for them
        connect(slotAction, &QAction::triggered, [=](bool checked)
        {
            if (checked)
            {
                int slot = slotAction->text().split(" ").at(1).toInt();
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
    connect(this->action_Help_Update, &QAction::triggered, this, &MainWindow::on_Action_Help_Update);
}

#ifdef UPDATER
void MainWindow::checkForUpdates(bool silent, bool force)
{
    if (!force && !CoreSettingsGetBoolValue(SettingsID::GUI_CheckForUpdates))
    {
        return;
    }

    // only check for updates on the stable versions unless forced
    QString currentVersion = QString::fromStdString(CoreGetVersion());
    if (!force && currentVersion.size() != 6)
    {
        return;
    }

    QString dateTimeFormat = "dd-MM-yyyy_hh:mm";
    QString lastUpdateCheckDateTimeString = QString::fromStdString(CoreSettingsGetStringValue(SettingsID::GUI_LastUpdateCheck));
    QDateTime lastUpdateCheckDateTime = QDateTime::fromString(lastUpdateCheckDateTimeString, dateTimeFormat);
    QDateTime currentDateTime = QDateTime::currentDateTime();

    // only check for updates once every hour unless forced
    if (!force &&
        lastUpdateCheckDateTime.isValid() &&
        lastUpdateCheckDateTime.addSecs(3600) > currentDateTime)
    {
        return;
    }

    // update last update check date & time
    CoreSettingsSetValue(SettingsID::GUI_LastUpdateCheck, currentDateTime.toString(dateTimeFormat).toStdString());

    // whether or not the update check is silent
    this->ui_SilentUpdateCheck = silent;

    // execute update check
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
    else if (timerId == this->ui_UpdateSaveStateSlotTimerId)
    {
        this->updateSaveStateSlotActions(CoreIsEmulationRunning(), false);
        this->killTimer(timerId);
        this->ui_UpdateSaveStateSlotTimerId = 0;
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
        if (!this->ui_SilentUpdateCheck)
        {
            this->showErrorMessage("Failed to check for updates!", reply->errorString());
        }
        reply->deleteLater();
        return;
    }

    QJsonDocument jsonDocument = QJsonDocument::fromJson(reply->readAll());
    QJsonObject jsonObject = jsonDocument.object();

    QString currentVersion = QString::fromStdString(CoreGetVersion());
    QString latestVersion = jsonObject.value("tag_name").toString();

    reply->deleteLater();

    // do nothing when versions match
    if (currentVersion == latestVersion)
    {
        if (!this->ui_SilentUpdateCheck)
        {
            this->showErrorMessage("You're already on the latest version!");
        }
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

#ifdef APPIMAGE_UPDATER
    this->on_Action_System_Exit();
#else // normal updater
    Dialog::InstallUpdateDialog installUpdateDialog(this, QCoreApplication::applicationDirPath(), downloadUpdateDialog.GetTempDirectory(), downloadUpdateDialog.GetFileName());
    ret = installUpdateDialog.exec();
    if (ret != QDialog::Accepted)
    {
        return;
    }
#endif // APPIMAGE_UPDATER
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

void MainWindow::on_Action_System_SpeedFactor(int factor)
{
    if (!CoreSetSpeedFactor(factor))
    {
        this->showErrorMessage("CoreSetSpeedFactor() Failed!", QString::fromStdString(CoreGetError()));
    }
    else
    {
        OnScreenDisplaySetMessage("Playback speed: " + std::to_string(CoreGetSpeedFactor()) + "%");
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

        // refresh savestate slot times in 1 second,
        // kill any existing timers
        if (this->ui_UpdateSaveStateSlotTimerId != 0)
        {
            this->killTimer(this->ui_UpdateSaveStateSlotTimerId);
            this->ui_UpdateSaveStateSlotTimerId = 0;
        }
        this->ui_UpdateSaveStateSlotTimerId = this->startTimer(1000);
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
    if (!this->ui_ShowUI)
    {
        return;
    }

    CoreSettingsSetValue(SettingsID::GUI_Toolbar, checked);
    this->toolBar->setVisible(checked);
    this->ui_ShowToolbar = checked;
}

void MainWindow::on_Action_View_StatusBar(bool checked)
{
    if (!this->ui_ShowUI)
    {
        return;
    }

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

void MainWindow::on_Action_Help_Update(void)
{
#ifdef UPDATER
    this->checkForUpdates(false, true);
#endif // UPDATER
}

void MainWindow::on_Emulation_Started(void)
{
    this->logDialog.Clear();
}

void MainWindow::on_Emulation_Finished(bool ret)
{
    if (!ret)
    {
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

    // show error message to the user
    // after switching back to the ROM browser
    if (!ret)
    {
        this->showErrorMessage("EmulationThread::run Failed", this->emulationThread->GetLastError());
    }
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

    if (this->ui_ShowMenubar && this->menuBar()->isHidden())
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

    if (this->ui_ShowUI)
    {
        this->removeActions();
    }

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

    if (this->ui_ShowUI)
    {
        this->addActions();
    }
}

void MainWindow::on_VidExt_ResizeWindow(int width, int height)
{
    // account for HiDPI scaling
    if (this->devicePixelRatio() != 1)
    {
        width  = (int)std::ceil((double)((double)width  / this->devicePixelRatio()));
        height = (int)std::ceil((double)((double)height / this->devicePixelRatio()));
    }

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

    if (!this->ui_ShowUI)
    {
        this->addActions();
    }

    this->resize(width, height);

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

        if (this->ui_ShowMenubar && !this->menuBar()->isHidden())
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

        if (this->ui_ShowUI)
        {
            this->addActions();
        }
    }
    else
    {
        if (this->isFullScreen())
        {
            this->showNormal();
        }

        if (this->ui_ShowMenubar && this->menuBar()->isHidden())
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

        if (this->ui_ShowUI)
        {
            this->removeActions();
        }
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

void MainWindow::on_Core_StateCallback(CoreStateCallbackType type, int value)
{
    if (type == CoreStateCallbackType::SaveStateLoaded)
    {
        if (value == 0)
        {
            OnScreenDisplaySetMessage("Failed to load save state.");
        }
    }
    else if (type == CoreStateCallbackType::SaveStateSaved)
    {
        if (value == 0)
        {
            OnScreenDisplaySetMessage("Failed to save state.");
        }
    }
}
