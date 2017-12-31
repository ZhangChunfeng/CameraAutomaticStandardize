#ifndef DOUBLECCDDIALOG_H
#define DOUBLECCDDIALOG_H

#include <QTimer>
#include <QCloseEvent>
#include <QListWidgetItem>
#include "ui_doubleccddialog.h"

class DoubleCCDDialog : public QDialog, private Ui::DoubleCCDDialog
{
    Q_OBJECT

public:
    explicit DoubleCCDDialog(QWidget *parent = 0);
    QStringList jai_exposureTime();
    void jai_deviceType_add(QListWidgetItem* itm);
    void jai_deviceType_clear();
    int  get_image_pixelFormat();
    void setCurrentRow(int row);
    QString jai_deviceType_choose();

private slots:
    void on_JaiOKBtn_clicked();
    void on_JaiCancelBtn_clicked();
    void Check_Input_isEmpty();

private:
    void readSetting();
    void writeSetting();

private:
    QString   selectedCamera;
    QTimer   *m_jaiRefreshTimer;
    bool      m_isSuccess;
    int       m_pixelFOrmat;
};

#endif // DOUBLECCDDIALOG_H
