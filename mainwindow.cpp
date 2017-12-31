#include <QTableWidgetItem>
#include <QSerialPortInfo>
#include <qwt_legend.h>
#include <qwt_symbol.h>
#include <qwt_plot_grid.h>
#include <qwt_plot_magnifier.h>
#include <qwt_plot_zoomer.h>
#include <qwt_event_pattern.h>
#include <QDebug>
#include <QDesktopWidget>
#include "mainwindow.h"
#include "hklogindlg.h"
#include "cameraTypedef.h"
#include "baslerlogindlg.h"
#include "parameterconfigdlg.h"
#include "showtabledialog.h"
#include "datarelationsdialog.h"
#include "imageConvert.h"
#include "planckparameterdialog.h"
#include "doubleccddialog.h"
#include "scan5dialog.h"
#include "planckfit.h"
#include "poly2fit.h"
#include "cameraTypedef.h"

unsigned char pswapbuff[BUFF_RGB_SIZE];
const int timeInterval = 3;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent)
  , KEYCOUNT(1)
  , CAMERA_TYPE(10)
  , Camera_Format(1)
  , m_mode(0)
  , m_scene(parent)
  , m_imageItem()
  , m_ellipse()
  , m_isDrawing(false)
  , m_StateMachine(0)
  , m_conditionCnt(0)
  , m_counter(0)
  , m_index(0)
  , m_condition(0)
  , m_ImageType(1)
{
    setupUi(this);
    setWindowTitle(QStringLiteral("温度场自动标定软件"));
    qsrand(QTime(0, 0, 0).secsTo(QTime::currentTime()));
    openSerialPortButton->setEnabled(false);
    m_Com = new SPComm();
    m_hkDriver = new HKNVRDriver();
    m_gigeCamera = new AutoGigeCamera();
    m_jaiCamera = new JaiGigeCamera();
    for(int i = 0; i < 10; i++) {
        symbolLight[i] = new QwtSymbol;
    }
    symbolTemp  = new QwtSymbol;
    m_refreshImageTimer    = new QTimer(this);
    m_runOperationTimer    = new QTimer(this);
    m_refreshTimePlotTimer = new QTimer(this);
    m_scan5Plot = new Plot();
    connect(m_refreshTimePlotTimer, SIGNAL(timeout()), this, SLOT(refreshTimePlot()));
    connect(m_refreshImageTimer, SIGNAL(timeout()), this, SLOT(readImageOfCamera()));
    connect(m_runOperationTimer, SIGNAL(timeout()), this, SLOT(Standardize_Operation()));
    QList<QSerialPortInfo> list = QSerialPortInfo::availablePorts();     //获取可用的串口
    foreach (QSerialPortInfo info, list ) {
        portNameComboBox->addItem(info.portName());
    }
    m_planckParameterSetList.append(0);
    m_planckParameterSetList.append(0);
    m_planckParameterSetList.append(0);
    timeRelation_PlotInit();
    Interface_IconInit();
    m_refreshTimePlotTimer->start(3000);
    graphicsView->setScene(&m_scene);
    m_scene.addItem(&m_imageItem);
    m_scene.addItem(&m_ellipse);
    m_ellipse.setPen(QPen(Qt::red));
    m_ellipse.setBrush(Qt::NoBrush);
    m_scene.installEventFilter(this);
}

MainWindow::~MainWindow()
{
    if(m_Com){
        delete m_Com;
        m_Com = NULL;
    }
    if(m_hkDriver){
        delete m_hkDriver;
        m_hkDriver = NULL;
    }
    if(m_gigeCamera){
        delete m_gigeCamera;
        m_gigeCamera = NULL;
    }
    if(m_jaiCamera){
        delete m_jaiCamera;
        m_jaiCamera = NULL;
    }
    curveTemp.detach();
    for(int i = 0; i < 10; i++) {
        curveLight[i].detach();
    }
    if(m_scan5Plot) {
        delete m_scan5Plot;
        m_scan5Plot = NULL;
    }
//    for(int j = 0; j < 10; j++) {
//        delete symbolLight[j];
//        symbolLight[j] = NULL;
//    }
//    delete symbolTemp;
//    symbolTemp = NULL;
}

QPixmap converToPixmap(HKGSFrame& gsFrame)
{
    QSize sz(gsFrame.nWidth, gsFrame.nHeight);               //图片格式转换
    if (gsFrame.nType == GSFRAME_TYPE_YV12) {
        QImage originalImage(sz.width(), sz.height(), QImage::Format_RGB32);
        YV12ToRGB32_FFmpeg(sz.width(), sz.height(), (uchar*)gsFrame.data.data(), originalImage.width(), originalImage.height(), originalImage.bits());
        return QPixmap::fromImage(originalImage);
    }
    return QPixmap();
}

