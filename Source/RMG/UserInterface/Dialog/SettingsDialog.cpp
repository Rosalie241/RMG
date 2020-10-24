#include "SettingsDialog.hpp"

using namespace UserInterface::Dialog;

SettingsDialog::SettingsDialog(QWidget *parent) : QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint)
{
    this->setupUi(this);

    // TODO
    this->treeWidget->topLevelItem(0)->setSelected(true);
}

SettingsDialog::~SettingsDialog(void)
{
}
