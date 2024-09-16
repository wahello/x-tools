﻿/***************************************************************************************************
 * Copyright 2023-2024 x-tools-author(x-tools@outlook.com). All rights reserved.
 *
 * The file is encoded using "utf8 with bom", it is a part of xTools project.
 *
 * xTools is licensed according to the terms in the file LICENCE(GPL V3) in the root of the source
 * code directory.
 **************************************************************************************************/
#include "ModbusAssistant.h"
#include "ui_ModbusAssistant.h"

#include <QAbstractSocket>
#include <QCheckBox>
#include <QComboBox>
#include <QDateTime>
#include <QGroupBox>
#include <QHostAddress>
#include <QItemDelegate>
#include <QLineEdit>
#include <QMessageBox>
#include <QModbusServer>
#include <QModbusTcpClient>
#include <QModbusTcpServer>
#include <QNetworkAddressEntry>
#include <QNetworkInterface>
#include <QPushButton>
#include <QRegularExpression>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QSpinBox>
#include <QStandardItemModel>
#include <QTextBrowser>
#include <QtEndian>
#if QT_VERSION < QT_VERSION_CHECK(6, 2, 0)
#include <QModbusRtuSerialMaster>
#include <QModbusRtuSerialSlave>
#else
#include <QModbusRtuSerialClient>
#include <QModbusRtuSerialServer>
#endif

#include "App/Settings.h"
#include "Common/xTools.h"
#include "ModbusFactory.h"

#define RXCOLOR "green"
#define TXCOLOR "blue"
#define RXFLAG "rx:"
#define TXFLAG "tx:"
#define MAX_HISTORY_INDEX 9

struct ModbusSettingKeys
{
    const QString deviceIndex = "xToolsModbus/deviceIndex";

    const QString portName = "xToolsModbus/portName";
    const QString parity = "xToolsModbus/parity";
    const QString baudRate = "xToolsModbus/baudRate";
    const QString dataBits = "xToolsModbus/dataBits";
    const QString stopBits = "xToolsModbus/stopBits";
    const QString customBaudRate = "xToolsModbus/customBaudRate";

    const QString address = "xToolsModbus/address";
    const QString port = "xToolsModbus/port";
    const QString customAddress = "xToolsModbus/customAddress";

    const QString clientTimeout = "xToolsModbus/clientTimeout";
    const QString clientRepeatTime = "xToolsModbus/clientRepeatTime";

    const QString serverIsBusy = "xToolsModbus/serverIsBusy";
    const QString serverJustListen = "xToolsModbus/serverJustListen";
    const QString serverAddress = "xToolsModbus/serverAddress";

    const QString functionCode = "xToolsModbus/functionCode";
    const QString targetAddress = "xToolsModbus/targetAddress";
    const QString startAddress = "xToolsModbus/startAddress";
    const QString addressNumber = "xToolsModbus/addressNumber";

    const QString sendHistory = "xToolsModbus/sendHistory";
    const QString sendHistoryIndex = "xToolsModbus/sendHistoryIndex";
    const QString pdu = "pdu";
};

class ReadOnlyDelegate : public QItemDelegate
{
public:
    ReadOnlyDelegate(QWidget *parent = Q_NULLPTR)
        : QItemDelegate(parent)
    {}
    QWidget *createEditor(QWidget *parent,
                          const QStyleOptionViewItem &option,
                          const QModelIndex &index) const override
    {
        Q_UNUSED(parent);
        Q_UNUSED(option);
        Q_UNUSED(index);
        return Q_NULLPTR;
    }
};

QList<quint16> vectorTolist(const QVector<quint16> &vector)
{
    QList<quint16> list;
    for (int i = 0; i < vector.length(); vector.count()) {
        list.append(vector.at(i));
    }
    return list;
}

QVector<quint16> listToVector(const QList<quint16> &list)
{
    QVector<quint16> vector;
    for (int i = 0; i < list.length(); list.count()) {
        vector.append(list.at(i));
    }
    return vector;
}

ModbusAssistant::ModbusAssistant(QWidget *parent)
    : QWidget{parent}
    , ui(new Ui::ModbusAssistant)
    , m_modbusDevice(Q_NULLPTR)
    , m_registerModel(Q_NULLPTR)
    , m_keyCtx(new ModbusSettingKeys)
{
    if (!m_settings) {
        m_settings = xTools::Settings::instance();
    }

    ui->setupUi(this);

    initComponents();
    initSettings();
    initSignals();

    updateUiState(false);
    onDeviceTypeChanged();
    updateClientTableView();
    updateClientReadWriteButtonState();
}

ModbusAssistant::~ModbusAssistant()
{
    delete ui;
}

void ModbusAssistant::initComponents()
{
    initComponentDevices();
    initComponentAddress();
    initComponentPortName();
    initComponnetBaudRate();
    initComponnetDataBits();
    initComponnetStopBits();
    initComponnetParity();
    initComponentFunctionCode();
    initComponentRegisterTableView();
    initComponentInput();
    initComponentRegisterTabWidget();
}

void ModbusAssistant::initComponentDevices()
{
    ui->comboBoxDeviceList->addItem(tr("RtuClient"), ModbusFactory::ModbusRtuSerialClient);
    ui->comboBoxDeviceList->addItem(tr("RtuServer"), ModbusFactory::ModbusRtuSerialServer);
    ui->comboBoxDeviceList->addItem(tr("TcpClient"), ModbusFactory::ModbusTcpClient);
    ui->comboBoxDeviceList->addItem(tr("TcpServer"), ModbusFactory::ModbusTcpServer);
}

