/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef ROWID_HPP
#define ROWID_HPP

#include <QString>

enum class RowID
{
    GoodName = 0,
    InternalName,
    MD5,
    Invalid
};

typedef struct
{
    RowID Id;
    QString Text;
    int Size;
} RowTitle_t;

static const RowTitle_t g_RowTitles[] = 
{
    { RowID::GoodName, "Name", 250 },
    { RowID::InternalName, "Internal Name", 100 },
    { RowID::MD5, "MD5", 100 }
};

#endif // ROWID_HPP
