/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "CheatsCommon.hpp"

#include <QJsonObject>

#include <RMG-Core/Core.hpp>

using namespace CheatsCommon;

//
// Local Functions
//

static int find_json_cheat(const QJsonArray& json, const CoreCheat& cheat)
{
    QJsonObject cheatObject;
    QString targetCheatName = QString::fromStdString(cheat.Name);

    for (qsizetype i = 0; i < json.size(); i++)
    {
        cheatObject = json[i].toObject();

        if (cheatObject["name"].toString() == targetCheatName)
        {
            return (int)i;
        }
    }

    return -1;
}

//
// Exported Function
//

bool CheatsCommon::IsCheatEnabled(bool netplay, const QJsonArray& json, const CoreCheat& cheat)
{
    if (netplay)
    {
        int index = find_json_cheat(json, cheat);
        return index != -1 && json[index].toObject().contains("codes");
    }
    else
    {
        return CoreIsCheatEnabled(cheat);
    }
}

bool CheatsCommon::EnableCheat(bool netplay, QJsonArray& json, const CoreCheat& cheat, bool enabled)
{
    if (netplay)
    {
        QJsonObject cheatObject;
        QJsonArray codesArray;
        QString targetCheatName = QString::fromStdString(cheat.Name);

        if (enabled)
        {
            for (const auto& cheatCode : cheat.CheatCodes)
            {
                QJsonObject codeObject;
                codeObject["address"] = (qint64)cheatCode.Address;
                codeObject["value"]   = (qint64)cheatCode.Value;
                codeObject["use_option"]   = cheatCode.UseOptions;
                codeObject["option_index"] = cheatCode.OptionIndex;
                codeObject["option_size"]  = cheatCode.OptionSize;
                codesArray.push_back(codeObject);
            }

            cheatObject["name"]        = targetCheatName;
            cheatObject["codes"]       = codesArray;
            cheatObject["has_options"] = cheat.HasOptions;

            int index = find_json_cheat(json, cheat);
            if (index != -1)
            {
                // copy over the option object
                cheatObject["option"] = json[index].toObject().value("option").toObject();
                json[index] = cheatObject;
            }
            else
            {
                json.push_back(cheatObject);
            }
        }
        else
        {
            int index = find_json_cheat(json, cheat);
            if (index != -1)
            {
                cheatObject = json[index].toObject();

                // if cheat object has an option key
                // we have to keep that key because
                // else the UI will behave inconsistent
                // with the non-netplay cheats dialog
                if (cheatObject.contains("option"))
                {
                    cheatObject.remove("codes");
                    json[index] = cheatObject;
                }
                else
                {
                    json.removeAt(index);
                }
            }
        }

        return true;
    }
    else
    {
        return CoreEnableCheat(cheat, enabled);
    }
}

bool CheatsCommon::HasCheatOptionSet(bool netplay, const QJsonArray& json, const CoreCheat& cheat)
{
    if (netplay)
    {
        int index = find_json_cheat(json, cheat);
        return index != -1 && json[index].toObject().contains("option");
    }
    else
    {
        return CoreHasCheatOptionSet(cheat);
    }
}

