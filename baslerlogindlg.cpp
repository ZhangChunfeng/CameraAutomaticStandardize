#include <QDebug>
#include <QSettings>
#include "baslerlogindlg.h"

BaslerLoginDlg::BaslerLoginDlg(QWidget *parent) :
    QDialog(parent)
{
    setupUi(this);
    readSetting();
    setWindowIcon(QIcon(":/new/prefix1/images/goldstar.png"));
    isSuccess = false;
    setWindowTitle(QStringLiteral("Basler单枪相机登录界面"));
    refreshLoginTimer = new QTimer(this);
    connect(refreshLoginTimer, SIGNAL(timeout()), this, SLOT(refrehLogin()));
    refreshLoginTimer->start(100);
}

void BaslerLoginDlg::refrehLogin()
{
    //检查是否可以登录
    if(exposureTimeLineEdit->text().isEmpty()){
        baslerLoginButton->setEnabled(false);
        return;
    }
    baslerLoginButton->setEnabled(true);
}

int BaslerLoginDlg::getBaslerKey()
{
    if(isSuccess){
        return keyComboBox->currentIndex() + 1;
    }
    return -1;
}

QStringList BaslerLoginDlg::getExposureTime() const
{
    return exposureTimeLineEdit->text().split(QRegExp("\\W+"), QString::SkipEmptyParts);
}

void BaslerLoginDlg::on_baslerLoginButton_clicked()
{
    isSuccess = true;
    qDebug()<<"Basler to Login Successful.";
    refreshLoginTimer->stop();
    accept();
}

void BaslerLoginDlg::on_baslerCancelButton_clicked()
{
    refreshLoginTimer->stop();
    reject();
}

void BaslerLoginDlg::closeEvent(QCloseEvent *event)
{
    if(close()){
        writeSetting();
        event->accept();
    }else{
        event->ignore();
    }
}

void BaslerLoginDlg::writeSetting()
{
    QSettings setting("Basler", "Application");
    setting.beginGroup("baslerDlg");
    setting.setValue("key", keyComboBox->currentIndex());
    setting.setValue("time", exposureTimeLineEdit->text());
    setting.endGroup();
}

void BaslerLoginDlg::readSetting()
{
    QSettings setting("Basler", "Application");
    setting.beginGroup("baslerDlg");
    keyComboBox->setCurrentIndex(setting.value("key").toInt());
    exposureTimeLineEdit->setText(setting.value("time").toString());
    setting.endGroup();
}
