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

#include <string>
#include <vector>

#include <QDialog>
#include <QHBoxLayout>
#include <QStandardItemModel>
#include <QTreeWidget>
#include <QWidget>

#include <RMG-Core/Core.hpp>

// needed for KeyBindButton in ui_SettingsDialog
#include "UserInterface/Widget/KeybindButton.hpp"

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
    bool romOpened = false;

    CoreRomSettings currentGameSettings;
    CoreRomSettings defaultGameSettings;
    std::string     gameSection;

    std::vector<CorePlugin> pluginList;

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
    void load64DDSettings(void);
    void loadHotkeySettings(void);
    void loadInterfaceEmulationSettings(void);
    void loadInterfaceRomBrowserSettings(void);
    void loadInterfaceLogWindowSettings(void);
    void loadInterfaceStyleSettings(void);
    void loadInterfaceMiscSettings(void);

    void loadDefaultCoreSettings(void);
    void loadDefaultGameSettings(void);
    void loadDefaultGameCoreSettings(void);
    void loadDefaultGamePluginSettings(void);
    void loadDefaultPluginSettings(void);
    void loadDefaultDirectorySettings(void);
    void loadDefault64DDSettings(void);
    void loadDefaultHotkeySettings(void);
    void loadDefaultInterfaceEmulationSettings(void);
    void loadDefaultInterfaceRomBrowserSettings(void);
    void loadDefaultInterfaceLogWindowSettings(void);
    void loadDefaultInterfaceStyleSettings(void);
    void loadDefaultInterfaceMiscSettings(void);

    void saveSettings(void);
    void saveCoreSettings(void);
    void saveGameSettings(void);
    void saveGameCoreSettings(void);
    void saveGamePluginSettings(void);
    void savePluginSettings(void);
    void saveDirectorySettings(void);
    void save64DDSettings(void);
    void saveHotkeySettings(void);
    void saveInterfaceEmulationSettings(void);
    void saveInterfaceRomBrowserSettings(void);
    void saveInterfaceLogWindowSettings(void);
    void saveInterfaceStyleSettings(void);
    void saveInterfaceMiscSettings(void);

    void commonHotkeySettings(int);
    void commonPluginSettings(int);
    void commonInterfaceStyleSettings(int);

    void setIconsForEmulationInfoText(void);
    void hideEmulationInfoText(void);

    void chooseDirectory(QLineEdit *);
    void chooseIPLRom(QLineEdit *);

    bool applyPluginSettings(void);

  protected:
    void closeEvent(QCloseEvent* event) Q_DECL_OVERRIDE;

  private slots:
    void on_buttonBox_clicked(QAbstractButton *);

    void on_changeScreenShotDirButton_clicked(void);
    void on_changeSaveStateDirButton_clicked(void);
    void on_changeSaveSramDirButton_clicked(void);
    void on_changeUserDataDirButton_clicked(void);
    void on_changeUserCacheDirButton_clicked(void);

    void on_changeJapaneseIPLRomPathButton_clicked(void);
    void on_changeAmericanIPLRomPathButton_clicked(void);
    void on_changeDevelopmentIPLRomPathButton_clicked(void);

    void on_KeybindButton_KeybindingChanged(KeybindButton* button);
  public:
    SettingsDialog(QWidget *parent);
    ~SettingsDialog(void);

    void ShowGameTab(void);
};
} // namespace Dialog
} // namespace UserInterface

#endif // SETTINGSDIALOG_HPP