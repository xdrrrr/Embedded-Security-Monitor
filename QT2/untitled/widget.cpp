#include "widget.h"

Widget::Widget(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
    initSerialPort();
    lastAlarmState = 0; // 初始化：上一次报警状态为“正常”
}

Widget::~Widget()
{
    if (serial && serial->isOpen()) serial->close();
    delete serial;
    delete sendTimer;
}

void Widget::setupUI()
{

    // 主布局
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // ========== 串口配置区域 ==========
    QGroupBox *serialGroup = new QGroupBox("串口配置");
    QGridLayout *serialLayout = new QGridLayout(serialGroup);

    // 串口控件
    QLabel *portLabel = new QLabel("串口号:");
    comPortCombo = new QComboBox();
    QLabel *baudLabel = new QLabel("波特率:");
    baudRateCombo = new QComboBox();
    baudRateCombo->addItem("115200");
    baudRateCombo->setEnabled(false);

    refreshBtn = new QPushButton("刷新");
    connectBtn = new QPushButton("连接");
    disconnectBtn = new QPushButton("断开");
    disconnectBtn->setEnabled(false);

    statusLabel = new QLabel("状态: 未连接");

    // 添加到布局
    serialLayout->addWidget(portLabel, 0, 0);
    serialLayout->addWidget(comPortCombo, 0, 1);
    serialLayout->addWidget(baudLabel, 1, 0);
    serialLayout->addWidget(baudRateCombo, 1, 1);
    serialLayout->addWidget(refreshBtn, 0, 2);
    serialLayout->addWidget(connectBtn, 0, 3);
    serialLayout->addWidget(disconnectBtn, 1, 2);
    serialLayout->addWidget(statusLabel, 1, 3);

    // ========== 实时数据显示区域 ==========
    QGroupBox *dataGroup = new QGroupBox("环境数据");
    QGridLayout *dataLayout = new QGridLayout(dataGroup);

    // 数据展示控件
    QLabel *tempTitle = new QLabel("温度:");
    tempLabel = new QLabel("--");
    QLabel *humidTitle = new QLabel("湿度:");
    humidLabel = new QLabel("--");
    QLabel *lightTitle = new QLabel("光照:");
    lightLabel = new QLabel("--");
    lightProgress = new QProgressBar();
    lightProgress->setRange(0, 1000);
    QLabel *alarmTitle = new QLabel("警报状态:");
    alarmLabel = new QLabel("正常");
    alarmLabel->setStyleSheet("color: green;");
    QLabel *angleTitle = new QLabel("舵机角度:");
    curAngleLabel = new QLabel("--");

    // 新增：远程消警按钮（初始禁用，串口连接后启用）
    alarmOffBtn = new QPushButton("远程消警");
    alarmOffBtn->setEnabled(false); // 初始禁用
    alarmOffBtn->setStyleSheet("background-color: #ff4444; color: white;"); // 红色按钮，突出警示功能

    // 修改布局：将「报警状态+消警按钮」放在同一行，按钮在状态右侧
    dataLayout->addWidget(tempTitle, 0, 0);
    dataLayout->addWidget(tempLabel, 0, 1);
    dataLayout->addWidget(humidTitle, 1, 0);
    dataLayout->addWidget(humidLabel, 1, 1);
    dataLayout->addWidget(lightTitle, 2, 0);
    dataLayout->addWidget(lightLabel, 2, 1);
    dataLayout->addWidget(lightProgress, 3, 0, 1, 2); // 光照进度条占2列
    dataLayout->addWidget(alarmTitle, 4, 0);         // 报警状态标题（第4行第0列）
    dataLayout->addWidget(alarmLabel, 4, 1);         // 报警状态文本（第4行第1列）
    dataLayout->addWidget(alarmOffBtn, 4, 2);        // 消警按钮（第4行第2列，新增）
    dataLayout->addWidget(angleTitle, 5, 0);
    dataLayout->addWidget(curAngleLabel, 5, 1);

    // ========== 舵机控制区域 ==========
    QGroupBox *controlGroup = new QGroupBox("舵机控制");
    QVBoxLayout *controlLayout = new QVBoxLayout(controlGroup);

    // 舵机控制控件
    angleSlider = new QSlider(Qt::Horizontal);
    angleSlider->setRange(0, 180);
    angleSlider->setValue(90);
    angleSlider->setEnabled(false);

    angleSpin = new QSpinBox();
    angleSpin->setRange(0, 180);
    angleSpin->setValue(90);
    angleSpin->setEnabled(false);

    QHBoxLayout *angleLayout = new QHBoxLayout();
    angleLayout->addWidget(new QLabel("角度:"));
    angleLayout->addWidget(angleSpin);
    angleLayout->addWidget(new QLabel("°"));

    // 添加到布局
    controlLayout->addWidget(angleSlider);
    controlLayout->addLayout(angleLayout);

    // ========== 日志管理区域 ==========
    QGroupBox *logGroup = new QGroupBox("日志管理");
    QVBoxLayout *logLayout = new QVBoxLayout(logGroup);

    // 报警日志
    QLabel *alarmLogTitle = new QLabel("实时报警日志:");
    alarmText = new QTextEdit();
    alarmText->setReadOnly(true);

    // 历史日志
    QLabel *historyLogTitle = new QLabel("历史日志:");
    logTable = new QTableWidget();
    logTable->setColumnCount(5);
    logTable->setHorizontalHeaderLabels({"时间", "报警类型", "温度(℃)", "湿度(%)", "光照(Lux)"});
    logTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    // 操作按钮
    getLogBtn = new QPushButton("读取日志");
    exportLogBtn = new QPushButton("导出日志");
    getLogBtn->setEnabled(false);
    exportLogBtn->setEnabled(false);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addWidget(getLogBtn);
    btnLayout->addWidget(exportLogBtn);

    // 添加到布局
    logLayout->addWidget(alarmLogTitle);
    logLayout->addWidget(alarmText);
    logLayout->addWidget(historyLogTitle);
    logLayout->addWidget(logTable);
    logLayout->addLayout(btnLayout);

    // ========== 添加到主布局 ==========
    mainLayout->addWidget(serialGroup);
    mainLayout->addWidget(dataGroup);

    QHBoxLayout *midLayout = new QHBoxLayout();
    midLayout->addWidget(controlGroup);
    midLayout->addWidget(logGroup);
    mainLayout->addLayout(midLayout);

    // ========== 连接信号和槽 ==========
    connect(refreshBtn, &QPushButton::clicked, this, &Widget::onRefreshClicked);
    connect(connectBtn, &QPushButton::clicked, this, &Widget::onConnectClicked);
    connect(disconnectBtn, &QPushButton::clicked, this, &Widget::onDisconnectClicked);
    connect(angleSlider, &QSlider::valueChanged, this, &Widget::onAngleSliderChanged);
    connect(angleSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &Widget::onAngleSpinChanged);
    connect(getLogBtn, &QPushButton::clicked, this, &Widget::onGetLogClicked);
    connect(exportLogBtn, &QPushButton::clicked, this, &Widget::onExportLogClicked);
    connect(alarmOffBtn, &QPushButton::clicked, this, &Widget::onAlarmOffClicked); // 新增

    // 初始化发送定时器
    sendTimer = new QTimer(this);
    sendTimer->setInterval(100);
    connect(sendTimer, &QTimer::timeout, this, &Widget::onSendTimeout);

    // 设置窗口属性
    setWindowTitle("安防监控上位机");
    resize(900, 600);
}

