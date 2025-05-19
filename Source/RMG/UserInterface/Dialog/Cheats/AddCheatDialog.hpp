/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020-2025 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef ADDCHEATDIALOG_HPP
#define ADDCHEATDIALOG_HPP

#include <QWidget>
#include <QDialog>

#include <RMG-Core/Cheats.hpp>

#include "ui_AddCheatDialog.h"

namespace UserInterface
{
namespace Dialog
{
class AddCheatDialog : public QDialog, private Ui::AddCheatDialog
{
    Q_OBJECT

  public:
    AddCheatDialog(QWidget *parent, QString file);
    ~AddCheatDialog(void);

    // enables edit mode & sets cheat
    void SetCheat(CoreCheat cheat);

  private:
    bool updateMode = false;
    CoreCheat oldCheat;
    QString file;

    void setPlainTextEditLines(QPlainTextEdit* plainTextEdit, std::vector<std::string> lines);
    bool validate(void);
    void validateOkButton(void);

    QStringList getLines(QTextDocument* textDocument);
    bool getCheat(CoreCheat& cheat);
    bool addCheat(void);
    bool updateCheat(void);

  private slots:
    void accept(void) Q_DECL_OVERRIDE;

    void on_nameLineEdit_textChanged(void);
    void on_codeTextEdit_textChanged(void);
    void on_optionsTextEdit_textChanged(void);
};
} // namespace Dialog
} // namespace UserInterface

#endif // ADDCHEATDIALOG_HPP