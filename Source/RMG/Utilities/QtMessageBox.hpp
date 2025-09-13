/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020-2025 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef QTMESSAGEBOX_HPP
#define QTMESSAGEBOX_HPP

#include <QString>
#include <QWidget>

namespace Utilities
{
namespace QtMessageBox
{
	void Info(QWidget* parent, QString text, QString details = "");

	void Error(QWidget* parent, QString text, QString details = "");

	bool Question(QWidget* parent, QString text, QString checkBoxText, bool& checkBoxValue);
}
} // namespace Utilities

#endif // QTMESSAGEBOX_HPP