void ModbusAssistant::initComponentAddress()
{
    ui->comboBoxAddress->clear();
    QList<QHostAddress> addresses = QNetworkInterface::allAddresses();
    for (QHostAddress &address : addresses) {
        if (address.protocol() == QAbstractSocket::IPv4Protocol) {
            ui->comboBoxAddress->addItem(address.toString());
        }
    }
}

void ModbusAssistant::initComponentPortName()
{
    ui->comboBoxPortName->clear();
    QList<QSerialPortInfo> infos = QSerialPortInfo::availablePorts();
    for (QSerialPortInfo &info : infos) {
        ui->comboBoxPortName->addItem(info.portName());
    }
}

void ModbusAssistant::initComponnetBaudRate()
{
    ui->comboBoxBaudRate->clear();
    QList<qint32> bds = QSerialPortInfo::standardBaudRates();
    for (qint32 &bd : bds) {
        ui->comboBoxBaudRate->addItem(QString::number(bd), bd);
    }
}

void ModbusAssistant::initComponnetDataBits()
{
    ui->comboBoxDataBits->clear();
    ui->comboBoxDataBits->addItem("8", QSerialPort::Data8);
    ui->comboBoxDataBits->addItem("7", QSerialPort::Data7);
    ui->comboBoxDataBits->addItem("6", QSerialPort::Data6);
    ui->comboBoxDataBits->addItem("5", QSerialPort::Data5);
}

void ModbusAssistant::initComponnetStopBits()
{
    ui->comboBoxStopBits->clear();
    ui->comboBoxStopBits->addItem("1", QSerialPort::OneStop);
#ifdef Q_OS_WIN
    ui->comboBoxStopBits->addItem("1.5", QSerialPort::OneAndHalfStop);
#endif
    ui->comboBoxStopBits->addItem("2", QSerialPort::TwoStop);
}

void ModbusAssistant::initComponnetParity()
{
    ui->comboBoxParity->clear();
    ui->comboBoxParity->addItem(tr("NoParity"), QSerialPort::NoParity);
    ui->comboBoxParity->addItem(tr("EvenParity"), QSerialPort::EvenParity);
    ui->comboBoxParity->addItem(tr("OddParity"), QSerialPort::OddParity);
    ui->comboBoxParity->addItem(tr("SpaceParity"), QSerialPort::SpaceParity);
    ui->comboBoxParity->addItem(tr("MarkParity"), QSerialPort::MarkParity);
}

void ModbusAssistant::initComponentFunctionCode()
{
    const QString str0x01 = tr("0x01-ReadCoils");
    const QString str0x02 = tr("0x02-ReadDiscreteInputs");
    const QString str0x03 = tr("0x03-ReadHoldingRegisters");
    const QString str0x04 = tr("0x04-ReadInputRegisters");
    const QString str0x05 = tr("0x05-WriteSingleCoil");
    const QString str0x06 = tr("0x06-WriteSingleRegister");
    const QString str0x0f = tr("0x0f-WriteMultipleCoils");
    const QString str0x10 = tr("0x10-WriteMultipleRegisters");

    int func0x01 = QModbusDataUnit::Coils;
    int func0x02 = QModbusDataUnit::DiscreteInputs;
    int func0x03 = QModbusDataUnit::HoldingRegisters;
    int func0x04 = QModbusDataUnit::InputRegisters;
    int func0x05 = QModbusDataUnit::Coils;
    int func0x06 = QModbusDataUnit::HoldingRegisters;
    int func0x0f = QModbusDataUnit::Coils;
    int func0x10 = QModbusDataUnit::HoldingRegisters;

    ui->comboBoxFunctionCode->addItem(str0x01, func0x01);
    ui->comboBoxFunctionCode->addItem(str0x02, func0x02);
    ui->comboBoxFunctionCode->addItem(str0x03, func0x03);
    ui->comboBoxFunctionCode->addItem(str0x04, func0x04);
    ui->comboBoxFunctionCode->addItem(str0x05, func0x05);
    ui->comboBoxFunctionCode->addItem(str0x06, func0x06);
    ui->comboBoxFunctionCode->addItem(str0x0f, func0x0f);
    ui->comboBoxFunctionCode->addItem(str0x10, func0x10);
}

void ModbusAssistant::initComponentRegisterTableView()
{
    QTableView *table_view = CreateTableView(1, ui->tabViewClientRegisters);
    m_registerModel = qobject_cast<QStandardItemModel *>(table_view->model());
}

void ModbusAssistant::initComponentInput()
{
#if 0
    QRegularExpression re("([0-9a-fA-F][0-9a-fA-F][ ])*");
    QLineEdit *le = ui->comboBoxPdu->lineEdit();
    QRegularExpressionValidator *rev = new QRegularExpressionValidator(re, le);
    le->setValidator(rev);
    le->clear();
#endif
}

void ModbusAssistant::initComponentRegisterTabWidget()
{
    QTabWidget *tabWidget = ui->tabWidgetServerRegisters;
    QStringList titles = QStringList() << tr("Coils") << tr("DiscreteInputs")
                                       << tr("InputRegisters") << tr("HoldingRegisters");
    for (QString &title : titles) {
        QTableView *tableView = CreateTableView(UINT16_MAX + 1, Q_NULLPTR);
        tableView->verticalHeader()->hide();
        tabWidget->addTab(tableView, title);
    }
}

void ModbusAssistant::initSettings()
{
    initSettingsDevice();
    initSettingsNetwork();
    initSettingsSerialPort();
    initSettingsClient();
    initSettingsServer();
    initSettingsClientOperations();
    initSettingsInput();
}

void ModbusAssistant::initSettingsDevice()
{
    int deviceIndex = m_settings->value(m_keyCtx->deviceIndex).toInt();
    if (deviceIndex >= 0 && deviceIndex < ui->comboBoxDeviceList->count()) {
        ui->comboBoxDeviceList->setCurrentIndex(deviceIndex);
    }
}

