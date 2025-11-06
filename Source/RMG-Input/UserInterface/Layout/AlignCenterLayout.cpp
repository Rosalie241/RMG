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

    // take spacing for top & bottom into account
    size.setHeight(size.height() + (this->spacing() * 2));

    return size;
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

void AlignCenterLayout::setGeometry(const QRect& rect)
{
    QLayout::setGeometry(rect);

    QLayoutItem* leftItem = this->items.at(0);
    QLayoutItem* centerItem = this->items.at(1);
    QLayoutItem* rightItem = this->items.at(2);


    // the side widgets are limited to 25% of available width
    // the middle widget is limited to 1/3rd of the available width
    //
    // this also takes into account the situation where there's an overlap
    // between widgets, which is something this avoids.
    const int spacing = this->spacing();
    const int itemHeight = centerItem->minimumSize().height() + spacing;
    const int rectWidth = rect.width();
    const int topLocation = rect.y() + spacing;
    const int centerItemWidth = std::min(centerItem->sizeHint().width(), rectWidth / 3);
    const int centerItemLocation = ((rectWidth / 2)) - (centerItemWidth / 2);
    const int maxSideItemWidth = std::min(centerItemLocation, rectWidth / 4) - spacing;
    const int sideItemWidth = std::min(leftItem->minimumSize().width(), maxSideItemWidth);

    leftItem->setGeometry(QRect(rect.x(), topLocation, sideItemWidth, itemHeight));
    centerItem->setGeometry(QRect(rect.x() + centerItemLocation, topLocation, centerItemWidth, itemHeight));
    rightItem->setGeometry(QRect(rect.x() + rectWidth - sideItemWidth, topLocation, sideItemWidth, itemHeight));
}
