/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "QtMessageBox.hpp"

#include <QMessageBox>

using namespace Utilities;

void QtMessageBox::Error(QWidget* parent, QString text, QString details)
{
    QMessageBox msgBox(parent->isVisible() ? parent : parent->parentWidget());
    msgBox.setIcon(QMessageBox::Icon::Critical);
    msgBox.setWindowTitle("Error");
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