void ModbusAssistant::initSettingsNetwork()
{
    QString address = m_settings->value(m_keyCtx->address).toString();
    ui->comboBoxAddress->setCurrentText(address);

    QVariant portValiant = m_settings->value(m_keyCtx->port);
    int port = portValiant.toInt();
    if (portValiant.isNull()) {
        port = 502;
    }
    ui->spinBoxPort->setValue(port);
}

void ModbusAssistant::initSettingsSerialPort()
{
    auto SetComboBoxIndex = [](int index, QComboBox *cb) {
        if (index >= 0 && index < cb->count()) {
            cb->setCurrentIndex(index);
        }
    };

    QString portName = m_settings->value(m_keyCtx->portName).toString();
    int index = ui->comboBoxPortName->findText(portName);
    SetComboBoxIndex(index, ui->comboBoxPortName);

    index = m_settings->value(m_keyCtx->parity).toInt();
    SetComboBoxIndex(index, ui->comboBoxParity);

    QString bd = m_settings->value(m_keyCtx->baudRate).toString();
    ui->comboBoxBaudRate->setCurrentText(bd);

    index = m_settings->value(m_keyCtx->dataBits).toInt();
    SetComboBoxIndex(index, ui->comboBoxDataBits);

    index = m_settings->value(m_keyCtx->stopBits).toInt();
    SetComboBoxIndex(index, ui->comboBoxStopBits);
}

void ModbusAssistant::initSettingsClient()
{
    int timeout = m_settings->value(m_keyCtx->clientTimeout).toInt();
    ui->spinBoxTimeout->setValue(timeout < 100 ? 100 : timeout);

    const QString key = m_keyCtx->clientRepeatTime;
    int repeatTimes = m_settings->value(key).toInt();
    ui->spinBoxRepeatTime->setValue(repeatTimes);
}

void ModbusAssistant::initSettingsServer()
{
    bool isBusy = m_settings->value(m_keyCtx->serverIsBusy).toBool();
    ui->checkBoxDeviceBusy->setChecked(isBusy);

    QString key = m_keyCtx->serverJustListen;
    bool just_listen = m_settings->value(key).toBool();
    ui->checkBoxListenOnlyMode->setChecked(just_listen);

    int address = m_settings->value(m_keyCtx->serverAddress).toInt();
    ui->spinBoxServerAddress->setValue(address);
}

void ModbusAssistant::initSettingsClientOperations()
{
    int index = m_settings->value(m_keyCtx->functionCode).toInt();
    if (index >= 0 && index < ui->comboBoxFunctionCode->count()) {
        ui->comboBoxFunctionCode->setCurrentIndex(index);
    }

    int address = m_settings->value(m_keyCtx->targetAddress).toInt();
    ui->spinBoxDeviceAddress->setValue(address);

    int start = m_settings->value(m_keyCtx->startAddress).toInt();
    ui->spinBoxStartAddress->setValue(start);

    int number = m_settings->value(m_keyCtx->addressNumber).toInt();
    ui->spinBoxQuantity->setValue(number);
}

void ModbusAssistant::initSettingsInput()
{
    ui->lineEditPdu->clear();
    m_settings->beginReadArray(m_keyCtx->sendHistory);
    for (int i = 0; i < 10; i++) {
        m_settings->setArrayIndex(i);
        QString text = m_settings->value(m_keyCtx->pdu).toString();
        if (!text.isEmpty()) {
            //ui->comboBoxPdu->addItem(text);
        }
    }
    m_settings->endArray();

    //int index = m_settings->value(m_keyCtx->sendHistoryIndex).toInt();
    //ui->comboBoxPdu->setCurrentIndex(index - 1);
}

void ModbusAssistant::initSignals()
{
    initSignalsDevice();
    initSignalsNetworking();
    initSignalsSerialPort();
    initSignalsClient();
    initSignalsServer();
    initSignalsClientOperations();
}

void ModbusAssistant::initSignalsDevice()
{
    connect(ui->comboBoxDeviceList,
            static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this,
            &ModbusAssistant::onDeviceTypeChanged);
    connect(ui->pushButtonOpen, &QPushButton::clicked, this, &ModbusAssistant::onOpenClicked);
    connect(ui->pushButtonClose, &QPushButton::clicked, this, &ModbusAssistant::onCloseClicked);
}

void ModbusAssistant::initSignalsNetworking()
{
    connect(ui->comboBoxAddress,
            &QComboBox::currentTextChanged,
            this,
            &ModbusAssistant::onAddressChanged);
    connect(ui->spinBoxPort,
            static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this,
            &ModbusAssistant::onPortChanged);
}

void ModbusAssistant::initSignalsSerialPort()
{
    connect(ui->comboBoxPortName,
            &QComboBox::currentTextChanged,
            this,
            &ModbusAssistant::onPortNameChanged);
    connect(ui->comboBoxParity,
            static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this,
            &ModbusAssistant::onParityChanged);
    connect(ui->comboBoxBaudRate,
            &QComboBox::currentTextChanged,
            this,
            &ModbusAssistant::onBaudRateChanged);
    connect(ui->comboBoxDataBits,
            static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this,
            &ModbusAssistant::onDataBitsChanged);
    connect(ui->comboBoxStopBits,
            static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this,
            &ModbusAssistant::onStopBistChanged);
}

void ModbusAssistant::initSignalsClient()
{
    connect(ui->spinBoxTimeout,
            static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this,
            &ModbusAssistant::onClientTimeoutChanged);
    connect(ui->spinBoxRepeatTime,
            static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this,
            &ModbusAssistant::onClientRepeatTimeChanged);
}

