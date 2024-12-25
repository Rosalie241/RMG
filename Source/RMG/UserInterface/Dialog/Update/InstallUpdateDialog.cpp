/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "InstallUpdateDialog.hpp"
#include "Utilities/QtMessageBox.hpp"

#include <RMG-Core/Core.hpp>

#include <QMessageBox>
#include <QProcess>
#include <QDir>
#include <QTextStream>

using namespace UserInterface::Dialog;
using namespace Utilities;

InstallUpdateDialog::InstallUpdateDialog(QWidget *parent, QString installationDirectory, QString temporaryDirectory, QString filename) : QDialog(parent)
{
    this->setupUi(this);

    this->installationDirectory = installationDirectory;
    this->temporaryDirectory = temporaryDirectory;
    this->filename = filename;
    this->startTimer(100);
}

InstallUpdateDialog::~InstallUpdateDialog(void)
{
}

void InstallUpdateDialog::install(void)
{
    QString fullFilePath;
    fullFilePath = this->temporaryDirectory;
    fullFilePath += "/" + this->filename;   

    QString appPath = QCoreApplication::applicationDirPath();
    QString appPid  = QString::number(QCoreApplication::applicationPid());
    QString logPath = QString::fromStdU32String(CoreGetUserCacheDirectory().u32string()) + "/updater.log";


    // convert paths to use the right path seperator
    this->temporaryDirectory = QDir::toNativeSeparators(this->temporaryDirectory);
    fullFilePath             = QDir::toNativeSeparators(fullFilePath);
    appPath                  = QDir::toNativeSeparators(appPath);
    logPath                  = QDir::toNativeSeparators(logPath);

    // remove log file when it exists
    QFile qLogFile(logPath);
    if (qLogFile.exists())
    {
        qLogFile.remove();
    }

    QString outputToLogLine = " >> \"" + logPath + "\" 2>&1";

    if (this->filename.endsWith(".exe"))
    {
        this->label->setText("Executing " + this->filename + "...");
        QStringList scriptLines =
        {
            "@echo off",
            "(",
            "   echo == Attemping to kill PID " + appPid + outputToLogLine,
            "   taskkill /F /PID:"              + appPid + outputToLogLine,
            "   echo == Attemping to start \'" + fullFilePath + "\'"                                    + outputToLogLine,
            "   \"" + fullFilePath + "\" /CLOSEAPPLICATIONS /NOCANCEL /MERGETASKS=\"!desktopicon\"  /SILENT /DIR=\"" + appPath + "\"" + outputToLogLine,
            ")",
            "IF NOT ERRORLEVEL 0 (",
            "   start \"\" cmd /c \"echo Rosalie's Mupen GUI failed to update, check the updater.log file in the user cache directory for more information && pause\"",
            ")",
            // remove temporary directory at last
            "rmdir /S /Q \"" + this->temporaryDirectory + "\"",
        };
        this->writeAndRunScript(scriptLines);
        this->accept();
        return;
    }

    this->label->setText("Extracting " + this->filename + "...");
    this->progressBar->setValue(50);

    QDir dir(this->temporaryDirectory);
    if (!dir.mkdir("extract"))
    {
        QtMessageBox::Error(this, "QDir::mkdir() Failed", "");
        this->reject();
        return;
    }

    QString extractDirectory;
    extractDirectory = this->temporaryDirectory;
    extractDirectory += "/extract";

    if (!CoreUnzip(fullFilePath.toStdU32String(), extractDirectory.toStdU32String()))
    {
        QtMessageBox::Error(this, "CoreUnzip() Failed", QString::fromStdString(CoreGetError()));
        this->reject();
        return;
    }

    this->label->setText("Executing update script...");
    this->progressBar->setValue(100);

    extractDirectory = QDir::toNativeSeparators(extractDirectory);

    QStringList scriptLines = 
    {
        "@echo off",
        "(",
        "   echo == Attempting to remove \'" + fullFilePath + "\'" + outputToLogLine,
        "   del /F /Q \""                    + fullFilePath + "\"" + outputToLogLine,
        "   echo == Attemping to kill PID " + appPid               + outputToLogLine,
        "   taskkill /F /PID:"              + appPid               + outputToLogLine,
        "   echo == Attemping to copy \'" + extractDirectory + "\' to \'" + appPath + "\'"  + outputToLogLine,
        "   xcopy /S /Y /I \""            + extractDirectory + "\\*\" \"" + appPath + "\""  + outputToLogLine,
        "   echo == Attemping to start \'" + appPath + "\\RMG.exe\'"           + outputToLogLine,
        "   start \"\" \""                 + appPath + "\\RMG.exe\""           + outputToLogLine,
        ")",
        "IF NOT ERRORLEVEL 0 (",
        "   start \"\" cmd /c \"echo Rosalie's Mupen GUI failed to update, check the updater.log file in the user cache directory for more information && pause\"",
        ")",
        // remove temporary directory at last
        "rmdir /S /Q \"" + this->temporaryDirectory + "\"",
    };
    this->writeAndRunScript(scriptLines);
}

void InstallUpdateDialog::writeAndRunScript(QStringList stringList)
{
    QString scriptPath;
    scriptPath = this->temporaryDirectory;
    scriptPath += "/update.bat";

    QFile scriptFile(scriptPath);
    if (!scriptFile.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QtMessageBox::Error(this, "QFile::open() Failed", "");
        return;
    }

    QTextStream textStream(&scriptFile);

    // write script to file
    for (const QString& str : stringList)
    {
        textStream << str << "\n";
    }

    scriptFile.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner);
    scriptFile.close();

    this->launchProcess(scriptPath, {});
}

void InstallUpdateDialog::launchProcess(QString file, QStringList arguments)
{
    QProcess process;
    process.setProgram(file);
    process.setArguments(arguments);
    process.startDetached();
}

void InstallUpdateDialog::timerEvent(QTimerEvent *event)
{
    this->killTimer(event->timerId());
    this->install();
}
