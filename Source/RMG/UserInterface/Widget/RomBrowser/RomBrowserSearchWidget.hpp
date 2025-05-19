/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020-2025 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef ROMBROWSERSEARCHWIDGET_HPP
#define ROMBROWSERSEARCHWIDGET_HPP

#include <QPushButton>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QWidget>

namespace UserInterface
{
namespace Widget
{
class RomBrowserSearchWidget : public QWidget
{
	Q_OBJECT

public:
	RomBrowserSearchWidget(QWidget* parent);
	~RomBrowserSearchWidget(void);

	void FocusSearchText(void);
	void ClearSearchTerm(void);
	
signals:
	void SearchTextChanged(const QString& text);

private:
	QHBoxLayout* widgetLayout = nullptr;
	QLineEdit* lineEdit = nullptr;
	QPushButton* closeButton = nullptr;

private slots:
	void on_lineEdit_textChanged(const QString& text);
	void on_closeButton_clicked(void);
};
}
}

#endif // ROMBROWSERSEARCHWIDGET_HPP
