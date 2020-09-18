/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
*/
#ifndef NOFOCUSDELEGATE_HPP
#define NOFOCUSDELEGATE_HPP

#include <QStyledItemDelegate>

namespace UserInterface
{
    class NoFocusDelegate : public QStyledItemDelegate
    {
    protected:
        void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    };
} // namespace UserInterface

#endif // NOFOCUSDELEGATE_HPP