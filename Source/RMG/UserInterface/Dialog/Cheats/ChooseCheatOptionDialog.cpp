/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "ChooseCheatOptionDialog.hpp"
#include "CheatsCommon.hpp"

#include <QFileInfo>

#include <RMG-Core/Core.hpp>

Q_DECLARE_METATYPE(CoreCheatOption);

using namespace UserInterface::Dialog;

ChooseCheatOptionDialog::ChooseCheatOptionDialog(CoreCheat cheat, bool netplay, QJsonArray cheatsJson, QWidget *parent) : QDialog(parent)
{
    qRegisterMetaType<CoreCheatOption>();

    this->setupUi(this);
    this->cheat = cheat;
    this->netplay = netplay;
    this->cheatsJson = cheatsJson;
    
    CoreCheatOption setCheatOption;

    bool checkCheatOption = CheatsCommon::HasCheatOptionSet(this->netplay, this->cheatsJson, cheat) &&
                            CheatsCommon::GetCheatOption(this->netplay, this->cheatsJson, cheat, setCheatOption);

    for (CoreCheatOption& option : cheat.CheatOptions)
    {
        QTreeWidgetItem* item = new QTreeWidgetItem();

        item->setText(0, QString::fromStdString(option.Name));
        if (checkCheatOption)
        {
            item->setCheckState(0, (option.Name == setCheatOption.Name ? Qt::CheckState::Checked : Qt::CheckState::Unchecked));
        }
        else
        {
            item->setCheckState(0, Qt::CheckState::Unchecked);
        }
        item->setData(0, Qt::UserRole, QVariant::fromValue(option));
        this->cheatOptionsTreeWidget->addTopLevelItem(item);
    }
}

ChooseCheatOptionDialog::~ChooseCheatOptionDialog()
{
}

QJsonArray ChooseCheatOptionDialog::GetJson(void)
{
    return this->cheatsJson;
}

void ChooseCheatOptionDialog::on_cheatOptionsTreeWidget_itemChanged(QTreeWidgetItem *item, int column)
{
    bool enabled = (item->checkState(column) == Qt::CheckState::Checked);

    if (enabled)
    {
        for (int i = 0; i < this->cheatOptionsTreeWidget->topLevelItemCount(); i++)
        {
            QTreeWidgetItem* widgetItem = this->cheatOptionsTreeWidget->topLevelItem(i);

            if (widgetItem != item)
            {
                widgetItem->setCheckState(column, Qt::CheckState::Unchecked);
            }
        }
    }
}

void ChooseCheatOptionDialog::accept(void)
{
    for (int i = 0; i < this->cheatOptionsTreeWidget->topLevelItemCount(); i++)
    {
        QTreeWidgetItem* item = this->cheatOptionsTreeWidget->topLevelItem(i);

        if (item->checkState(0) == Qt::CheckState::Checked)
        {
            CoreCheatOption cheatOption = item->data(0, Qt::UserRole).value<CoreCheatOption>();
            CheatsCommon::SetCheatOption(this->netplay, this->cheatsJson, this->cheat, cheatOption);
        }
    }

    QDialog::accept();
}
