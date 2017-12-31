#ifndef PARAMETERCONFIGDLG_H
#define PARAMETERCONFIGDLG_H

#include <QTimer>
#include "ui_parameterconfigdlg.h"

class ParameterConfigDlg : public QDialog, private Ui::ParameterConfigDlg
{
    Q_OBJECT

public:
    explicit ParameterConfigDlg(QWidget *parent = 0);
    int getActionOfTemp();
    QList<float> getTempValue() const;
    QList<float> getStableOfConfigParameter() const;

private slots:
    void on_parameterOkButton_clicked();
    void on_parameterCancelButton_clicked();
    void refreshConfig();

protected:
    void readSetting();
    void writeSetting();

private:
    QTimer     *refreshTimer;
    bool       isFinish;
};

#endif // PARAMETERCONFIGDLG_H
