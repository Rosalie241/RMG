/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020-2025 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "RomBrowserSearchWidget.hpp"

using namespace UserInterface::Widget;

RomBrowserSearchWidget::RomBrowserSearchWidget(QWidget* parent) : QWidget(parent)
{
	// configure search line edit
	this->lineEdit = new QLineEdit(this);
	this->lineEdit->setPlaceholderText("Search games...");
    connect(this->lineEdit, &QLineEdit::textChanged, this, &RomBrowserSearchWidget::on_lineEdit_textChanged);

    // configure close button
	this->closeButton = new QPushButton(this);
    this->closeButton->setText("Close");
    connect(this->closeButton, &QPushButton::clicked, this, &RomBrowserSearchWidget::on_closeButton_clicked);

    // configure layout
	this->widgetLayout = new QHBoxLayout(this);
    this->widgetLayout->addWidget(this->lineEdit);
    this->widgetLayout->addWidget(this->closeButton);
    this->setLayout(this->widgetLayout);
}

RomBrowserSearchWidget::~RomBrowserSearchWidget(void)
{
}

void RomBrowserSearchWidget::FocusSearchText(void)
{
	this->lineEdit->setFocus();
}

void RomBrowserSearchWidget::ClearSearchTerm(void)
{
	this->lineEdit->setText("");
}

void RomBrowserSearchWidget::on_lineEdit_textChanged(const QString& text)
{
	emit this->SearchTextChanged(text);
}

void RomBrowserSearchWidget::on_closeButton_clicked(void)
{
	this->hide();
	this->ClearSearchTerm();
}

