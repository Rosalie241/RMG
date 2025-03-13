/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020-2025 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "LogDialog.hpp"

#include <QFileInfo>
#include <QFont>
#include <QIcon>

using namespace UserInterface::Dialog;

LogDialog::LogDialog(QWidget *parent) : QDialog(parent)
{
    this->setupUi(this);
    this->setWindowIcon(QIcon(":Resource/RMG.png"));
    this->setWindowFlags(this->windowFlags() | Qt::WindowMinimizeButtonHint);

    try
    { // for some reason on Windows 10 for some users, 
      // ->setFont() will crash in DwriteCreateFactory,
      // so catch the exception and do nothing
        QFont font("monospace");
#ifdef _WIN32
        font.setStyleHint(QFont::TypeWriter);
#endif
        this->plainTextEdit->setFont(font);
    }
    catch (...)
    {
    }
}

LogDialog::~LogDialog(void)
{
}

int LogDialog::GetLineCount(void)
{
    return this->plainTextEdit->document()->lineCount();
}

void LogDialog::AddMessages(const QList<CoreCallbackMessage>& messages)
{
    QString text;

    for (const auto& message : messages)
    {
        text = message.Context;

        switch (message.Type)
        {
        case CoreDebugMessageType::Error:
            text += "[ERROR]   ";
            break;
        case CoreDebugMessageType::Info:
            text += "[INFO]    ";
            break;
        case CoreDebugMessageType::Verbose:
            text += "[VERBOSE] ";
            break;
        case CoreDebugMessageType::Warning:
            text += "[WARNING] ";
            break;
        case CoreDebugMessageType::Status:
            text += "[STATUS]  ";
            break;
        }

        text += message.Message;

        this->plainTextEdit->appendPlainText(text);
    }
}

void LogDialog::Clear(void)
{
    this->plainTextEdit->clear();
}

