#include <QSettings>
#include <QDebug>
#include "scan5dialog.h"

Scan5Dialog::Scan5Dialog(QWidget *parent) :
    QDialog(parent)
{
    setupUi(this);

    ipText = "";
    ribandIndex = 0;
    isContourline = false;
    isSuccess = false;
    timer = new QTimer(this);
    scan5LoginOKBtn->setEnabled(false);
    readSetting();
    setWindowTitle(QStringLiteral("scan5登录界面"));
    setWindowIcon(QIcon(":/new/prefix1/images/goldstar.png"));
    connect(timer, SIGNAL(timeout()), this, SLOT(refreshLoginDialog()));
    timer->start(200);
}

void Scan5Dialog::refreshLoginDialog()
{
    if(ipText.isEmpty()) {
        isSuccess = false;
    } else {
        scan5LoginOKBtn->setEnabled(true);
        isSuccess = true;
    }
    ipText = scan5IPLineEdit->text();
    ribandIndex = scan5RibandComboBox->currentIndex();
    isContourline = scan5CheckBox->isChecked();
}

void Scan5Dialog::on_scan5LoginOKBtn_clicked()
{
    writeSetting();
    timer->stop();
    accept();
}

void Scan5Dialog::on_scan5LoginCancelBtn_clicked()
{
    timer->stop();
    reject();
}

void Scan5Dialog::on_comboBox_currentIndexChanged(int index)
{
    ribandIndex = index;
}

void Scan5Dialog::on_scan5CheckBox_stateChanged(int arg1)
{
    if(arg1 == Qt::Checked) {
        isContourline = true;
    } else {
        isContourline = false;
    }
}

void Scan5Dialog::closeEvent(QCloseEvent *event)
{
    if(isSuccess) {
        writeSetting();
        event->accept();
    } else {
        event->ignore();
    }
    timer->stop();
    QDialog::closeEvent(event);
}

void Scan5Dialog::writeSetting()
{
    QSettings settings("Scan5LoginDialog", "Application");
    settings.beginGroup("Scan5LoginDlg");
    settings.setValue("scan5ip", scan5IPLineEdit->text());
    settings.setValue("scan5riband", scan5RibandComboBox->currentIndex());
    settings.setValue("isContourline", scan5CheckBox->isChecked());
    settings.endGroup();
}

void Scan5Dialog::readSetting()
{
    QSettings setting("Scan5LoginDialog", "Application");
    setting.beginGroup("Scan5LoginDlg");
    scan5IPLineEdit->setText(setting.value("scan5ip").toString());
    scan5RibandComboBox->setCurrentIndex(setting.value("scan5riband").toInt());
    scan5CheckBox->setChecked(setting.value("isContourline").toBool());
    setting.endGroup();
}

QString Scan5Dialog::getScan5Ip() const
{
    return ipText;
}

int Scan5Dialog::getScan5Riband() const
{
    return ribandIndex;
}

bool Scan5Dialog::isScan5ContourLine()
{
    return isContourline;
}
