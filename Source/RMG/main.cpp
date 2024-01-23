/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include <UserInterface/MainWindow.hpp>

#include <QApplication>
#include <QCommandLineParser>
#include <QFile>
#include <QDir>

#include <RMG-Core/Core.hpp>
#include <iostream>
#include <cstdlib>

#ifndef _WIN32
#include <signal.h>
#endif

//
// Local Functions
//

void message_handler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QByteArray localMsg = msg.toLocal8Bit();
    bool showDebugQtMessages = false;

    const char* env = std::getenv("RMG_SHOW_DEBUG_QT_MESSAGES");
    showDebugQtMessages = env != nullptr && std::string(env) == "1";

    std::string typeString;

    switch (type)
    {
    case QtDebugMsg:
        if (!showDebugQtMessages)
        {
            return;
        }
        typeString = "[QT DEBUG] ";
        break;
    case QtWarningMsg:
        if (!showDebugQtMessages)
        {
            return;
        }
        typeString = "[QT WARNING] ";
        break;
    case QtInfoMsg:
        if (!showDebugQtMessages)
        {
            return;
        }
        typeString = "[QT INFO] ";
        break;
    case QtCriticalMsg:
        typeString = "[QT CRITICAL] ";
        break;
    case QtFatalMsg:
        typeString = "[QT FATAL] ";
        break;
    }

    std::cerr << typeString << localMsg.constData() << std::endl;
}

void signal_handler(int sig)
{
    QGuiApplication::quit();
}

//
// Exported Functions
//

