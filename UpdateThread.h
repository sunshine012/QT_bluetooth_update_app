#ifndef UPDATETHREAD_H
#define UPDATETHREAD_H
#include <QThread>
#include <QFile>
#include <QFileDialog>
#include <QMessageBox>
#include <QTimer>
#include <QTextStream>
#include <QMutex>
#include <QByteArray>
#include <QMetaType>

enum UpdateState
{
    CMD_VS,
    CMD_VS_WAIT,
    CMD_BOOT,
    CMD_BOOT_WAIT,
    GET_VERSION,
    GET_VERSION_WAIT,
    CMD_SENDADDR,
    CMD_SENDADDR_WAIT,
    SEND_ADDR,
    SEND_ADDR_WAIT,
    CMD_SENDDATA,
    CMD_SENDDATA_WAIT,
    SENDING_FIRMWARE_DATA,
    SENDING_FIRMWARE_DATA_WAIT,
    UPDATEFINISHED
};

enum WidgetID
{
    WG_EDIT_UPDATE_AREA = 0,
    WG_EDIT_UPDATE_ADDR,
    WG_EDIT_SENDDATA_BUFFER,
    WG_EDIT_RECEIVEDATA_BUFFER,
    WG_BUTTON_START_UPDATE,
    WG_BUTTON_STOP_UPDATE
};
Q_DECLARE_METATYPE(WidgetID);

class UpdateThreadObj : public QObject
{
    Q_OBJECT

public:
    UpdateThreadObj(QObject *parent = NULL);
    ~UpdateThreadObj();

    void setState(UpdateState i);
    void setInifile(QString filename);
    void stop();

private:
    void StringToHex(QString str, QByteArray& senddata);
    char ConvertHexChar(char ch);
    int  bytesToInt(QByteArray bytes);
    bool checkUpdateFinish(QString str1, QString str2);

signals:
    void sendSerialData(const QByteArray &);
    void changeWidgeStatus(const WidgetID wgnum, const QString &);

public slots:
    void update_run();

public:
     QByteArray receiveBuffer;
     UpdateState currentState;

private:
    QString IniFilename;
    QMutex mutex;
    QTimer *utimer;
};

#define P18F87_FLASH_ROW_SIZE           64             //erase bank size

#endif // THREAD_UPDATE_H
