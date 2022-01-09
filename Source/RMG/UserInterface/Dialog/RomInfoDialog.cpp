/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "RomInfoDialog.hpp"

#include <QMessageBox>
#include <QFileInfo>

using namespace UserInterface::Dialog;

#include <iostream>

RomInfoDialog::RomInfoDialog(QString file, QWidget *parent) : QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint)
{
    this->setupUi(this);

    bool needCloseRom = false;

    CoreRomHeader romHeader;
    CoreRomSettings romSettings;

    // try to open ROM
    if (!CoreOpenRom(file.toStdString()))
    {
        this->showErrorMessage(parent, "CoreOpenRom() Failed", QString::fromStdString(CoreGetError()));
        return;
    }

    // try to retrieve ROM header
    if (!CoreGetCurrentRomHeader(romHeader))
    {
        this->showErrorMessage(parent, "CoreGetCurrentRomHeader() Failed", QString::fromStdString(CoreGetError()));
        return;
    }

    // try to retrieve ROM settings
    if (!CoreGetCurrentRomSettings(romSettings))
    {
        this->showErrorMessage(parent, "CoreGetCurrentRomSettings() Failed", QString::fromStdString(CoreGetError()));
        return;
    }

    QFileInfo qFileInfo(file);

    this->romNameLineEdit->setText(QString::fromStdString(romHeader.Name));
    this->md5LineEdit->setText(QString::fromStdString(romSettings.MD5));
    this->fileNameLineEdit->setText(qFileInfo.fileName());
    this->locationLineEdit->setText(qFileInfo.absolutePath());
    this->crc1LineEdit->setText(QString::number(romHeader.CRC1, 16).toUpper());
    this->crc2LineEdit->setText(QString::number(romHeader.CRC2, 16).toUpper());

    if (!CoreCloseRom())
    {
        this->showErrorMessage(parent, "CoreCloseRom() Failed", QString::fromStdString(CoreGetError()));
        return;
    }
}

RomInfoDialog::~RomInfoDialog(void)
{
}

void RomInfoDialog::showErrorMessage(QWidget* parent, QString error, QString details)
{
    QMessageBox msgBox(parent);
    msgBox.setIcon(QMessageBox::Icon::Critical);
    msgBox.setWindowTitle("Error");
    msgBox.setText(error);
    msgBox.setDetailedText(details);
    msgBox.addButton(QMessageBox::Ok);
    msgBox.exec();
}
