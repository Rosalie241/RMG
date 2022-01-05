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
#include <QDir>
#include <QFile>

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    UserInterface::MainWindow window;

    QDir::setCurrent(app.applicationDirPath());

    if (!window.Init(&app))
    {
        return 1;
    }

    window.show();

    // try to find an argument
    // with a file that exists,
    // if such an argument exists,
    // attempt to open that ROM
    for (int i = 1; i < argc; i++)
    {
        QString file = argv[i];
        if (QFile().exists(file))
        {
            window.OpenROM(file);
            break;
        }
    }

    return app.exec();
}