void ModbusAssistant::initSignalsServer()
{
    connect(ui->checkBoxDeviceBusy,
            &QCheckBox::clicked,
            this,
            &ModbusAssistant::onServerIsBusyChanged);
    connect(ui->checkBoxListenOnlyMode,
            &QCheckBox::clicked,
            this,
            &ModbusAssistant::onServerJustListenChanged);
    connect(ui->spinBoxServerAddress,
            static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this,
            &ModbusAssistant::onServerAddressChanged);
}

void ModbusAssistant::initSignalsClientOperations()
{
    connect(ui->comboBoxFunctionCode,
            &QComboBox::currentTextChanged,
            this,
            &ModbusAssistant::onFunctionCodeChanged);
    connect(ui->spinBoxDeviceAddress,
            static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this,
            &ModbusAssistant::onTargetAddressChanged);
    connect(ui->spinBoxStartAddress,
            static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this,
            &ModbusAssistant::onStartAddressChanged);
    connect(ui->spinBoxQuantity,
            static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this,
            &ModbusAssistant::onAddressNumberChanged);
    connect(ui->pushButtonRead, &QPushButton::clicked, this, &ModbusAssistant::onReadClicked);
    connect(ui->pushButtonWrite, &QPushButton::clicked, this, &ModbusAssistant::onWriteClicked);
    connect(ui->send_, &QPushButton::clicked, this, &ModbusAssistant::onSendClicked);
}

void ModbusAssistant::onErrorOccurred()
{
    outputMessage(m_modbusDevice->errorString(), true, "", "error");
    if (m_modbusDevice->error() == QModbusDevice::ConnectionError) {
        const QString msg = m_modbusDevice->errorString();
        onCloseClicked();
        QMessageBox::warning(this, tr("Error occured"), msg);
    }
}

void ModbusAssistant::onDeviceTypeChanged()
{
    int type = ui->comboBoxDeviceList->currentData().toInt();
    bool isSerial = (type == ModbusFactory::ModbusRtuSerialClient
                     || type == ModbusFactory::ModbusRtuSerialServer);
    bool isClient = (type == ModbusFactory::ModbusRtuSerialClient
                     || type == ModbusFactory::ModbusTcpClient);

    // Hide ui component first then show ui component,
    // or the window will be resize to the max size of default.
    if (isSerial) {
        ui->widgetNetworkSettings->setHidden(true);
        ui->widgetSerialPortSettings->setHidden(false);
    } else {
        ui->widgetSerialPortSettings->setHidden(true);
        ui->widgetNetworkSettings->setHidden(false);
    }

    if (isClient) {
        ui->widgetConsole->setVisible(true);
        ui->widgetServerParameters->setHidden(true);
        ui->widgetServerRegisters->setHidden(true);
        ui->widgetClientParameters->setHidden(false);
        ui->widgetClientRegisters->setHidden(false);
    } else {
        ui->widgetConsole->setVisible(false);
        ui->widgetClientRegisters->setHidden(true);
        ui->widgetClientParameters->setHidden(true);
        ui->widgetServerParameters->setHidden(false);
        ui->widgetServerRegisters->setHidden(false);
    }

    m_settings->setValue(m_keyCtx->deviceIndex, type);
}

void ModbusAssistant::onCloseClicked()
{
    ModbusFactory::Instance()->DeleteModbusDevuce(&m_modbusDevice);
    updateUiState(false);
}

void ModbusAssistant::onOpenClicked()
{
    ui->pushButtonOpen->setEnabled(false);
    ModbusFactory::Instance()->DeleteModbusDevuce(&m_modbusDevice);

    m_modbusDevice = CreateModbusDevice();

    if (ModbusFactory::Instance()->IsServerDevice(m_modbusDevice)) {
        if (!updateServerMap(m_modbusDevice)) {
            ui->pushButtonOpen->setEnabled(true);
            qWarning() << "Can not reset server map!";
            return;
        }

        updateServerParameters();

        QModbusServer *server = qobject_cast<QModbusServer *>(m_modbusDevice);
        updateServerRegistersData();
        connect(server, &QModbusServer::dataWritten, this, &ModbusAssistant::onDateWritten);
    } else if (ModbusFactory::Instance()->IsClientDevice(m_modbusDevice)) {
        updateClientParameters();
    } else {
        ui->pushButtonOpen->setEnabled(true);
        qWarning() << "Can not create modbus devices!";
        return;
    }

    connect(m_modbusDevice, &QModbusDevice::errorOccurred, this, &ModbusAssistant::onErrorOccurred);
    ModbusFactory *factory = ModbusFactory::Instance();
    bool connected = factory->ConnectDeivce(m_modbusDevice);
    if (!connected) {
        QString errStr = m_modbusDevice->errorString();
        QString info = tr("Can not open device: %1."
                          "Please check the parameters and try again!")
                           .arg(errStr);
        QMessageBox::warning(this, tr("Can not open device"), info);
    }

    updateUiState(connected);
}

void ModbusAssistant::onAddressChanged()
{
    m_settings->setValue(m_keyCtx->address, ui->comboBoxAddress->currentText());
}

void ModbusAssistant::onPortChanged()
{
    m_settings->setValue(m_keyCtx->port, ui->spinBoxPort->value());
}

void ModbusAssistant::onPortNameChanged()
{
    m_settings->setValue(m_keyCtx->portName, ui->comboBoxPortName->currentText());
}

void ModbusAssistant::onParityChanged()
{
    m_settings->setValue(m_keyCtx->parity, ui->comboBoxParity->currentIndex());
}

void ModbusAssistant::onBaudRateChanged()
{
    m_settings->setValue(m_keyCtx->baudRate, ui->comboBoxBaudRate->currentText());
}

void ModbusAssistant::onDataBitsChanged()
{
    m_settings->setValue(m_keyCtx->dataBits, ui->comboBoxDataBits->currentIndex());
}

