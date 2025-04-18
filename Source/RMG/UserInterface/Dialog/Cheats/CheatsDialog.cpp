/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020-2025 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "CheatsDialog.hpp"

#include "ChooseCheatOptionDialog.hpp"
#include "Utilities/QtMessageBox.hpp"
#include "AddCheatDialog.hpp"
#include "CheatsCommon.hpp"

#include <QJsonObject>
#include <QFileInfo>

#include <RMG-Core/Settings.hpp>
#include <RMG-Core/Error.hpp>
#include <RMG-Core/Rom.hpp>

Q_DECLARE_METATYPE(CoreCheat);

using namespace UserInterface::Dialog;
using namespace Utilities;

CheatsDialog::CheatsDialog(QWidget *parent, QString file, bool netplay, QJsonArray cheatsJson) : QDialog(parent)
{
    qRegisterMetaType<CoreCheat>();

    this->file = file;
    this->netplay = netplay;
    this->cheatsJson = cheatsJson;

    this->setupUi(this);
    this->loadCheats();
}

CheatsDialog::~CheatsDialog(void)
{
}

bool CheatsDialog::HasFailed(void)
{
    return this->failedToParseCheats;
}

QJsonArray CheatsDialog::GetJson(void)
{
    return this->cheatsJson;
}

void CheatsDialog::loadCheats(void)
{
    std::vector<CoreCheat> cheats;

    if (!CoreGetCurrentCheats(file.toStdU32String(), cheats))
    {
        QtMessageBox::Error(this, "CoreGetCurrentCheats() Failed", QString::fromStdString(CoreGetError()));
        this->failedToParseCheats = true;
        return;
    }

    CheatsCommon::AddCheatsToTreeWidget(this->netplay, this->cheatsJson, this->file, cheats, this->cheatsTreeWidget, false);
}

void CheatsDialog::on_cheatsTreeWidget_itemChanged(QTreeWidgetItem *item, int column)
{
    if (item == nullptr || item->data(column, Qt::UserRole).isNull())
    {
        return;
    }


    CoreCheat cheat = item->data(column, Qt::UserRole).value<CoreCheat>();
    bool enabled = (item->checkState(column) == Qt::CheckState::Checked);

    if (enabled && cheat.HasOptions && !CheatsCommon::HasCheatOptionSet(this->netplay, this->cheatsJson, this->file, cheat))
    {
        // ask user to select an option
        this->on_cheatsTreeWidget_itemDoubleClicked(item, column);
        
        // make sure the user selected an option,
        // if they did then enable the cheat as requested,
        // if not, reset the checkbox
        if (!CheatsCommon::HasCheatOptionSet(this->netplay, this->cheatsJson, this->file, cheat))
        {
            item->setCheckState(0, Qt::CheckState::Unchecked);
            return;
        }
    }

    CheatsCommon::EnableCheat(this->netplay, this->cheatsJson, this->file, cheat, enabled);
}

void CheatsDialog::on_cheatsTreeWidget_currentItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous)
{
    if (current == nullptr || current->data(0, Qt::UserRole).isNull())
    { // we can't edit/remove a section
        this->removeCheatButton->setEnabled(false);
        this->editCheatButton->setEnabled(false);
        this->notesTextEdit->setPlainText("");
        return;
    }

    CoreCheat cheat = current->data(0, Qt::UserRole).value<CoreCheat>();

    this->removeCheatButton->setEnabled(CoreCanRemoveCheat(cheat));
    this->editCheatButton->setEnabled(true);
    this->notesTextEdit->setPlainText(QString::fromStdString(cheat.Note));
}

void CheatsDialog::on_cheatsTreeWidget_itemDoubleClicked(QTreeWidgetItem *item, int column)
{
    if (item == nullptr || item->data(column, Qt::UserRole).isNull())
    {
        return;
    }

    CoreCheat cheat = item->data(column, Qt::UserRole).value<CoreCheat>();
    if (!cheat.HasOptions)
    {
        return;
    }

    Dialog::ChooseCheatOptionDialog dialog(this, this->file, cheat, this->netplay, this->cheatsJson);
    dialog.exec();

    // update json
    this->cheatsJson = dialog.GetJson();

    // re-load cheat options
    item->setText(column, CheatsCommon::GetCheatTreeWidgetItemName(this->netplay, this->cheatsJson, this->file, cheat));
}

void CheatsDialog::on_addCheatButton_clicked(void)
{
    AddCheatDialog dialog(this, this->file);
    dialog.exec();

    // re-load cheats
    this->loadCheats();
}

void CheatsDialog::on_editCheatButton_clicked(void)
{
    QTreeWidgetItem* item = this->cheatsTreeWidget->currentItem();

    if (item == nullptr || item->data(0, Qt::UserRole).isNull())
    {
        return;
    }

    CoreCheat cheat = item->data(0, Qt::UserRole).value<CoreCheat>();

    AddCheatDialog dialog(this, this->file);
    dialog.SetCheat(cheat);
    int ret = dialog.exec();
    if (ret == QDialog::Accepted)
    {
        // re-load cheats
        this->loadCheats();
    }
}

void CheatsDialog::on_removeCheatButton_clicked(void)
{
    QTreeWidgetItem* item = this->cheatsTreeWidget->currentItem();

    if (item == nullptr || item->data(0, Qt::UserRole).isNull())
    {
        return;
    }

    CoreCheat cheat = item->data(0, Qt::UserRole).value<CoreCheat>();

    // try to remove cheat
    if (!CoreRemoveCheat(this->file.toStdU32String(), cheat))
    {
        QtMessageBox::Error(this, "CoreRemoveCheat() Failed", QString::fromStdString(CoreGetError()));
        return;
    }

    // re-load cheats
    this->loadCheats();
}

void CheatsDialog::accept(void)
{
    if (this->netplay)
    {
        QDialog::accept();
        return;
    }

    CoreSettingsSave();

    if (CoreHasRomOpen() && !CoreApplyCheats())
    {
        QtMessageBox::Error(this, "CoreApplyCheats() Failed", QString::fromStdString(CoreGetError()));
        return;
    }

    QDialog::accept();
}
