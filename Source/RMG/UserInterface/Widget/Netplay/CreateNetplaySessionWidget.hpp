/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020-2025 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef CREATENETPLAYSESSIONWIDGET_HPP
#define CREATENETPLAYSESSIONWIDGET_HPP

#include "CreateNetplaySessionEmptyWidget.hpp"

#include <QStackedWidget>
#include <QListWidget>

// ROM data
struct NetplayRomData
{
    QString GoodName;
    QString MD5;
    QString File;
};

namespace UserInterface
{
namespace Widget
{
class CreateNetplaySessionWidget : public QStackedWidget
{
    Q_OBJECT

  public:
    CreateNetplaySessionWidget(QWidget *);
    ~CreateNetplaySessionWidget(void);

    void AddRomData(QString goodName, QString md5, QString file);

    void RefreshDone(void);
    
    bool IsCurrentRomValid(void);
    bool GetCurrentRom(NetplayRomData& data);

  private:
    Widget::CreateNetplaySessionEmptyWidget* emptyWidget = nullptr;

    QListWidget* listWidget = nullptr;

  private slots:
    void on_listWidget_currentRowChanged(int index);

  signals:
    void OnRomChanged(bool valid);

};
} // namespace Widget
} // namespace UserInterface

#endif // CREATENETPLAYSESSIONWIDGET_HPP