void ModbusAssistant::onStopBistChanged()
{
    m_settings->setValue(m_keyCtx->stopBits, ui->comboBoxStopBits->currentIndex());
}

void ModbusAssistant::onInvokeRefresh()
{
    initComponentPortName();
}

void ModbusAssistant::onClientTimeoutChanged()
{
    m_settings->setValue(m_keyCtx->clientTimeout, ui->spinBoxTimeout->value());
    updateClientParameters();
}

void ModbusAssistant::onClientRepeatTimeChanged()
{
    m_settings->setValue(m_keyCtx->clientRepeatTime, ui->spinBoxRepeatTime->value());
    updateClientParameters();
}

void ModbusAssistant::onServerIsBusyChanged()
{
    m_settings->setValue(m_keyCtx->serverIsBusy, ui->checkBoxDeviceBusy->isChecked());
    updateServerParameters();
}

void ModbusAssistant::onServerJustListenChanged()
{
    m_settings->setValue(m_keyCtx->serverJustListen, ui->checkBoxListenOnlyMode->isChecked());
    updateServerParameters();
}

void ModbusAssistant::onServerAddressChanged()
{
    m_settings->setValue(m_keyCtx->serverAddress, ui->spinBoxServerAddress->value());
    updateServerParameters();
}

void ModbusAssistant::onFunctionCodeChanged()
{
    m_settings->setValue(m_keyCtx->functionCode, ui->comboBoxFunctionCode->currentIndex());
    updateClientReadWriteButtonState();
}

void ModbusAssistant::onTargetAddressChanged()
{
    m_settings->setValue(m_keyCtx->targetAddress, ui->spinBoxDeviceAddress->value());
}

void ModbusAssistant::onStartAddressChanged()
{
    m_settings->setValue(m_keyCtx->startAddress, ui->spinBoxStartAddress->value());
    updateClientTableView();
}

void ModbusAssistant::onAddressNumberChanged()
{
    m_settings->setValue(m_keyCtx->addressNumber, ui->spinBoxQuantity->value());
    updateClientTableView();
}

void ModbusAssistant::onReadClicked()
{
    if (!isConnected()) {
        return;
    }

    if (!ModbusFactory::Instance()->IsClientDevice(m_modbusDevice)) {
        return;
    }

    int register_type = ui->comboBoxFunctionCode->currentData().toInt();
    quint16 start_address = ui->spinBoxStartAddress->value();
    quint16 quantity = ui->spinBoxQuantity->value();
    quint16 spinBoxServerAddress = ui->spinBoxDeviceAddress->value();
    quint8 function_code = getClientFunctionCode();

    qInfo() << "[SendReadRequest]"
            << "register type:" << register_type << "start address:" << start_address
            << "quantity:" << quantity << "server address:" << spinBoxServerAddress;

    typedef QModbusDataUnit::RegisterType RegisterType;
    RegisterType type = static_cast<RegisterType>(register_type);
    QModbusDataUnit data_unit(type, start_address, quantity);
    QModbusClient *client = qobject_cast<QModbusClient *>(m_modbusDevice);
    QModbusReply *reply = client->sendReadRequest(data_unit, spinBoxServerAddress);
    if (!ModbusFactory::Instance()->IsValidModbusReply(reply)) {
        return;
    }

    QString info = ui->comboBoxFunctionCode->currentText();
    outputMessage(info, false, TXCOLOR, TXFLAG);
    connect(reply, &QModbusReply::finished, this, [=]() {
        outputModbusReply(reply, function_code);

        if (reply->error() == QModbusDevice::NoError) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
            updateClientTableViewData(reply->result().values());
#else
            updateClientTableViewData(vectorTolist(reply->result().values()));
#endif
            reply->deleteLater();
        }
    });
}

void ModbusAssistant::onWriteClicked()
{
    if (!isConnected()) {
        return;
    }

    int registerType = ui->comboBoxFunctionCode->currentData().toInt();
    int start_address = ui->spinBoxStartAddress->value();
    int spinBoxServerAddress = ui->spinBoxDeviceAddress->value();
    quint8 function_code = getClientFunctionCode();
    QList<quint16> values = getClientRegisterValue();
    ModbusFactory *factory = ModbusFactory::Instance();
    QModbusReply *reply = factory->SendWriteRequest(m_modbusDevice,
                                                    registerType,
                                                    start_address,
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
                                                    values,
#else
                                                    listToVector(values),
#endif
                                                    spinBoxServerAddress);
    if (ModbusFactory::Instance()->IsValidModbusReply(reply)) {
        connect(reply, &QModbusReply::finished, this, [=]() {
            outputModbusReply(reply, function_code);
            reply->deleteLater();
        });

        QString info = ui->comboBoxFunctionCode->currentText();
        outputMessage(info, false, TXCOLOR, TXFLAG);
    }
}