void Widget::initSerialPort()
{
    serial = new QSerialPort(this);
    connect(serial, &QSerialPort::readyRead, this, &Widget::onSerialReadyRead);
    updatePortList();
}

void Widget::updatePortList()
{
    comPortCombo->clear();
    foreach(const QSerialPortInfo &info, QSerialPortInfo::availablePorts()) {
        comPortCombo->addItem(info.portName());
    }
}

void Widget::onRefreshClicked()
{
    updatePortList();
}

void Widget::onConnectClicked()
{
    if (comPortCombo->currentText().isEmpty()) return;

    serial->setPortName(comPortCombo->currentText());
    serial->setBaudRate(115200);
    serial->setDataBits(QSerialPort::Data8);
    serial->setParity(QSerialPort::NoParity);
    serial->setStopBits(QSerialPort::OneStop);
    serial->setFlowControl(QSerialPort::NoFlowControl);

    if (serial->open(QIODevice::ReadWrite)) {
        connectBtn->setEnabled(false);
        disconnectBtn->setEnabled(true);
        refreshBtn->setEnabled(false);
        comPortCombo->setEnabled(false);
        angleSlider->setEnabled(true);
        angleSpin->setEnabled(true);
        getLogBtn->setEnabled(true);
        alarmOffBtn->setEnabled(true); // 启用消警按钮（新增）
        statusLabel->setText("状态: 已连接");
        angleSlider->setValue(90);
    } else {
        QMessageBox::critical(this, "连接失败", "无法打开串口: " + serial->errorString());
    }
}

