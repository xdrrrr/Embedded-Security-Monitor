#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QTimer>
#include <QMessageBox>
#include <QFile>
#include <QTableWidget>
#include <QProgressBar>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QSlider>
#include <QSpinBox>
#include <QTextEdit>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QFileDialog>
#include <QHeaderView>
#include <QDateTime>
#include <QDebug>
#include <QTcpServer>
#include <QLineEdit>



class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(QWidget *parent = nullptr);
    ~Widget();

private slots:
    void onSerialReadyRead();
    void onSendTimeout();
    void onRefreshClicked();
    void onConnectClicked();
    void onDisconnectClicked();
    void onAngleSliderChanged(int value);
    void onAngleSpinChanged(int value);
    void onGetLogClicked();
    void onExportLogClicked();
    void onAlarmOffClicked();
private:
    void setupUI();
    void initSerialPort();
    void updatePortList();
    void processDataFrame(const QString &frame);
    void processLogData(const QString &line);
    void sendCommand(QString command);
    void showAlarm(const QString &message);
    void addAlarmLog(const QString &type, const QString &params);

    // 串口控件
    QComboBox *comPortCombo;
    QComboBox *baudRateCombo;
    QPushButton *refreshBtn;
    QPushButton *connectBtn;
    QPushButton *disconnectBtn;
    QLabel *statusLabel;

    // 数据展示控件
    QLabel *tempLabel;
    QLabel *humidLabel;
    QLabel *lightLabel;
    QProgressBar *lightProgress;
    QLabel *alarmLabel;
    QLabel *curAngleLabel;

    // 舵机控制控件
    QSlider *angleSlider;
    QSpinBox *angleSpin;

    // 日志管理控件
    QTextEdit *alarmText;
    QTableWidget *logTable;
    QPushButton *getLogBtn;
    QPushButton *exportLogBtn;

    // 其他成员
    QSerialPort *serial;
    QTimer *sendTimer;
    QList<QStringList> logData;
    int resendCount = 0;
    QString lastCommand;
    int currentServoAngle = 90;

    //服务端
    QTcpServer *tcpServer;
    QTcpSocket *clientSocket;
    bool isServerRunning;

    // UI组件指针
    QLineEdit *portEdit;
    QPushButton *startStopBtn;
    QTextEdit *receiveArea;
    QLineEdit *sendEdit;
    QPushButton *sendBtn;
    QPushButton *clearBtn;

    QPushButton *alarmOffBtn; // 新增：远程消警按钮
    int lastAlarmState; // 新增：存储上一次的报警状态（0=正常，1=入侵报警，2=环境报警）
};

#endif // WIDGET_H
