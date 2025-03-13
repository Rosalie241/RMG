/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020-2025 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef ROMINFODIALOG_HPP
#define ROMINFODIALOG_HPP

#include <RMG-Core/RomSettings.hpp>
#include <RMG-Core/RomHeader.hpp>

#include <QWidget>
#include <QDialog>

#include "ui_RomInfoDialog.h"

namespace UserInterface
{
namespace Dialog
{
class RomInfoDialog : public QDialog, private Ui::RomInfoDialog
{
    Q_OBJECT

  public:
    RomInfoDialog(QString file, CoreRomHeader romHeader, CoreRomSettings romSettings, QWidget *parent);
    ~RomInfoDialog(void);
};
} // namespace Dialog
} // namespace UserInterface

#endif // ROMINFODIALOG_HPP