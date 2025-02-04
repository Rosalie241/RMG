/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef CHEATSDIALOG_HPP
#define CHEATSDIALOG_HPP

#include <QJsonArray>
#include <QDialog>
#include <QWidget>

#include "ui_CheatsDialog.h"

namespace UserInterface
{
namespace Dialog
{
class CheatsDialog : public QDialog, private Ui::CheatsDialog
{
    Q_OBJECT

  public:
    CheatsDialog(QWidget *parent, QString file = "", bool netplay = false, QJsonArray cheatsJson = {});
    ~CheatsDialog(void);

    bool HasFailed(void);
    QJsonArray GetJson(void);


  private:
    bool needCloseRom = false;
    bool failedToParseCheats = false;
    bool netplay = false;
    QJsonArray cheatsJson;
    QString file;

    void loadCheats(void);

  private slots:
    void on_cheatsTreeWidget_itemChanged(QTreeWidgetItem *item, int column);
    void on_cheatsTreeWidget_currentItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous);
    void on_cheatsTreeWidget_itemDoubleClicked(QTreeWidgetItem *item, int column);

    void on_addCheatButton_clicked(void);
    void on_editCheatButton_clicked(void);
    void on_removeCheatButton_clicked(void);
    
    void accept(void) Q_DECL_OVERRIDE;
};
} // namespace Dialog
} // namespace UserInterface

#endif // CHEATSDIALOG_HPP