void MainWindow::readImageOfCamera()
{
    HKGSFrame   hkFrame;
    BaslerFrame gigeFrame;
    JaiGSFrame  jaiFrame;
    int expousre, gain;
    QList<int> redHis, greenHis, blueHis;
    switch(CAMERA_TYPE)
    {
    case 0:
        if (m_hkDriver->getFrame(1, hkFrame)) {              //读入海康相机图像
            m_ImageGray  = hkFrame.data;
            m_ImageWidth = hkFrame.nWidth;
            m_ImageType  = 1;
            m_imageItem.setPixmap(converToPixmap(hkFrame));
        }
        m_mode  = 0;
        m_index = 0;
        break;
    case 1:                                                   //读入单枪相机图像
        if (m_gigeCamera->getFrameWithExpAndGain(gigeFrame, expousre, gain)) {
            if ((gigeFrame.nType == FrameGray)) {
                m_mode       = 0;
                m_ImageType  = 1;
                m_ImageGray  = gigeFrame.data;
                m_ImageWidth = gigeFrame.nWidth;
                //Set the target index.
                for(int i = 0; i < KEYCOUNT; i++) {
                    if(expousre == EXPO_TIME[i]) {
                        m_index = i;
                    }
                }
                curExposureTimeLineEdit->setText(QString::number(expousre, 'f', 2));
                qDebug() << "Target  Index:" << m_index;
                qDebug() << "Current Index:" << gigeFrame.seqIndex;
                QImage image((uchar*)gigeFrame.data.data(), gigeFrame.nWidth, gigeFrame.nHeight, QImage::Format_Indexed8);
                m_imageItem.setPixmap(QPixmap::fromImage(image));
            }
            if((gigeFrame.nType == FrameRGB888)){
                m_mode = 1;
                QImage image((uchar*)gigeFrame.data.data(), gigeFrame.nWidth, gigeFrame.nHeight, QImage::Format_RGB888);
                m_imageItem.setPixmap(QPixmap::fromImage(image));
            }
        }
        break;
    case 2:                                                   //读入双枪相机图像
        if (m_jaiCamera->getFrame(jaiFrame, &redHis, &greenHis, &blueHis)) {
            if ((jaiFrame.nType >= JaiFrameGray8Bit) && (jaiFrame.nType <= JaiFrameGray12BitOCCU2ByteMSB)) {
                QByteArray temp;
                int count = jaiFrame.data.count();
                if (jaiFrame.nType == JaiFrameGray8Bit) {
                    m_ImageType   = 1;
                    temp = jaiFrame.data;
                } else if (jaiFrame.nType == JaiFrameGray10BitOCCU2ByteMSB) {
                    m_ImageType   = 2;
                    temp.reserve( count >> 1);
                    for (int i = 0; i < count; i++) {
                        temp.append(jaiFrame.data.at(++i));
                    }
                } else if (jaiFrame.nType == JaiFrameGray12BitOCCU2ByteMSB) {
                    m_ImageType   = 3;
                    temp.reserve( count >> 1);
                    for (int i = 0; i < count; i++) {
                        temp.append(jaiFrame.data.at(++i));
                    }
                }
                m_mode = 0;
                m_ImageGray = jaiFrame.data;
                m_ImageWidth = jaiFrame.nWidth;

                //改变双枪的黑白模式的曝光时间
                m_index = m_condition;
                m_jaiCamera->setExposureTime(EXPO_TIME[m_condition]);
                curExposureTimeLineEdit->setText(QString::number(EXPO_TIME[m_condition],'f', 2));
                qDebug()<<"Current Index:"<<m_index<<",Current Exposure_Time:"<<EXPO_TIME[m_condition];

                QImage image((uchar*)temp.data(), jaiFrame.nWidth, jaiFrame.nHeight, QImage::Format_Indexed8);
                m_imageItem.setPixmap(QPixmap::fromImage(image));

            } else {
                //双枪的彩色模式只可用来显示，不能测温
                m_mode = 1;
                QImage image((uchar*)jaiFrame.data.data(), jaiFrame.nWidth, jaiFrame.nHeight, QImage::Format_RGB888);
                m_imageItem.setPixmap(QPixmap::fromImage(image));
            }
        }
        break;
    case 3:                                                   //读入scan5图像
        break;
    default:
        break;
    }
    CapturingAreaAndBrightness();
}

static bool isInEllipse(int x, int y, QRectF rc)
{
    float a = rc.width() / 2;
    float b = rc.height() / 2;
    x -= rc.center().x();
    y -= rc.center().y();
    if ( (x * x) / (a * a) + (y * y)/(b * b) <= 1.0) {
        return true;
    } else {
        return false;
    }
}

void MainWindow::CapturingAreaAndBrightness()
{
    //截取选取的区域并进行显示
    QRectF crop = m_ellipse.rect();
    QRectF realRc = m_ellipse.mapRectToItem(&m_imageItem, crop);
    QPixmap cropped = m_imageItem.pixmap().copy(realRc.toRect());
    QImage image;
    image = QImage(cropped.toImage()).convertToFormat(QImage::Format_ARGB32);
    for(int i = 0; i < image.width(); i++) {
        for (int j = 0; j < image.height(); j++) {
            if (!isInEllipse(i, j, image.rect())) {
                image.setPixel(i, j, Qt::transparent);
            }
        }
    }
    displayAreaLabel->setPixmap(QPixmap::fromImage(image));
    //计算当前图片的亮度值
    Calculate_CurrentImage_Brightness(realRc);
}

void MainWindow::Calculate_CurrentImage_Brightness(const QRectF &rect)
{
    QRectF rc = rect;
    float sum = 0;
    long fullImageWidth = m_ImageWidth;
    long totalCnt = 0;
    for(int i = rc.top(); i < rc.bottom(); i++){
        for(int j = rc.left(); j < rc.right(); j++){
            if (isInEllipse(j, i, rc)) {
                int index = i * fullImageWidth + j;
                uint16_t gray ;
                if (m_ImageType == 1) {
                    gray = (uint16_t) (m_ImageGray.at(index) & 0xff);
                } else if (m_ImageType == 2) {
                    index = index << 1;
                    gray = (0xff & m_ImageGray.at(index + 1));
                    gray = gray << 8;
                    gray += (uint)m_ImageGray.at(index);
                    gray = gray >> 6;
                } else if (m_ImageType == 3) {
                    index = index << 1;
                    gray = (0xff & m_ImageGray.at(index + 1));
                    gray = gray << 8;
                    gray += (uint)m_ImageGray.at(index);
                    gray = gray >> 4;
                }
                sum += gray;
                totalCnt++;
            }
        }
    }
    float v = 0;
    if (totalCnt != 0){
        v = sum / totalCnt;
    } else {
        v = 0;
    }
    currentBrightnessLineEdit->setText(QString::number(v, 'f', 2));

    //计算当前亮度值对应的软件温度值
    float x  = currentBrightnessLineEdit->text().toFloat();
    float a0 = m_planckParameterSetList.at(0);
    float b0 = m_planckParameterSetList.at(1);
    float c0 = m_planckParameterSetList.at(2);
    float y  = 0;
    y = b0 / qLn(a0 / (c0 + x) + 1) - 273.5;
    softTemperatureLineEdit->setText(QString::number(y, 'f', 2));
}

