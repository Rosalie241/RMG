/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef COLUMNID_HPP
#define COLUMNID_HPP

#include <QString>

enum class ColumnID
{
    GoodName = 0,
    InternalName,
    MD5,
    Invalid
};

typedef struct
{
    ColumnID Id;
    QString Text;
    int Size;
} ColumnTitle_t;

static const ColumnTitle_t g_ColumnTitles[] = 
{
    { ColumnID::GoodName, "Name", 250 },
    { ColumnID::InternalName, "Internal Name", 100 },
    { ColumnID::MD5, "MD5", 100 }
};

#endif // COLUMNID_HPP
