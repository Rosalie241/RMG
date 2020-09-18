#ifndef ROMSEARCHERTHREAD_HPP
#define ROMSEARCHERTHREAD_HPP

/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
*/
#include "../Globals.hpp"

#include <QThread>
#include <QString>

namespace Thread
{
    class RomSearcherThread : public QThread
    {
        Q_OBJECT

    public:
        RomSearcherThread(void);
        ~RomSearcherThread(void);

        void SetDirectory(QString);
        void SetRecursive(bool);
        void SetMaximumFiles(int);

        void run(void) override;

    private:
        QString rom_Directory;
        bool rom_Search_Recursive;
        int rom_Search_MaxItems;
        int rom_Search_Count;

        void rom_Search(QString);
        bool rom_Get_Info(QString, M64P::Wrapper::RomInfo_t*);

    signals:
        void on_Rom_Found(M64P::Wrapper::RomInfo_t);
    };
} // namespace Thread

#endif // ROMSEARCHERTHREAD_HPP
