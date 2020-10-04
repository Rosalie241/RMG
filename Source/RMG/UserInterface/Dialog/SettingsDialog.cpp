#include "SettingsDialog.hpp"

using namespace UserInterface::Dialog;

SettingsDialog::SettingsDialog(QWidget *parent) : QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint)
{
    this->ui_Init();
    this->ui_Setup();
}

void SettingsDialog::ui_Init(void)
{
    this->ui_Layout = new QHBoxLayout(this);
    this->ui_TreeWidget = new QTreeWidget(this);
}

void SettingsDialog::ui_Setup(void)
{
    this->setWindowTitle("Settings");

    QStringList list;
    list.append("Core");
    list.append("Plugins");
    list.append("ROM Selection");

    QList<QTreeWidgetItem*> list2;

    for (int i = 0; i < list.size(); i++)
        list2.append(new QTreeWidgetItem(QStringList() << list.at(i)));

    this->ui_TreeWidget->addTopLevelItems(list2);
    this->ui_TreeWidget->setHeaderLabel("Settings");
    list2.at(0)->setSelected(true);

    this->ui_Layout->addWidget(this->ui_TreeWidget);

    this->setLayout(this->ui_Layout);
}
