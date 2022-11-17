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
#include <Config.hpp>

#include <QApplication>
#include <QCommandLineParser>
#include <QDir>
#include <QFile>

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    UserInterface::MainWindow window;

    QDir::setCurrent(app.applicationDirPath());

    QCoreApplication::setApplicationName("Rosalie's Mupen GUI");
    QCoreApplication::setApplicationVersion(VERSION_STR);

    // setup commandline parser
    QCommandLineParser parser;
    // default options
    parser.addHelpOption();
    parser.addVersionOption();
    // custom options
    QCommandLineOption fullscreenOption({"f", "fullscreen"}, "Launches ROM in fullscreen mode");
    parser.addOption(fullscreenOption);
    parser.addPositionalArgument("ROM", "ROM to open");

    // parse arguments
    parser.process(app);

    // initialize window
    if (!window.Init(&app))
    {
        return 1;
    }
    window.show();

    QStringList args = parser.positionalArguments();
    if (!args.empty())
    {
        window.OpenROM(args.at(0), parser.isSet(fullscreenOption));
    }

    return app.exec();
}