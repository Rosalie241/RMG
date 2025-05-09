/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020-2025 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef CHEATSCOMMONH_HPP
#define CHEATSCOMMONH_HPP

#include <QTreeWidget>
#include <QJsonArray>
#include <QString>

#include <RMG-Core/Cheats.hpp>

namespace CheatsCommon
{
    // Checks whether cheat is enabled
    bool IsCheatEnabled(bool netplay, const QJsonArray& json, QString file, const CoreCheat& cheat);

    // Enables Cheat
    bool EnableCheat(bool netplay, QJsonArray& json, QString file, const CoreCheat& cheat, bool enabled);

    // Returns whether cheat has option set
    bool HasCheatOptionSet(bool netplay, const QJsonArray& json, QString file, const CoreCheat& cheat);

    // Retrieves cheat option
    bool GetCheatOption(bool netplay, const QJsonArray& json, QString file, const CoreCheat& cheat, CoreCheatOption& option);

    // Sets cheat option
    bool SetCheatOption(bool netplay, QJsonArray& json, QString file, const CoreCheat& cheat, CoreCheatOption& option);

    // Parses json into cheat list
    bool ParseCheatJson(const QJsonArray& json, std::vector<CoreCheat>& cheats);

    // Retrieves tree widget item name from cheat
    QString GetCheatTreeWidgetItemName(bool netplay, QJsonArray& json, QString file, const CoreCheat& cheat);

    // Adds cheats to QTreeWidget
    bool AddCheatsToTreeWidget(bool netplay, QJsonArray& json, QString file, const std::vector<CoreCheat>& cheats, QTreeWidget* cheatsTreeWidget, bool readonly);

    // Attempts to find QTreeWidget item in section
    QTreeWidgetItem* FindTreeWidgetItem(QTreeWidget* cheatsTreeWidget, QStringList sections, int size, QString itemText);
}

#endif // CHEATSCOMMONH_HPP
