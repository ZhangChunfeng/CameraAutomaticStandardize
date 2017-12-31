#ifndef HKLOGINDLG_H
#define HKLOGINDLG_H

#include <QTimer>
#include <QCloseEvent>
#include "ui_hklogindlg.h"

class hkLoginDlg : public QDialog, private Ui::hkLoginDlg
{
    Q_OBJECT

public:
    explicit hkLoginDlg(QWidget *parent = 0);
    QString getIPAddress() const;
    QString getUserName() const;
    QString getPassword() const;

protected:
    void closeEvent(QCloseEvent *);

private slots:
    void on_hkLoginButton_clicked();
    void on_hkCancelButton_clicked();
    void refreshLogin();

private:
    void readSetting();
    void writeSetting();

private:
    QTimer    *refreshTimer;
    bool      isFinished;
};

#endif // HKLOGINDLG_H
