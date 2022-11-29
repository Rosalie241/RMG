/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "AddCheatDialog.hpp"

#include <QMessageBox>
#include <QPushButton>
#include <QTextBlock>
#include <QCloseEvent>
#include <QRegularExpression>
#include <QFileInfo>
#include <iostream>

#include <RMG-Core/Core.hpp>
#include <qpushbutton.h>

using namespace UserInterface::Dialog;

AddCheatDialog::AddCheatDialog(QWidget *parent) : QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint)
{
    this->setupUi(this);

    this->validateOkButton();
}

AddCheatDialog::~AddCheatDialog(void)
{
}

void AddCheatDialog::SetCheat(CoreCheat cheat)
{
    // change window title
    this->setWindowTitle("Edit Cheat");

    // fill in UI elements
    this->nameLineEdit->setText(QString::fromStdString(cheat.Name));
    this->authorLineEdit->setText(QString::fromStdString(cheat.Author));
    this->notesTextEdit->setPlainText(QString::fromStdString(cheat.Note));

    this->nameLineEdit->setCursorPosition(0);
    this->authorLineEdit->setCursorPosition(0);

    std::vector<std::string> codeLines;
    std::vector<std::string> optionLines;

    // try to retrieve the cheat text lines
    if (!CoreGetCheatLines(cheat, codeLines, optionLines))
    {
        return;
    }

    this->setPlainTextEditLines(this->codeTextEdit, codeLines);
    this->setPlainTextEditLines(this->optionsTextEdit, optionLines);

    this->updateMode = true;
    this->oldCheat = cheat;
}

void AddCheatDialog::setPlainTextEditLines(QPlainTextEdit* plainTextEdit, std::vector<std::string> lines)
{
    // reset text edit
    plainTextEdit->clear();

    // add lines
    for (std::string& line : lines)
    {
        plainTextEdit->appendPlainText(QString::fromStdString(line));
    }

    // reset cursor
    plainTextEdit->moveCursor(QTextCursor::Start);
    plainTextEdit->ensureCursorVisible();
}

void AddCheatDialog::showErrorMessage(QString error, QString details)
{
    QMessageBox msgBox(this);
    msgBox.setIcon(QMessageBox::Icon::Critical);
    msgBox.setWindowTitle("Error");
    msgBox.setText(error);
    msgBox.setDetailedText(details);
    msgBox.addButton(QMessageBox::Ok);
    msgBox.exec();
}

bool AddCheatDialog::validate(void)
{
    QTextDocument* document;
    bool foundOption = false;
    int  optionSize = -1;
    QRegularExpression hexRegExpr("^[0-9A-F]+$");


    // ensure name isn't empty
    QString name = this->nameLineEdit->text();
    if (name.isEmpty())
    {
        return false;
    }
    else
    { // name can't start with or end with '\'
        if (name.startsWith("\\") ||
            name.endsWith("\\"))
        {
            return false;
        }
    }

    // ensure code lines aren't empty
    if (this->codeTextEdit->toPlainText().isEmpty())
    {
        return false;
    }

    // parse code lines
    document = this->codeTextEdit->document();
    for (int i = 0; i < document->lineCount(); i++)
    {
        QString line = document->findBlockByLineNumber(i).text();
        QStringList splitLine = line.split(' ');

        if (splitLine.size() != 2)
        {
            return false;
        }

        QString address = splitLine.at(0);
        QString value = splitLine.at(1);

        if (address.size() != 8 || value.size() != 4)
        {
            return false;
        }
        
        // address needs to be hex
        if (!hexRegExpr.match(address).hasMatch())
        {
            return false;
        }

        QString tmpValue = value;
        tmpValue.remove("?");

        // value without '?' should be hex
        if (!tmpValue.isEmpty() && !hexRegExpr.match(tmpValue).hasMatch())
        {
            return false;
        }

        int optionCount = value.count("?");
        if (optionCount > 0)
        {
            if (!foundOption)
            {
                optionSize = optionCount;
                foundOption = true;
            }
            else if (foundOption && optionCount != optionSize)
            { // if it doesn't match the existing size, fail
                return false;
            }
        }
    }


    // parse option lines
    document = this->optionsTextEdit->document();
    if (!foundOption)
    {
        return document->isEmpty();
    }

    for (int i = 0; i < document->lineCount(); i++)
    {
        QString line = document->findBlockByLineNumber(i).text();
        QStringList splitLine = line.split(' ');

        if (splitLine.size() < 2)
        {
            return false;
        }

        QString value = splitLine.at(0);
        QString name = splitLine.at(1);

        if (value.size() != optionSize)
        {
            return false;
        }

        if (name.isEmpty())
        {
            return false;
        }

        // value needs to be hex
        if (!hexRegExpr.match(value).hasMatch())
        {
            return false;
        }
    }

    return true;
}

