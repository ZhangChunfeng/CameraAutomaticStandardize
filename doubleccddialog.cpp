#include <QDebug>
#include <QSettings>
#include "doubleccddialog.h"

DoubleCCDDialog::DoubleCCDDialog(QWidget *parent) :
    QDialog(parent)
{
    setupUi(this);
    setWindowTitle(QStringLiteral("双枪相机登录界面"));
    setWindowIcon(QIcon(":/new/prefix1/images/goldstar.png"));
    m_jaiRefreshTimer = new QTimer(this);
    readSetting();
    m_isSuccess = false;
    JaiOKBtn->setEnabled(false);
    connect(m_jaiRefreshTimer, SIGNAL(timeout()), this, SLOT(Check_Input_isEmpty()));
    m_jaiRefreshTimer->start(1000);
}

void DoubleCCDDialog::Check_Input_isEmpty()
{
    //检查是否输入曝光时间
    if(!exposureTimeLineEdit->text().isEmpty()){
        JaiOKBtn->setEnabled(true);
    }
}

void DoubleCCDDialog::on_JaiOKBtn_clicked()
{
    //成功登录
    m_pixelFOrmat = cbPixelFormat->currentIndex();
    m_isSuccess = true;
    m_jaiRefreshTimer->stop();
    QListWidgetItem* item = listWidget->currentItem();
    if (item) {
        selectedCamera = item->data(Qt::UserRole).toString();
    }
    writeSetting();
    qDebug()<<"Login to Jai Successful.";
    accept();
}

void DoubleCCDDialog::on_JaiCancelBtn_clicked()
{
    //登录失败
    m_isSuccess = false;
    m_jaiRefreshTimer->stop();
    qDebug()<<"Login to Jai Failure.";
    reject();
}

void DoubleCCDDialog::readSetting()
{
    //读配置文件，保留上次的设置
    QSettings setting("DoubleCCD", "Application");
    setting.beginGroup("DoubleCCDDlg");
    exposureTimeLineEdit->setText(setting.value("time").toString());
    cbPixelFormat->setCurrentIndex(setting.value("PixelIndex").toInt());
    listWidget->setCurrentRow(setting.value("ListRow").toInt());
    setting.endGroup();
}

void DoubleCCDDialog::writeSetting()
{
    //成功登录后，保存此次参数设置的结果
    QSettings setting("DoubleCCD", "Application");
    setting.beginGroup("DoubleCCDDlg");
    setting.setValue("time", exposureTimeLineEdit->text());
    setting.setValue("PixelIndex", cbPixelFormat->currentIndex());
    setting.setValue("ListRow", listWidget->currentRow());
    setting.endGroup();
}

QStringList DoubleCCDDialog::jai_exposureTime()
{
    //留出读取曝光时间的接口
    QStringList tempList;
    QStringList timeList;
    QString str;
    str = exposureTimeLineEdit->text();
    tempList = str.split(QRegExp("\\W+"), QString::SkipEmptyParts);
    for(int i = 0; i < tempList.size(); i++){
        timeList.append(tempList.at(i));
    }
    return timeList;
//    if(m_isSuccess){
//        return timeList;
//    }else{
//        QStringList s;
//        for(int j = 0; j < tempList.size(); j++){
//            s.append(tempList.at(j));
//            s.clear();
//        }
//        return s;
//    }
}

void DoubleCCDDialog::jai_deviceType_add(QListWidgetItem* itm)
{
    //向设备类型清单中添加新的内容
    listWidget->addItem(itm);
}

void DoubleCCDDialog::setCurrentRow(int row)
{
    //设置默认类型
    QListWidgetItem *selectedItem = NULL;
    if ((row >= 0) && (row < listWidget->count())){
        selectedItem = listWidget->item(row);
    }
    if (selectedItem) {
        listWidget->setCurrentRow(0);
        selectedItem->setSelected(true);
    }
}

void DoubleCCDDialog::jai_deviceType_clear()
{
    //将设备类型清单内容清空
    listWidget->clear();
}

int DoubleCCDDialog::get_image_pixelFormat()
{
    return m_pixelFOrmat;
}

QString DoubleCCDDialog::jai_deviceType_choose()
{
    return selectedCamera;
}
