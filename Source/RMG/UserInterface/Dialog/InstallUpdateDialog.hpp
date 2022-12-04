/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef INSTALLUPDATEDIALOG_HPP
#define INSTALLUPDATEDIALOG_HPP

#include <QWidget>
#include <QDialog>

#include <RMG-Core/Core.hpp>

#include "ui_InstallUpdateDialog.h"

namespace UserInterface
{
namespace Dialog
{
class InstallUpdateDialog : public QDialog, private Ui::InstallUpdateDialog
{
    Q_OBJECT

  private:
    QString installationDirectory;
    QString temporaryDirectory;
    QString filename;

    void install(void);

    void writeAndRunScript(QStringList stringList);
    void launchProcess(QString file, QStringList arguments);
    void showErrorMessage(QString error, QString details);

  public:
    InstallUpdateDialog(QWidget *parent, QString installationDirectory, QString temporaryDirectory, QString filename);
    ~InstallUpdateDialog(void);
};
} // namespace Dialog
} // namespace UserInterface

#endif // INSTALLUPDATEDIALOG_HPP