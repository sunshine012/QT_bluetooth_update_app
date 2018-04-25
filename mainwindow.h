#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include <QDebug>
#include "UpdateThread.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

signals:
    void startupdate();

private slots:
    void on_pushButton_OpenCloseComm_clicked();
    void on_pushButton_SendData_clicked();
    void on_pushButton_ClearData_clicked();
    void Read_data();
    void on_pushButton_Openfile_clicked();
    void on_pushButton_startupdate_clicked();
    void SendObjSerialData(const QByteArray &);
    void UpdateWidgeStatus(const WidgetID wgnum, const QString &);
    void on_pushButton_stopUpdate_clicked();

private:
    Ui::MainWindow *ui;
    QSerialPort *serial;
    QThread *m_ObjThread;
    UpdateThreadObj *m_Obj;
};



#endif // MAINWINDOW_H
