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
#include <QTimerEvent>

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

void RomBrowserLoadingWidget::SetCurrentRomIndex(int index, int count)
{
    this->romIndex = index;
    this->romCount = count;
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
            this->dotCount = 0;
            this->romIndex = 0;
            this->romCount = 0;
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

void RomBrowserLoadingWidget::updateLoadingText()
{
    QString loadingText = "Loading";

    if (this->elapsedTimeSinceLoading.isValid() &&
        this->elapsedTimeSinceLoading.elapsed() >= 5000)
    {
        QString romCountString = QString::number(this->romCount);

        loadingText += " [";
        loadingText += QString::number(this->romIndex).rightJustified(romCountString.size(), '0', false);
        loadingText += "/";
        loadingText += romCountString;
        loadingText += "]";
    }

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

void RomBrowserLoadingWidget::timerEvent(QTimerEvent *event)
{
    if (event->timerId() != this->loadingLabelTimerId)
    {
        return;
    }

   this->updateLoadingText();
}
