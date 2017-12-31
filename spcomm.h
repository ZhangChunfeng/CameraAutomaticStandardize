#ifndef SPCOMM_H
#define SPCOMM_H

#include <QObject>
#include <QSerialPort>
#include <QElapsedTimer>

class SPComm : public QObject
{
    Q_OBJECT

public:
    explicit SPComm(QObject *parent = 0);
    ~SPComm();
    bool isOpen()const;
    void setPortName(const QString &name);
    QString portName() const;
    void setBaudRate(int baudRate);
    int baudRate() const;
    virtual bool open();
    virtual void close();
    virtual bool clear();
    void readData();
    void writeData(float sendValue);
    float receiveMsg();

protected:
    QString        m_portName;
    int            m_baudRate;
    float          readValue;
    QSerialPort*   m_serialPort;
    QByteArray     m_readBuff;
    QElapsedTimer  m_validTime;

private:
    QString getDisplayString(QByteArray hexByteArray);
    QByteArray clearStart(QByteArray &array);

signals:

public slots:
    void readMyCom(void);
    void dataArrived();
};

#endif // SPCOMM_H
