#ifndef DATARELATIONSDIALOG_H
#define DATARELATIONSDIALOG_H

#include <qwt_plot_curve.h>
#include "ui_datarelationsdialog.h"

class DataRelationsDialog : public QDialog, private Ui::DataRelationsDialog
{
    Q_OBJECT

public:
    explicit DataRelationsDialog(QWidget *parent = 0);
    void Sample_CurveSettingPlot(const QString &title,
                                 const QVector<double> &xData,
                                 const QVector<double> &yData);
    void All_CurveSettingPlot(const QString &title,
                              const QVector<double> &xData,
                              const QVector<double> &yData);

private:
    void relationPlotInit();
    void plotSettingInit(QwtPlot *plot);
};

#endif // DATARELATIONSDIALOG_H
