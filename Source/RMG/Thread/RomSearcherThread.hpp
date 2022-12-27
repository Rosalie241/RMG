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

#include <QString>
#include <QThread>
#include <RMG-Core/Core.hpp>

namespace Thread
{
class RomSearcherThread : public QThread
{
    Q_OBJECT

  public:
    RomSearcherThread(QObject *);
    ~RomSearcherThread(void);

    void SetDirectory(QString);
    void SetRecursive(bool);
    void SetMaximumFiles(int);
    void Stop(void);

    void run(void) override;

  private:
    QString directory;
    bool recursive = false;
    int  maxItems = 0;
    bool stop = false;

    void searchDirectory(QString);

  signals:
    void RomFound(QString file, CoreRomHeader header, CoreRomSettings settings);
    void Finished(bool canceled);
};
} // namespace Thread

#endif // ROMSEARCHERTHREAD_HPP