void MainWindow::mousePress(QGraphicsSceneMouseEvent *event)
{
    if ((event->buttons() & Qt::LeftButton) && (m_isDrawing))
    {
        m_startPt = event->scenePos();
        QRectF rc(m_startPt, m_startPt);
        m_ellipse.setRect(rc);
    }
}

void MainWindow::mouseMove(QGraphicsSceneMouseEvent *event)
{
    if ((event->buttons() & Qt::LeftButton) && m_isDrawing) {
        QRectF rc(m_startPt, event->scenePos());
        if (event->modifiers() & Qt::ControlModifier) {
            QPointF center = rc.center();
            qreal d = QLineF(rc.topLeft(), rc.bottomRight()).length();
            rc.setHeight(d);
            rc.setWidth(d);
            rc.moveCenter(center);
        }
        m_ellipse.setRect(rc);
    }
}

void MainWindow::mouseRelease(QGraphicsSceneMouseEvent *event)
{
    Q_UNUSED(event);
    m_isDrawing = false;
}

bool MainWindow::eventFilter(QObject *obj, QEvent *e)
{
    if (obj == &m_scene) {
        if (e->type() == QEvent::GraphicsSceneMousePress) {
            mousePress((QGraphicsSceneMouseEvent*)e);
        } else if (e->type() == QEvent::GraphicsSceneMouseMove) {
            mouseMove((QGraphicsSceneMouseEvent*)e);
        } else if (e->type() == QEvent::GraphicsSceneMouseRelease) {
            mouseRelease((QGraphicsSceneMouseEvent*)e);
        }
    }
    return QMainWindow::eventFilter(obj, e);
}

void MainWindow::refreshTimePlot()
{
    if(m_realTemperature.isEmpty()){
        return;
    }else if(m_realTemperature[0].isEmpty()){
        return;
    }
    bool isUpdate;
    isUpdate = (bool)(m_StateMachine != 3);
    //显示亮度随时间的变化关系
    QPolygonF lightPoints[10];
    for(int i = 0; i < m_realLight.size(); i++){
        for(int j = 0; j < m_realLight[i].size(); j++){
            int time = m_timeOfProgress.at(0).secsTo(m_timeOfProgress.at(j));
            lightPoints[i]<<QPointF( time, m_realLight[i].at(j));
//            lightPoints[i]<<QPointF( KEYCOUNT * j * timeInterval, m_realLight[i].at(j));
        }
       curveLight[i].setPen(colorList.at(i), 1);
       curveLight[i].setTitle(QString("%1").arg(EXPO_TIME[i]));
       curveLight[i].setRenderHint( QwtPlotItem::RenderAntialiased, true );
       curveLight[i].setSymbol(symbolLight[i]);
       curveLight[i].setSamples(lightPoints[i]);
       curveLight[i].attach(Time_Light_qwtPlot);
    }
    Time_Light_qwtPlot->setAxisAutoScale(QwtPlot::yLeft, true);
    Time_Light_qwtPlot->setAxisAutoScale(QwtPlot::xBottom, isUpdate);
    Time_Light_qwtPlot->replot();

    //显示温度随时间的变化关系
    QPolygonF tempPoints;
    for(int i = 0; i < m_realTemperature[0].size(); i++){
        int time = m_timeOfProgress.at(0).secsTo(m_timeOfProgress.at(i));
        tempPoints<<QPointF(time, m_realTemperature[0].at(i));
//        tempPoints<<QPointF( KEYCOUNT * i * timeInterval, m_realTemperature[0].at(i));
    }
    curveTemp.setPen(Qt::red, 1);
    curveTemp.setRenderHint( QwtPlotItem::RenderAntialiased, true );
    curveTemp.setSymbol(symbolTemp);
    curveTemp.setSamples(tempPoints);
    curveTemp.attach(Time_Temperature_qwtPlot);
    /*-----------设置坐标值超出最大范围自动缩放、刷新--------------*/
    Time_Temperature_qwtPlot->setAxisAutoScale(QwtPlot::yLeft, true);
    Time_Temperature_qwtPlot->setAxisAutoScale(QwtPlot::xBottom, isUpdate);
    Time_Temperature_qwtPlot->replot();
    qDebug()<<"Display datas in charts";
}

void MainWindow::timeRelation_PlotInit()
{
    QTime time;
    if(m_timeOfProgress.isEmpty()){
        time = QTime::currentTime();
        m_timeOfProgress.append(time);
    }else{
        time = m_timeOfProgress.at(0);
    }
    QString xTitle = QDateTime::currentDateTime().toString("yyyy-MM-dd");
    Time_Light_qwtPlot->setTitle(QStringLiteral("实时亮度曲线"));
    Time_Light_qwtPlot->setAxisScale(QwtPlot::yLeft, 0, 260);
    Time_Light_qwtPlot->setAxisScaleDraw(QwtPlot::xBottom, new TimeScaleDraw(time));
    Time_Light_qwtPlot->setAxisTitle(QwtPlot::yLeft, QStringLiteral("亮度值"));
    Time_Light_qwtPlot->setAxisTitle(QwtPlot::xBottom, xTitle);
    qwtPlotTimeSettingInit(Time_Light_qwtPlot);
    Time_Temperature_qwtPlot->setTitle(QStringLiteral("实时温度曲线"));
    Time_Temperature_qwtPlot->setAxisScale(QwtPlot::yLeft, 300, 1700);
    Time_Temperature_qwtPlot->setAxisScaleDraw(QwtPlot::xBottom, new TimeScaleDraw(time));
    Time_Temperature_qwtPlot->setAxisTitle(QwtPlot::yLeft, QStringLiteral("温度值"));
    Time_Temperature_qwtPlot->setAxisTitle(QwtPlot::xBottom, xTitle);
    qwtPlotTimeSettingInit(Time_Temperature_qwtPlot);
}

