#include <QDebug>
#include "planckparameterdialog.h"

PlanckParameterDialog::PlanckParameterDialog(QWidget *parent) :
    QDialog(parent)
{
    setupUi(this);
    setWindowIcon(QIcon(":/new/prefix1/images/goldstar.png"));
    setWindowTitle(QStringLiteral("普朗克系数验证设置界面"));
    planckSettingOKButton->setEnabled(false);
    isFinished = false;
    m_isFinishedTimer = new QTimer(this);
    connect(m_isFinishedTimer, SIGNAL(timeout()), this, SLOT(refreshSettingFinnish()));
    m_isFinishedTimer->start(100);
}

void PlanckParameterDialog::refreshSettingFinnish()
{
    if(aParameterLineEdit->text().isEmpty()){
        return;
    }
    if(bParameterLineEdit->text().isEmpty()){
        return;
    }
    if(cParameterLineEdit->text().isEmpty()){
        return;
    }
    planckSettingOKButton->setEnabled(true);
}

void PlanckParameterDialog::on_planckSettingOKButton_clicked()
{
    isFinished = true;
    qDebug()<<"Planck Parameters to Set Successful.";
    m_isFinishedTimer->stop();
    accept();
}

void PlanckParameterDialog::on_planckSettingCancelButton_clicked()
{
    m_isFinishedTimer->stop();
    qDebug()<<"Planck Parameters to Set Failure.";
    reject();
}

QList<float> PlanckParameterDialog::getPlanckParameterOfValidation() const
{
    QList<float> parameterList;
    if(isFinished){
        parameterList.append(aParameterLineEdit->text().toFloat());
        parameterList.append(bParameterLineEdit->text().toFloat());
        parameterList.append(cParameterLineEdit->text().toFloat());
    }else{
        parameterList.append(0);
        parameterList.append(0);
        parameterList.append(0);
    }
    return parameterList;
}
