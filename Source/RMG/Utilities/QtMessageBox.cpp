/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020-2025 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "QtMessageBox.hpp"

#include <QAbstractButton>
#include <QMessageBox>
#include <QCheckBox>

using namespace Utilities;

//
// Internal Functions
//

static void show_messagebox(QMessageBox::Icon icon, QString title, QWidget* parent, QString text, QString details)
{
    QMessageBox msgBox(parent->isVisible() ? parent : parent->parentWidget());
    msgBox.setIcon(icon);
    msgBox.setWindowTitle(title);
    msgBox.setText(text);
    msgBox.setDetailedText(details);
    msgBox.addButton(QMessageBox::Ok);

    // expand details by default
    if (!details.isEmpty())
    {
        for (const auto& button : msgBox.buttons())
        {
            if (msgBox.buttonRole(button) == QMessageBox::ActionRole)
            {
                button->click();
                break;
            }
        }
    }

    msgBox.exec();
}

//
// Exported Functions
//

void QtMessageBox::Info(QWidget* parent, QString text, QString details)
{
    show_messagebox(QMessageBox::Icon::Information, "Information", parent, text, details);
}

void QtMessageBox::Error(QWidget* parent, QString text, QString details)
{
    show_messagebox(QMessageBox::Icon::Critical, "Error", parent, text, details);
}

bool QtMessageBox::Question(QWidget* parent, QString text, QString checkBoxText, bool& checkBoxValue)
{
    QMessageBox msgBox(parent);
    msgBox.setIcon(QMessageBox::Icon::Question);
    msgBox.setText(text);
    msgBox.addButton(QMessageBox::Button::Yes);
    msgBox.addButton(QMessageBox::Button::No);

    QCheckBox* checkBox = new QCheckBox(&msgBox);
    checkBox->setText(checkBoxText);
    checkBox->setChecked(checkBoxValue);

    msgBox.setCheckBox(checkBox);

    int ret = msgBox.exec();
    checkBoxValue = checkBox->isChecked();
    return ret == QMessageBox::Button::Yes;
}