void MainWindow::Interface_IconInit()
{
    for(int i = 0; i < 10; i++) {
        symbolLight[i]->setPen(QPen(Qt::red, 1));
        symbolLight[i]->setBrush(Qt::blue);
        symbolLight[i]->setSize(1, 1);
        symbolLight[i]->setStyle(QwtSymbol::Ellipse);
    }

    symbolTemp->setPen(QPen(Qt::cyan, 1));
    symbolTemp->setBrush(Qt::yellow);
    symbolTemp->setSize(1, 1);
    symbolTemp->setStyle(QwtSymbol::Ellipse);

    setWindowIcon(QIcon(":/new/prefix1/images/goldstar.png"));
    openSerialPortButton->setIcon(QIcon(":/new/prefix1/images/start.png"));
//    openSerialPortButton->setStyleSheet("QPushButton{background-color:black;color: white; "
//                                        "border-radius: 10px; border: 2px groove gray; border-style: outset;}"
//                                        "QPushButton:hover{background-color:white; color: black;}"
//                                        "QPushButton:pressed{background-color:rgb(85, 170, 255);"
//                                        "border-style: inset; }");
    closeSerialPortButton->setIcon(QIcon(":/new/prefix1/images/stop.png"));
//    closeSerialPortButton->setStyleSheet("QPushButton{background-color:black;color: white; "
//                                         "border-radius: 10px; border: 2px groove gray; border-style: outset;}"
//                                         "QPushButton:hover{background-color:white; color: black;}"
//                                         "QPushButton:pressed{background-color:rgb(85, 170, 255);"
//                                         "border-style: inset; }");
}

void MainWindow::qwtPlotTimeSettingInit(QwtPlot *plot)
{
    plot->setCanvasBackground(Qt::black);
    plot->setAxisScale(QwtPlot::xBottom, 0, 60 * 60 * 6);
    plot->setAxisLabelRotation( QwtPlot::xBottom, 0);
    plot->setAxisLabelAlignment(QwtPlot::xBottom, Qt::AlignHCenter);
    plot->setAxisAutoScale(QwtPlot::xBottom, true);
    plot->setAxisAutoScale(QwtPlot::yLeft, true);
    plot->insertLegend(new QwtLegend());
    QwtPlotGrid *grid = new QwtPlotGrid();
    grid->setMajorPen(Qt::yellow, 1, Qt::DotLine);
    grid->attach(plot);
    //使用滚轮进行坐标的伸缩
    (void) new QwtPlotMagnifier(plot->canvas());
    //选择区域进行坐标局部放大，左键可选择区域，右键可还原区域
    QwtPlotZoomer *zoomer = new QwtPlotZoomer(plot->canvas());
    zoomer->setRubberBandPen(QColor(Qt::red));
    zoomer->setTrackerPen(QColor(Qt::black));
    zoomer->setMousePattern(QwtEventPattern::MouseSelect2, Qt::RightButton, Qt::ControlModifier );
    zoomer->setMousePattern(QwtEventPattern::MouseSelect3, Qt::RightButton );
    if(!plot){
        delete grid;
        grid = NULL;
        delete zoomer;
        zoomer = NULL;
    }
}

void MainWindow::on_hkAction_triggered()
{
    //海康网络相机登录
    hkLoginDlg hkDlg;
    int ret = hkDlg.exec();
    if(ret == QDialog::Accepted){
        QString ip   = hkDlg.getIPAddress();
        QString user = hkDlg.getUserName();
        QString pwd  = hkDlg.getPassword();
        qDebug()<< "IP:"<< ip <<",UserName:"<< user <<",Password:"<< pwd;                                           //连接海康HKNVR相机
        if (!m_hkDriver->syncdoLogin(ip, 8000, user, pwd)) {
            qCritical()<<"Login to hk device failed!"<<m_hkDriver->getLastError();
        } else {
            CAMERA_TYPE = 0;
            KEYCOUNT = 1;
            Camera_Format = 1;
            memoryInitialise();
            hkAction->setEnabled(false);
            baslerAction->setEnabled(false);
            doubleAction->setEnabled(false);
            m_hkDriver->startPlay(1);
            m_refreshImageTimer->start(40);
        }
    }
}

void MainWindow::on_baslerAction_triggered()
{
    //Basler单枪相机登录,连接单枪baslerCCD相机
    BaslerLoginDlg baslerDlg;
    int ret = baslerDlg.exec();
    if(ret == QDialog::Accepted){
        int cnt = baslerDlg.getBaslerKey();
        CAMERA_TYPE = 1;
        QStringList strList = baslerDlg.getExposureTime();
        KEYCOUNT = qMin(cnt, strList.size());
        qDebug()<<"KEYCOUNT IS: "<<KEYCOUNT;
        for(int i = 0; i < KEYCOUNT; i++){
            QString temp = strList.at(i);
            EXPO_TIME[i] = temp.toInt();
        }
        Camera_Format = 2;
        memoryInitialise();
        OpenGigeCameraOfInitial();
        if(m_gigeCamera->startPlay()){
            hkAction->setEnabled(false);
            baslerAction->setEnabled(false);
            doubleAction->setEnabled(false);
            m_refreshImageTimer->start(40);
            qDebug()<<"Login to BaslerCCD Successful.";

            QList<SeqParam> testParams;
            m_gigeCamera->getMeasurementParams(testParams);
            QList<SeqParam> seqList;
            SeqParam p, temp;
            m_gigeCamera->getDisplayParam(p);
            temp = p;
            for (int i = 0; i < testParams.count(); i++) {
                p.frameCount   = 25;
                p.isDiaplay    = false;
                p.exposureTime = EXPO_TIME[i % KEYCOUNT];
//                p.exposureTime = testParams.at(i).exposureTime;
                qDebug()<<"exposureTime:"<<testParams.at(i).exposureTime;
                seqList.append(p);
            }
            m_gigeCamera->configMeasurementParam(seqList);
            temp.frameCount = 25;
            m_gigeCamera->configDisplayParam(temp);
            m_gigeCamera->setWorkModeSeq(true);
        }
    }
}

