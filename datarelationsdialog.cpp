#include <qwt.h>
#include <qwt_symbol.h>
#include <qwt_plot_canvas.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_grid.h>
#include <qwt_legend.h>
#include <qwt_plot_magnifier.h>
#include <qwt_plot_zoomer.h>
#include "datarelationsdialog.h"

DataRelationsDialog::DataRelationsDialog(QWidget *parent) :
    QDialog(parent)
{
    setupUi(this);
    setWindowIcon(QIcon(":/new/prefix1/images/goldstar.png"));
    setWindowTitle(QStringLiteral("温度与对应亮度的关系曲线图"));
    relationPlotInit();
}

void DataRelationsDialog::relationPlotInit()
{
    Sample_RelationsPlot->setTitle(QStringLiteral("采样下温度与对应亮度关系曲线图"));
    plotSettingInit(Sample_RelationsPlot);
    All_RelationsPlot->setTitle(QStringLiteral("全程下温度与对应亮度关系曲线图"));
    plotSettingInit(All_RelationsPlot);
}

void DataRelationsDialog::plotSettingInit(QwtPlot *plot)
{
    plot->setCanvasBackground( Qt::black );
    plot->setAxisTitle( QwtPlot::xBottom, QStringLiteral("亮度值"));
    plot->setAxisTitle( QwtPlot::yLeft, QStringLiteral("黑体温度值(℃)"));
    plot->setAxisScale( QwtPlot::xBottom, 0, 5000 );
    plot->setAxisScale( QwtPlot::yLeft, 300.0, 1700.0 );
    plot->setAxisAutoScale(QwtPlot::xBottom, true);
    plot->setAxisAutoScale(QwtPlot::yLeft, true);
    plot->insertLegend( new QwtLegend() );
    QwtPlotGrid *grid = new QwtPlotGrid();
    grid->setMajorPen(Qt::yellow, 0, Qt::DotLine);
    grid->attach(plot);
    (void) new QwtPlotMagnifier(plot->canvas());       //鼠标滚轮进行曲线缩放
    QwtPlotZoomer *zoomer = new QwtPlotZoomer( plot->canvas());
    zoomer->setRubberBandPen( QColor( Qt::red ));      //区域选择缩放，左键选择区域，右键恢复
    zoomer->setTrackerPen( QColor( Qt::black ));
    zoomer->setMousePattern(QwtEventPattern::MouseSelect2,Qt::RightButton, Qt::ControlModifier );
    zoomer->setMousePattern(QwtEventPattern::MouseSelect3,Qt::RightButton );
    if(!plot){
        delete grid;
        delete zoomer;
    }
}

void DataRelationsDialog::Sample_CurveSettingPlot(const QString &title,
                                                  const QVector<double> &xData,
                                                  const QVector<double> &yData)
{
    QwtPlotCurve *curve = new QwtPlotCurve();
    curve->setTitle(title);
    curve->setPen(qRgb(rand()%255, rand()%255, rand()%255), 1);
    curve->setRenderHint( QwtPlotItem::RenderAntialiased, true );
    QwtSymbol *symbol = new QwtSymbol(QwtSymbol::Ellipse,QBrush(Qt::yellow),QPen(Qt::cyan,1),QSize(1,1));
    curve->setSymbol(symbol);
    curve->setSamples(xData, yData);
    curve->attach(Sample_RelationsPlot);
    if(!Sample_RelationsPlot){
        delete curve;
        delete symbol;
    }
}

void DataRelationsDialog::All_CurveSettingPlot(const QString &title,
                                               const QVector<double> &xData,
                                               const QVector<double> &yData)
{
    QwtPlotCurve *curve = new QwtPlotCurve();
    curve->setTitle(title);
    curve->setPen(qRgb(rand()%255, rand()%255, rand()%255), 1);
    curve->setRenderHint( QwtPlotItem::RenderAntialiased, true );
    QwtSymbol *symbol = new QwtSymbol(QwtSymbol::Ellipse,QBrush(Qt::yellow),QPen(Qt::red,1),QSize(1,1));
    curve->setSymbol(symbol);
    curve->setSamples(xData, yData);
    curve->attach(All_RelationsPlot);
    if(!All_RelationsPlot){
        delete curve;
        delete symbol;
    }
}
