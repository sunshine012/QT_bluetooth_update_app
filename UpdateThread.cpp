#include "UpdateThread.h"
#include <QDebug>

UpdateThreadObj::UpdateThreadObj(QObject *parent):QObject(parent)
{
    utimer = new QTimer(this);
    utimer->setSingleShot(true);
}

UpdateThreadObj::~UpdateThreadObj()
{
    if(utimer->isActive())
        killTimer(utimer->timerId());
}

void UpdateThreadObj::update_run()
{
    QByteArray byr;
    QString str;
    int cnt;
    char checksum_self, checksum_received, checksum_toolcal;
    bool fileError = true;
    QString Frmfilefullname, FrmOffset, FrmStAddr, FrmEndAddr;
    QString EEPROMfilefullname, EEPROMOffset, EEPROMStAddr, EEPROMEndAddr;
    QString tempLine, TextOneLine, TextFourLine, WritingAddr;

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
                if(FrmEndAddr.length() == 4)
                    FrmEndAddr.insert(0, "00");
                else if(FrmEndAddr.length() == 5)
                    FrmEndAddr.insert(0, '0');
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

    QFile Firmwarefile(Frmfilefullname);
    Firmwarefile.open(QIODevice::ReadOnly | QIODevice::Text);
    QTextStream textStream(&Firmwarefile);

    while(1)
    {
        if(currentState == UPDATEFINISHED)
            return;

        switch(currentState)
        {
            case CMD_VS:
                str = "{VS}";
                emit sendSerialData(str.toLatin1());
                emit changeWidgeStatus(WG_EDIT_SENDDATA_BUFFER, str);
                setState(CMD_VS_WAIT);
                utimer->start(100);
                break;

            case CMD_VS_WAIT:
                if(utimer->remainingTime())
                    break;
                else
                {
                    setState(CMD_BOOT);
                    receiveBuffer.clear();
                }
                break;

            case CMD_BOOT:
                str = "{BOOT}";
                emit sendSerialData(str.toLatin1());
                emit changeWidgeStatus(WG_EDIT_SENDDATA_BUFFER, str);
                setState(CMD_BOOT_WAIT);
                utimer->start(100);
                break;

            case CMD_BOOT_WAIT:
                if(utimer->remainingTime())
                    break;
                else
                {
                    setState(GET_VERSION);
                    receiveBuffer.clear();
                }
                break;

            case GET_VERSION:
                str = "05";
                StringToHex(str, byr);
                emit sendSerialData(byr);
                emit changeWidgeStatus(WG_EDIT_SENDDATA_BUFFER, str);
                setState(GET_VERSION_WAIT);
                utimer->start(100);
                break;

            case GET_VERSION_WAIT:
                if(receiveBuffer.contains("192-160002-E"))
                {
                    setState(CMD_SENDADDR);
                    emit changeWidgeStatus(WG_EDIT_RECEIVEDATA_BUFFER, QString(receiveBuffer));
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

            case CMD_SENDADDR:
                str = "03";
                StringToHex(str, byr);
                emit sendSerialData(byr);
                emit changeWidgeStatus(WG_EDIT_SENDDATA_BUFFER, str);
                setState(CMD_SENDADDR_WAIT);
                utimer->start(100);
                break;

            case CMD_SENDADDR_WAIT:
                if(receiveBuffer.contains(byr))
                {
                    setState(SEND_ADDR);
                    emit changeWidgeStatus(WG_EDIT_RECEIVEDATA_BUFFER, QString(receiveBuffer));
                    receiveBuffer.clear();

                    if(FrmStAddr.length() == 4)
                        FrmStAddr.insert(0,"00");
                    else if(FrmStAddr.length() == 5)
                        FrmStAddr.insert(0,'0');

                    cnt = 0;
                }
                else if(utimer->remainingTime())
                    break;
                else
                {
                    qDebug()<<"currentState:"<<currentState;
                    setState(UPDATEFINISHED);
                }
                break;

            case SEND_ADDR:
                if(cnt == 0)
                    str = FrmStAddr.mid(0, 2);
                else if(cnt == 1)
                    str = FrmStAddr.mid(2, 2);
                else
                    str = FrmStAddr.mid(4, 2);
                StringToHex(str, byr);
                emit sendSerialData(byr);
                emit changeWidgeStatus(WG_EDIT_SENDDATA_BUFFER, str);
                setState(SEND_ADDR_WAIT);
                utimer->start(100);
                break;

            case SEND_ADDR_WAIT:
                if(receiveBuffer.contains(byr))
                {
                    cnt++;
                    receiveBuffer.clear();

                    if(cnt < 0x03)
                        setState(SEND_ADDR);
                    else
                    {
                        setState(CMD_SENDDATA);
                        TextOneLine = textStream.readLine(); // skip the first line
                        emit changeWidgeStatus(WG_EDIT_UPDATE_AREA, "Firmware");
                    }
                }
                else if(utimer->remainingTime())
                    break;
                else
                {
                    qDebug()<<"currentState:"<<currentState;
                    setState(UPDATEFINISHED);
                }
                break;

            case CMD_SENDDATA:
                str = "02";
                StringToHex(str, byr);
                emit sendSerialData(byr);
                emit changeWidgeStatus(WG_EDIT_SENDDATA_BUFFER, str);
                setState(CMD_SENDDATA_WAIT);
                utimer->start(100);
                break;

            case CMD_SENDDATA_WAIT:
                if(receiveBuffer.contains(byr))
                {
                    setState(SENDING_FIRMWARE_DATA);
                    emit changeWidgeStatus(WG_EDIT_RECEIVEDATA_BUFFER, QString(receiveBuffer));
                    receiveBuffer.clear();
                    TextFourLine.clear();
                    TextOneLine.clear();
                    cnt = 0;
                }
                else if(utimer->remainingTime())
                    break;
                else
                {
                    qDebug()<<"currentState:"<<currentState;
                    setState(UPDATEFINISHED);
                }
                break;

            case SENDING_FIRMWARE_DATA:
                while(cnt < 4)
                {
                    tempLine = textStream.readLine();
                    WritingAddr = tempLine.mid(4, 6);
                    TextOneLine = tempLine.mid(10, 32);
                    TextFourLine += TextOneLine;
                    TextOneLine.clear();
                    cnt++;
                }
                StringToHex(TextFourLine, byr);

                checksum_self = 0;
                for(cnt = 0; cnt < P18F87_FLASH_ROW_SIZE; cnt++)
                    checksum_self += byr[cnt];
                byr.resize(P18F87_FLASH_ROW_SIZE + 1);
                byr[P18F87_FLASH_ROW_SIZE] = checksum_self;

                emit sendSerialData(byr);
                emit changeWidgeStatus(WG_EDIT_UPDATE_ADDR, WritingAddr);
                setState(SENDING_FIRMWARE_DATA_WAIT);
                utimer->start(2000);
                break;

            case SENDING_FIRMWARE_DATA_WAIT:
                if(receiveBuffer.length() == 0x02)
                {
                    checksum_received = receiveBuffer[0];
                    checksum_toolcal  = receiveBuffer[1];
                    if(checksum_self == checksum_received &&
                       checksum_self == checksum_toolcal)
                    {
                        if(checkUpdateFinish(FrmEndAddr, WritingAddr))
                        {
                            str = "04";
                            StringToHex(str, byr);
                            emit sendSerialData(byr);
                            emit changeWidgeStatus(WG_EDIT_SENDDATA_BUFFER, str);
                            setState(UPDATEFINISHED);
                        }
                        else
                            setState(CMD_SENDDATA);
                    }
                    else
                    {
                        setState(UPDATEFINISHED);
                        qDebug()<<"Write address"<<WritingAddr;
                    }
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

void UpdateThreadObj::stop()
{
    setState(UPDATEFINISHED);
}

void UpdateThreadObj::StringToHex(QString str, QByteArray& senddata)  //字符串转换成16进制数据0-F
{
    int hexdata,lowhexdata;
    int hexdatalen = 0;
    int len = str.length();
    senddata.resize(len/2);
    char lstr,hstr;
    for(int i=0; i<len; )
    {
        //char lstr,
        hstr=str[i].toLatin1();
        if(hstr == ' ')
        {
            i++;
            continue;
        }
        i++;
        if(i >= len)
            break;
        lstr = str[i].toLatin1();
        hexdata = ConvertHexChar(hstr);
        lowhexdata = ConvertHexChar(lstr);
        if((hexdata == 16) || (lowhexdata == 16))
            break;
        else
            hexdata = hexdata*16+lowhexdata;
        i++;
        senddata[hexdatalen] = (char)hexdata;
        hexdatalen++;
    }
    senddata.resize(hexdatalen);
}

char UpdateThreadObj::ConvertHexChar(char ch)
{
    if((ch >= '0') && (ch <= '9'))
            return ch-0x30;
        else if((ch >= 'A') && (ch <= 'F'))
            return ch-'A'+10;
        else if((ch >= 'a') && (ch <= 'f'))
            return ch-'a'+10;
        else return ch-ch;//不在0-f范围内的会发送成0
}

int UpdateThreadObj::bytesToInt(QByteArray bytes)
{
    int result;
    result  = bytes[2] & 0x0000FF;
    result |= (bytes[1] << 8) & 0x00FF00;
    result |= (bytes[0] << 16) & 0xFF0000;
    return result;
}

bool UpdateThreadObj::checkUpdateFinish(QString str1, QString str2)
{
    QByteArray byr1, byr2;
    int temp1, temp2;
    StringToHex(str1, byr1);
    StringToHex(str2, byr2);
    temp1 = bytesToInt(byr1);
    temp2 = bytesToInt(byr2);
    if(temp1 == (temp2 + 16))
        return true;
    else
        return false;
}






