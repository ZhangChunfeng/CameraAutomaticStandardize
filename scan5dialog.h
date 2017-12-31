#ifndef SCAN5DIALOG_H
#define SCAN5DIALOG_H

#include <QTimer>
#include <QCloseEvent>
#include "ui_scan5dialog.h"

class Scan5Dialog : public QDialog, private Ui::Scan5Dialog
{
    Q_OBJECT

public:
    explicit Scan5Dialog(QWidget *parent = 0);
    QString getScan5Ip() const;
    int getScan5Riband() const;
    bool isScan5ContourLine();

protected:
    void closeEvent(QCloseEvent *event);
    void readSetting();
    void writeSetting();

private slots:
    void on_scan5LoginOKBtn_clicked();
    void on_scan5LoginCancelBtn_clicked();
    void on_comboBox_currentIndexChanged(int index);
    void on_scan5CheckBox_stateChanged(int arg1);
    void refreshLoginDialog();

private:
    QString ipText;
    int     ribandIndex;
    bool    isContourline;
    QTimer  *timer;
    bool    isSuccess;
};

#endif // SCAN5DIALOG_H
