#include "UpdateThread.h"
#include <QDebug>

UpdateThreadObj::UpdateThreadObj(QObject *parent):QObject(parent)
{
    utimer = new QTimer(this);
    utimer->setSingleShot(true);
}

UpdateThreadObj::~UpdateThreadObj()
{
    killTimer(utimer->timerId());
}

void UpdateThreadObj::update_run()
{
    QByteArray str;
    bool fileError = true;
    QString Frmfilefullname, FrmOffset, FrmStAddr, FrmEndAddr;
    QString EEPROMfilefullname, EEPROMOffset, EEPROMStAddr, EEPROMEndAddr;

    // 打开文件并读取文件内容
    qDebug()<<"m_objThread id:"<<QThread::currentThreadId();

    QFile file(IniFilename);
    if(file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QTextStream textStream(&file);
        while(!textStream.atEnd())
        {
            QString textline = textStream.readLine();
            if(textline.contains("[FIRMWARE]", Qt::CaseSensitive))
            {
                fileError = false;
                Frmfilefullname = textStream.readLine().section('=', 1, 1, QString::SectionDefault);
                FrmOffset       = textStream.readLine().section('=', 1, 1, QString::SectionDefault);
                FrmStAddr       = textStream.readLine().section("=0x", 1, 1, QString::SectionDefault);
                FrmEndAddr      = textStream.readLine().section("=0x", 1, 1, QString::SectionDefault);
            }
            else if(textline.contains("[EEPROM]", Qt::CaseSensitive))
            {
                EEPROMfilefullname = textStream.readLine().section('=', 1, 1, QString::SectionDefault);
                EEPROMOffset       = textStream.readLine().section("=0x", 1, 1, QString::SectionDefault);
                EEPROMStAddr       = textStream.readLine().section("=0x", 1, 1, QString::SectionDefault);
                EEPROMEndAddr      = textStream.readLine().section("=0x", 1, 1, QString::SectionDefault);
            }
        }
        if(fileError == true)
        {
            QMessageBox::information(NULL, NULL, "The ini file is wrong!");
        }
        else
        {
            QFile Firmwarefile(Frmfilefullname);
            if(Firmwarefile.open(QIODevice::ReadOnly | QIODevice::Text))
            {
                emit changeWidgeStatus(WG_EDIT_SENDDATA_BUFFER, "Firmware file open successfully!");
                emit changeWidgeStatus(WG_BUTTON_START_UPDATE, "DISABLE");
                emit changeWidgeStatus(WG_BUTTON_STOP_UPDATE, "ENABLE");

                currentState = CMD_VS;
                receiveBuffer.clear();
                QTextStream FrmtextStream(&Firmwarefile);
            }
            else
            {
                emit changeWidgeStatus(WG_EDIT_SENDDATA_BUFFER, "Firmware file open failed!");
                currentState = UPDATEFINISHED;
            }
        }
    }
    else
    {
        emit changeWidgeStatus(WG_EDIT_SENDDATA_BUFFER, "The ini file open error");
        currentState = UPDATEFINISHED;
    }

    while(currentState != UPDATEFINISHED)
    {
        //QThread::msleep(5);
        switch(currentState)
        {
            case CMD_VS:
                str = "{VS}";
                emit sendSerialData(str);
                emit changeWidgeStatus(WG_EDIT_SENDDATA_BUFFER, QString(str));
                setState(CMD_VS_WAIT);
                utimer->start(10000);
                break;

            case CMD_VS_WAIT:
                if(receiveBuffer.contains("{VS}"))
                {
                    setState(CMD_BOOT);
                    emit changeWidgeStatus(WG_EDIT_RECEIVEDATA_BUFFER, QString(str));
                    receiveBuffer.clear();
                }
                else if(utimer->remainingTime())
                    break;
                else
                {
                    qDebug()<<"currentState:"<<currentState;
                    setState(UPDATEFINISHED);
                }
                break;

            case CMD_BOOT:
                str = "{BOOT}";
                emit sendSerialData(str);
                emit changeWidgeStatus(WG_EDIT_SENDDATA_BUFFER, QString(str));
                setState(CMD_BOOT_WAIT);
                utimer->start(100);
                break;

            case CMD_BOOT_WAIT:
                if(receiveBuffer.contains("{BOOT}"))
                {
                    setState(GET_VERSION);
                    emit changeWidgeStatus(WG_EDIT_RECEIVEDATA_BUFFER, QString(str));
                    receiveBuffer.clear();
                }
                else if(utimer->remainingTime())
                    break;
                else
                {
                    qDebug()<<"currentState:"<<currentState;
                    setState(UPDATEFINISHED);
                }
                break;

            case GET_VERSION:
                //sleep(1);
                //setState(UPDATEFINISHED);
                break;

            case GET_VERSION_WAIT:
                break;

            case CMD_SENDADDR:
                break;

            case CMD_SENDADDR_WAIT:
                break;

            case SEND_ADDR:
                break;

            case SEND_ADDR_WAIT:
                break;

            case CMD_SENDDATA:
                break;

            case CMD_SENDDATA_WAIT:
                break;

            case SENDING_FIRMWARE_DATA:
                break;

            default:
                break;
        }
    }
    return;
}

void UpdateThreadObj::setState(UpdateState i)
{
    mutex.lock();
    currentState = i;
    mutex.unlock();
}

void UpdateThreadObj::setInifile(QString filename)
{
    IniFilename = filename;
}

void UpdateThreadObj::receiveSerialData(const QByteArray &str)
{
    receiveBuffer += str;
    utimer->start(100);
}

void UpdateThreadObj::onthreadfinished()
{
    emit changeWidgeStatus(WG_EDIT_SENDDATA_BUFFER, "thread finished");
    emit changeWidgeStatus(WG_BUTTON_START_UPDATE, "ENABLE");
    emit changeWidgeStatus(WG_BUTTON_STOP_UPDATE,  "DISABLE");
}

void UpdateThreadObj::stop()
{
    emit changeWidgeStatus(WG_EDIT_SENDDATA_BUFFER, "STOP button pressed");
    setState(UPDATEFINISHED);
}







