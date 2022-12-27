/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "RomBrowserLoadingWidget.hpp"

#include <QLabel>
#include <QHBoxLayout>

using namespace UserInterface::Widget;

RomBrowserLoadingWidget::RomBrowserLoadingWidget(QWidget* parent) : QWidget(parent)
{
    QHBoxLayout* layout = new QHBoxLayout(this);

    this->loadingLabel = new QLabel(this);
    this->loadingLabel->setText("Loading");
    this->loadingLabel->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);

    layout->addWidget(loadingLabel);

    this->setLayout(layout);
}

RomBrowserLoadingWidget::~RomBrowserLoadingWidget()
{
}

void RomBrowserLoadingWidget::SetWidgetIndex(int index)
{
    this->widgetIndex = index;
}

void RomBrowserLoadingWidget::on_RomBrowserWidget_currentChanged(int index)
{
    // start timer when it isnt running
    // and when we've switched to our widget,
    // else kill the timer
    if (this->widgetIndex == index)
    {
        if (this->loadingLabelTimerId == -1)
        {
            this->loadingLabelTimerId = this->startTimer(1000);
        }
    }
    else
    {
        if (this->loadingLabelTimerId != -1)
        {
            this->killTimer(this->loadingLabelTimerId);
            this->loadingLabelTimerId = -1;
        }
    }
}

void RomBrowserLoadingWidget::timerEvent(QTimerEvent *event)
{
    if (this->loadingLabel->text().size() >= 10)
    {
        this->loadingLabel->setText("Loading");
    }
    else
    {
        this->loadingLabel->setText(this->loadingLabel->text() + ".");
    }
}
