#include "mainwindow.h"
#include "ui_mainwindow.h"

QString filefullname;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    // Search available serial ports
    foreach (const QSerialPortInfo &Info, QSerialPortInfo::availablePorts())
    {
        QSerialPort serialport;
        serialport.setPort(Info);
        ui->comboBox_CommPort->addItem(serialport.portName());
    }
    ui->comboBox_CommPort->setCurrentIndex(0);
    ui->comboBox_Baudrate->setCurrentIndex(5);
    ui->pushButton_SendData->setEnabled(false);
    ui->pushButton_startupdate->setEnabled(false);
    ui->pushButton_stopUpdate->setEnabled(false);

    m_ObjThread = new QThread;
    qDebug()<<"m_ObjThread created";
    m_Obj = new UpdateThreadObj;
    qDebug()<<"m_Obj created";
    m_Obj->moveToThread(m_ObjThread);
    qDebug()<<"m_Obj moved to m_ObjThread";
    qDebug()<<"current thread id:"<<QThread::currentThreadId();

    qRegisterMetaType<WidgetID>("WidgetID");

    // 连接信号与槽， 结束信号
    connect(m_ObjThread, &QThread::finished, m_ObjThread, &QThread::deleteLater);
    connect(m_ObjThread, &QThread::finished, m_Obj, &QObject::deleteLater);
    // 连接信号与槽， 发送串口数据， 接收串口数据
    connect(this, &MainWindow::trSerialData, m_Obj, &UpdateThreadObj::receiveSerialData);
    connect(m_Obj, &UpdateThreadObj::sendSerialData, this, &MainWindow::SendObjSerialData);
    // 连接信号与槽， 开始升级， 更新窗口显示数据
    connect(this, &MainWindow::startupdate, m_Obj, &UpdateThreadObj::update_run);
    connect(m_Obj, &UpdateThreadObj::changeWidgeStatus, this, &MainWindow::UpdateWidgeStatus);
}

MainWindow::~MainWindow()
{
    m_ObjThread->quit();
    m_ObjThread->wait();
    delete ui;
}

void MainWindow::on_pushButton_OpenCloseComm_clicked()
{
    if(ui->pushButton_OpenCloseComm->text() == tr("打开串口"))
    {
        serial = new QSerialPort;
        // 设置串口名
        serial->setPortName(ui->comboBox_CommPort->currentText());
        // 打开串口
        serial->open(QIODevice::ReadWrite);
        // 设置波特率
        serial->setBaudRate(ui->comboBox_Baudrate->currentText().toInt());
        // 设置数据位
        serial->setDataBits(QSerialPort::Data8);
        // 设置奇偶校验位
        serial->setParity(QSerialPort::NoParity);
        // 设置停止位
        serial->setStopBits(QSerialPort::OneStop);
        // 设置流控制
        serial->setFlowControl(QSerialPort::NoFlowControl);
        // 关闭菜单设置
        ui->comboBox_CommPort->setEnabled(false);
        ui->comboBox_Baudrate->setEnabled(false);
        ui->pushButton_OpenCloseComm->setText(tr("关闭串口"));
        ui->pushButton_SendData->setEnabled(true);

        if(filefullname.length() != 0)
        {
            ui->pushButton_startupdate->setEnabled(true);
            ui->pushButton_stopUpdate->setEnabled(false);
        }

        // 连接信号槽
        QObject::connect(serial, &QSerialPort::readyRead, this, &MainWindow::Read_data);
    }
    else
    {
        // 关闭串口
        serial->clear();
        serial->close();
        serial->deleteLater();

        //恢复设置功能
        ui->comboBox_CommPort->setEnabled(true);
        ui->comboBox_Baudrate->setEnabled(true);
        ui->pushButton_OpenCloseComm->setText(tr("打开串口"));
        ui->pushButton_SendData->setEnabled(false);

        if(ui->lineEdit_filename->text().isEmpty())
        {
            ui->pushButton_startupdate->setEnabled(false);
            ui->pushButton_stopUpdate->setEnabled(false);
        }

    }
}

void MainWindow::on_pushButton_SendData_clicked()
{
    serial->write(ui->plainTextEdit_SendDataBuffer->toPlainText().toLatin1());
}

void MainWindow::on_pushButton_ClearData_clicked()
{
    ui->textEdit_ReceiveDataBuffer->clear();
}

void MainWindow::Read_data()
{
    QByteArray buf = serial->readAll();
    if(!buf.isEmpty())
    {
        emit trSerialData(buf);
        m_ObjThread->msleep(5);
        ui->textEdit_ReceiveDataBuffer->append(QString(buf));
    }
    buf.clear();
}

void MainWindow::on_pushButton_Openfile_clicked()
{
    // 获取完整文件名
    filefullname = QFileDialog::getOpenFileName(this, "Open file", "C:\\CBT\\CBT-350 Development\\Output_files", tr("ini file(*.ini)"));
    if(filefullname.length() != 0)
    {
        // 获取文件信息
        QFileInfo fileinfo = QFileInfo(filefullname);
        // 获取文件名
        QString filename = fileinfo.fileName();
        // 显示文件名
        ui->lineEdit_filename->setText(filename);

        if(serial->isOpen())
        {
            ui->pushButton_startupdate->setEnabled(true);
            ui->pushButton_stopUpdate->setEnabled(false);
        }
    }
}

void MainWindow::on_pushButton_startupdate_clicked()
{
    m_Obj->setInifile(filefullname);
    m_ObjThread->start();
    qDebug()<<"m_ObjThread start";
    emit startupdate();
    qDebug()<<"m_ObjThread start signal send out";
}

void MainWindow::on_pushButton_stopUpdate_clicked()
{
    m_Obj->stop();
}

void MainWindow::SendObjSerialData(const QByteArray &str)
{
    serial->write(str);
}

void MainWindow::UpdateWidgeStatus(const WidgetID wgnum, const QString &str)
{
    switch(wgnum)
    {
        case WG_EDIT_UPDATE_AREA:
            break;

        case WG_EDIT_UPDATE_ADDR:
            break;

        case WG_EDIT_SENDDATA_BUFFER:
            ui->plainTextEdit_SendDataBuffer->appendPlainText(str);
            break;

        case WG_EDIT_RECEIVEDATA_BUFFER:
            ui->textEdit_ReceiveDataBuffer->append(str);
            break;

        case WG_BUTTON_START_UPDATE:
            if(str.contains("ENABLE"))
                ui->pushButton_startupdate->setEnabled(true);
            else if(str.contains("DISABLE"))
                ui->pushButton_startupdate->setEnabled(false);
            break;

        case WG_BUTTON_STOP_UPDATE:
            if(str.contains("ENABLE"))
                ui->pushButton_stopUpdate->setEnabled(true);
            else if(str.contains("DISABLE"))
                ui->pushButton_stopUpdate->setEnabled(false);
            break;

        default:
            break;
    }
}

