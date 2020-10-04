#ifndef SETTINGSDIALOG_HPP
#define SETTINGSDIALOG_HPP

#include <QDialog>
#include <QWidget>
#include <QTreeWidget>
#include <QHBoxLayout>
#include <QStandardItemModel>

namespace UserInterface
{
    namespace Dialog
    {
        class SettingsDialog : public QDialog
        {
        public:
            SettingsDialog(QWidget *parent);

        private:
            QHBoxLayout* ui_Layout;
            QTreeWidget* ui_TreeWidget;

            void ui_Init(void);
            void ui_Setup(void);

            void model_Init(void);
            void model_Setup(void);
        };
    } // namespace Dialog
} // namespace UserInterface

#endif // SETTINGSDIALOG_HPP