void AddCheatDialog::validateOkButton(void)
{
    QPushButton* okButton = this->buttonBox->button(QDialogButtonBox::Ok);
    okButton->setEnabled(this->validate());
}

bool AddCheatDialog::getCheat(CoreCheat& cheat)
{
    std::vector<std::string> lines;

    QTextDocument* document;
    QString name;
    QString author;
    QString note;

    name = this->nameLineEdit->text();
    author = this->authorLineEdit->text();
    note = this->notesTextEdit->toPlainText();

    lines.push_back("$" + name.toStdString());
    if (!author.isEmpty())
    {
        lines.push_back("Author=" + author.toStdString());
    }
    if (!note.isEmpty())
    {
        lines.push_back("Note=" + note.toStdString());
    }

    document = this->codeTextEdit->document();
    for (int i = 0; i < document->lineCount(); i++)
    {
        QString line = document->findBlockByLineNumber(i).text();
        lines.push_back(line.toStdString());
    }
    document = this->optionsTextEdit->document();
    for (int i = 0; i < document->lineCount(); i++)
    {
        QString line = document->findBlockByLineNumber(i).text();
        lines.push_back(line.toStdString());
    }

    if (!CoreParseCheat(lines, cheat))
    {
        this->showErrorMessage("CoreParseCheat() Failed!", QString::fromStdString(CoreGetError()));
        return false;
    }

    return true;
}

bool AddCheatDialog::addCheat(void)
{
    CoreCheat cheat;

    if (!this->getCheat(cheat))
    {
        return false;
    }

    if (!CoreAddCheat(cheat))
    {
        this->showErrorMessage("CoreAddCheat() Failed!", QString::fromStdString(CoreGetError()));
        return false;
    }

    return true;
}

bool AddCheatDialog::updateCheat(void)
{
    CoreCheat cheat;

    if (!this->getCheat(cheat))
    {
        return false;
    }

    // we don't need to do anything,
    // when the user didn't change anything
    if (this->oldCheat == cheat)
    {
        return true;
    }

    if (!CoreUpdateCheat(this->oldCheat,cheat))
    {
        this->showErrorMessage("CoreUpdateCheat() Failed!", QString::fromStdString(CoreGetError()));
        return false;
    }

    return true;
}

void AddCheatDialog::accept(void)
{
    bool ret;

    if (!this->validate())
    {
        this->showErrorMessage("Validating Cheat Failed!", "");
        return;
    }

    if (this->updateMode)
    {
        ret = this->updateCheat();
    }
    else
    {
        ret = this->addCheat();
    }

    // don't close dialog on failure
    if (!ret)
    {
        return;
    }

    QDialog::accept();
}

void AddCheatDialog::on_nameLineEdit_textChanged(QString text)
{
    this->validateOkButton();
}

void AddCheatDialog::on_codeTextEdit_textChanged(void)
{
    this->validateOkButton();
}

void AddCheatDialog::on_optionsTextEdit_textChanged(void)
{
    this->validateOkButton();
}