void ModbusAssistant::onSendClicked()
{
    if (!isConnected()) {
        return;
    }

    quint8 spinBoxServerAddress = ui->spinBoxDeviceAddress->value();
    QByteArray pdu = getClientPdu();
    QByteArray data = pdu;
    if (!data.isEmpty()) {
        data = data.remove(0, 1);
    }

    int function_code = int(QModbusDataUnit::Invalid);
    if (!pdu.isEmpty()) {
        function_code = pdu.at(0);
    }
    ModbusFactory *factory = ModbusFactory::Instance();
    QModbusReply *reply = factory->SendRawRequest(m_modbusDevice,
                                                  spinBoxServerAddress,
                                                  function_code,
                                                  data);

    qWarning() << "Send raw request:"
               << "server address:" << spinBoxServerAddress << "function code:" << function_code
               << "data:" << QString(xTools::xTools::byteArray2Hex(pdu, ' '));
    if (ModbusFactory::Instance()->IsValidModbusReply(reply)) {
        connect(reply, &QModbusReply::finished, this, [=]() {
            outputModbusReply(reply, function_code);
            reply->deleteLater();
        });

        QString info = "pdu(No server address, no crc):";
        info += QString(xTools::xTools::byteArray2Hex(pdu, ' '));
        outputMessage(info, false, TXCOLOR, TXFLAG);
    }

    // Update settings data.
    int index = m_settings->value(m_keyCtx->sendHistoryIndex).toInt();
    bool ret = writeSettingsArray(m_keyCtx->sendHistory,
                                  m_keyCtx->pdu,
                                  QString(xTools::xTools::byteArray2Hex(pdu, ' ')),
                                  index,
                                  MAX_HISTORY_INDEX);
    if (!ret) {
        return;
    }
#if 0
    if (index > ui->comboBoxPdu->count()) {
        ui->comboBoxPdu->addItem(QString(xTools::xTools::byteArrray2Hex(pdu, ' ')));
    } else {
        ui->comboBoxPdu->insertItem(index, QString(xTools::xTools::byteArrray2Hex(pdu, ' ')));
    }
#endif

    index = index + 1 > MAX_HISTORY_INDEX ? 0 : index + 1;
    m_settings->setValue(m_keyCtx->sendHistoryIndex, index);
}

void ModbusAssistant::onDateWritten(QModbusDataUnit::RegisterType table, int address, int size)
{
    qInfo() << "Data written:"
            << "table:" << table << "start address:" << address << "size:" << size;
    QTableView *tv = getTableView(table);
    QStandardItemModel *model = qobject_cast<QStandardItemModel *>(tv->model());
    QModbusServer *server = qobject_cast<QModbusServer *>(m_modbusDevice);
    QList<quint16> data = ModbusFactory::Instance()->GetServerData(server, table, address, size);
    size = qMin<int>(data.count(), size);
    for (int i = 0; i < size; i++) {
        int row = address + i;
        int base = 16;
        int width = base == 2 ? 16 : (base == 10 ? 5 : 4);
        int value = data.at(i);
        QString cooked_str = QString::number(value, base);
        cooked_str = cooked_str.rightJustified(width, '0', true);
        QStandardItem *item = model->item(row, 1);
        if (item) {
            item->setData(cooked_str, Qt::DisplayRole);
            item->setTextAlignment(Qt::AlignCenter);
        }
    }

    tv->viewport()->update();
}

void ModbusAssistant::onItemChanged(QStandardItem *item)
{
    if (!item) {
        return;
    }

    if (item->column() != 1) {
        return;
    }

    if (ModbusFactory::Instance()->IsServerDevice(m_modbusDevice)) {
        int address = item->row();
        int current_index = ui->tabWidgetServerRegisters->currentIndex();
        QModbusDataUnit::RegisterType table = QModbusDataUnit::Invalid;
        if (current_index == 0) {
            table = QModbusDataUnit::Coils;
        } else if (current_index == 1) {
            table = QModbusDataUnit::DiscreteInputs;
        } else if (current_index == 2) {
            table = QModbusDataUnit::InputRegisters;
        } else if (current_index == 3) {
            table = QModbusDataUnit::HoldingRegisters;
        } else {
            Q_ASSERT_X(false, __FUNCTION__, "Unknow table type!");
            return;
        }

        quint16 value = item->text().toInt(Q_NULLPTR, 16);
        ModbusFactory::Instance()->SetServerData(m_modbusDevice, table, address, value);
    }
}

QModbusDevice *ModbusAssistant::CreateModbusDevice()
{
    QModbusDevice *device = Q_NULLPTR;
    int type = ui->comboBoxDeviceList->currentData().toInt();
    if (ModbusFactory::Instance()->IsRtuSerialDeviceType(type)) {
        QString port_name = ui->comboBoxPortName->currentText();
        int parity = ui->comboBoxParity->currentData().toInt();
        int baud_rate = ui->comboBoxBaudRate->currentData().toInt();
        int data_bits = ui->comboBoxDataBits->currentData().toInt();
        int stop_bits = ui->comboBoxStopBits->currentData().toInt();
        ModbusFactory *factory = ModbusFactory::Instance();
        device = factory->CreateRtuSerialDevice(type,
                                                port_name,
                                                parity,
                                                baud_rate,
                                                data_bits,
                                                stop_bits);
    } else if (ModbusFactory::Instance()->IsTcpDeviceType(type)) {
        QString address = ui->comboBoxAddress->currentText();
        int port = ui->spinBoxPort->value();
        ModbusFactory *factory = ModbusFactory::Instance();
        device = factory->CreateTcpDevice(type, address, port);
    } else {
        Q_ASSERT_X(false, __FUNCTION__, "Unknown device type");
    }

    return device;
}

QTableView *ModbusAssistant::CreateTableView(int rowCount, QTableView *tableView)
{
    if (!tableView) {
        tableView = new QTableView(this);
    }

    QHeaderView *hv = tableView->horizontalHeader();
    QStandardItemModel *model = new QStandardItemModel(tableView);
    QStringList labels = QStringList() << tr("Address") << tr("Value") << tr("Description");
    model->setHorizontalHeaderLabels(labels);
    model->setColumnCount(3);
    model->setRowCount(rowCount);
    tableView->setModel(model);
    tableView->verticalHeader()->hide();
    tableView->setItemDelegateForColumn(0, new ReadOnlyDelegate(tableView));
    updateClientTableViewAddress(tableView, 0);
    hv->setStretchLastSection(true);

    // Set the default value to 0.
    model->blockSignals(true);
    for (int row = 0; row < rowCount; row++) {
        QModelIndex index = model->index(row, 1);
        QMap<int, QVariant> roles;
        roles.insert(Qt::DisplayRole, "0000");
        model->setItemData(index, roles);

        QStandardItem *item = model->item(row, 1);
        item->setTextAlignment(Qt::AlignCenter);
    }
    model->blockSignals(false);

    connect(model, &QStandardItemModel::itemChanged, this, &ModbusAssistant::onItemChanged);

    return tableView;
}

