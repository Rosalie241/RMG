/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef SETTINGSDIALOG_HPP
#define SETTINGSDIALOG_HPP

#include <QDialog>
#include <QHBoxLayout>
#include <QStandardItemModel>
#include <QTreeWidget>
#include <QWidget>

#include "ui_SettingsDialog.h"

namespace UserInterface
{
namespace Dialog
{
class SettingsDialog : public QDialog, private Ui::SettingsDialog
{
    Q_OBJECT

  private:
    void restoreDefaults(int);
    void reloadSettings(int);

    void loadCoreSettings(void);
    void loadGameSettings(void);
    void loadGameCoreSettings(void);
    void loadGamePluginSettings(void);
    void loadPluginSettings(void);
    void loadDirectorySettings(void);

    void loadDefaultGameSettings(void);
    void loadDefaultGamePluginSettings(void);
    void loadDefaultDirectorySettings(void);

    void saveSettings(void);
    void saveCoreSettings(void);
    void saveGameSettings(void);
    void savePluginSettings(void);
    void saveDirectorySettings(void);

    void hideEmulationInfoText(void);

  private slots:
    void on_treeWidget_currentItemChanged(QTreeWidgetItem *, QTreeWidgetItem *);
    void on_buttonBox_clicked(QAbstractButton *);

  public:
    SettingsDialog(QWidget *parent);
    ~SettingsDialog(void);
};
} // namespace Dialog
} // namespace UserInterface

#endif // SETTINGSDIALOG_HPP