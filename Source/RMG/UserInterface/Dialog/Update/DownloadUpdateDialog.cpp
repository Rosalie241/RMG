/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020-2025 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "DownloadUpdateDialog.hpp"
#include "Utilities/QtMessageBox.hpp"

#include <QNetworkAccessManager>
#include <QDesktopServices>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QTemporaryDir>
#include <QPushButton>
#include <QFileInfo>
#include <QProcess>
#include <QFile>

using namespace UserInterface::Dialog;
using namespace Utilities;

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
        QtMessageBox::Error(this, "Failed to download update file", this->reply->errorString());
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
        QtMessageBox::Error(this, "Failed to create temporary directory", "");
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
        QtMessageBox::Error(this, "APPIMAGE variable is empty or invalid", "");
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
        QtMessageBox::Error(this, "QFile::open() Failed", "");
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
        QtMessageBox::Error(this, "std::rename() Failed", QString(strerror(errno)));
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

