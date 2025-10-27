/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020-2025 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "AlignCenterLayout.hpp"

using namespace UserInterface::Layout;

AlignCenterLayout::AlignCenterLayout(int spacing) : QLayout()
{
	this->setSpacing(spacing);
}

AlignCenterLayout::AlignCenterLayout(int spacing, QWidget* parent) : QLayout(parent)
{
	this->setSpacing(spacing);
}

AlignCenterLayout::~AlignCenterLayout(void)
{
	for (int i = 0; i < this->items.count(); i++)
	{
		delete this->items.at(i);
	}
	this->items.clear();
}

void AlignCenterLayout::addItem(QLayoutItem *item)
{
	this->items.append(item);
}

QSize AlignCenterLayout::sizeHint() const
{
    return this->minimumSize();
}

QSize AlignCenterLayout::minimumSize() const
{
    QSize size(0, 0);
    int count = this->items.count();
    for (int i = 0; i < count; i++)
    {
    	QLayoutItem* item = this->items.at(i);
        size = size.expandedTo(item->minimumSize());
    }

    return size + count * QSize(spacing(), spacing());
}

QSize AlignCenterLayout::maximumSize() const
{
    return this->minimumSize();
}

int AlignCenterLayout::count() const
{
	return this->items.count();
}

QLayoutItem* AlignCenterLayout::itemAt(int index) const
{
    return index >= 0 && index < this->items.size() ? this->items.value(index) : nullptr;
}

QLayoutItem* AlignCenterLayout::takeAt(int index)
{
    return index >= 0 && index < this->items.size() ? this->items.takeAt(index) : nullptr;
}

void AlignCenterLayout::setGeometry(const QRect &r)
{
    QLayout::setGeometry(r);

    if (this->items.size() != 3)
        return;

    QLayoutItem* leftItem = this->items.at(0);
    QLayoutItem* centerItem = this->items.at(1);
    QLayoutItem* rightItem = this->items.at(2);

    const int spacing = this->spacing();
    const int itemHeight = centerItem->minimumSize().height() + spacing;
    const int topLocation = r.y() + spacing;
    const int centerItemWidth = centerItem->sizeHint().width();
    const int centerItemLocation = (r.width() / 2) - (centerItemWidth / 2);
    const int sideItemWidth = leftItem->minimumSize().width();
    int leftItemWidth = sideItemWidth;
    int rightItemWidth = sideItemWidth;
    if (sideItemWidth >= centerItemLocation)
    {
        leftItemWidth  = (centerItemLocation - (spacing * 2));
        rightItemWidth = centerItemLocation;
    }

    leftItem->setGeometry(QRect(r.x(), topLocation, leftItemWidth, itemHeight));
    centerItem->setGeometry(QRect(centerItemLocation, topLocation, centerItemWidth, itemHeight));
    rightItem->setGeometry(QRect(r.width() - rightItemWidth + spacing, topLocation, rightItemWidth, itemHeight));
}