void MainWindow::OpenGigeCameraOfInitial()
{
    QStringList idList;
    AutoGigeCamera::initPylonSdk();
    AutoGigeCamera::getDeviceList(idList);
    m_gigeCamera->closeCamera();
    if (idList.isEmpty()) {
        return;
    }
    if (m_gigeCamera->openCamera(idList.at(0))) {
        qDebug()<<"open device successful";
        m_gigeCamera->setWorkModeSeq(true);
        m_gigeCamera->loadParamFromCamera();
        m_gigeCamera->loadCameraConfig(QStringLiteral(":/genLib/acA1300-30gc_21748070.pfs"));
    }
}

void MainWindow::on_parameterConfigAction_triggered()
{
    //配置相应的参数，包括温度最高值、最低值、步幅值以及稳定误差等。
    QList<float> tempList;
    QList<float> deviaList;
    ParameterConfigDlg parameterDlg;
    int ret = parameterDlg.exec();
    if(ret == QDialog::Accepted){
        actionOfProcess = parameterDlg.getActionOfTemp();
        tempList  = parameterDlg.getTempValue();
        deviaList = parameterDlg.getStableOfConfigParameter();
        lowTempValue  = tempList.at(0);
        highTempValue = tempList.at(1);
        stepTempValue = tempList.at(2);
        tempDeviation  = deviaList.at(0);
        lightDeviation = deviaList.at(1);
        stableWaitTime = deviaList.at(2);
        tempList.clear();
        deviaList.clear();
        qDebug()<<"Action OF Standardize:              "<<actionOfProcess;
        qDebug()<<"Low Temperature Value OF Setting:   "<<lowTempValue;
        qDebug()<<"High Temperature Value OF Setting:  "<<highTempValue;
        qDebug()<<"Step Temperature Value OF Setting:  "<<stepTempValue;
        qDebug()<<"Setting Temperature Deviation:      "<<tempDeviation;
        qDebug()<<"Setting Lightness Deviation:        "<<lightDeviation;
        qDebug()<<"Setting the Waitting Time OF Stable:"<<stableWaitTime;
        if((lowTempValue >= 400) && (lowTempValue <= 1600)){
            if((highTempValue >= 400) && (highTempValue <= 1600)){
                if(lowTempValue <= highTempValue){
                    openSerialPortButton->setEnabled(true);
                    qDebug()<<"SerialPort can be Open Later.";
                }
            }
        }
    }
}

void MainWindow::on_tableShowAction_triggered()
{
    ShowTableDialog tableDialog;
    if(m_stableTemperature.isEmpty()){
        tableDialog.exec();
        return;
    }else if(m_stableTemperature[0].isEmpty()){
        tableDialog.exec();
        return;
    }

    QStringList str;
    if(CAMERA_TYPE == 0){
        str<<QStringLiteral("温度值")<<QStringLiteral("亮度值");
    }else if(CAMERA_TYPE == 1){
        str<<QStringLiteral("温度值");
        for(int i = 0; i < KEYCOUNT; i++){
            str<<QStringLiteral("Mono8:") + QString("%1").arg(EXPO_TIME[i]);
        }
    }else if(CAMERA_TYPE == 2){
        str<<QStringLiteral("温度值");
        for(int j = 0; j < KEYCOUNT; j++){
            str<<QString("%1").arg(EXPO_TIME[j]);
        }
    }
    qDebug()<<str;

    int col = 0;
    col = str.size();
    int row = 0;
    row = m_stableTemperature[0].size() + 1;
    tableDialog.setTableColumnCount(col);
    tableDialog.setTableRowCount(row);
    tableDialog.tableHeaderItemSetting(str);
    for(int i = 1; i < tableDialog.getTableColumnCount(); i++){
        for(int j = 1; j < tableDialog.getTableRowCount(); j++){
            QString s  = QString("%1").arg(m_stableTemperature[0].at(j - 1));
            tableDialog.tableCellContentSetting(j, 0, s);
            QString st = QString("%1").arg(m_stableLight[i - 1].at(j - 1));
            tableDialog.tableCellContentSetting(j, i, st);
        }
    }
    int ret = tableDialog.exec();
    if(!ret){
        qDebug()<<"Show table successful.";
    }else{
        qDebug()<<"Show table failure";
    }
}

void MainWindow::on_openSerialPortButton_clicked()
{
    /*************打开串口********************/
    if(!m_Com) {
        m_Com->deleteLater();
        m_Com = NULL;
    }
    m_Com->setPortName(portNameComboBox->currentText());
    qDebug()<<tr("PortName:")<<m_Com->portName();
    m_Com->setBaudRate(9600);
    qDebug()<<tr("BaudRate:")<<QString("%1").arg(m_Com->baudRate());
    if(m_Com->open()) {
        qDebug()<< tr("Open SerialPort Successful!");
        m_Com->readData();
        m_runOperationTimer->start(1000);
    } else {
        qDebug()<< tr("Open SerialPort Failure!");
    }
}

void MainWindow::memoryInitialise()
{
    //初始化
    QList<float>  float_list[100];
    for(int i = 0; i < KEYCOUNT; i++){
        float_list[i].append(i);
        float_list[i].clear();
        m_realLight.append(float_list[i]);
        m_realTemperature.append(float_list[i]);
        m_stableLight.append(float_list[i]);
        m_stableTemperature.append(float_list[i]);
        colorList.append(qRgb(rand()%255, rand()%255, rand()%255));
    }
    qDebug()<<"Memory Initialise.";
}

void MainWindow::Standardize_Operation()
{
    m_counter++;
    switch (m_StateMachine)
    {
    case 0:
        SetPoint_Operation();
        qDebug()<<"Send Request Of Setting SetPoint.";
        break;
    case 1:
        if(m_counter % 6 == 0){
            ReachSetpoint_Operation();
            qDebug()<<"Wait Until A Reaching SetPoint State.";
        }
        break;
    case 2:
        AnalyzingStable_Operation();
        qDebug()<<"Data Recorded In The Stable Time.";
        break;
    case 3:
        planckfitTest();
        m_StateMachine = 0;
        m_counter = 0;
        m_runOperationTimer->stop();
        break;
    default:
        break;
    }
    qDebug()<<"My State-Machine:"<<m_StateMachine;

    if (m_conditionCnt++ > timeInterval) {
        realTimeBrightRead();
        m_condition++;
        if(m_condition == KEYCOUNT){
            m_condition = 0;
        }
        m_conditionCnt = 0;
    }
    m_Com->readData();
    float m = m_Com->receiveMsg();
    revValueLineEdit->setText(QString::number(m, 'f', 2));
    qDebug()<<"Reading the real-time data every three seconds.";
}

