/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020-2025 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "CreateNetplaySessionWidget.hpp"

Q_DECLARE_METATYPE(NetplayRomData);

using namespace UserInterface::Widget;

//
// Exported Functions
// 

CreateNetplaySessionWidget::CreateNetplaySessionWidget(QWidget* parent) : QStackedWidget(parent)
{
    // configure signal types
    qRegisterMetaType<NetplayRomData>("NetplayRomData");

    // configure empty widget
    this->emptyWidget = new Widget::CreateNetplaySessionEmptyWidget(this);
    this->addWidget(this->emptyWidget);

    // configure list widget
    this->listWidget = new QListWidget(this);
    this->addWidget(this->listWidget);
    connect(this->listWidget, &QListWidget::currentRowChanged, this, &CreateNetplaySessionWidget::on_listWidget_currentRowChanged);

    this->setCurrentWidget(this->listWidget);
}

CreateNetplaySessionWidget::~CreateNetplaySessionWidget()
{
}

void CreateNetplaySessionWidget::AddRomData(QString goodName, QString md5, QString file)
{
    const NetplayRomData romData =
    {
        goodName,
        md5,
        file
    };

    QListWidgetItem* item = new QListWidgetItem();
    item->setData(Qt::UserRole, QVariant::fromValue(romData));
    item->setText(goodName);
    this->listWidget->addItem(item);
}

void CreateNetplaySessionWidget::RefreshDone(void)
{
    if (this->listWidget->count() == 0)
    {
        this->setCurrentWidget(this->emptyWidget);
    }
    else
    {
        this->listWidget->sortItems();
        this->setCurrentWidget(this->listWidget);
    }
}

bool CreateNetplaySessionWidget::IsCurrentRomValid()
{
    return this->currentWidget() == this->listWidget &&
            this->listWidget->currentItem() != nullptr;
}

bool CreateNetplaySessionWidget::GetCurrentRom(NetplayRomData& data)
{
    if (!this->IsCurrentRomValid())
    {
        return false;
    }

    QListWidgetItem* item = this->listWidget->currentItem();
    if (item == nullptr)
    {
        return false;
    }

    data = item->data(Qt::UserRole).value<NetplayRomData>();
    return true;
}

void CreateNetplaySessionWidget::on_listWidget_currentRowChanged(int index)
{
    emit this->OnRomChanged(index != -1);
}
