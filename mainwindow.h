#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QGraphicsPixmapItem>
#include <QGraphicsEllipseItem>
#include <QGraphicsSceneMouseEvent>
#include <QCloseEvent>
#include <qwt_scale_draw.h>
#include <qwt_plot_curve.h>
#include <qwt_symbol.h>
#include <qwt_text.h>
#include <QTime>
#include <QElapsedTimer>
#include "ui_mainwindow.h"
#include "spcomm.h"
#include "plot.h"
#include "hkNVRDriver.h"
#include "baslercamera.h"
#include "autoGigeCamera.h"
#include "jaiGigeCamera.h"
#include "scan5sdk.h"

class TimeScaleDraw: public QwtScaleDraw
{
public:
    TimeScaleDraw( const QTime &base ):
        baseTime( base )
    {
    }
    virtual QwtText label( double v ) const
    {
        QTime upTime = baseTime.addSecs( static_cast<int>( v ) );
        return upTime.toString("hh:mm:ss");
    }
private:
    QTime baseTime;
};

class MainWindow : public QMainWindow, private Ui::MainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_hkAction_triggered();
    void on_baslerAction_triggered();
    void on_parameterConfigAction_triggered();
    void on_tableShowAction_triggered();
    void on_openSerialPortButton_clicked();
    void on_closeSerialPortButton_clicked();
    void on_relationAction_triggered();
    void on_selectAreaButton_clicked();
    void on_quitAction_triggered();
    void readImageOfCamera();
    void refreshTimePlot();
    void Standardize_Operation();
    void on_planckParameter_Validation_Action_triggered();
    void on_doubleAction_triggered();
    void on_actionScan5_triggered();

private:
    void timeRelation_PlotInit();
    void Interface_IconInit();
    void qwtPlotTimeSettingInit(QwtPlot *plot);
    bool eventFilter(QObject *obj, QEvent *e);
    void mouseRelease(QGraphicsSceneMouseEvent *event);
    void mouseMove(QGraphicsSceneMouseEvent *event);
    void mousePress(QGraphicsSceneMouseEvent *event);
    void CapturingAreaAndBrightness();
    void Calculate_CurrentImage_Brightness(const QRectF  &rect);
    void stableBrightAndTempRead();
    void realTimeBrightRead();
    bool isStable();
    void memoryInitialise();
    void SetPoint_Operation();
    void ReachSetpoint_Operation();
    void AnalyzingStable_Operation();
    void planckfitTest();
    void Fit_Planck_Result();
    void Fit_Poly_Result();
    void OpenGigeCameraOfInitial();
    void closeEvent(QCloseEvent *event);
    void convertIndex(int &targetIndex, int &sourIndex);
    static void dataArrived(uint16_t* data, int count, uint16_t rotatSpeed, uint16_t temp, void* user);
    void handleData(uint16_t* data, int count, uint16_t rotatSpeed, uint16_t temp);

private:
    QwtPlotCurve         curveLight[10];
    QwtSymbol            *symbolLight[10];
    QwtPlotCurve         curveTemp;
    QwtSymbol            *symbolTemp;

    QGraphicsScene       m_scene;
    QGraphicsPixmapItem  m_imageItem;
    QGraphicsEllipseItem m_ellipse;
    bool                 m_isDrawing;
    QPointF              m_startPt;
    int                  KEYCOUNT;
    int                  Camera_Format;
    int                  m_mode;
    int                  m_condition;
    int                  m_conditionCnt;
    int                  CAMERA_TYPE;
    int                  EXPO_TIME[10];
    float                setPoint;
    float                lowTempValue;
    float                highTempValue;
    float                stepTempValue;
    float                stableWaitTime;
    float                tempDeviation;
    float                lightDeviation;
    int                  actionOfProcess;
    SPComm               *m_Com;
    HKNVRDriver          *m_hkDriver;
    AutoGigeCamera       *m_gigeCamera;
    JaiGigeCamera        *m_jaiCamera;
    QList<QList<float> >  m_realTemperature;
    QList<QList<float> >  m_realLight;
    QList<QList<float> >  m_stableTemperature;
    QList<QList<float> >  m_stableLight;
    QList<float>          analyzingStableList;
    QList<QTime>          m_timeOfProgress;
    QTimer                *m_refreshImageTimer;
    QTimer                *m_runOperationTimer;
    QTimer                *m_refreshTimePlotTimer;
    QByteArray            m_ImageGray;
    long                  m_ImageWidth;
    int                   m_ImageType;
    QList<float>          m_planckParameterSetList;
    QList<QColor>         colorList;
    int                   m_StateMachine;
    int                   m_counter;
    int                   m_index;
    Plot                  *m_scan5Plot;
    Scan5SDK              m_scan5SDK;
    QElapsedTimer         scan5ElapsedTimer;
    int                   fpsCount;
    QVector<double>       plotData;
};

#endif // MAINWINDOW_H