void MainWindow::SetPoint_Operation()
{
    static int i = 0;
    switch (actionOfProcess)
    {
    case 0:
        setPoint = lowTempValue + i * stepTempValue;
        if(setPoint <= highTempValue){
            m_Com->writeData(setPoint);
            m_StateMachine = 1;
            setValueLineEdit->setText(QString::number(setPoint,'f', 2));
            qDebug()<<QString("Send setpoint(%1) in the heating process.").arg(setPoint);
        }else{
            i = 0;
            m_StateMachine = 3;
            qDebug()<<"End the heating progress";
        }
        break;
    case 1:
        setPoint = highTempValue - i * stepTempValue;
        if(setPoint >= lowTempValue){
            m_Com->writeData(setPoint);
            m_StateMachine = 1;
            setValueLineEdit->setText(QString::number(setPoint,'f', 2));
            qDebug()<<QString("Send setpoint(%1) in the cooling process.").arg(setPoint);
        }else{
            i = 0;
            m_StateMachine = 3;
            qDebug()<<"End the cooling progress";
        }
        break;
    default:
        break;
    }
    i++;
}

void MainWindow::ReachSetpoint_Operation()
{
    switch (actionOfProcess)
    {
    case 0:
        if(revValueLineEdit->text().toFloat() >= setPoint){
            m_StateMachine = 2;
            m_counter = 0;
            qDebug()<<QString("Reach setPoint(%1)in the heating process.").arg(setPoint);
        }
        break;
    case 1:
        if(revValueLineEdit->text().toFloat() <= setPoint){
            m_StateMachine = 2;
            m_counter = 0;
            qDebug()<<QString("Reach setPoint(%1)in the cooling process.").arg(setPoint);
        }
        break;
    default:
        break;
    }
}

void MainWindow::AnalyzingStable_Operation()
{
    analyzingStableList.append(revValueLineEdit->text().toFloat());
    if((m_StateMachine == 2) && isStable()){
        if(m_counter >= (60 * stableWaitTime)){
            stableBrightAndTempRead();
        }
    } else {
        m_counter = 0;
    }
}

bool MainWindow::isStable()
{
    int a = analyzingStableList.size();
    if( a >= 2 ){
        float p = analyzingStableList.first();
        float q = analyzingStableList.last();
        if(qAbs(p - q) < tempDeviation){
            return true;
        } else {
            analyzingStableList.clear();
            analyzingStableList.append(q);
            return false;
        }
    } else {
        return false;
    }
}

void MainWindow::stableBrightAndTempRead()
{
    //稳定时读取亮度与温度值
    static int num = 0;
    QString str1 = revValueLineEdit->text();
    QString str2 = currentBrightnessLineEdit->text();
    int index = 0;
    convertIndex(index, m_index);

    if(num == index){
        m_stableTemperature[index].append(str1.toFloat());
        stableTemperatureLineEdit->setText(str1);
        if(str2.isEmpty()){
            float g = m_realLight[index].last();
            m_stableLight[index].append(g);
            stableBrightnessLineEdit->setText(QString::number(g));
        }else{
            m_stableLight[index].append(str2.toFloat());
            stableBrightnessLineEdit->setText(str2);
        }
        num++;
        if(num >= m_stableTemperature.size()){
            num = 0;
            m_counter = 0;
            m_StateMachine = 0;
            qDebug()<<"StableLight:"<<m_stableLight;
            qDebug()<<"StableTemp :"<<m_stableTemperature;
            qDebug()<<"Stable state's temp-values:"<<analyzingStableList;
            analyzingStableList.clear();
        }
    }
}

void MainWindow::convertIndex(int &targetIndex, int &sourIndex)
{
    if(CAMERA_TYPE == 0){
        if(sourIndex == 0){
            targetIndex = 0;
        }
    }else if(CAMERA_TYPE == 1){
        targetIndex = sourIndex;
//        if(sourIndex == 1){
//            targetIndex = 0;
//        }else if(sourIndex == 3){
//            targetIndex = 1;
//        }else if(sourIndex == 5){
//            targetIndex = 2;
//        }
    }else if(CAMERA_TYPE == 2){
        targetIndex = sourIndex;
    }
}

void MainWindow::realTimeBrightRead()
{
    //实时读取对应的亮度与温度值
    static int num = 0;
    float c = currentBrightnessLineEdit->text().toFloat();
    float f = revValueLineEdit->text().toFloat();
    int index = 0;
    convertIndex(index, m_index);

    if(num == index) {
        if(f != 0) {
            m_realTemperature[index].append(f);
            m_realLight[index].append(c);
        }
//        if(m_realLight[index].isEmpty()) {
//            m_realLight[index].append(c);
//        } else if(qAbs(c - m_realLight[index].last()) > 5) {
//            m_realLight[index].append(m_realLight[index].last());
//        } else {
//            m_realLight[index].append(c);
//        }
        num++;
        if(num == m_realLight.size()) {
            num = 0;
            m_timeOfProgress.append(QTime::currentTime());
        }
    }
}

void MainWindow::on_closeSerialPortButton_clicked()
{
    //Close the serialport, and stop refreshing the plot
    m_refreshTimePlotTimer->stop();
    if(m_Com->isOpen()){
        m_Com->close();
        qDebug() << "Close SerialPort Successful!";
    } else {
        qDebug() << "The serialport has been off.";
    }
}

