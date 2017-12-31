#include <QDebug>
#include <QSettings>
#include "parameterconfigdlg.h"

ParameterConfigDlg::ParameterConfigDlg(QWidget *parent) :
    QDialog(parent)
{
    setupUi(this);
    isFinish = false;
    readSetting();
    setWindowIcon(QIcon(":/new/prefix1/images/goldstar.png"));
    setWindowTitle(QStringLiteral("参数设置"));
    refreshTimer = new QTimer(this);
    connect(refreshTimer, SIGNAL(timeout()), this, SLOT(refreshConfig()));
    refreshTimer->start(100);
}

void ParameterConfigDlg::refreshConfig()
{
    if(lowValueSetLineEdit->text().isEmpty()){
        parameterOkButton->setEnabled(false);
        return;
    }
    if(highTempSetLineEdit->text().isEmpty()){
        parameterOkButton->setEnabled(false);
        return;
    }
    if(tempStepLineEdit->text().isEmpty()){
        parameterOkButton->setEnabled(false);
        return;
    }
    if(tempStableDeviationLineEdit->text().isEmpty()){
        parameterOkButton->setEnabled(false);
        return;
    }
    if(lightStableDeviationLineEdit->text().isEmpty()){
        parameterOkButton->setEnabled(false);
        return;
    }
    if(stableWaitTimeLineEdit->text().isEmpty()){
        parameterOkButton->setEnabled(false);
        return;
    }
    isFinish = true;
    parameterOkButton->setEnabled(true);
}

int ParameterConfigDlg::getActionOfTemp()
{
    return actionComboBox->currentIndex();
}

QList<float> ParameterConfigDlg::getTempValue() const
{
    QList<float> list;
    float low = lowValueSetLineEdit->text().toFloat();
    float high = highTempSetLineEdit->text().toFloat();
    float step = tempStepLineEdit->text().toFloat();
    if(isFinish){
        list.append(low);
        list.append(high);
        list.append(step);
        return list;
    }
    list.append(0);
    list.append(0);
    list.append(0);
    return list;
    qDebug()<<"Login Of Config Parameter is error.";

}

QList<float> ParameterConfigDlg::getStableOfConfigParameter() const
{
    QList<float> list;
    float tempDev = tempStableDeviationLineEdit->text().toFloat();
    float lightDev = lightStableDeviationLineEdit->text().toFloat();
    float waitT = stableWaitTimeLineEdit->text().toFloat();
    if(isFinish){
        list.append(tempDev);
        list.append(lightDev);
        list.append(waitT);
        return list;
    }
    list.append(0);
    list.append(0);
    list.append(0);
    return list;
    qDebug()<<"Config Parameter error.";
}

void ParameterConfigDlg::on_parameterOkButton_clicked()
{
    if(isFinish == true) {
        writeSetting();
        refreshTimer->stop();
        qDebug()<<"Config Parameter Successful.";
        accept();
    }
}

void ParameterConfigDlg::on_parameterCancelButton_clicked()
{
    refreshTimer->stop();
    reject();
}

void ParameterConfigDlg::readSetting()
{
    QSettings setting("ParaConfig", "Application");
    setting.beginGroup("ParaConfigDlg");
    lowValueSetLineEdit->setText(setting.value("lowValue").toString());
    highTempSetLineEdit->setText(setting.value("highTemp").toString());
    tempStepLineEdit->setText(setting.value("tempStep").toString());
    tempStableDeviationLineEdit->setText(setting.value("tempStable").toString());
    lightStableDeviationLineEdit->setText(setting.value("lightStable").toString());
    stableWaitTimeLineEdit->setText(setting.value("stableWait").toString());
    setting.endGroup();
}

void ParameterConfigDlg::writeSetting()
{
    QSettings setting("ParaConfig", "Application");
    setting.beginGroup("ParaConfigDlg");
    setting.setValue("lowValue", lowValueSetLineEdit->text());
    setting.setValue("highTemp", highTempSetLineEdit->text());
    setting.setValue("tempStep", tempStepLineEdit->text());
    setting.setValue("tempStable", tempStableDeviationLineEdit->text());
    setting.setValue("lightStable", lightStableDeviationLineEdit->text());
    setting.setValue("stableWait", stableWaitTimeLineEdit->text());
    setting.endGroup();
}
