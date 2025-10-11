/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020-2025 Rosalie Wanders <rosalie@mailbox.org>
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

#include <QStandardItemModel>
#include <QHBoxLayout>
#include <QTreeWidget>
#include <QDialog>
#include <QWidget>
#include <QColor>
#include <QMutex>

#include <RMG-Core/RomSettings.hpp>
#include <RMG-Core/RomHeader.hpp>
#include <RMG-Core/Plugins.hpp>
#include <RMG-Core/Rom.hpp>

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
    enum class SettingsDialogAction
    {
      ConnectSignals = 0,
      LoadSettings,
      LoadDefaultSettings,
      SaveSettings
    };

    bool showGameSettings = false;

    QString currentGameFile;
    CoreRomType     currentGameType;
    CoreRomHeader   currentGameHeader;
    CoreRomSettings currentGameSettings;
    CoreRomSettings defaultGameSettings;
    std::string     gameSection;

    int             keybindButtonTimerId  = -1;
    int             keybindButtonTimeLeft = 5;
    KeybindButton*  currentKeybindButton  = nullptr;

    QColor currentBackgroundColor;
    QColor currentTextColor;

    std::vector<CorePlugin> pluginList;

    int currentIndex(void);

    void restoreDefaults(int);
    void loadSettings(int);
    void saveSettings(int);

    void loadCoreSettings(void);
    void loadGameSettings(void);
    void loadGameCoreSettings(void);
    void loadGamePluginSettings(void);
    void loadPluginSettings(void);
    void loadDirectorySettings(void);
    void load64DDSettings(void);
    void loadHotkeySettings(void);
    void loadInterfaceGeneralSettings(void);
    void loadInterfaceEmulationSettings(void);
    void loadInterfaceRomBrowserSettings(void);
    void loadInterfaceLogSettings(void);
    void loadInterfaceOSDSettings(void);
    void loadInterfaceNetplaySettings(void);

    void loadDefaultCoreSettings(void);
    void loadDefaultGameSettings(void);
    void loadDefaultGameCoreSettings(void);
    void loadDefaultGamePluginSettings(void);
    void loadDefaultPluginSettings(void);
    void loadDefaultDirectorySettings(void);
    void loadDefault64DDSettings(void);
    void loadDefaultHotkeySettings(void);
    void loadDefaultInterfaceGeneralSettings(void);
    void loadDefaultInterfaceEmulationSettings(void);
    void loadDefaultInterfaceRomBrowserSettings(void);
    void loadDefaultInterfaceLogSettings(void);
    void loadDefaultInterfaceOSDSettings(void);
    void loadDefaultInterfaceNetplaySettings(void);

    void saveSettings(void);
    void saveCoreSettings(void);
    void saveGameSettings(void);
    void saveGameCoreSettings(void);
    void saveGamePluginSettings(void);
    void savePluginSettings(void);
    void saveDirectorySettings(void);
    void save64DDSettings(void);
    void saveHotkeySettings(void);
    void saveInterfaceGeneralSettings(void);
    void saveInterfaceEmulationSettings(void);
    void saveInterfaceRomBrowserSettings(void);
    void saveInterfaceLogSettings(void);
    void saveInterfaceOSDSettings(void);
    void saveInterfaceNetplaySettings(void);

    void commonHotkeySettings(SettingsDialogAction);
    void commonPluginSettings(SettingsDialogAction);

    void setIconsForEmulationInfoText(void);
    void hideEmulationInfoText(void);

    void chooseDirectory(QLineEdit *, QString caption);
    void chooseFile(QLineEdit *, QString caption, QString filter = "", QString md5 = "");
    void chooseColor(QPushButton *, QColor *, bool skipChoice = false);

    bool applyPluginSettings(void);

  protected:
    void closeEvent(QCloseEvent* event) Q_DECL_OVERRIDE;
    void timerEvent(QTimerEvent* event) Q_DECL_OVERRIDE;

  private slots:
    void on_buttonBox_clicked(QAbstractButton *);

    void on_changeScreenShotDirButton_clicked(void);
    void on_changeSaveStateDirButton_clicked(void);
    void on_changeSaveSramDirButton_clicked(void);

    void on_changeJapaneseIPLRomPathButton_clicked(void);
    void on_changeAmericanIPLRomPathButton_clicked(void);
    void on_changeDevelopmentIPLRomPathButton_clicked(void);

    void on_clearJapaneseIPLRomPathButton_clicked(void);
    void on_clearAmericanIPLRomPathButton_clicked(void);
    void on_clearDevelopmentIPLRomPathButton_clicked(void);

    void on_changeBackgroundColorButton_clicked(void);
    void on_changeTextColorButton_clicked(void);

    void on_KeybindButton_KeybindingChanged(KeybindButton* button);
    void on_KeybindButton_Clicked(KeybindButton* button);

    void on_coreCpuEmulatorComboBox_currentIndexChanged(int);

    void on_changeNTSCPifRomButton_clicked(void);
    void on_changePALPifRomButton_clicked(void);
  public:
    SettingsDialog(QWidget *parent, QString file = "");
    ~SettingsDialog(void);

    void ShowGameTab(void);
};
} // namespace Dialog
} // namespace UserInterface

#endif // SETTINGSDIALOG_HPP