void MainWindow::on_relationAction_triggered()
{
    //数据关系变化曲线图
    DataRelationsDialog relationDlg;
    QVector<double> yData;
    QVector<double> xData;

    for(int i = 0; i < m_stableLight.size(); i++){
        for(int j = 0; j < m_stableLight[i].size(); j++){
            xData.append(m_stableLight[i].at(j));
            yData.append(m_stableTemperature[i].at(j));
        }
        if(0 == CAMERA_TYPE){
            relationDlg.Sample_CurveSettingPlot(tr("Sampling Datas"), xData, yData);
        }else if(1 == CAMERA_TYPE){
            QString ss = QString("Mono8-%1").arg(EXPO_TIME[i]);
            relationDlg.Sample_CurveSettingPlot(ss,xData,yData);
        }else if(2 == CAMERA_TYPE){
            QString stt = QString("%1").arg(EXPO_TIME[i]);
            relationDlg.Sample_CurveSettingPlot(stt, xData,yData);
        }
        xData.clear();
        yData.clear();
    }
    for(int i = 0; i < m_realLight.size(); i++){
        for(int j = 0; j < m_realLight[i].size(); j++){
            xData.append(m_realLight[i].at(j));
            yData.append(m_realTemperature[i].at(j));
        }
        if(0 == CAMERA_TYPE){
            relationDlg.All_CurveSettingPlot("All Datas", xData, yData);
        }else if(1 == CAMERA_TYPE){
            QString st = QString("Mono8-%1").arg(EXPO_TIME[i]);
            relationDlg.All_CurveSettingPlot(st,xData,yData);
        }else if(2 == CAMERA_TYPE){
            QString str = QString("%1").arg(EXPO_TIME[i]);
            relationDlg.All_CurveSettingPlot(str,xData,yData);
        }
        xData.clear();
        yData.clear();
    }
    int ret = relationDlg.exec();
    if(ret = QDialog::Accepted) {
        qDebug() << "The relationship curve of data is displayed.";
    } else if(ret == QDialog::Rejected) {
        qDebug() << "Cannot display the relationship curve.";
        return;
    }
}

void MainWindow::on_selectAreaButton_clicked()
{
    m_isDrawing = true;
}

void MainWindow::on_quitAction_triggered()
{
    m_refreshImageTimer->stop();
    //Close the SerialPort
    if(m_Com->isOpen()){
        m_Com->close();
    }
    //Close the GigeCamera
    if(m_gigeCamera){
        m_gigeCamera->setWorkModeSeq(false);
        m_gigeCamera->stopPlay();
        m_gigeCamera->saveCameraConfig(QStringLiteral(":/genLib/acA1300-30gc_21748070.pfs"));
//        m_gigeCamera->releasePylonSdk();
    }
    //Close the JaiCamera
    if(m_jaiCamera){
        m_jaiCamera->closeCamera();
    }
    //Close the scan5
    m_scan5SDK.stopSend();
    m_scan5SDK.close();
    this->close();
}

void MainWindow::on_planckParameter_Validation_Action_triggered()
{
    //Gets the planck parameter to be validated
    PlanckParameterDialog planckParameterDlg;
    int ret = planckParameterDlg.exec();
    if(ret == QDialog::Accepted){
        m_planckParameterSetList = planckParameterDlg.getPlanckParameterOfValidation();
        foreach (float para, m_planckParameterSetList) {
            qDebug()<<para;
        }
    }
}

void MainWindow::planckfitTest()
{
    //=============================================//
    //Points to fit the situation
    //=============================================//
    if(CAMERA_TYPE == 2) {
        Fit_Planck_Result();
    } else {
        Fit_Poly_Result();
    }
}

void MainWindow::Fit_Planck_Result()
{
    //=============================================//
    //The result of planck fitting
    //=============================================//
    int size = m_stableLight[0].size();
    QList<double> xt;
    QList<double> yb;
    for(int i = 0; i < size; i++){
        xt.append(m_stableLight[0].at(i));
        yb.append(m_stableTemperature[0].at(i));
    }
    double out_val;
    PointVal* points = new PointVal[size];

    double x0[3];
    x0[0] = 130000;
    x0[1] = 9000;
    x0[2] = -13;
    double x[3];

    for (int i = 0; i < size; i++) {
        points[i].x = xt.at(i);
        points[i].y = yb.at(i);
    }
    bool flag = findParam(x0, points, size, x, &out_val);
    qDebug() << "flag is "<<flag;
    qDebug() << QString("Fit result:%1, %2, %3").arg(x[0]).arg(x[1]).arg(x[2]);
    qDebug() << QString("Fit residual:%1").arg(out_val);

    //The fitting results are shown.
    realflagPlanckLineEdit->setText(QString("%1").arg(flag));
    realaPlanckLineEdit->setText(QString::number(x[0]));
    realbPlanckLineEdit->setText(QString::number(x[1]));
    realcPlanckLineEdit->setText(QString::number(x[2]));
    realDeviationPlanckLineEdit->setText(QString("%1").arg(out_val));
    if(points != NULL){
        delete points;
    }
}

