#ifndef PLANCKPARAMETERDIALOG_H
#define PLANCKPARAMETERDIALOG_H

#include <QTimer>
#include "ui_planckparameterdialog.h"

class PlanckParameterDialog : public QDialog, private Ui::PlanckParameterDialog
{
    Q_OBJECT

public:
    explicit PlanckParameterDialog(QWidget *parent = 0);
    QList<float> getPlanckParameterOfValidation() const;

private slots:
    void on_planckSettingOKButton_clicked();
    void on_planckSettingCancelButton_clicked();
    void refreshSettingFinnish();

private:
    bool    isFinished;
    QTimer  *m_isFinishedTimer;
};

#endif // PLANCKPARAMETERDIALOG_H
