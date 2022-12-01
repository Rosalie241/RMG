/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "CheatsDialog.hpp"

#include "AddCheatDialog.hpp"
#include "ChooseCheatOptionDialog.hpp"

#include <QMessageBox>
#include <QFileInfo>
#include <iostream>

#include <RMG-Core/Core.hpp>

Q_DECLARE_METATYPE(CoreCheat);

using namespace UserInterface::Dialog;

CheatsDialog::CheatsDialog(QWidget *parent) : QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint)
{
    qRegisterMetaType<CoreCheat>();

    this->setupUi(this);
    this->loadCheats();
}

CheatsDialog::~CheatsDialog(void)
{
}

void CheatsDialog::loadCheats(void)
{
    std::vector<CoreCheat> cheats;

    // reset tree widget
    this->cheatsTreeWidget->clear();

    if (!CoreGetCurrentCheats(cheats))
    {
        this->showErrorMessage("CoreGetCurrentCheats() Failed!", QString::fromStdString(CoreGetError()));
        return;
    }

    for (CoreCheat& cheat : cheats)
    {
        QString name = QString::fromStdString(cheat.Name);
        QString section;
        QStringList sections = name.split("\\");
        bool enabled = CoreIsCheatEnabled(cheat);

        for (int i = 0; i < sections.size(); i++)
        {
            section = sections.at(i);

            // when item already exists,
            // we don't need to add it anymore
            QTreeWidgetItem* foundItem = this->findItem(sections, i, section);
            if (foundItem != nullptr)
            {
                continue;
            }

            QTreeWidgetItem* item = new QTreeWidgetItem();
            item->setText(0, section);

            // if at the last item,
            // add the cheat as data
            // and make it a checkbox
            if (i == (sections.size() - 1))
            {
                item->setText(0, this->getTreeWidgetItemTextFromCheat(cheat));
                item->setCheckState(0, (enabled ? Qt::CheckState::Checked : Qt::CheckState::Unchecked));
                item->setData(0, Qt::UserRole, QVariant::fromValue(cheat));
            }

            if (i == 0)
            { 
                this->cheatsTreeWidget->addTopLevelItem(item);
            }
            else
            {
                QTreeWidgetItem* foundParent = this->findItem(sections, i - 1, sections.at(i - 1));
                if (foundParent != nullptr)
                {
                    foundParent->addChild(item);
                }

                // when the cheat is enabled & we're at the last item,
                // expand each parent
                if (i == (sections.size() - 1) && enabled)
                {
                    QTreeWidgetItem* parent = foundParent;
                    while (parent != nullptr)
                    {
                        parent->setExpanded(true);
                        parent = parent->parent();
                    }
                }
            }
        }
    }

    this->cheatsTreeWidget->sortItems(0, Qt::SortOrder::AscendingOrder);
}

QTreeWidgetItem* CheatsDialog::findItem(QStringList sections, int size, QString itemText)
{
    QTreeWidgetItem* parent;
    bool foundItem = false;
    QList<QTreeWidgetItem*> foundItems = this->cheatsTreeWidget->findItems(itemText, Qt::MatchExactly | Qt::MatchRecursive, 0);

    // loop over all the found items,
    // then loop over all the parents
    // to make sure they match aswell
    for (auto& item : foundItems)
    {
        parent = item;
        foundItem = true;

        for (int i = size; i >= 0; i--)
        {
            // make sure the parent matches
            if (parent == nullptr || parent->text(0) != sections.at(i))
            {
                foundItem = false;
                break;
            }

            // make sure that when there are no sections left,
            // that there are no more parents either
            if (i == 0 && parent->parent() != nullptr)
            {
                foundItem = false;
                break;
            }

            parent = parent->parent();
        }

        // when we've found the item,
        // return it
        if (foundItem)
        {
            return item;
        }
    }

    return nullptr;
}

QString CheatsDialog::getTreeWidgetItemTextFromCheat(CoreCheat cheat)
{
    QString cheatName = QString::fromStdString(cheat.Name).split('\\').last();
    QString text;

    if (cheat.HasOptions)
    {
        CoreCheatOption cheatOption;
        if (!CoreHasCheatOptionSet(cheat) || !CoreGetCheatOption(cheat, cheatOption))
        {
            text = cheatName + " (=> ???? - Not Set)";
        }
        else
        {
            text = cheatName + " (=> ";
            text += QString::fromStdString(cheatOption.Name);
            text += ")";
        }
    }
    else
    {
        text = cheatName;
    }

    return text;
}

void CheatsDialog::showErrorMessage(QString error, QString details)
{
    QMessageBox msgBox(this);
    msgBox.setIcon(QMessageBox::Icon::Critical);
    msgBox.setWindowTitle("Error");
    msgBox.setText(error);
    msgBox.setDetailedText(details);
    msgBox.addButton(QMessageBox::Ok);
    msgBox.exec();
}

void CheatsDialog::on_cheatsTreeWidget_itemChanged(QTreeWidgetItem *item, int column)
{
    if (item == nullptr || item->data(column, Qt::UserRole).isNull())
    {
        return;
    }


    CoreCheat cheat = item->data(column, Qt::UserRole).value<CoreCheat>();
    bool enabled = (item->checkState(column) == Qt::CheckState::Checked);

    if (enabled && cheat.HasOptions && !CoreHasCheatOptionSet(cheat))
    {
        // ask user to select an option
        this->on_cheatsTreeWidget_itemDoubleClicked(item, column);
        
        // make sure the user selected an option,
        // if they did then enable the cheat as requested,
        // if not, reset the checkbox
        if (!CoreHasCheatOptionSet(cheat))
        {
            item->setCheckState(0, Qt::CheckState::Unchecked);
            return;
        }
    }

    CoreEnableCheat(cheat, enabled);
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

    Dialog::ChooseCheatOptionDialog dialog(cheat, this);
    dialog.exec();

    // re-load cheat options
    item->setText(column, this->getTreeWidgetItemTextFromCheat(cheat));
}

void CheatsDialog::on_addCheatButton_clicked(void)
{
    AddCheatDialog dialog(this);
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

    AddCheatDialog dialog(this);
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
    if (!CoreRemoveCheat(cheat))
    {
        this->showErrorMessage("CoreRemoveCheat() Failed!", QString::fromStdString(CoreGetError()));
        return;
    }

    // re-load cheats
    this->loadCheats();
}

void CheatsDialog::accept(void)
{
    CoreSettingsSave();

    if (!CoreApplyCheats())
    {
        this->showErrorMessage("CoreApplyCheats() Failed!", QString::fromStdString(CoreGetError()));
        return;
    }

    QDialog::accept();
}
