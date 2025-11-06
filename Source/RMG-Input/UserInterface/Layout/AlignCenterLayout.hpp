/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020-2025 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef ALIGNCENTERLAYOUT_HPP
#define ALIGNCENTERLAYOUT_HPP

#include <QLayoutItem>
#include <QLayout>
#include <QList>

namespace UserInterface
{
namespace Layout
{
class AlignCenterLayout : public QLayout
{
private:
    QList<QLayoutItem *> items;

public:
    AlignCenterLayout(int spacing);
    ~AlignCenterLayout(void);

    void addItem(QLayoutItem* item) override;
    QSize sizeHint() const override;
    QSize minimumSize() const override;
    QSize maximumSize() const override;
    int count() const override;
    QLayoutItem *itemAt(int) const override;
    QLayoutItem *takeAt(int) override;
    void setGeometry(const QRect& rect) override;
};
}
}

#endif // ALIGNCENTERLAYOUT_HPP