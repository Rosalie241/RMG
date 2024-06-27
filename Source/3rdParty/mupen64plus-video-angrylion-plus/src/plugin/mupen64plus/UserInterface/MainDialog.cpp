/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "MainDialog.hpp"
#include "../api/m64p_types.h"
#include "../api/m64p_config.h"
#include "../gfx_m64p.h"

using namespace UserInterface;

MainDialog::MainDialog(QWidget* parent) : QDialog(parent)
{
    this->setupUi(this);

    // set screen size combobox
    int width = ConfigGetParamInt(configVideoAngrylionPlus, KEY_SCREEN_WIDTH);
    int height = ConfigGetParamInt(configVideoAngrylionPlus, KEY_SCREEN_HEIGHT);

    // taken from GLideN64's GLideNUI
    QRegularExpression windowedRegExp("([3-9][0-9]{2}|[1-7][0-9]{3}) ?x ?([2-9][0-9]{2}|[1-3][0-9]{3})");
    QValidator *windowedValidator = new QRegularExpressionValidator(windowedRegExp, this);
    this->screenSizeComboBox->setValidator(windowedValidator);

    QString resolutionString = QString::number(width) + " x " + QString::number(height);

    // add resolution if it doesnt exist
    if (this->screenSizeComboBox->findText(resolutionString) == -1)
    {
        this->screenSizeComboBox->addItem(resolutionString);
    }
    // set resolution as current item
    this->screenSizeComboBox->setCurrentText(resolutionString);

    // set numWorkers combobox
    this->numWorkersSpinBox->setValue(ConfigGetParamInt(configVideoAngrylionPlus, KEY_NUM_WORKERS));

    // set viMode combobox
    this->viModeComboBox->setCurrentIndex(ConfigGetParamInt(configVideoAngrylionPlus, KEY_VI_MODE));

    // set viInterpolation combobox
    this->viInterpolationComboBox->setCurrentIndex(ConfigGetParamInt(configVideoAngrylionPlus, KEY_VI_INTERP));

    // set DpCompat combobox
    this->dpCompatComboBox->setCurrentIndex(ConfigGetParamInt(configVideoAngrylionPlus, KEY_DP_COMPAT));

    this->parallelCheckBox->setChecked(ConfigGetParamBool(configVideoAngrylionPlus, KEY_PARALLEL));
    this->busyLoopCheckBox->setChecked(ConfigGetParamBool(configVideoAngrylionPlus, KEY_BUSY_LOOP));

    this->viWidescreenCheckBox->setChecked(ConfigGetParamBool(configVideoAngrylionPlus, KEY_VI_WIDESCREEN));
    this->viHideOverscanCheckBox->setChecked(ConfigGetParamBool(configVideoAngrylionPlus, KEY_VI_HIDE_OVERSCAN));
    this->viIntegerScalingCheckBox->setChecked(ConfigGetParamBool(configVideoAngrylionPlus, KEY_VI_INTEGER_SCALING));
    this->viVsyncCheckBox->setChecked(ConfigGetParamBool(configVideoAngrylionPlus, KEY_VI_VSYNC));
}

MainDialog::~MainDialog()
{
}

void MainDialog::on_buttonBox_clicked(QAbstractButton* button)
{
    QPushButton *pushButton = (QPushButton *)button;
    QPushButton *okButton = this->buttonBox->button(QDialogButtonBox::Ok);
    QPushButton *defaultButton = this->buttonBox->button(QDialogButtonBox::RestoreDefaults);

    if (pushButton == okButton)
    {
        // screen size
        int width  = this->screenSizeComboBox->currentText().split(" x ").at(0).toInt();
        int height = this->screenSizeComboBox->currentText().split(" x ").at(1).toInt();
        ConfigSetParameter(configVideoAngrylionPlus, KEY_SCREEN_WIDTH, M64TYPE_INT, &width);
        ConfigSetParameter(configVideoAngrylionPlus, KEY_SCREEN_HEIGHT, M64TYPE_INT, &height);

        // numWorkers
        int numWorkersValue = this->numWorkersSpinBox->value();
        ConfigSetParameter(configVideoAngrylionPlus, KEY_NUM_WORKERS, M64TYPE_INT, &numWorkersValue);

        // viMode
        int viModeValue = this->viModeComboBox->currentIndex();
        ConfigSetParameter(configVideoAngrylionPlus, KEY_VI_MODE, M64TYPE_INT, &viModeValue);

        // viInterpolation
        int viInterpolationValue = this->viInterpolationComboBox->currentIndex();
        ConfigSetParameter(configVideoAngrylionPlus, KEY_VI_INTERP, M64TYPE_INT, &viInterpolationValue);

        // DpCompat
        int dpCompatValue = this->dpCompatComboBox->currentIndex();
        ConfigSetParameter(configVideoAngrylionPlus, KEY_DP_COMPAT, M64TYPE_INT, &dpCompatValue);

        // checkboxes
        int parallelValue = this->parallelCheckBox->isChecked() ? 1 : 0;
        int busyLoopValue = this->busyLoopCheckBox->isChecked() ? 1 : 0;

        int viWidescreenValue = this->viWidescreenCheckBox->isChecked() ? 1 : 0;
        int viHideOverscanValue = this->viHideOverscanCheckBox->isChecked() ? 1 : 0;
        int viIntegerScalingValue = this->viIntegerScalingCheckBox->isChecked() ? 1 : 0;
        int viVsyncValue = this->viVsyncCheckBox->isChecked() ? 1 : 0;

        ConfigSetParameter(configVideoAngrylionPlus, KEY_PARALLEL, M64TYPE_BOOL, &parallelValue);
        ConfigSetParameter(configVideoAngrylionPlus, KEY_BUSY_LOOP, M64TYPE_BOOL, &busyLoopValue);

        ConfigSetParameter(configVideoAngrylionPlus, KEY_VI_WIDESCREEN, M64TYPE_BOOL, &viWidescreenValue);
        ConfigSetParameter(configVideoAngrylionPlus, KEY_VI_HIDE_OVERSCAN, M64TYPE_BOOL, &viHideOverscanValue);
        ConfigSetParameter(configVideoAngrylionPlus, KEY_VI_INTEGER_SCALING, M64TYPE_BOOL, &viIntegerScalingValue);
        ConfigSetParameter(configVideoAngrylionPlus, KEY_VI_VSYNC, M64TYPE_BOOL, &viVsyncValue);

        ConfigSaveSection("Video-AngrylionPlus");   
    }
    else if (pushButton == defaultButton)
    {
        n64video_config config;
        n64video_config_init(&config);

        this->screenSizeComboBox->setCurrentText("640 x 480");
        this->numWorkersSpinBox->setValue(config.num_workers);
        this->viModeComboBox->setCurrentIndex(config.vi.mode);
        this->viInterpolationComboBox->setCurrentIndex(config.vi.interp);
        this->dpCompatComboBox->setCurrentIndex(config.dp.compat);
        this->parallelCheckBox->setChecked(config.parallel);
        this->busyLoopCheckBox->setChecked(config.busyloop);
        this->viWidescreenCheckBox->setChecked(config.vi.widescreen);
        this->viHideOverscanCheckBox->setChecked(config.vi.hide_overscan);
        this->viIntegerScalingCheckBox->setChecked(config.vi.integer_scaling);
        this->viVsyncCheckBox->setChecked(config.vi.vsync);
    }
}
