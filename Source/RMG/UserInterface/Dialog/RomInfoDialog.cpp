/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020-2025 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "RomInfoDialog.hpp"

#include <QFileInfo>
#include <QDir>

using namespace UserInterface::Dialog;

RomInfoDialog::RomInfoDialog(QWidget *parent, QString file, CoreRomType romType, CoreRomHeader romHeader, CoreRomSettings romSettings) : QDialog(parent)
{
    this->setupUi(this);

    QFileInfo qFileInfo(file);

    this->romNameLineEdit->setText(QString::fromStdString(romHeader.Name));
    this->romNameLineEdit->setCursorPosition(0);
    this->md5LineEdit->setText(QString::fromStdString(romSettings.MD5));
    this->md5LineEdit->setCursorPosition(0);
    this->fileNameLineEdit->setText(qFileInfo.fileName());
    this->fileNameLineEdit->setCursorPosition(0);
    this->locationLineEdit->setText(QDir::toNativeSeparators(qFileInfo.absolutePath()));
    this->locationLineEdit->setCursorPosition(0);
    this->crc1LineEdit->setText(QString::number(romHeader.CRC1, 16).toUpper());
    this->crc2LineEdit->setText(QString::number(romHeader.CRC2, 16).toUpper());
    this->gameIDLineEdit->setText(QString::fromStdString(romHeader.GameID));
    this->gameRegionLineEdit->setText(QString::fromStdString(romHeader.Region));
    this->systemRegionLineEdit->setText(romType == CoreRomType::Disk
                                            ? "Unknown"
                                            : (romHeader.SystemType == CoreSystemType::NTSC ? "NTSC" : "PAL"));
}

RomInfoDialog::~RomInfoDialog(void)
{
}
