/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef CHOOSECHEATOPTIONDIALOG_HPP
#define CHOOSECHEATOPTIONDIALOG_HPP

#include <QJsonArray>
#include <QDialog>
#include <QWidget>

#include <RMG-Core/Core.hpp>

#include "ui_ChooseCheatOptionDialog.h"

namespace UserInterface
{
namespace Dialog
{
class ChooseCheatOptionDialog : public QDialog, private Ui::ChooseCheatOptionDialog
{
    Q_OBJECT

  private:
    CoreCheat cheat;
    bool netplay;
    QJsonArray cheatsJson;

  public:
    ChooseCheatOptionDialog(CoreCheat cheat, bool netplay, QJsonArray cheatsJson,  QWidget *parent);
    ~ChooseCheatOptionDialog(void);

    QJsonArray GetJson(void);

  private slots:
    void on_cheatOptionsTreeWidget_itemChanged(QTreeWidgetItem *item, int column);
    void accept(void) Q_DECL_OVERRIDE;
};
} // namespace Dialog
} // namespace UserInterface

#endif // CHOOSECHEATOPTIONDIALOG_HPP