void MainWindow::Fit_Poly_Result()
{
    //===============================================//
    //The result of polynomial fitting
    //===============================================//
    int size = m_stableLight[0].size();
    QList<double> xt;
    QList<double> yb;
    for(int i = 0; i < size; i++){
        xt.append(m_stableLight[0].at(i));
        yb.append(m_stableTemperature[0].at(i));
    }
    PointVal* points = new PointVal[size];
    double x[3];

    for (int i = 0; i < size; i++) {
        points[i].x = xt.at(i);
        points[i].y = yb.at(i);
    }
    bool flag = findParam(points, size, x);
    qDebug()<<"flag is "<<flag;
    qDebug()<<QString("Fit result:%1, %2, %3").arg(x[0]).arg(x[1]).arg(x[2]);

    realflagPlanckLineEdit->setText(QString("%1").arg(flag));
    realaPlanckLineEdit->setText(QString::number(x[0]));
    realbPlanckLineEdit->setText(QString::number(x[1]));
    realcPlanckLineEdit->setText(QString::number(x[2]));
    if(points != NULL){
        delete[] points;
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    //=======================================//
    //Close the main window event
    //=======================================//
    if(this->close()){
        on_quitAction_triggered();
        on_closeSerialPortButton_clicked();
        event->accept();
        qDebug()<<"Accept the event of closing the mainwindow.";
    }else{
        event->ignore();
        qDebug()<<"Ignore the event of closing the mainwindow.";
    }
    QMainWindow::closeEvent(event);
}

void MainWindow::on_doubleAction_triggered()
{
    //=========================================//
    //  DoubleCCD Login
    //=========================================//
    DoubleCCDDialog doubleDlg;

    QStringList timeList;
    QStringList idList;
    //Gets the available device types
    QStringList nameList = JaiGigeCamera::getDeviceList(idList);
    doubleDlg.jai_deviceType_clear();
    timeList = doubleDlg.jai_exposureTime();
    qDebug()<<"List of DoubleCCD's Exposure Time:"<<timeList;

    //Remove duplicates
    for(int i = 0; i < nameList.size(); i++) {
        for(int j = i + 1; j < nameList.size(); j++) {
            if(nameList.at(j) == nameList.at(i)) {
                nameList.removeAt(j);
            } else {
                continue;
            }
        }
    }
    //Initialize the login device list
    int index = 0;
    foreach (QString name, nameList) {
        QListWidgetItem *item = new QListWidgetItem(name);
        item->setData(Qt::UserRole, idList.at(index++));
        doubleDlg.jai_deviceType_add(item);
        qDebug()<<QString("Device Type %1 : ").arg(index) + item->text();
    }
    if(!nameList.isEmpty()) {
        doubleDlg.setCurrentRow(0);
    }

    int ret = doubleDlg.exec();
    QString caemera = doubleDlg.jai_deviceType_choose();
    if ((ret == QDialog::Accepted) && (!caemera.isEmpty())){
        qDebug()<<"Current Device Type---"<<caemera;
        KEYCOUNT      = timeList.size();
        Camera_Format = 2;
        CAMERA_TYPE   = 2;
        qDebug()<<"KEYCOUNT: "<<KEYCOUNT;
        qDebug()<<"Camera_Format: "<<Camera_Format;
        qDebug()<<"CAMERA_TYPE: "<<CAMERA_TYPE;
        memoryInitialise();
        for(int i = 0; i < KEYCOUNT; i++){
            QString temp = timeList.at(i);
            EXPO_TIME[i] = temp.toInt();
            qDebug()<<EXPO_TIME[i];
        }
        int pixelFormat = doubleDlg.get_image_pixelFormat();
        m_jaiCamera->closeCamera();
        if (m_jaiCamera->openCamera(caemera)) {
            switch (pixelFormat) {
            case 0:
                m_jaiCamera->setPixelFormat("Mono8");
                break;
            case 1:
                m_jaiCamera->setPixelFormat("Mono10");
                break;
            case 2:
                m_jaiCamera->setPixelFormat("Mono12");
                break;
            case 3:
                m_jaiCamera->setPixelFormat("RGB8Packed");
                break;
            default:
                break;
            }
            if (m_jaiCamera->startPlayStream(0, true)) {
                m_refreshImageTimer->start(40);
                hkAction->setEnabled(false);
                baslerAction->setEnabled(false);
                qDebug()<<"Login DoubleCCD Successful.";
            }
            if (KEYCOUNT > 0) {
                m_jaiCamera->setExposureTime(EXPO_TIME[0]);
            }
        }
    } else {
        qDebug()<<"Login DoubleCCD Failure.";
    }
}

void MainWindow::on_actionScan5_triggered()
{
    Scan5Dialog scan5Dlg;
    int ret = scan5Dlg.exec();
    if(ret == QDialog::Accepted) {
        m_scan5SDK.setDataHandCallBack(&dataArrived, this);
        if(m_scan5SDK.connectTo(scan5Dlg.getScan5Ip())) {
            m_scan5SDK.startSend();
            scan5ElapsedTimer.start();
            m_scan5Plot->setColorMap(scan5Dlg.getScan5Riband());
            m_scan5Plot->setAlpha(50);
            m_scan5Plot->showContour(scan5Dlg.isScan5ContourLine());
            KEYCOUNT      = 1;
            Camera_Format = 1;
            CAMERA_TYPE   = 3;
            memoryInitialise();
            hkAction->setEnabled(false);
            baslerAction->setEnabled(false);
            doubleAction->setEnabled(false);
            m_refreshImageTimer->start(40);
            qDebug()<<"Scan5 login successful.";
        } else {
            qDebug()<<"Connecting to scan5 finds error.";
            qDebug()<<"Please login scan5 again.";
        }
    } else {
        qDebug()<<"Scan5 Login failure.";
    }
}

void MainWindow::dataArrived(uint16_t *data, int count, uint16_t rotatSpeed, uint16_t temp, void *user)
{
    MainWindow* w = (MainWindow*)user;
    if (w) {
        w->handleData(data, count, rotatSpeed, temp);
    }
}

void test(Plot* plot)
{
    QVector<double> d;
    for (float y = -3; y < 3; y += 0.1f) {
        for (float x = -3; x < 3; x += 0.1f) {

            const double c = 0.842;
            //            const double c = 0.3;

            const double v1 = x * x + ( y - c ) * ( y + c );
            const double v2 = x * ( y + c ) + x * ( y + c );

            d<< 10.0 / ( v1 * v1 + v2 * v2 ) ;
        }
    }
    plot->setMetrix(d, 61);
}

void MainWindow::handleData(uint16_t *data, int count, uint16_t rotatSpeed, uint16_t temp)
{
    fpsCount++;
    int ms = scan5ElapsedTimer.elapsed();
    if (ms > 1000) {
        qDebug()<<"fps: "<<fpsCount*1000.0/ms<<" speed:"<<(void*)rotatSpeed<<" temp:"<<(void*)temp;
        fpsCount = 0;
        scan5ElapsedTimer.restart();
    }
    for (int i = 0; i < count; i++) {
        plotData.append((data[i])* 100.0 /0x3ff );
    }
    if (plotData.count() > (4 * count)) {
        m_scan5Plot->setMetrix(plotData, count);
        plotData.clear();
        plotData.reserve(5 * count);
    }
    // 以下部分为性能效果测试部分
//    test(plot);
}
