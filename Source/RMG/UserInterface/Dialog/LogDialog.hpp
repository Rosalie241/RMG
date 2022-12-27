/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef LOGDIALOG_HPP
#define LOGDIALOG_HPP

#include <QWidget>
#include <QDialog>

#include <RMG-Core/Core.hpp>

#include "ui_LogDialog.h"

namespace UserInterface
{
namespace Dialog
{
class LogDialog : public QDialog, private Ui::LogDialog
{
    Q_OBJECT

  private:

  public:
    LogDialog(QWidget* parent = nullptr);
    ~LogDialog(void);

    void Clear(void);
  public slots:
    void AddLogLine(CoreDebugMessageType type, QString context, QString line);
};
} // namespace Dialog
} // namespace UserInterface

#endif // LOGDIALOG_HPP