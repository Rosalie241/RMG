/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "NetplaySessionBrowserLoadingWidget.hpp"

#include <QLabel>
#include <QHBoxLayout>
#include <QTimerEvent>

using namespace UserInterface::Widget;

NetplaySessionBrowserLoadingWidget::NetplaySessionBrowserLoadingWidget(QWidget* parent) : QWidget(parent)
{
    QHBoxLayout* layout = new QHBoxLayout(this);

    this->loadingLabel = new QLabel(this);
    this->loadingLabel->setText("Loading");
    this->loadingLabel->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);

    layout->addWidget(loadingLabel);

    this->setLayout(layout);
}

NetplaySessionBrowserLoadingWidget::~NetplaySessionBrowserLoadingWidget()
{
}

void NetplaySessionBrowserLoadingWidget::SetWidgetIndex(int index)
{
    this->widgetIndex = index;
}

void NetplaySessionBrowserLoadingWidget::on_NetplaySessionBrowserWidget_currentChanged(int index)
{
    // start timer when it isnt running
    // and when we've switched to our widget,
    // else kill the timer
    if (this->widgetIndex == index)
    {
        if (this->loadingLabelTimerId == -1)
        {
            this->dotCount = 0;
            this->elapsedTimeSinceLoading.start();
            // reset loading text
            this->updateLoadingText();
            this->loadingLabelTimerId = this->startTimer(1000);
        }
    }
    else
    {
        if (this->loadingLabelTimerId != -1)
        {
            this->elapsedTimeSinceLoading.invalidate();
            this->killTimer(this->loadingLabelTimerId);
            this->loadingLabelTimerId = -1;
        }
    }
}

void NetplaySessionBrowserLoadingWidget::updateLoadingText()
{
    QString loadingText = "Loading";

    if (dotCount <= 3)
    {
        for (int i = 0; i < dotCount; i++)
        {
            loadingText += ".";
        }

        dotCount++;
    }
    else
    {
        dotCount = 1;
    }

    this->loadingLabel->setText(loadingText);
}

void NetplaySessionBrowserLoadingWidget::timerEvent(QTimerEvent *event)
{
    if (event->timerId() != this->loadingLabelTimerId)
    {
        return;
    }

   this->updateLoadingText();
}
