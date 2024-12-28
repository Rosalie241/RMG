/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "AboutDialog.hpp"
#include <RMG-Core/VersionHash.hpp>
#include <RMG-Core/Core.hpp>

using namespace UserInterface::Dialog;

AboutDialog::AboutDialog(QWidget* parent) : QDialog(parent)
{
    this->setupUi(this);

    this->versionLabel->setText(QString("%1").arg(QString::fromStdString(VERSION_HASH)));
}