bool CheatsCommon::GetCheatOption(bool netplay, const QJsonArray& json, const CoreCheat& cheat, CoreCheatOption& option)
{
    if (netplay)
    {
        QJsonObject cheatObject;
        QJsonObject optionObject;

        int index = find_json_cheat(json, cheat);
        if (index == -1)
        {
            return false;
        }

        cheatObject  = json[index].toObject();
        optionObject = cheatObject["option"].toObject();
        option.Name  = optionObject["name"].toString().toStdString();
        option.Size  = optionObject["size"].toInt();
        option.Value = optionObject["value"].toInteger();
        return true;
    }
    else
    {
        return CoreGetCheatOption(cheat, option);
    }
}
bool CheatsCommon::SetCheatOption(bool netplay, QJsonArray& json, const CoreCheat& cheat, CoreCheatOption& option)
{
    if (netplay)
    {
        QJsonObject cheatObject;
        QJsonObject optionObject;
        QString targetCheatName = QString::fromStdString(cheat.Name);

        int index = find_json_cheat(json, cheat);
        if (index != -1)
        {
            cheatObject = json[index].toObject();
            optionObject["name"]  = QString::fromStdString(option.Name);
            optionObject["size"]  = option.Size;
            optionObject["value"] = (qint64)option.Value;

            cheatObject["option"] = optionObject;

            json[index] = cheatObject;
        }
        else
        { // create dummy object when option has been set
          // without the user having the cheat enabled
            optionObject["name"]  = QString::fromStdString(option.Name);
            optionObject["size"]  = option.Size;
            optionObject["value"] = (qint64)option.Value;

            cheatObject["name"]   = targetCheatName;
            cheatObject["option"] = optionObject;

            json.append(cheatObject);
        }

        return true;
    }
    else
    {
        return CoreGetCheatOption(cheat, option);
    }
}

bool CheatsCommon::ParseCheatJson(const QJsonArray& json, std::vector<CoreCheat>& cheats)
{
    QJsonObject cheatObject;
    QJsonArray cheatCodesArray;
    QJsonObject cheatCodeObject;
    QJsonObject cheatOptionObject;
    CoreCheat cheat;
    CoreCheatCode cheatCode;
    CoreCheatOption cheatOption;
    bool skipCheat = false;

    for (qsizetype i = 0; i < json.size(); i++)
    {
        cheatObject = json[i].toObject();

        if (!cheatObject.contains("name") ||
            !cheatObject.contains("codes") ||
            !cheatObject.contains("has_options"))
        { // skip disabled cheats
            continue;
        }

        // reset cheat
        cheat = {};
        skipCheat = false;

        cheat.Name       = cheatObject["name"].toString().toStdString();
        cheat.HasOptions = cheatObject["has_options"].toBool();

        if (cheat.Name.empty())
        { // name validation
            continue;
        }

        // parse cheat codes
        cheatCodesArray = cheatObject["codes"].toArray();
        for (qsizetype x = 0; x < cheatCodesArray.size(); x++)
        {
            cheatCodeObject = cheatCodesArray[x].toObject();

            // ensure json object contains required keys
            if (!cheatCodeObject.contains("address") ||
                !cheatCodeObject.contains("value") ||
                !cheatCodeObject.contains("use_option") ||
                !cheatCodeObject.contains("option_index") ||
                !cheatCodeObject.contains("option_size"))
            {
                skipCheat = true;
                break;
            }

            cheatCode.Address = (uint32_t)cheatCodeObject["address"].toInteger();
            cheatCode.Value   = cheatCodeObject["value"].toInt();
            cheatCode.UseOptions  = cheatCodeObject["use_option"].toBool();
            cheatCode.OptionIndex = cheatCodeObject["option_index"].toInt();
            cheatCode.OptionSize  = cheatCodeObject["option_size"].toInt();

            cheat.CheatCodes.push_back(cheatCode);
        }

        // parse (optional) cheat option
        if (cheatObject.contains("option"))
        {
            cheatOptionObject = cheatObject["option"].toObject();

            // ensure json object has required keys
            if (!cheatOptionObject.contains("name") ||
                !cheatOptionObject.contains("size") ||
                !cheatOptionObject.contains("value"))
            {
                skipCheat = true;
            }
            else
            {
                cheatOption.Name  = cheatOptionObject["name"].toString().toStdString();
                cheatOption.Size  = cheatOptionObject["size"].toInt();
                cheatOption.Value = (uint32_t)cheatOptionObject["value"].toInteger();
                cheat.CheatOptions.push_back(cheatOption);
            }
        }

        if (skipCheat)
        {
            continue;
        }

        cheats.push_back(cheat);
    }

    return true;
}
