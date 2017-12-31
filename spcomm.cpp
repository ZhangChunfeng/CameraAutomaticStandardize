#include <QDebug>
#include <QTimer>
#include <QDateTime>
#include "spcomm.h"

SPComm::SPComm(QObject *parent) : QObject(parent)
{
    //初始化
    readValue = 0;
    m_portName = "";
    m_baudRate = 9600;
    m_serialPort = new QSerialPort(this);

    connect(m_serialPort, &QSerialPort::readyRead, this, &SPComm::readMyCom);
}

SPComm::~SPComm()
{
    delete m_serialPort;
}

static QSerialPort::BaudRate getBaudRate(int baudRate)
{
    switch(baudRate)
    {
    case 1200:
        return QSerialPort::Baud1200;
    case 2400:
        return QSerialPort::Baud2400;
    case 4800:
        return QSerialPort::Baud4800;
    case 9600:
        return QSerialPort::Baud9600;
    case 19200:
        return QSerialPort::Baud19200;
    case 38400:
        return QSerialPort::Baud38400;
    case 57600:
        return QSerialPort::Baud57600;
    case 115200:
        return QSerialPort::Baud115200;
    default:
        return QSerialPort::Baud9600;
    }
}

void SPComm::setPortName(const QString &name)
{
    m_portName = name;
}

QString SPComm::portName() const
{
    return m_portName;
}

int SPComm::baudRate() const
{
    return m_baudRate;
}

void SPComm::setBaudRate(int baudRate)
{
    m_baudRate = baudRate;
}

bool SPComm::open()
{
    //用来打开串口，调用前，先设置串口名和波特率
    m_serialPort->setPortName(m_portName);
    if (!m_serialPort->setBaudRate(getBaudRate(m_baudRate))) {
        qWarning()<<"setBaudRate error: "<<m_serialPort->errorString();
        return false;
    }
    m_serialPort->setParity(QSerialPort::EvenParity);
    m_serialPort->setDataBits(QSerialPort::Data7);
    m_serialPort->setStopBits(QSerialPort::OneStop);
    m_serialPort->setFlowControl(QSerialPort::NoFlowControl);
    m_serialPort->setReadBufferSize(1024);
    bool isSuccess = m_serialPort->open(QSerialPort::ReadWrite);
    if(!isSuccess){
        qDebug()<<"Open SerialPort fail: "<<m_serialPort->errorString();
        return false;
    }
    return true;
}

bool SPComm::isOpen() const
{
    return m_serialPort->isOpen();
}

void SPComm::close()
{
    if(m_serialPort->isOpen()){
        m_serialPort->close();
    }
}

bool SPComm::clear()
{
    //清除数据，重启串口
    if (m_serialPort->isOpen()){
        m_serialPort->clear();
        this->close();
        return this->open();
    }
    return false;
}

void SPComm::readData()
{
    //用来接收串口发来的数据的命令
    QByteArray array;
    array.append(0x04);
    array.append('0');
    array.append('0');
    array.append('1');
    array.append('1');
    array.append('P');
    array.append('V');
    array.append(0x05);
    qDebug()<<"Request For Receiving Message:"<<array;
    m_serialPort->write(array);
}

float SPComm::receiveMsg()
{
    if(readValue != 0){
        qDebug()<<"Read Data From the Device:"<<readValue;
        return readValue;
    }
    return 0;
}

void SPComm::writeData(float sendValue)
{
    //发送数据到串口,比如发送协议
    if((sendValue < 400) || (sendValue > 1600)){
        qWarning()<<"Data for sending is error.";
        return ;
    }
    QByteArray array;
    array.append(0x04);
    array.append('0');
    array.append('0');
    array.append('1');
    array.append('1');
    array.append(0x02);
    QByteArray check2;
    check2.append('S');
    check2.append('L');
    check2.append(QString::number(sendValue, 'f', 2));
    check2.append(0x03);

    uchar sum = 0;
    int len = check2.length();
    for (int i = 0; i < len; i++) {
        sum = sum ^ check2.at(i);
    }
    check2.append(sum);
    array += check2;
    qDebug()<<"Send Data To the Device:"<<array.toHex();
    m_serialPort->write(array);
}


void SPComm::readMyCom(void)
{
    if (m_validTime.elapsed() > 500) {
        m_readBuff.clear();
    }
    if (m_readBuff.isEmpty()) {
        QTimer::singleShot(1000, this, &SPComm::dataArrived);
    }
    QByteArray temp = m_serialPort->readAll();
    QString s;
    for (int i = 0; i < temp.length(); i++) {
        s.append(temp.at(i));
    }
    qDebug()<<"Data arrived: "<<temp.toHex()<<" : "<<s;
    m_validTime.restart();
    m_readBuff.append(temp);
}

void SPComm::dataArrived()
{
    if (!m_readBuff.isEmpty()) {
        QString str = QDateTime::currentDateTime().toString("HH:mm:ss : ");
        str +=  getDisplayString(m_readBuff);
        if (m_readBuff.length() >= 8) {
            qDebug()<<m_readBuff;
            readValue = clearStart(m_readBuff).toFloat();
            qDebug()<<clearStart(m_readBuff);
        }
        qDebug()<<str;
    }
    m_readBuff.clear();
}

QString SPComm::getDisplayString(QByteArray hexByteArray)
{
    QString str;
    QString st;
    qDebug()<<hexByteArray.toHex();
    int len = hexByteArray.length();
    for(int i = 0; i < len; i++) {
        str += QString::number(hexByteArray.at(i), 16) + " ";
        st  += hexByteArray.at(i);
    }
    return str + " : " + st;
}

QByteArray SPComm::clearStart(QByteArray &array)
{
    //取出有效数据位
    if(array.isEmpty()){
        qDebug()<<"Empty?: "<<array.isEmpty();
        return false;
    }
    int pos = array.indexOf("PV") + 2;
    QByteArray arr =  array.mid(pos);
    int i = arr.indexOf("\x03");
    return arr.left(i);
}
