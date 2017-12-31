#include <QDebug>
#include <QSettings>
#include "hklogindlg.h"

hkLoginDlg::hkLoginDlg(QWidget *parent) :
    QDialog(parent)
{
    setupUi(this);
    readSetting();
    isFinished = false;
    setWindowIcon(QIcon(":/new/prefix1/images/goldstar.png"));
    setWindowTitle(QStringLiteral("海康网络相机用户登录界面"));
    refreshTimer = new QTimer(this);
    connect(refreshTimer, SIGNAL(timeout()), this, SLOT(refreshLogin()));
    refreshTimer->start(100);
}
void hkLoginDlg::refreshLogin()
{
    if(ipLineEdit->text().isEmpty()){
        hkLoginButton->setEnabled(false);
        return;
    }
    if(userLineEdit->text().isEmpty()){
        hkLoginButton->setEnabled(false);
        return;
    }
    if(passwordLineEdit->text().isEmpty()){
        hkLoginButton->setEnabled(false);
        return;
    }
    hkLoginButton->setEnabled(true);
}

void hkLoginDlg::on_hkLoginButton_clicked()
{
    refreshTimer->stop();
    qDebug()<<"Login to configure hk finished.";
    isFinished = true;
    accept();
}

void hkLoginDlg::on_hkCancelButton_clicked()
{
    refreshTimer->stop();
    reject();
}

QString hkLoginDlg::getIPAddress() const
{
    if(isFinished){
        return ipLineEdit->text();
    }
    return " ";
}

QString hkLoginDlg::getUserName() const
{
    if(isFinished){
        return userLineEdit->text();
    }
    return " ";
}

QString hkLoginDlg::getPassword() const
{
    if(isFinished){
        return passwordLineEdit->text();
    }
    return " ";
}

void hkLoginDlg::closeEvent(QCloseEvent *event)
{
    if(close()){
        writeSetting();
        event->accept();
    }else{
        event->ignore();
    }
    QDialog::closeEvent(event);
}

void hkLoginDlg::writeSetting()
{
    QSettings setting("hkDialog", "Application");
    setting.beginGroup("HK");
    setting.setValue("ip", ipLineEdit->text());
    setting.setValue("user", userLineEdit->text());
    setting.setValue("password", passwordLineEdit->text());
    setting.endGroup();
}

void hkLoginDlg::readSetting()
{
    QSettings settings("hkDialog", "Application");
    settings.beginGroup("HK");
    ipLineEdit->setText(settings.value("ip").toString());
    userLineEdit->setText(settings.value("user").toString());
    passwordLineEdit->setText(settings.value("password").toString());
    settings.endGroup();
}