int main(int argc, char **argv)
{
    // install message handler
    qInstallMessageHandler(message_handler);

#ifndef _WIN32
    // on Linux we need to install signal handlers,
    // so we can exit cleanly when the user presses
    // i.e control+c
    signal(SIGINT,  signal_handler);
    signal(SIGTERM, signal_handler);

    // on Linux, wayland works only on some compositors,
    // it works on KDE plasma and sway (on 2023-07-26),
    // but i.e doesn't work on GNOME wayland or labwc, 
    // so to compromise the situation, we'll force xwayland
    // unless RMG_WAYLAND is set to 1, which'll force wayland
    // as qt platform, so users can experiment with the
    // wayland support themselves
    const char* wayland = std::getenv("RMG_WAYLAND");
    if (wayland != nullptr &&
        std::string(wayland) == "1")
    {
        setenv("QT_QPA_PLATFORM", "wayland", 1);
    }
    else
    {
        setenv("QT_QPA_PLATFORM", "xcb", 1);
    }

    // on Linux, Qt on some distributions
    // fails to load libvulkan, so to fix that
    // we'll help out and tell it about libvulkan.so.1,
    // but we wont overwrite the variable value if it
    // already contains one, the user might set it
    // to a custom value themselves
    setenv("QT_VULKAN_LIB", "libvulkan.so.1", 0);

    // ensure the default OpenGL format
    // doesn't have vsync enabled by default,
    // only needed for linux (for now)
    // and ensure OpenGL 3.3 is specified by default (for wayland)
    QSurfaceFormat format = QSurfaceFormat::defaultFormat();
    format.setSwapInterval(0);
    format.setMajorVersion(3);
    format.setMinorVersion(3);
    QSurfaceFormat::setDefaultFormat(format);

    // ensure that the desktop file is correctly
    // specified, else the window icon will be
    // the generic wayland icon on wayland
    QGuiApplication::setDesktopFileName("com.github.Rosalie241.RMG");
#endif

    QApplication app(argc, argv);

    UserInterface::MainWindow window;

#ifdef PORTABLE_INSTALL
    // only change current directory
    // when we're in portable directory mode
    if (CoreGetPortableDirectoryMode())
    {
        QDir::setCurrent(app.applicationDirPath());
    }
#endif

    QCoreApplication::setApplicationName("Rosalie's Mupen GUI");
    QCoreApplication::setApplicationVersion(QString::fromStdString(CoreGetVersion()));

    // setup commandline parser
    QCommandLineParser parser;
    // default options
    parser.addHelpOption();
    parser.addVersionOption();
    // custom options
#ifndef PORTABLE_INSTALL
    QCommandLineOption libPathOption("lib-path", "Changes the path where the libraries are stored", "path");
    QCommandLineOption corePathOption("core-path", "Changes the path where the core library is stored", "path");
    QCommandLineOption pluginPathOption("plugin-path", "Changes the path where the plugins are stored", "path");
    QCommandLineOption sharedDataPathOption("shared-data-path", "Changes the path where the shared data is stored", "path");
    libPathOption.setFlags(QCommandLineOption::HiddenFromHelp);
    corePathOption.setFlags(QCommandLineOption::HiddenFromHelp);
    pluginPathOption.setFlags(QCommandLineOption::HiddenFromHelp);
    sharedDataPathOption.setFlags(QCommandLineOption::HiddenFromHelp);
#endif // PORTABLE_INSTALL
    QCommandLineOption debugMessagesOption({"d", "debug-messages"}, "Prints debug callback messages to stdout");
    QCommandLineOption fullscreenOption({"f", "fullscreen"}, "Launches ROM in fullscreen mode");
    QCommandLineOption noGuiOption({"n", "nogui"}, "Hides GUI elements (menubar, toolbar, statusbar)");
    QCommandLineOption quitAfterEmulationOption({"q", "quit-after-emulation"}, "Quits RMG when emulation has finished");
    QCommandLineOption diskOption("disk", "64DD Disk to open ROM in combination with", "64DD Disk");

#ifndef PORTABLE_INSTALL
    parser.addOption(libPathOption);
    parser.addOption(corePathOption);
    parser.addOption(pluginPathOption);
    parser.addOption(sharedDataPathOption);
#endif // PORTABLE_INSTALL
    parser.addOption(debugMessagesOption);
    parser.addOption(fullscreenOption);
    parser.addOption(noGuiOption);
    parser.addOption(quitAfterEmulationOption);
    parser.addOption(diskOption);
    parser.addPositionalArgument("ROM", "ROM to open");

    // parse arguments
    parser.process(app);

#ifndef PORTABLE_INSTALL
    // set path overrides before initializing
    QString libPathOverride        = parser.value(libPathOption);
    QString corePathOveride        = parser.value(corePathOption);
    QString pluginPathOverride     = parser.value(pluginPathOption);
    QString sharedDataPathOverride = parser.value(sharedDataPathOption);
    if (!libPathOverride.isEmpty())
    {
        CoreSetLibraryPathOverride(libPathOverride.toStdString());
    }
    if (!corePathOveride.isEmpty())
    {
        CoreSetCorePathOverride(corePathOveride.toStdString());
    }
    if (!pluginPathOverride.isEmpty())
    {
        CoreSetPluginPathOverride(pluginPathOverride.toStdString());
    }
    if (!sharedDataPathOverride.isEmpty())
    {
        CoreSetSharedDataPathOverride(sharedDataPathOverride.toStdString());
    }
#endif // PORTABLE_INSTALL

    // print debug callbacks to stdout if needed
    CoreSetPrintDebugCallback(parser.isSet(debugMessagesOption));

    // specified ROM path to launch
    QStringList args = parser.positionalArguments();

    CoreAddCallbackMessage(CoreDebugMessageType::Info, 
            "Initializing on " + QGuiApplication::platformName().toStdString());

    // initialize window
    if (!window.Init(&app, !parser.isSet(noGuiOption), !args.empty()))
    {
        return 1;
    }

    if (!args.empty())
    {
        window.OpenROM(args.at(0), parser.value(diskOption), parser.isSet(fullscreenOption), parser.isSet(quitAfterEmulationOption));
    }

    // show window
    window.show();

    return app.exec();
}
