#ifndef SETTINGSDIALOG_HPP
#define SETTINGSDIALOG_HPP

#include <QDialog>
#include <QWidget>
#include <QTreeWidget>
#include <QHBoxLayout>
#include <QStandardItemModel>

#include "ui_SettingsDialog.h"

namespace UserInterface
{
    namespace Dialog
    {
        class SettingsDialog : public QDialog, private Ui::SettingsDialog
        {
            Q_OBJECT

        public:
            SettingsDialog(QWidget *parent);
            ~SettingsDialog(void);
        };
    } // namespace Dialog
} // namespace UserInterface

#endif // SETTINGSDIALOG_HPP