void ModbusAssistant::updateUiState(bool connected)
{
    ui->comboBoxDeviceList->setEnabled(!connected);
    ui->pushButtonClose->setEnabled(connected);
    ui->pushButtonOpen->setEnabled(!connected);
    ui->widgetNetworkSettings->setEnabled(!connected);
    ui->widgetSerialPortSettings->setEnabled(!connected);
#if 0
    ui->clientGroupBox->setEnabled(!connected);
    ui->widgetServerParameters->setEnabled(!connected);
#endif
}

void ModbusAssistant::updateClientTableView()
{
    int number = ui->spinBoxQuantity->value();
    int rowCount = m_registerModel->rowCount();
    if (number > rowCount) {
        m_registerModel->insertRows(rowCount, number - rowCount);
    } else if (number < rowCount) {
        m_registerModel->removeRows(number, rowCount - number);
    }

    // Update address.
    int start_address = ui->spinBoxStartAddress->value();
    updateClientTableViewAddress(ui->tabViewClientRegisters, start_address);
}

void ModbusAssistant::updateClientTableViewData(const QList<quint16> &values)
{
    for (int row = 0; row < values.count(); row++) {
        int value = values.at(row);
        QModelIndex index = m_registerModel->index(row, 1);
        QMap<int, QVariant> roles;
        QString str = QString("%1").arg(QString::number(value, 16), 4, '0');
        roles.insert(Qt::DisplayRole, str);
        m_registerModel->setItemData(index, roles);
        QStandardItem *item = m_registerModel->item(row, 1);
        if (item) {
            item->setTextAlignment(Qt::AlignCenter);
        }
    }

    // Refresh the view, or the new value will not be show.
    ui->tabViewClientRegisters->viewport()->update();
}

void ModbusAssistant::updateClientReadWriteButtonState()
{
    QStringList list = ui->comboBoxFunctionCode->currentText().split('-');
    int code = list.length() ? list.first().toInt(Q_NULLPTR, 16) : 0;
    bool is_reading_operation = false;
    if (code == 0x01 || code == 0x02 || code == 0x03 || code == 0x04) {
        is_reading_operation = true;
    }

    if (code == 0x05 || code == 0x06) {
        ui->spinBoxQuantity->setValue(1);
    }

    ui->pushButtonRead->setEnabled(is_reading_operation);
    ui->pushButtonWrite->setEnabled(!is_reading_operation);
}

void ModbusAssistant::updateClientParameters()
{
    int timeout = ui->spinBoxTimeout->value();
    int repeat_time = ui->spinBoxRepeatTime->value();
    ModbusFactory::Instance()->SetClientDeviceParameters(m_modbusDevice, timeout, repeat_time);
}

void ModbusAssistant::updateClientTableViewAddress(QTableView *view, int start_address)
{
    auto *model = qobject_cast<QStandardItemModel *>(view->model());
    for (int row = 0; row < model->rowCount(); row++) {
        int address = row + start_address;
        QString text = QString("%1").arg(QString::number(address), 5, '0');
        QModelIndex index = model->index(row, 0);
        QMap<int, QVariant> roles;
        roles.insert(Qt::DisplayRole, text);
        model->setItemData(index, roles);

        QStandardItem *item = model->item(row, 0);
        if (item) {
            item->setTextAlignment(Qt::AlignCenter);
        }
    }
}

void ModbusAssistant::updateServerParameters()
{
    bool device_busy = ui->checkBoxDeviceBusy->isChecked();
    bool listen_only_mode = ui->checkBoxListenOnlyMode->isChecked();
    int address = ui->spinBoxServerAddress->value();
    ModbusFactory::Instance()->SetServerDeviceParameters(m_modbusDevice,
                                                         address,
                                                         device_busy,
                                                         listen_only_mode);
}

bool ModbusAssistant::updateServerMap(QModbusDevice *server)
{
    if (server && qobject_cast<QModbusServer *>(server)) {
        QVector<quint16> values(UINT16_MAX + 1, 0);
        QModbusDataUnit dataUnit(QModbusDataUnit::Coils, 0, values);

        QModbusDataUnitMap map;
        map.insert(QModbusDataUnit::Coils, dataUnit);
        map.insert(QModbusDataUnit::DiscreteInputs, dataUnit);
        map.insert(QModbusDataUnit::HoldingRegisters, dataUnit);
        map.insert(QModbusDataUnit::InputRegisters, dataUnit);

        QModbusServer *cooked_server = qobject_cast<QModbusServer *>(server);
        cooked_server->blockSignals(true);
        bool is_ok = cooked_server->setMap(map);
        cooked_server->blockSignals(false);
        return is_ok;
    }

    return false;
}

void ModbusAssistant::updateServerRegistersData()
{
    for (int i = 0; i < 4; i++) {
        QWidget *widget = ui->tabWidgetServerRegisters->widget(i);
        QTableView *table_view = qobject_cast<QTableView *>(widget);
        auto *model = qobject_cast<QStandardItemModel *>(table_view->model());
        int type = QModbusDataUnit::Invalid;
        if (i == 0) {
            type = QModbusDataUnit::Coils;
        } else if (i == 1) {
            type = QModbusDataUnit::DiscreteInputs;
        } else if (i == 2) {
            type = QModbusDataUnit::InputRegisters;
        } else if (i == 3) {
            type = QModbusDataUnit::HoldingRegisters;
        } else {
            qWarning() << "Unknown register type.";
            continue;
        }

        for (int row = 0; row < model->rowCount(); row++) {
            QStandardItem *item = model->item(row, 1);
            quint16 value = item ? item->text().toInt(Q_NULLPTR, 16) : 0;
            auto table = static_cast<QModbusDataUnit::RegisterType>(type);
            ModbusFactory::Instance()->SetServerData(m_modbusDevice, table, row, value, false);
        }
    }
}