void Widget::onDisconnectClicked()
{
    serial->close();
    connectBtn->setEnabled(true);
    disconnectBtn->setEnabled(false);
    refreshBtn->setEnabled(true);
    comPortCombo->setEnabled(true);
    angleSlider->setEnabled(false);
    angleSpin->setEnabled(false);
    getLogBtn->setEnabled(false);
    alarmOffBtn->setEnabled(false); // 禁用消警按钮（新增）
    statusLabel->setText("状态: 未连接");
}

void Widget::onSerialReadyRead()
{
    if (!serial->isOpen()) return;

    // 读取所有可用数据
    QByteArray data = serial->readAll();

    // 分割为行
    QList<QByteArray> lines = data.split('\n');

    foreach(const QByteArray &lineData, lines) {
        if (lineData.isEmpty()) continue;

        QString line = QString::fromLatin1(lineData).trimmed();

        if (line.isEmpty()) continue;

        qDebug() << "接收数据:" << line;

        // 处理实时数据帧
        if (line.startsWith('$')) {
            qDebug() << "处理DATA帧";
            processDataFrame(line);
        }
        // 处理日志数据帧
        else if (line.startsWith("LOG:")) {
            qDebug() << "处理LOG帧";
            processLogData(line);
        }
        // 处理ACK响应
        else if (line.startsWith("ACK") && sendTimer->isActive()) {
            qDebug() << "收到ACK响应";
            sendTimer->stop();
            resendCount = 0;
        }
    }
}

void Widget::processDataFrame(const QString &frame)
{
    // 移除$和校验和部分
    QString dataPart = frame.mid(1);
    int starIndex = dataPart.indexOf('*');
    if (starIndex != -1) {
        dataPart = dataPart.left(starIndex);
    }

    // 分割数据字段
    QStringList parts = dataPart.split(',');

    // 验证字段数量
    if (parts.size() < 5) {
        qDebug() << "无效DATA帧: 字段不足";
        return;
    }

    // 示例数据格式：$DATA,25,60,500,90,0*
    if (parts[0] != "DATA") {
        qDebug() << "无效DATA帧: 标识错误";
        return;
    }

    // 温度
    tempLabel->setText(parts[1] + "°C");

    // 湿度
    humidLabel->setText(parts[2] + "%");

    // 光照
    bool lightOk;
    int light = parts[3].toInt(&lightOk);
    if (lightOk) {
        lightLabel->setText(QString::number(light) + " Lux");
        lightProgress->setValue(light);
    }

    // 新增：处理下位机的消警响应（$ACK,ALARM_OFF,OK*）
        if (parts[0] == "ACK") { // 识别ACK响应帧
            if (parts.size() >= 2 && parts[1] == "ALARM_OFF" && parts[2] == "OK") {
                // 消警成功：更新界面报警状态+提示用户
                alarmLabel->setText("正常");
                alarmLabel->setStyleSheet("color: green;");
                QMessageBox::information(this, "消警成功", "下位机已响应消警指令，报警状态已解除！");

                // 可选：记录消警日志
                addAlarmLog("消警", "远程消警指令执行成功");
            }
            return; // 处理完ACK响应，无需继续执行后续数据解析
        }

    // 舵机角度
    bool angleOk;
    int angle = parts[4].toInt(&angleOk);
    if (angleOk && angle >= 0 && angle <= 180) {
        currentServoAngle = angle;
        curAngleLabel->setText(QString::number(angle) + "°");

        // 更新滑块位置（不触发发送）
        angleSlider->blockSignals(true);
        angleSpin->blockSignals(true);
        angleSlider->setValue(angle);
        angleSpin->setValue(angle);
        angleSlider->blockSignals(false);
        angleSpin->blockSignals(false);
    }

    // 报警状态（可选的第六个字段）
    if (parts.size() >= 6) {
        bool alarmOk;
        int currentAlarmState = parts[5].toInt(&alarmOk); // 当前报警状态
        QString alarmText;

        if (alarmOk) {
            // 1. 先更新界面显示的报警文本和颜色
            switch (currentAlarmState) {
                case 0: // 正常
                    alarmText = "正常";
                    alarmLabel->setStyleSheet("color: green;");
                    break;
                case 2: // 入侵报警
                    alarmText = "入侵报警";
                    alarmLabel->setStyleSheet("color: red;");
                    break;
                case 1: // 环境报警
                    alarmText = "环境异常";
                    alarmLabel->setStyleSheet("color: orange;");
                    break;
                default:
                    alarmText = "未知状态";
                    break;
            }
            alarmLabel->setText(alarmText);

            // 2. 核心：仅“报警状态变化且当前是报警”时弹窗（去重逻辑）
            if (currentAlarmState != 0) { // 当前是报警状态（入侵/环境）
                if (currentAlarmState != lastAlarmState) { // 与上一次状态不同（新报警）
                    showAlarm(alarmText + "！"); // 弹窗提示
                    addAlarmLog((currentAlarmState == 2) ? "入侵" : "环境", parts.mid(1, 3).join(',')); // 记录日志
                    lastAlarmState = currentAlarmState; // 更新上一次状态为当前报警状态
                }
            } else { // 当前是正常状态
                lastAlarmState = 0; // 重置上一次状态为正常（避免下次报警不弹窗）
            }
        }
    }
    qDebug() << "DATA帧处理完成";
}

