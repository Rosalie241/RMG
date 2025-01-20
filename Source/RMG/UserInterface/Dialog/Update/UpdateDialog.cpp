/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "UpdateDialog.hpp"
#include "DownloadUpdateDialog.hpp"
#include "Utilities/QtMessageBox.hpp"

#include <QFileInfo>
#include <QPushButton>
#include <QJsonArray>
#include <QDesktopServices>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTemporaryDir>
#include <QFile>
#include <QProcess>

using namespace UserInterface::Dialog;
using namespace Utilities;

UpdateDialog::UpdateDialog(QWidget *parent, QJsonObject jsonObject, bool forced) : QDialog(parent)
{
    this->setupUi(this);

    this->jsonObject = jsonObject;

    this->label->setText(jsonObject.value("tag_name").toString() + " Available");
    this->textEdit->setText(jsonObject.value("body").toString());

    // change ok button text to 'Update'
    QPushButton* button = this->buttonBox->button(QDialogButtonBox::Ok);
    button->setText("Update");

    // don't show the 'Don't check for updates again' checkbox,
    // when the user requested we check for updates
    this->disableUpdateCheckCheckBox->setHidden(forced);
}

UpdateDialog::~UpdateDialog(void)
{
}

void UpdateDialog::on_disableUpdateCheckCheckBox_stateChanged(int state)
{
    CoreSettingsSetValue(SettingsID::GUI_CheckForUpdates, (state == Qt::Unchecked));
    CoreSettingsSave();
}

QString UpdateDialog::GetFileName(void)
{
    return this->filename;
}

QUrl UpdateDialog::GetUrl(void)
{
    return this->url;
}

void UpdateDialog::accept(void)
{
    QJsonArray jsonArray = jsonObject["assets"].toArray();
    QString filenameToDownload;
    QUrl urlToDownload;

#ifdef _WIN32
    this->isWin32Setup = QFile::exists("unins000.exe") && QFile::exists("unins000.dat");
#endif // _WIN32

    for (const QJsonValue& value : jsonArray)
    {
        QJsonObject object = value.toObject();

        QString filename = object.value("name").toString().toLower();
        QString url      = object.value("browser_download_url").toString();

#ifdef _WIN32
        if (this->isWin32Setup)
        {
            if (filename.contains("windows64") &&
                filename.contains("setup") &&
                filename.endsWith(".exe"))
            {
                filenameToDownload = filename;
                urlToDownload = QUrl(url);
                break;
            }
        }
        else
        {
            if (filename.contains("windows64") &&
                filename.contains("portable") &&
                filename.endsWith(".exe"))
            {
                filenameToDownload = filename;
                urlToDownload = QUrl(url);
                break;
            }
        }
#else
        if (filename.contains("linux64") &&
            filename.contains("portable") &&
            filename.endsWith(".appimage"))
        {
            filenameToDownload = filename;
            urlToDownload = QUrl(url);
            break;
        }
#endif // _WIN32
    }

    if (filenameToDownload.isEmpty())
    {
        QtMessageBox::Error(this, "Failed to find update file");
        QDialog::reject();
        return;
    }

    this->url = urlToDownload;
    this->filename = filenameToDownload;
    QDialog::accept();
}
