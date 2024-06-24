/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "DownloadUpdateDialog.hpp"

#include <QMessageBox>
#include <QFileInfo>
#include <QPushButton>
#include <QDesktopServices>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QTemporaryDir>
#include <QFile>
#include <QProcess>

using namespace UserInterface::Dialog;

DownloadUpdateDialog::DownloadUpdateDialog(QWidget *parent, QUrl url, QString filename) : QDialog(parent)
{
    this->setupUi(this);

    this->filename = filename;
    this->label->setText("Downloading " + filename + "...");

    QNetworkAccessManager* networkAccessManager = new QNetworkAccessManager(this);
    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);

    this->reply = networkAccessManager->get(request);
    connect(reply, &QNetworkReply::downloadProgress, this, &DownloadUpdateDialog::on_reply_downloadProgress);
    connect(reply, &QNetworkReply::finished, this, &DownloadUpdateDialog::on_reply_finished);
}

DownloadUpdateDialog::~DownloadUpdateDialog(void)
{
}

QString DownloadUpdateDialog::GetTempDirectory(void)
{
    return this->temporaryDirectory;
}

QString DownloadUpdateDialog::GetFileName(void)
{
    return this->filename;
}

void DownloadUpdateDialog::showErrorMessage(QString error, QString details)
{
    QMessageBox msgBox((this->isVisible() ? this : this->parentWidget()));
    msgBox.setIcon(QMessageBox::Icon::Critical);
    msgBox.setWindowTitle("Error");
    msgBox.setText(error);
    msgBox.setDetailedText(details);
    msgBox.addButton(QMessageBox::Ok);
    msgBox.exec();
}

void DownloadUpdateDialog::on_reply_downloadProgress(qint64 size, qint64 total)
{
    this->progressBar->setMaximum(total);
    this->progressBar->setMinimum(0);
    this->progressBar->setValue(size);
}

void DownloadUpdateDialog::on_reply_finished(void)
{
    if (this->reply->error())
    {
        this->showErrorMessage("Failed to download update file", this->reply->errorString());
        this->reply->deleteLater();
        this->reject();
        return;
    }

    QString filePath;

#ifndef APPIMAGE_UPDATER
    QTemporaryDir temporaryDir;
    temporaryDir.setAutoRemove(false);
    if (!temporaryDir.isValid())
    {
        this->showErrorMessage("Failed to create temporary directory", "");
        this->reply->deleteLater();
        this->reject();
        return;
    }

    this->temporaryDirectory = temporaryDir.path();
    filePath = temporaryDir.filePath(this->filename);
#else
    const char* appImageEnv = std::getenv("APPIMAGE");
    if (appImageEnv == nullptr ||
        !QFile(appImageEnv).exists()) 
    {
        this->showErrorMessage("APPIMAGE variable is empty or invalid", "");
        this->reply->deleteLater();
        this->reject();
        return;
    }

    filePath = appImageEnv;
    filePath += ".update";
#endif // APPIMAGE_UPDATER

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        this->showErrorMessage("QFile::open() Failed", "");
        this->reply->deleteLater();
        this->reject();
        return;
    }

    file.write(this->reply->readAll());
#ifdef APPIMAGE_UPDATER
    file.setPermissions(QFile(appImageEnv).permissions());
#endif // APPIMAGE_UPDATER
    file.close();

#ifdef APPIMAGE_UPDATER
    int ret = std::rename(filePath.toStdString().c_str(), appImageEnv);
    if (ret != 0)
    {
        this->showErrorMessage("std::rename() Failed", QString(strerror(errno)));
        this->reply->deleteLater();
        this->reject();
        return;
    }

    QProcess process;
    process.setProgram(appImageEnv);
    process.startDetached();
#endif // APPIMAGE_UPDATER

    this->reply->deleteLater();
    this->accept();
}

