/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "NetplaySessionPasswordDialog.hpp"
#include "NetplayCommon.hpp"

#include <QRegularExpressionValidator>
#include <QRegularExpression>

using namespace UserInterface::Dialog;

NetplaySessionPasswordDialog::NetplaySessionPasswordDialog(QWidget *parent) : QDialog(parent)
{
    this->setupUi(this);

    QRegularExpression re(NETPLAYCOMMON_PASSWORD_REGEX);
    this->lineEdit->setValidator(new QRegularExpressionValidator(re, this));
}

NetplaySessionPasswordDialog::~NetplaySessionPasswordDialog(void)
{
}

QString NetplaySessionPasswordDialog::GetPassword(void)
{
    return this->password;
}

void NetplaySessionPasswordDialog::accept()
{
    this->password = this->lineEdit->text();
    QDialog::accept();
}
