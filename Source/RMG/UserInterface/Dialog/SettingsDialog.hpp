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

// needed for KeyBindButton in ui_SettingsDialog
#include "UserInterface/Widget/KeyBindButton.hpp"
#include "M64P/Wrapper/Types.hpp"

using namespace UserInterface::Widget;

#include "ui_SettingsDialog.h"

namespace UserInterface
{
namespace Dialog
{
class SettingsDialog : public QDialog, private Ui::SettingsDialog
{
    Q_OBJECT

  private:
    bool inGame = false;
    M64P::Wrapper::RomInfo_t gameInfo = {0};
    M64P::Wrapper::RomInfo_t defaultGameInfo = {0};
    QString gameSection;

    QList<M64P::Wrapper::Plugin_t> pluginList;

    int currentIndex(void);

    void restoreDefaults(int);
    void reloadSettings(int);
    void saveSettings(int);

    void loadCoreSettings(void);
    void loadGameSettings(void);
    void loadGameCoreSettings(void);
    void loadGamePluginSettings(void);
    void loadPluginSettings(void);
    void loadDirectorySettings(void);
    void loadHotkeySettings(void);
    void loadInterfaceSettings(void);

    void loadDefaultCoreSettings(void);
    void loadDefaultGameSettings(void);
    void loadDefaultGameCoreSettings(void);
    void loadDefaultGamePluginSettings(void);
    void loadDefaultDirectorySettings(void);
    void loadDefaultHotkeySettings(void);
    void loadDefaultInterfaceSettings(void);

    void saveSettings(void);
    void saveCoreSettings(void);
    void saveGameSettings(void);
    void saveGameCoreSettings(void);
    void saveGamePluginSettings(void);
    void savePluginSettings(void);
    void saveDirectorySettings(void);
    void saveHotkeySettings(void);
    void saveInterfaceSettings(void);

    void commonHotkeySettings(int);

    void hideEmulationInfoText(void);

    void chooseDirectory(QLineEdit *);

  private slots:
    void on_buttonBox_clicked(QAbstractButton *);

    void on_changeScreenShotDirButton_clicked(void);
    void on_changeSaveStateDirButton_clicked(void);
    void on_changeSaveSramDirButton_clicked(void);
    void on_changeSharedDataDirButton_clicked(void);
    void on_changeUserDataDirButton_clicked(void);
    void on_changeUserCacheDirButton_clicked(void);

  public:
    SettingsDialog(QWidget *parent);
    ~SettingsDialog(void);
};
} // namespace Dialog
} // namespace UserInterface

#endif // SETTINGSDIALOG_HPP