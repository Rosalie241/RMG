/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020-2025 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "InputProfileDB.hpp"
#include "main.hpp"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>

#include <RMG-Core/m64p/api/m64p_types.h>
#include <RMG-Core/Directories.hpp>


//
// Public Functions
//

bool Utilities::InputProfileDB::Load(void)
{
	std::filesystem::path profileDatabaseFilePath;
    profileDatabaseFilePath = CoreGetSharedDataDirectory();
    profileDatabaseFilePath += "/InputProfileDB.json";

    QFile jsonFile(profileDatabaseFilePath);
    
    if (!jsonFile.open(QFile::ReadOnly))
    {
    	std::string message = "InputProfileDB::Load(): Failed to open DB";
    	PluginDebugMessage(M64MSG_WARNING, message);
    	return false;
    }

    QJsonParseError error;
    QJsonDocument jsonDocument = QJsonDocument::fromJson(jsonFile.readAll(), &error);
    if (error.error != QJsonParseError::NoError)
    {
    	std::string message = "InputProfileDB::Load(): Failed to load json: ";
    	message += error.errorString().toStdString();
    	PluginDebugMessage(M64MSG_WARNING, message);
    	return false;
    }

    const struct
    {
        N64ControllerButton button;
        InputAxisDirection axis;
        QString baseKey;
    } jsonButtonMappings[] =
    {
        { N64ControllerButton::A, InputAxisDirection::Invalid, "A_" },
        { N64ControllerButton::B, InputAxisDirection::Invalid, "B_" },
        { N64ControllerButton::Start, InputAxisDirection::Invalid, "Start_" },
        { N64ControllerButton::DpadUp, InputAxisDirection::Invalid, "DpadUp_" },
        { N64ControllerButton::DpadDown, InputAxisDirection::Invalid, "DpadDown_" },
        { N64ControllerButton::DpadLeft, InputAxisDirection::Invalid, "DpadLeft_" },
        { N64ControllerButton::DpadRight, InputAxisDirection::Invalid, "DpadRight_" },
        { N64ControllerButton::CButtonUp, InputAxisDirection::Invalid, "CButtonUp_" },
        { N64ControllerButton::CButtonDown, InputAxisDirection::Invalid, "CButtonDown_" },
        { N64ControllerButton::CButtonLeft, InputAxisDirection::Invalid, "CButtonLeft_" },
        { N64ControllerButton::CButtonRight, InputAxisDirection::Invalid, "CButtonRight_" },
        { N64ControllerButton::LeftShoulder, InputAxisDirection::Invalid, "LeftTrigger_", },
        { N64ControllerButton::RightShoulder, InputAxisDirection::Invalid, "RightTrigger_" },
        { N64ControllerButton::ZTrigger, InputAxisDirection::Invalid, "ZTrigger_" },
        { N64ControllerButton::Invalid, InputAxisDirection::Up, "AnalogStickUp_" },
    	{ N64ControllerButton::Invalid, InputAxisDirection::Down, "AnalogStickDown_" },
    	{ N64ControllerButton::Invalid, InputAxisDirection::Left, "AnalogStickLeft_" },
    	{ N64ControllerButton::Invalid, InputAxisDirection::Right, "AnalogStickRight_" },
    };


    InputProfileDBEntry entry;
    InputProfileDBInputEntry inputEntry;

    for (const auto& arrayValue : jsonDocument.array())
    {
    	// reset entry state
    	entry = {};
    	entry.valid = true;

        QJsonObject jsonObject = arrayValue.toObject();

        QJsonArray deviceNameArray = jsonObject["DeviceName"].toArray();
        for (const auto& deviceName : deviceNameArray)
        {
        	entry.deviceNames.push_back(deviceName.toString());
        }

        entry.deviceType = static_cast<InputDeviceType>(jsonObject["DeviceType"].toInt());

        // add all button mappings
        for (const auto& buttonMapping : jsonButtonMappings)
        {
        	// reset input entry state
        	inputEntry = {};

        	QStringList inputTypes = jsonObject[buttonMapping.baseKey + "InputType"].toString().split(';');
            QStringList names = jsonObject[buttonMapping.baseKey + "Name"].toString().split(';');
            QStringList data = jsonObject[buttonMapping.baseKey + "Data"].toString().split(';');
            QStringList extraData = jsonObject[buttonMapping.baseKey + "ExtraData"].toString().split(';');

            inputEntry.button = buttonMapping.button;
            inputEntry.axis = buttonMapping.axis;
        	inputEntry.names = names;

            size_t size = std::min(inputTypes.size(), std::min(names.size(), std::min(data.size(), extraData.size())));
            for (int i = 0; i < size; i++)
            {
            	inputEntry.inputTypes.push_back(static_cast<InputType>(inputTypes[i].toInt()));
            	inputEntry.data.push_back(data[i].toInt());
            	inputEntry.extraData.push_back(extraData[i].toInt());
        	}

        	entry.inputEntries.push_back(inputEntry);
        }

        // add to list
        this->entries.push_back(entry);
    }

	return true;
}


InputProfileDBEntry Utilities::InputProfileDB::FindEntry(const InputDevice& device, bool fallback)
{
	QString deviceName = QString::fromStdString(device.name);

	for (const auto& dbEntry : this->entries)
	{
		if (dbEntry.deviceType == device.type)
		{
			for (const auto& name : dbEntry.deviceNames)
			{
				if (name == deviceName)
				{
					return dbEntry;
				}
			}
		}
	}

    if (fallback)
    {
        return this->FindEntry({ InputDeviceType::Joystick, "fallback_profile" }, false);
    }

    // nothing found, return invalid entry
	return { false };
}