quint8 ModbusAssistant::getClientFunctionCode()
{
    QString txt = ui->comboBoxFunctionCode->currentText();
    QStringList list = txt.split('-', xSkipEmptyParts);
    if (list.length()) {
        return list.first().toInt(Q_NULLPTR, 16);
    }

    return 0;
}

QList<quint16> ModbusAssistant::getClientRegisterValue()
{
    QList<quint16> values;
    for (int row = 0; row < m_registerModel->rowCount(); row++) {
        QStandardItem *item = m_registerModel->item(row, 1);
        if (item) {
            QString text = item->text();
            values.append(text.toInt(Q_NULLPTR, 16));
        } else {
            values.append(0);
        }
    }

    return values;
}

QByteArray ModbusAssistant::getClientPdu()
{
    QString text = ui->lineEditPdu->text();
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
    QStringList valueList = text.split(' ', QString::SkipEmptyParts);
#else
    QStringList valueList = text.split(' ', Qt::SkipEmptyParts);
#endif
    QByteArray data;
    for (QString &value : valueList) {
        data.append(char(value.toInt(Q_NULLPTR, 16)));
    }

    return data;
}

QTableView *ModbusAssistant::getTableView(QModbusDataUnit::RegisterType table)
{
    QWidget *tv = Q_NULLPTR;
    if (table == QModbusDataUnit::Coils) {
        tv = ui->tabWidgetServerRegisters->widget(0);
    } else if (table == QModbusDataUnit::DiscreteInputs) {
        tv = ui->tabWidgetServerRegisters->widget(1);
    } else if (table == QModbusDataUnit::InputRegisters) {
        tv = ui->tabWidgetServerRegisters->widget(2);
    } else if (table == QModbusDataUnit::HoldingRegisters) {
        tv = ui->tabWidgetServerRegisters->widget(3);
    } else {
        tv = ui->tabWidgetServerRegisters->widget(3);
        qWarning() << "Get table view failed: unknow register type!";
    }

    return qobject_cast<QTableView *>(tv);
}

QList<quint16> ModbusAssistant::getTableValues(QTableView *tableView, int row, int count)
{
    if (!tableView) {
        qWarning() << "Table view can not be null!";
        return QList<quint16>();
    }

    auto *model = qobject_cast<QStandardItemModel *>(tableView->model());
    if (!model) {
        qWarning() << "Model can not be null!";
        return QList<quint16>();
    }

    QList<quint16> values;
    for (int i = row; i < count; i++) {
        QStandardItem *item = model->item(i, 1);
        if (item) {
            QString text = item->text();
            values.append(text.toInt(Q_NULLPTR, 16));
        } else {
            values.append(0);
        }
    }

    return values;
}

void ModbusAssistant::outputModbusReply(QModbusReply *reply, int functionCode)
{
    if (reply->error() != QModbusDevice::NoError) {
        outputMessage(reply->errorString(), true, "", "error");
        QMessageBox::warning(this, tr("Error occured"), reply->errorString());
        return;
    }

    int spinBoxServerAddress = reply->serverAddress();
    if (reply->type() == QModbusReply::ReplyType::Raw) {
        QByteArray data = reply->rawResult().data();
        QString info = QString("(from address: %1, "
                               "function code: %2), "
                               "data unit: %3")
                           .arg(spinBoxServerAddress)
                           .arg(functionCode)
                           .arg(QString::fromLatin1(xTools::xTools::byteArray2Hex(data, ' ')));
        outputMessage(info, false, RXCOLOR, RXFLAG);
    } else if (reply->type() == QModbusReply::ReplyType::Common) {
        QString info = ui->comboBoxFunctionCode->currentText();
        outputMessage(info, false, RXCOLOR, RXFLAG);
    }
}

void ModbusAssistant::outputMessage(const QString &msg,
                                    bool isError,
                                    const QString &color,
                                    const QString &flag)
{
    QString cookedMsg = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
    cookedMsg = QString("<font color=silver>%1 </font>").arg(cookedMsg);

    QString cookedColor = color;
    if (cookedColor.isEmpty()) {
        if (isError) {
            cookedColor = "red";
        } else {
            cookedColor = "black";
        }
    }

    cookedMsg += flag.isEmpty() ? ""
                                : QString("<font color=%1>[%2]</font> ").arg(cookedColor, flag);
    cookedMsg += msg;
    ui->textBrowser->append(cookedMsg);
}

bool ModbusAssistant::isConnected()
{
    if (ModbusFactory::Instance()->IsConnected(m_modbusDevice)) {
        return true;
    }

    QMessageBox::warning(this,
                         tr("Device is not Ready"),
                         tr("The modbus device is not ready, "
                            "please check settings and try again later!"));
    return false;
}

bool ModbusAssistant::writeSettingsArray(
    const QString &group, const QString &key, const QString &value, int index, int maxIndex)
{
    m_settings->beginWriteArray(group);
    for (int i = 0; i < maxIndex; i++) {
        m_settings->setArrayIndex(i);
        QString v = m_settings->value(key).toString();
        if (v == value) {
            m_settings->endArray();
            return false;
        }
    }

    m_settings->setArrayIndex(index);
    m_settings->setValue(key, value);
    m_settings->endArray();
    return true;
}
