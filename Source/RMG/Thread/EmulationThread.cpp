/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "EmulationThread.hpp"
#include <RMG-Core/Core.hpp>

#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>

using namespace Thread;

EmulationThread::EmulationThread(QObject *parent) : QThread(parent)
{
#ifndef _WIN32
    // on Linux, musl has a too small stack size,
    // causing a crash in mesa when using paraLLEl,
    // so to fix it, set a stack size of 2MiB for
    // the emulation thread
    // see https://github.com/Rosalie241/RMG/issues/219
    this->setStackSize(0x200000);
#endif
}

EmulationThread::~EmulationThread(void)
{
}

void EmulationThread::SetRomFile(QString file)
{
    this->rom = file;
}

void EmulationThread::SetDiskFile(QString file)
{
    this->disk = file;
}

void EmulationThread::SetNetplay(QString address, int port, int player)
{
    this->address = address;
    this->port    = port;
    this->player  = player;
}

void EmulationThread::run(void)
{
    emit this->on_Emulation_Started();

    bool ret = CoreStartEmulation(this->rom.toStdU32String(), this->disk.toStdU32String(), 
                                  this->address.toStdString(), this->port, this->player);
    if (!ret)
    {
        this->errorMessage = QString::fromStdString(CoreGetError());
    }

    // reset state
    this->rom.clear();
    this->disk.clear();
    this->address.clear();
    this->port = -1;
    this->player = -1;

    emit this->on_Emulation_Finished(ret);
}

QString EmulationThread::GetLastError(void)
{
    return this->errorMessage;
}

bool validateCheat(const CoreCheat& cheat);

void EmulationThread::ApplyCheatsNetplay(QJsonObject cheats)
{
    CoreAddCallbackMessage(CoreDebugMessageType::Info, "Starting ApplyCheats");

    // Log the entire JSON object to verify its structure
    QJsonDocument cheatsDoc(cheats);
    QString cheatsObjectString = cheatsDoc.toJson(QJsonDocument::Compact);
    CoreAddCallbackMessage(CoreDebugMessageType::Info, ("Received cheats: " + cheatsObjectString).toStdString().c_str());

    if (cheats.contains("custom") && cheats.value("custom").isArray()) {
        QJsonArray customCheatsArray = cheats.value("custom").toArray();
        CoreAddCallbackMessage(CoreDebugMessageType::Info, "Parsed custom cheats JSON array");

        CoreCheat cheat;
        cheat.Name = "Netplay";

        for (const QJsonValue &value : customCheatsArray) {
            QString cheatString = value.toString();
            CoreAddCallbackMessage(CoreDebugMessageType::Info, ("Processing cheat: " + cheatString).toStdString().c_str());
            QStringList codeParts = cheatString.split(' '); // Remove '$' and split by space

            // Parse the cheat code and value
            if (codeParts.size() == 2) {
                bool addressOk, valueOk;
                uint32_t address = codeParts[0].toUInt(&addressOk, 16); // Convert address to unsigned int (hex base)
                uint32_t value = codeParts[1].toUInt(&valueOk, 16); // Convert value to unsigned int (hex base)

                if (addressOk && valueOk) {
                    CoreCheatCode code;
                    code.Address = address;
                    code.Value = value;
                    code.UseOptions = false; // No options for now
                    code.OptionIndex = 0;
                    code.OptionSize = 0;
                    cheat.CheatCodes.push_back(code);
                } else {
                    if (!addressOk) {
                        CoreAddCallbackMessage(CoreDebugMessageType::Error, ("Invalid address: " + codeParts[0].toStdString()).c_str());
                    }
                    if (!valueOk) {
                        CoreAddCallbackMessage(CoreDebugMessageType::Error, ("Invalid value: " + codeParts[1].toStdString()).c_str());
                    }
                }
            } else {
                CoreAddCallbackMessage(CoreDebugMessageType::Warning, ("Invalid cheat code format: " + cheatString.toStdString()).c_str());
            }
        }
        // Validate and apply the cheats
        if (!cheat.CheatCodes.empty()) {
            std::vector<CoreCheat> cheatsToApply;
            cheatsToApply.push_back(cheat); // Add the constructed cheat to the vector
            if (CoreApplyCheatsNetplay(cheatsToApply)) {
                CoreAddCallbackMessage(CoreDebugMessageType::Info, ("Netplay cheat added with " + QString::number(cheat.CheatCodes.size()) + " codes").toStdString().c_str());
            } else {
                CoreAddCallbackMessage(CoreDebugMessageType::Error, "Failed to add Netplay cheat");
            }
        } else {
            CoreAddCallbackMessage(CoreDebugMessageType::Warning, "Netplay cheat not found or has no codes");
        }
    } else {
        CoreAddCallbackMessage(CoreDebugMessageType::Warning, "Invalid custom cheats format: custom cheats array not found");
    }
}

// Define the validateCheat function
bool validateCheat(const CoreCheat& cheat)
{
    // Basic validation, add more as needed
    if (cheat.Name.empty()) {
        return false;
    }

    return true;
}