void Widget::processLogData(const QString &line)
{
    // 移除开头的"LOG:"
    QString dataPart = line.mid(4);

    // 分割数据字段
    QStringList parts = dataPart.split(',');

    // 验证字段数量
    if (parts.size() < 5) {
        qDebug() << "无效LOG帧: 字段不足";
        return;
    }

    // 格式：时间戳, 报警类型, 温度, 湿度, 光照
    int row = logTable->rowCount();
    logTable->insertRow(row);

    for (int i = 0; i < 5; i++) {
        if (i < parts.size()) {
            QTableWidgetItem *item = new QTableWidgetItem(parts[i]);
            logTable->setItem(row, i, item);
        } else {
            logTable->setItem(row, i, new QTableWidgetItem(""));
        }
    }

    logData.append(parts);
    exportLogBtn->setEnabled(true);
    qDebug() << "LOG帧处理完成";
}

void Widget::onAngleSliderChanged(int value)
{
    angleSpin->setValue(value);
    sendCommand(QString("SET_ANGLE,%1").arg(value));
}

void Widget::onAngleSpinChanged(int value)
{
    angleSlider->setValue(value);
}

void Widget::onGetLogClicked()
{
    // 只发送一次日志请求，不等待响应
    sendCommand("GET_LOG");
    qDebug() << "发送日志请求";

}

void Widget::onExportLogClicked()
{
    if (logData.isEmpty()) {
        QMessageBox::information(this, "提示", "没有日志数据可导出");
        return;
    }

    QString fileName = QFileDialog::getSaveFileName(
        this, "导出日志", "", "CSV 文件 (*.csv)");

    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "错误", "无法创建文件");
        return;
    }

    QTextStream out(&file);
    out << "time,type,temp,hum,light\n";

    for (const QStringList &row : logData) {
        out << row.join(',') << "\n";
    }

    file.close();
    QMessageBox::information(this, "完成", "日志导出成功");
}

void Widget::sendCommand(QString command)
{
    if (!serial->isOpen()) return;

    // 创建完整命令: #GET_LOG* 或 #SET_ANGLE,90*
    QString fullCommand = "#" + command + "*\r\n";

    // 发送命令
    serial->write(fullCommand.toUtf8());
    qDebug() << "发送命令:" << fullCommand.trimmed();
}


void Widget::onSendTimeout()
{
    if (resendCount < 3) {
        qDebug() << "重发命令(" << (resendCount + 1) << "/3)";
        serial->write(lastCommand.toUtf8());
        resendCount++;
    } else {
        sendTimer->stop();
        QMessageBox::warning(this, "通信错误", "指令未得到响应");
    }
}

void Widget::showAlarm(const QString &message)
{
    // 非模态弹窗：不阻塞界面，点击后自动关闭
    QMessageBox *alarmBox = new QMessageBox(QMessageBox::Critical, "系统报警", message, QMessageBox::Ok, this);
    alarmBox->setAttribute(Qt::WA_DeleteOnClose); // 关闭后自动释放内存
    alarmBox->show(); // 非模态显示（用show()而非exec()）
}

void Widget::addAlarmLog(const QString &type, const QString &params)
{
    QString timeStr = QDateTime::currentDateTime().toString("hh:mm:ss");
    QString logEntry = QString("[%1] %2: %3").arg(timeStr, type, params);
    alarmText->append(logEntry);
    qDebug() << "记录报警日志:" << logEntry;
}

// 新增：远程消警按钮点击事件
void Widget::onAlarmOffClicked()
{
    // 发送消警指令：sendCommand会自动拼接 # + 指令 + *，最终发送 #ALARM_OFF*
    sendCommand("ALARM_OFF");

    // 可选：添加用户提示（避免重复点击）
    QMessageBox::information(this, "提示", "已发送远程消警指令，等待下位机响应...");
}
