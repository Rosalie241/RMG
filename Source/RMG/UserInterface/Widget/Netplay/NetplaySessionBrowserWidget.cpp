/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "NetplaySessionBrowserWidget.hpp"
#include "UserInterface/NoFocusDelegate.hpp"

#include <QHeaderView>

Q_DECLARE_METATYPE(NetplaySessionData);

using namespace UserInterface::Widget;

//
// Exported Functions
// 

NetplaySessionBrowserWidget::NetplaySessionBrowserWidget(QWidget* parent) : QStackedWidget(parent)
{
    // configure signal types
    qRegisterMetaType<NetplaySessionData>("NetplaySessionData");

    // configure empty widget
    this->emptyWidget = new Widget::NetplaySessionBrowserEmptyWidget(this);
    this->addWidget(this->emptyWidget);

    // configure loading widget
    this->loadingWidget = new Widget::NetplaySessionBrowserLoadingWidget(this);
    connect(this, &QStackedWidget::currentChanged, this->loadingWidget, &NetplaySessionBrowserLoadingWidget::on_NetplaySessionBrowserWidget_currentChanged);
    this->loadingWidget->SetWidgetIndex(this->addWidget(this->loadingWidget));

    // configure table widget
    this->tableWidget = new QTableWidget(this);
    this->tableWidget->setFrameStyle(QFrame::NoFrame);
    this->tableWidget->setItemDelegate(new NoFocusDelegate(this));
    this->tableWidget->setWordWrap(false);
    this->tableWidget->setShowGrid(false);
    this->tableWidget->setSortingEnabled(true);
    this->tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    this->tableWidget->setSelectionBehavior(QTableView::SelectRows);
    this->tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    this->tableWidget->setVerticalScrollMode(QAbstractItemView::ScrollMode::ScrollPerPixel);
    this->tableWidget->verticalHeader()->hide();
    this->tableWidget->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    this->tableWidget->horizontalHeader()->setSectionsMovable(true);
    this->tableWidget->horizontalHeader()->setFirstSectionMovable(true);
    this->tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    this->tableWidget->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
    this->tableWidget->horizontalHeader()->setSortIndicatorShown(false);
    this->tableWidget->horizontalHeader()->setHighlightSections(false);
    this->tableWidget->horizontalHeader()->setStretchLastSection(true);
    this->addWidget(this->tableWidget);
    connect(this->tableWidget, &QTableWidget::currentItemChanged, this, &NetplaySessionBrowserWidget::on_tableWidget_currentItemChanged);

    // set up table widget's columns
    QStringList labels;
    labels << tr("Name");
    labels << tr("Game");
    labels << tr("Game MD5");
    labels << tr("Password?");
    this->tableWidget->setColumnCount(labels.size());
    this->tableWidget->setHorizontalHeaderLabels(labels);

    this->setCurrentWidget(this->loadingWidget);
}

NetplaySessionBrowserWidget::~NetplaySessionBrowserWidget()
{
}

void NetplaySessionBrowserWidget::Reset(void)
{
    this->setCurrentWidget(this->emptyWidget);
    this->tableWidget->model()->removeRows(0, this->tableWidget->rowCount());
}

void NetplaySessionBrowserWidget::StartRefresh(void)
{
    this->refreshTimer.start();
    this->setCurrentWidget(this->loadingWidget);
    this->tableWidget->model()->removeRows(0, this->tableWidget->rowCount());
}

void NetplaySessionBrowserWidget::AddSessionData(QString name, QString game, QString md5, bool password, int port, 
                                                 QString cpuEmulator, QString rspPlugin, QString gfxPlugin)
{
    const NetplaySessionData sessionData =
    {
        name,
        game,
        md5,
        password,
        port,
        cpuEmulator,
        rspPlugin,
        gfxPlugin
    };

    int row = this->tableWidget->rowCount();
    this->tableWidget->insertRow(row);

    // Session name
    QTableWidgetItem* tableWidgetItem1 = new QTableWidgetItem(name);
    tableWidgetItem1->setData(Qt::UserRole, QVariant::fromValue<NetplaySessionData>(sessionData));
    this->tableWidget->setItem(row, 0, tableWidgetItem1);

    // Game
    QTableWidgetItem* tableWidgetItem2 = new QTableWidgetItem(game);
    this->tableWidget->setItem(row, 1, tableWidgetItem2);

    // MD5
    QTableWidgetItem* tableWidgetItem3 = new QTableWidgetItem(md5);
    this->tableWidget->setItem(row, 2, tableWidgetItem3);

    // Password
    QTableWidgetItem* tableWidgetItem4 = new QTableWidgetItem(password ? "Yes" : "No");
    this->tableWidget->setItem(row, 3, tableWidgetItem4);
}

void NetplaySessionBrowserWidget::RefreshDone(void)
{
    if (this->tableWidget->rowCount() == 0)
    {
        this->setCurrentWidget(this->emptyWidget);
        return;
    }

    // prevent flicker by forcing the loading screen
    // to be shown at least 300ms
    qint64 elapsedTime = this->refreshTimer.elapsed();
    if (elapsedTime < 300)
    {
        this->startTimer(300 - elapsedTime);
        return;
    }

    this->setCurrentWidget(this->tableWidget);
}

bool NetplaySessionBrowserWidget::IsCurrentSessionValid()
{
    return this->currentWidget() == this->tableWidget &&
            this->tableWidget->currentItem() != nullptr;
}

bool NetplaySessionBrowserWidget::GetCurrentSession(NetplaySessionData& data)
{
    if (!this->IsCurrentSessionValid())
    {
        return false;
    }

    QTableWidgetItem* item = this->tableWidget->item(this->tableWidget->currentRow(), 0);
    if (item == nullptr)
    {
        return false;
    }

    data = item->data(Qt::UserRole).value<NetplaySessionData>();
    return true;
}

void NetplaySessionBrowserWidget::timerEvent(QTimerEvent *event)
{
    this->killTimer(event->timerId());
    this->setCurrentWidget(this->tableWidget);
}

void NetplaySessionBrowserWidget::on_tableWidget_currentItemChanged(QTableWidgetItem* current, QTableWidgetItem* previous)
{
    if (this->currentWidget() == this->tableWidget)
    {
        emit this->OnSessionChanged(current != nullptr);
    }
}
