#ifndef BASLERLOGINDLG_H
#define BASLERLOGINDLG_H

#include <QTimer>
#include <QCloseEvent>
#include "ui_baslerlogindlg.h"

class BaslerLoginDlg : public QDialog, private Ui::BaslerLoginDlg
{
    Q_OBJECT

public:
    explicit BaslerLoginDlg(QWidget *parent = 0);
    int getBaslerKey();
    QStringList getExposureTime() const;

protected:
    void closeEvent(QCloseEvent* event);

private:
    void readSetting();
    void writeSetting();

private slots:
    void on_baslerLoginButton_clicked();
    void on_baslerCancelButton_clicked();
    void refrehLogin();

private:
    QTimer       *refreshLoginTimer;
    bool         isSuccess;
};

#endif // BASLERLOGINDLG_H
