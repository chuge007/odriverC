#include "odrivemotorcontroller.h"

#include <QByteArray>
#include <QCanBus>
#include <QCanBusFrame>
#include <QDateTime>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QList>
#include <QProcess>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QCoreApplication>
#include <QtEndian>
#include <QtGlobal>

#include <algorithm>
#include <cstring>

namespace {
QString zh(const char *hex) { return QString::fromUtf8(QByteArray::fromHex(hex)); }
QList<quint8> normalizeNodeIds(const QList<quint8> &ids, quint8 fallback)
{
    QList<quint8> out;
    for (quint8 raw : ids) {
        quint8 id = static_cast<quint8>(raw & 0x3F);
        if (!out.contains(id)) out.append(id);
    }
    if (out.isEmpty()) out.append(static_cast<quint8>(fallback & 0x3F));
    std::sort(out.begin(), out.end());
    return out;
}

QString formatCanId(quint32 frameId)
{
    return QStringLiteral("0x%1").arg(frameId, 3, 16, QLatin1Char('0')).toUpper();
}

QString formatPayload(const QByteArray &payload)
{
    return payload.isEmpty() ? QStringLiteral("<empty>") : QString::fromLatin1(payload.toHex(' ')).toUpper();
}

QString formatAscii(const QByteArray &bytes)
{
    if (bytes.isEmpty()) {
        return QStringLiteral("<empty>");
    }

    QString out;
    out.reserve(bytes.size());
    for (unsigned char b : bytes) {
        if (b == '\r') {
            out += QStringLiteral("\\r");
            continue;
        }
        if (b == '\n') {
            out += QStringLiteral("\\n");
            continue;
        }
        if (b == '\t') {
            out += QStringLiteral("\\t");
            continue;
        }
        if (b >= 0x20 && b <= 0x7E) {
            out += QChar(char(b));
            continue;
        }
        out += QChar('.');
    }
    return out;
}

bool isLikelySerialInterfaceName(const QString &interfaceName)
{
    const QString trimmed = interfaceName.trimmed();
    if (trimmed.isEmpty()) {
        return false;
    }

    return trimmed.startsWith(QStringLiteral("COM"), Qt::CaseInsensitive)
            || trimmed.startsWith(QStringLiteral("/dev/"), Qt::CaseInsensitive)
            || trimmed.startsWith(QStringLiteral("tty"), Qt::CaseInsensitive)
            || trimmed.startsWith(QStringLiteral("cu."), Qt::CaseInsensitive);
}

bool isLikelyCandleInterfaceName(const QString &interfaceName)
{
    return interfaceName.trimmed().startsWith(QStringLiteral("candle"), Qt::CaseInsensitive);
}

bool runSlcanCommand(QSerialPort *serialPort,
                     const QByteArray &command,
                     QByteArray *response,
                     QString *errorMessage,
                     int timeoutMs,
                     bool requireAck)
{
    if (!serialPort || !serialPort->isOpen()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("SLCAN serial port is not open.");
        }
        return false;
    }

    if (response) {
        response->clear();
    }

    serialPort->readAll();
    const qint64 written = serialPort->write(command);
    if (written != command.size()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("SLCAN write failed: %1").arg(serialPort->errorString());
        }
        return false;
    }
    if (!serialPort->waitForBytesWritten(timeoutMs)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("SLCAN write timeout: %1").arg(serialPort->errorString());
        }
        return false;
    }

    QByteArray localResponse;
    const qint64 deadline = QDateTime::currentMSecsSinceEpoch() + timeoutMs;
    while (QDateTime::currentMSecsSinceEpoch() < deadline) {
        const int remainingMs = static_cast<int>(deadline - QDateTime::currentMSecsSinceEpoch());
        if (remainingMs <= 0) {
            break;
        }
        if (!serialPort->waitForReadyRead(remainingMs)) {
            continue;
        }
        localResponse.append(serialPort->readAll());
        if (localResponse.contains('\r') || localResponse.contains('\a')) {
            break;
        }
    }

    if (response) {
        *response = localResponse;
    }

    if (localResponse.contains('\a')) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("SLCAN command rejected: %1")
                    .arg(QString::fromLatin1(localResponse).trimmed());
        }
        return false;
    }

    if (requireAck && !localResponse.contains('\r')) {
        if (errorMessage) {
            *errorMessage = localResponse.trimmed().isEmpty()
                    ? QStringLiteral("SLCAN command timed out without acknowledgement.")
                    : QStringLiteral("Unexpected SLCAN response: %1")
                          .arg(QString::fromLatin1(localResponse).trimmed());
        }
        return false;
    }

    return true;
}
}

ODriveMotorController::ODriveMotorController(QObject *parent)
    : QObject(parent), m_device(nullptr), m_serialPort(nullptr), m_candleProcess(nullptr), m_nodeId(0), m_connected(false)
{
    m_trackedNodeIds << m_nodeId;
    m_pollTimer.setInterval(800);
    connect(&m_pollTimer, &QTimer::timeout, this, [this]() { requestTrackedTelemetry(true); });
    m_slcanReadTimer.setInterval(5);
    connect(&m_slcanReadTimer, &QTimer::timeout, this, &ODriveMotorController::readSlcanFrames);
}

ODriveMotorController::~ODriveMotorController() { disconnectDevice(); }

QStringList ODriveMotorController::availablePlugins() const
{
    QStringList plugins = QCanBus::instance()->plugins();
    plugins.prepend(QStringLiteral("auto"));
    plugins << QStringLiteral("slcan");
    plugins << QStringLiteral("candle");
    plugins.removeDuplicates();
    std::sort(plugins.begin(), plugins.end());
    return plugins;
}

QStringList ODriveMotorController::availableInterfaces(const QString &plugin, QString *errorMessage) const
{
    QStringList interfaces;
    if (plugin.trimmed().isEmpty()) {
        if (errorMessage) errorMessage->clear();
        return interfaces;
    }
    if (plugin == QStringLiteral("auto")) {
        for (const QSerialPortInfo &info : QSerialPortInfo::availablePorts()) {
            interfaces << info.portName();
        }
        interfaces << QStringLiteral("candle0");
        interfaces.removeDuplicates();
        std::sort(interfaces.begin(), interfaces.end());
        if (errorMessage) {
            *errorMessage = interfaces.isEmpty()
                    ? QStringLiteral("No serial ports or candleLight adapters detected for AUTO mode.")
                    : QString();
        }
        return interfaces;
    }
    if (plugin == QStringLiteral("slcan")) {
        for (const QSerialPortInfo &info : QSerialPortInfo::availablePorts()) interfaces << info.portName();
        interfaces.removeDuplicates();
        std::sort(interfaces.begin(), interfaces.end());
        if (errorMessage) *errorMessage = interfaces.isEmpty() ? QStringLiteral("No serial ports detected for SLCAN.") : QString();
        return interfaces;
    }
    if (plugin == QStringLiteral("candle")) {
        interfaces << QStringLiteral("candle0");
        if (errorMessage) *errorMessage = QString();
        return interfaces;
    }
    QString localError;
    for (const QCanBusDeviceInfo &info : QCanBus::instance()->availableDevices(plugin, &localError)) interfaces << info.name();
    interfaces.removeDuplicates();
    std::sort(interfaces.begin(), interfaces.end());
    if (errorMessage) *errorMessage = localError;
    return interfaces;
}

bool ODriveMotorController::connectDevice(const QString &plugin,
                                          const QString &interfaceName,
                                          int bitrate,
                                          int serialBaudRate,
                                          quint8 nodeId)
{
    disconnectDevice();
    const QString requestedPlugin = plugin.trimmed().isEmpty() ? QStringLiteral("auto") : plugin.trimmed();
    const QString requestedInterface = interfaceName.trimmed();
    m_nodeId = static_cast<quint8>(nodeId & 0x3F);
    m_trackedNodeIds = normalizeNodeIds(m_trackedNodeIds, m_nodeId);
    m_statusByNode.clear();
    m_activePlugin.clear();

    auto finishSlcanConnect = [&](const QString &serialInterface) {
        m_activePlugin = QStringLiteral("slcan");
        if (!connectSlcanDevice(serialInterface, bitrate, serialBaudRate)) {
            m_activePlugin.clear();
            return false;
        }
        requestTrackedTelemetry();
        return true;
    };

    auto finishCandleConnect = [&](const QString &candleInterface) {
        m_activePlugin = QStringLiteral("candle");
        if (!connectCandleDevice(candleInterface, bitrate)) {
            m_activePlugin.clear();
            return false;
        }
        requestTrackedTelemetry();
        return true;
    };

    if (requestedPlugin == QStringLiteral("auto")) {
        QStringList serialCandidates;
        if (requestedInterface.isEmpty()) {
            for (const QSerialPortInfo &info : QSerialPortInfo::availablePorts()) {
                serialCandidates << info.portName();
            }
        } else if (isLikelySerialInterfaceName(requestedInterface) || !isLikelyCandleInterfaceName(requestedInterface)) {
            serialCandidates << requestedInterface;
        }

        serialCandidates.removeDuplicates();
        for (const QString &serialInterface : serialCandidates) {
            if (finishSlcanConnect(serialInterface)) {
                emit logMessage(QStringLiteral("AUTO connected using SLCAN on %1").arg(serialInterface));
                return true;
            }
        }

        const QString candleInterface = requestedInterface.isEmpty() ? QStringLiteral("candle0") : requestedInterface;
        if ((requestedInterface.isEmpty() || isLikelyCandleInterfaceName(requestedInterface))
                && finishCandleConnect(candleInterface)) {
            emit logMessage(QStringLiteral("AUTO connected using candleLight on %1").arg(candleInterface));
            return true;
        }

        const QString msg = requestedInterface.isEmpty()
                ? QStringLiteral("AUTO connect failed. Set the adapter to CL00 USB<->CAN mode, then prefer SLCAN over the virtual COM port, or use candle if the adapter runs candleLight/gs_usb firmware.")
                : QStringLiteral("AUTO connect failed on %1. For this adapter, set CL00 USB<->CAN mode and try the virtual COM port via SLCAN first.")
                      .arg(requestedInterface);
        emit errorMessage(msg);
        emit logMessage(msg);
        return false;
    }

    if (requestedPlugin == QStringLiteral("slcan")) {
        return finishSlcanConnect(requestedInterface);
    }
    if (requestedPlugin == QStringLiteral("candle")) {
        return finishCandleConnect(requestedInterface.isEmpty() ? QStringLiteral("candle0") : requestedInterface);
    }

    m_activePlugin = requestedPlugin;
    QString errorString;
    m_device = QCanBus::instance()->createDevice(m_activePlugin, requestedInterface, &errorString);
    if (!m_device) {
        const QString msg = zh("e5889be5bbba2043414e20e8aebee5a487e5a4b1e8b4a5efbc9a2531").arg(errorString);
        emit errorMessage(msg); emit logMessage(msg); m_activePlugin.clear(); return false;
    }
    m_device->setConfigurationParameter(QCanBusDevice::BitRateKey, bitrate);
    m_device->setConfigurationParameter(QCanBusDevice::ReceiveOwnKey, false);
    connect(m_device, &QCanBusDevice::framesReceived, this, &ODriveMotorController::readFrames);
    connect(m_device, &QCanBusDevice::errorOccurred, this, &ODriveMotorController::handleDeviceError);
    connect(m_device, &QCanBusDevice::stateChanged, this, &ODriveMotorController::handleDeviceStateChange);
    if (!m_device->connectDevice()) {
        const QString msg = zh("e8bf9ee68ea52043414e20e8aebee5a487e5a4b1e8b4a5efbc9a2531").arg(m_device->errorString());
        emit errorMessage(msg); emit logMessage(msg); clearDevice(); m_activePlugin.clear(); return false;
    }
    handleDeviceStateChange(m_device->state());
    emit logMessage(QStringLiteral("CAN connected: plugin=%1, interface=%2, bitrate=%3").arg(m_activePlugin, requestedInterface).arg(bitrate));
    requestTrackedTelemetry();
    return true;
}

bool ODriveMotorController::connectSlcanDevice(const QString &interfaceName, int bitrate, int serialBaudRate)
{
    if (interfaceName.trimmed().isEmpty()) {
        const QString msg = QStringLiteral("SLCAN needs a serial interface name such as COM3.");
        emit errorMessage(msg);
        emit logMessage(msg);
        return false;
    }

    const QByteArray bitrateCmd = slcanBitrateCommand(bitrate);
    if (bitrateCmd.isEmpty()) {
        const QString msg = QStringLiteral("SLCAN unsupported bitrate: %1").arg(bitrate);
        emit errorMessage(msg); emit logMessage(msg); return false;
    }
    const int baudRate = serialBaudRate > 0 ? serialBaudRate : 1000000;

    QSerialPort *serialPort = new QSerialPort(this);
    serialPort->setPortName(interfaceName.trimmed());
    serialPort->setBaudRate(baudRate);
    serialPort->setDataBits(QSerialPort::Data8);
    serialPort->setParity(QSerialPort::NoParity);
    serialPort->setStopBits(QSerialPort::OneStop);
    serialPort->setFlowControl(QSerialPort::NoFlowControl);
    serialPort->setReadBufferSize(2048);

    if (!serialPort->open(QIODevice::ReadWrite)) {
        const QString msg = QStringLiteral("Failed to open SLCAN port %1 @ %2: %3")
                .arg(interfaceName)
                .arg(baudRate)
                .arg(serialPort->errorString());
        emit errorMessage(msg);
        emit logMessage(msg);
        delete serialPort;
        return false;
    }

    serialPort->flush();
    serialPort->clear(QSerialPort::AllDirections);

    // Mirror CANgaroo's Windows SLCAN bring-up sequence:
    // set UART to 1M, send bitrate command, then open with O\r\n.
    serialPort->write(bitrateCmd);
    serialPort->flush();
    serialPort->waitForBytesWritten(300);
    serialPort->write("O\r\n", 3);
    serialPort->flush();
    serialPort->waitForBytesWritten(300);

    m_serialPort = serialPort;
    m_slcanReadBuffer.clear();
    m_slcanProtocolMismatchWarned = false;
    connect(m_serialPort, &QSerialPort::errorOccurred, this, [this](QSerialPort::SerialPortError error) {
        if (!m_serialPort || error == QSerialPort::NoError) return;
        const QString msg = QStringLiteral("SLCAN serial error: %1").arg(m_serialPort->errorString());
        emit errorMessage(msg); emit logMessage(msg);
        if (error == QSerialPort::ResourceError || error == QSerialPort::DeviceNotFoundError) disconnectDevice();
    });
    setConnectedState(true);
    m_slcanReadTimer.start();
    m_pollTimer.start();
    emit logMessage(QStringLiteral("SLCAN connected (CANgaroo mode): interface=%1 serial=%2 can=%3")
                    .arg(interfaceName)
                    .arg(baudRate)
                    .arg(bitrate));
    return true;
}

bool ODriveMotorController::connectCandleDevice(const QString &interfaceName, int bitrate)
{
    clearCandleProcess();

    QString scriptPath = QCoreApplication::applicationDirPath() + QStringLiteral("/candle_bridge.py");
    if (!QFileInfo::exists(scriptPath)) {
        scriptPath = QCoreApplication::applicationDirPath() + QStringLiteral("/../candle_bridge.py");
    }
    emit logMessage(QStringLiteral("candle bridge script: %1").arg(scriptPath));
    m_candleProcess = new QProcess(this);
    m_candleProcess->setProgram(QStringLiteral("python"));
    m_candleProcess->setArguments(QStringList()
                                  << QStringLiteral("-u")
                                  << scriptPath
                                  << QStringLiteral("--channel") << interfaceName.trimmed()
                                  << QStringLiteral("--bitrate") << QString::number(bitrate));
    m_candleProcess->setProcessChannelMode(QProcess::SeparateChannels);

    connect(m_candleProcess, &QProcess::readyReadStandardOutput,
            this, &ODriveMotorController::readCandleOutput);
    connect(m_candleProcess, &QProcess::readyReadStandardError, this, [this]() {
        if (!m_candleProcess) {
            return;
        }
        const QByteArray data = m_candleProcess->readAllStandardError();
        if (!data.trimmed().isEmpty()) {
            emit logMessage(QStringLiteral("candle stderr: %1").arg(QString::fromUtf8(data).trimmed()));
        }
    });
    connect(m_candleProcess,
            QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this,
            [this](int exitCode, QProcess::ExitStatus exitStatus) {
        Q_UNUSED(exitStatus);
        emit logMessage(QStringLiteral("candle process exited: %1").arg(exitCode));
        m_pollTimer.stop();
        setConnectedState(false);
    });

    m_candleReadBuffer.clear();
    m_candleProcess->start();
    if (!m_candleProcess->waitForStarted(3000)) {
        const QString msg = QStringLiteral("Failed to start candle bridge: %1").arg(m_candleProcess->errorString());
        emit errorMessage(msg);
        emit logMessage(msg);
        clearCandleProcess();
        return false;
    }

    const qint64 deadline = QDateTime::currentMSecsSinceEpoch() + 5000;
    while (!isCandleConnected() && QDateTime::currentMSecsSinceEpoch() < deadline) {
        if (m_candleProcess->state() != QProcess::Running) {
            break;
        }
        if (!m_candleProcess->waitForReadyRead(300)) {
            continue;
        }
        readCandleOutput();
    }

    if (!isCandleConnected()) {
        const QString stdoutText = QString::fromUtf8(m_candleProcess->readAllStandardOutput()).trimmed();
        const QString stderrText = QString::fromUtf8(m_candleProcess->readAllStandardError()).trimmed();
        const QString msg = QStringLiteral("Candle bridge did not report ready state. state=%1 exitCode=%2 stdout=%3 stderr=%4")
                .arg(static_cast<int>(m_candleProcess->state()))
                .arg(m_candleProcess->exitCode())
                .arg(stdoutText.isEmpty() ? QStringLiteral("<empty>") : stdoutText)
                .arg(stderrText.isEmpty() ? QStringLiteral("<empty>") : stderrText);
        emit errorMessage(msg);
        emit logMessage(msg);
        clearCandleProcess();
        return false;
    }

    return true;
}

void ODriveMotorController::disconnectDevice()
{
    m_pollTimer.stop();
    m_slcanReadTimer.stop();
    clearDevice();
    m_statusByNode.clear();
    emit statusUpdated();
    setConnectedState(false);
    m_activePlugin.clear();
}

bool ODriveMotorController::isConnected() const { return m_connected; }
quint8 ODriveMotorController::nodeId() const { return m_nodeId; }
void ODriveMotorController::setDefaultNodeId(quint8 nodeId) { m_nodeId = static_cast<quint8>(nodeId & 0x3F); }
const ODriveMotorController::AxisStatus &ODriveMotorController::status() const { return status(m_nodeId); }
const ODriveMotorController::AxisStatus &ODriveMotorController::status(quint8 nodeId) const
{
    const auto it = m_statusByNode.constFind(static_cast<quint8>(nodeId & 0x3F));
    return it != m_statusByNode.constEnd() ? it.value() : m_emptyStatus;
}
void ODriveMotorController::setTrackedNodeIds(const QList<quint8> &nodeIds) { m_trackedNodeIds = normalizeNodeIds(nodeIds, m_nodeId); }
QList<quint8> ODriveMotorController::trackedNodeIds() const { return m_trackedNodeIds; }
QList<quint8> ODriveMotorController::recentResponsiveNodeIds(int maxAgeMs) const
{
    QList<quint8> nodeIds;
    const QDateTime now = QDateTime::currentDateTime();

    for (auto it = m_statusByNode.constBegin(); it != m_statusByNode.constEnd(); ++it) {
        if (!it.value().lastMessage.isValid()) {
            continue;
        }
        qint64 ageMs = it.value().lastMessage.msecsTo(now);
        if (ageMs < 0) {
            ageMs = 0;
        }
        if (ageMs <= maxAgeMs) {
            nodeIds << it.key();
        }
    }

    std::sort(nodeIds.begin(), nodeIds.end());
    return nodeIds;
}

bool ODriveMotorController::requestAxisState(AxisState state) { return requestAxisState(m_nodeId, state); }
bool ODriveMotorController::requestAxisState(quint8 nodeId, AxisState state)
{
    QByteArray payload; appendUInt32(payload, static_cast<quint32>(state)); return sendFrame(nodeId, SetAxisState, payload);
}
bool ODriveMotorController::requestIdle() { return requestIdle(m_nodeId); }
bool ODriveMotorController::requestIdle(quint8 nodeId) { return requestAxisState(nodeId, Idle); }
bool ODriveMotorController::requestClosedLoop() { return requestClosedLoop(m_nodeId); }
bool ODriveMotorController::requestClosedLoop(quint8 nodeId) { return requestAxisState(nodeId, ClosedLoopControl); }
bool ODriveMotorController::requestMotorCalibration() { return requestMotorCalibration(m_nodeId); }
bool ODriveMotorController::requestMotorCalibration(quint8 nodeId) { return requestAxisState(nodeId, MotorCalibration); }
bool ODriveMotorController::requestFullCalibration() { return requestFullCalibration(m_nodeId); }
bool ODriveMotorController::requestFullCalibration(quint8 nodeId) { return requestAxisState(nodeId, FullCalibrationSequence); }

bool ODriveMotorController::setControllerMode(ControlMode controlMode, InputMode inputMode) { return setControllerMode(m_nodeId, controlMode, inputMode); }
bool ODriveMotorController::setControllerMode(quint8 nodeId, ControlMode controlMode, InputMode inputMode)
{
    QByteArray payload; appendUInt32(payload, quint32(controlMode)); appendUInt32(payload, quint32(inputMode)); return sendFrame(nodeId, SetControllerMode, payload);
}
bool ODriveMotorController::setPosition(float p, float v, float t) { return setPosition(m_nodeId, p, v, t); }
bool ODriveMotorController::setPosition(quint8 nodeId, float positionTurns, float velocityFeedforwardTurnsPerSecond, float torqueFeedforwardNm)
{
    int scaledVelocity = qBound(-32768, qRound(velocityFeedforwardTurnsPerSecond * 1000.0f), 32767);
    int scaledTorque = qBound(-32768, qRound(torqueFeedforwardNm * 1000.0f), 32767);
    QByteArray payload; appendFloat(payload, positionTurns); appendInt16(payload, qint16(scaledVelocity)); appendInt16(payload, qint16(scaledTorque));
    return sendFrame(nodeId, SetInputPos, payload);
}
bool ODriveMotorController::setVelocity(float v, float t) { return setVelocity(m_nodeId, v, t); }
bool ODriveMotorController::setVelocity(quint8 nodeId, float velocityTurnsPerSecond, float torqueFeedforwardNm)
{
    QByteArray payload; appendFloat(payload, velocityTurnsPerSecond); appendFloat(payload, torqueFeedforwardNm); return sendFrame(nodeId, SetInputVel, payload);
}
bool ODriveMotorController::setTorque(float torqueNm) { return setTorque(m_nodeId, torqueNm); }
bool ODriveMotorController::setTorque(quint8 nodeId, float torqueNm)
{
    QByteArray payload; appendFloat(payload, torqueNm); return sendFrame(nodeId, SetInputTorque, payload);
}
bool ODriveMotorController::setLimits(float v, float c) { return setLimits(m_nodeId, v, c); }
bool ODriveMotorController::setLimits(quint8 nodeId, float velocityLimitTurnsPerSecond, float currentLimitAmps)
{
    QByteArray payload; appendFloat(payload, velocityLimitTurnsPerSecond); appendFloat(payload, currentLimitAmps); return sendFrame(nodeId, SetLimits, payload);
}
bool ODriveMotorController::estop() { return estop(m_nodeId); }
bool ODriveMotorController::estop(quint8 nodeId) { return sendFrame(nodeId, EstopCommand, QByteArray()); }
bool ODriveMotorController::clearErrors(bool identify) { return clearErrors(m_nodeId, identify); }
bool ODriveMotorController::clearErrors(quint8 nodeId, bool identify)
{
    QByteArray payload; payload.append(char(identify ? 1 : 0)); bool ok = sendFrame(nodeId, ClearErrors, payload); if (ok) requestAllTelemetry(nodeId, true); return ok;
}
void ODriveMotorController::requestAllTelemetry(bool quiet) { requestAllTelemetry(m_nodeId, quiet); }
void ODriveMotorController::requestAllTelemetry(quint8 nodeId, bool quiet)
{
    requestRemoteFrame(nodeId, GetError, quiet); requestRemoteFrame(nodeId, GetEncoderEstimates, quiet); requestRemoteFrame(nodeId, GetIq, quiet);
    requestRemoteFrame(nodeId, GetTemperature, quiet); requestRemoteFrame(nodeId, GetBusVoltageCurrent, quiet); requestRemoteFrame(nodeId, GetTorques, quiet); requestRemoteFrame(nodeId, GetPowers, quiet);
}
void ODriveMotorController::requestTrackedTelemetry(bool quiet)
{
    for (quint8 nodeId : normalizeNodeIds(m_trackedNodeIds, m_nodeId)) requestAllTelemetry(nodeId, quiet);
}

void ODriveMotorController::probeNode(quint8 nodeId)
{
    requestRemoteFrame(nodeId, GetEncoderEstimates, true);
    requestRemoteFrame(nodeId, GetBusVoltageCurrent, true);
}

void ODriveMotorController::probeAllNodes()
{
    emit logMessage(QStringLiteral("Probing CAN nodes 0..63 for ODrive replies."));
    for (int node = 0; node < 64; ++node) {
        probeNode(static_cast<quint8>(node));
    }
}

QString ODriveMotorController::axisStateName(quint8 axisState)
{
    switch (axisState) {
    case Undefined: return zh("e69caae5ae9ae4b989"); case Idle: return zh("e7a9bae997b2"); case StartupSequence: return zh("e590afe58aa8e5ba8fe58897");
    case FullCalibrationSequence: return zh("e695b4e5a597e6a0a1e58786"); case MotorCalibration: return zh("e794b5e69cbae6a0a1e58786");
    case EncoderIndexSearch: return zh("e7bc96e7a081e599a8e689bee99bb6"); case EncoderOffsetCalibration: return zh("e7bc96e7a081e599a8e5818fe7a7bbe6a0a1e58786");
    case ClosedLoopControl: return zh("e997ade78eafe68ea7e588b6"); case LockinSpin: return zh("e99481e79bb8e6978be8bdac");
    case EncoderDirFind: return zh("e7bc96e7a081e599a8e696b9e59091e69fa5e689be"); case Homing: return zh("e59b9ee99bb6");
    case EncoderHallPolarityCalibration: return zh("e99c8de5b094e69e81e680a7e6a0a1e58786"); case EncoderHallPhaseCalibration: return zh("e99c8de5b094e79bb8e4bd8de6a0a1e58786");
    case AnticoggingCalibration: return zh("e9bdbfe6a7bde8a1a5e581bfe6a0a1e58786"); case HarmonicCalibration: return zh("e8b090e6b3a2e6a0a1e58786");
    case HarmonicCalibrationCommutation: return zh("e68da2e79bb8e8b090e6b3a2e6a0a1e58786"); default: return zh("e69caae79fa5");
    }
}

QString ODriveMotorController::procedureResultName(quint8 procedureResult)
{
    switch (procedureResult) {
    case 0: return zh("e68890e58a9f"); case 1: return zh("e5bf99e7a28c"); case 2: return zh("e5b7b2e58f96e6b688"); case 3: return zh("e5b7b2e5a4b1e883bd");
    case 4: return zh("e697a0e5938de5ba94"); case 5: return zh("e69e81e5afb9e695b02f43505220e4b88de58cb9e9858d"); case 6: return zh("e79bb8e794b5e998bbe8b685e88c83e59bb4");
    case 7: return zh("e79bb8e794b5e6849fe8b685e88c83e59bb4"); case 8: return zh("e4b889e79bb8e4b88de5b9b3e8a1a1"); case 9: return zh("e794b5e69cbae7b1bbe59e8be697a0e69588");
    case 10: return zh("e99c8de5b094e78ab6e68081e99d9ee6b395"); case 11: return zh("e8b685e697b6"); case 12: return zh("e69caae9858de7bdaee99990e4bd8de5b0b1e59b9ee99bb6");
    case 13: return zh("e78ab6e68081e99d9ee6b395"); case 14: return zh("e69caae6a0a1e58786"); case 15: return zh("e69caae694b6e6959b"); default: return zh("e69caae79fa5");
    }
}

void ODriveMotorController::readFrames()
{
    if (!m_device) return;
    while (m_device->framesAvailable() > 0) {
        const QCanBusFrame frame = m_device->readFrame();
        if (!frame.isValid() || frame.frameType() == QCanBusFrame::RemoteRequestFrame) continue;
        if (!processFrame(static_cast<quint8>((frame.frameId() >> 5) & 0x3F), frame.frameId(), frame.payload())) {
            const QString rxLine = QStringLiteral("CAN RX DATA id=%1 dlc=%2 ascii=%3")
                    .arg(formatCanId(frame.frameId()),
                         QString::number(frame.payload().size()),
                         formatAscii(frame.payload()));
            emit rxMessage(rxLine);
        }
    }
}

void ODriveMotorController::readSlcanFrames()
{
    if (!m_serialPort) return;
    // CANgaroo polls the serial port on Windows because RX can stay silent
    // unless waitForReadyRead() is called periodically.
    m_serialPort->waitForReadyRead(1);
    if (m_serialPort->bytesAvailable() > 0) {
        const QByteArray data = m_serialPort->readAll();
        m_slcanReadBuffer.append(data);
        const QString rxLine = QStringLiteral("SLCAN raw RX ascii=%1").arg(formatAscii(data));
        emit rxMessage(rxLine);

        if (!m_slcanProtocolMismatchWarned) {
            const QByteArray lower = data.toLower();
            if (lower.contains("invalid property")
                    || lower.contains("invalid command")
                    || lower.contains("odrive")) {
                m_slcanProtocolMismatchWarned = true;
                const QString hint =
                        QStringLiteral("SLCAN warning: serial device is replying with ASCII protocol errors (e.g. 'invalid property'). "
                                       "This usually means you connected to the ODrive USB serial COM port, not a USB-CAN SLCAN adapter. "
                                       "Please pick the USB-CAN interface (often 'candle0' with plugin=candle, or a different COM port).");
                emit logMessage(hint);
                emit rxMessage(hint);
            }
        }
    }
    for (;;) {
        int idx = m_slcanReadBuffer.indexOf('\r');
        if (idx < 0) break;
        QByteArray line = m_slcanReadBuffer.left(idx);
        m_slcanReadBuffer.remove(0, idx + 1);
        processSlcanLine(line);
    }
}

void ODriveMotorController::readCandleOutput()
{
    if (!m_candleProcess) {
        return;
    }
    m_candleReadBuffer.append(m_candleProcess->readAllStandardOutput());
    while (true) {
        const int newlineIndex = m_candleReadBuffer.indexOf('\n');
        if (newlineIndex < 0) {
            break;
        }
        const QByteArray line = m_candleReadBuffer.left(newlineIndex).trimmed();
        m_candleReadBuffer.remove(0, newlineIndex + 1);
        processCandleLine(line);
    }
}

void ODriveMotorController::handleDeviceError(QCanBusDevice::CanBusError error)
{
    if (error == QCanBusDevice::NoError || !m_device) return;
    const QString msg = zh("43414e20e99499e8afafefbc9a2531").arg(m_device->errorString());
    emit errorMessage(msg); emit logMessage(msg);
}

void ODriveMotorController::handleDeviceStateChange(QCanBusDevice::CanBusDeviceState state)
{
    if (state == QCanBusDevice::ConnectedState) { setConnectedState(true); m_pollTimer.start(); }
    else if (state == QCanBusDevice::UnconnectedState) { m_pollTimer.stop(); setConnectedState(false); }
}

bool ODriveMotorController::isSlcanPlugin() const { return m_activePlugin == QStringLiteral("slcan"); }
bool ODriveMotorController::isCandlePlugin() const { return m_activePlugin == QStringLiteral("candle"); }
bool ODriveMotorController::isCanBusConnected() const { return m_device && m_device->state() == QCanBusDevice::ConnectedState; }
bool ODriveMotorController::isSlcanConnected() const { return m_serialPort && m_serialPort->isOpen(); }
bool ODriveMotorController::isCandleConnected() const { return m_candleProcess && m_candleProcess->state() == QProcess::Running && m_connected; }

bool ODriveMotorController::sendFrame(quint8 nodeId, CommandId commandId, const QByteArray &payload, bool quiet)
{
    if (isCandlePlugin()) {
        if (!isCandleConnected()) goto not_connected;
        return candleWriteFrame(frameIdFor(nodeId, commandId), payload, false, quiet, commandName(commandId), false);
    }
    if (isSlcanPlugin()) {
        if (!isSlcanConnected()) goto not_connected;
        return slcanWriteFrame(frameIdFor(nodeId, commandId), payload, false, quiet, commandName(commandId), false);
    }
    if (!isCanBusConnected()) {
not_connected:
        if (!quiet) {
            const QString msg = zh("43414e20e69caae8bf9ee68ea5efbc8ce697a0e6b395e58f91e98081202531").arg(commandName(commandId));
            emit errorMessage(msg); emit logMessage(msg);
        }
        return false;
    }
    if (!m_device->writeFrame(QCanBusFrame(frameIdFor(nodeId, commandId), payload))) {
        if (!quiet) {
            const QString msg = zh("e58f91e9808120253120e5a4b1e8b4a5efbc9a2532").arg(commandName(commandId), m_device->errorString());
            emit errorMessage(msg); emit logMessage(msg);
        }
        return false;
    }
    return true;
}

bool ODriveMotorController::requestRemoteFrame(quint8 nodeId, CommandId commandId, bool quiet)
{
    if (isCandlePlugin()) {
        if (!isCandleConnected()) return false;
        return candleWriteFrame(frameIdFor(nodeId, commandId), QByteArray(expectedReplyLength(commandId), '\0'), true, quiet, commandName(commandId), false);
    }
    if (isSlcanPlugin()) {
        if (!isSlcanConnected()) return false;
        return slcanWriteFrame(frameIdFor(nodeId, commandId), QByteArray(expectedReplyLength(commandId), '\0'), true, quiet, commandName(commandId), false);
    }
    if (!isCanBusConnected()) return false;
    QCanBusFrame frame(frameIdFor(nodeId, commandId), QByteArray(expectedReplyLength(commandId), '\0'));
    frame.setFrameType(QCanBusFrame::RemoteRequestFrame);
    if (!m_device->writeFrame(frame)) {
        const QString msg = zh("e8afb7e6b18220253120e5a4b1e8b4a5efbc9a2532").arg(commandName(commandId), m_device->errorString());
        emit errorMessage(msg); emit logMessage(msg); return false;
    }
    return true;
}

bool ODriveMotorController::slcanWriteFrame(quint32 frameId, const QByteArray &payload, bool remoteFrame, bool quiet, const QString &label, bool extendedFrame)
{
    QByteArray frame = encodeSlcanFrame(frameId, payload, remoteFrame, extendedFrame);
    if (frame.isEmpty()) {
        if (!quiet) {
            const QString msg = QStringLiteral("SLCAN only supports standard 11-bit CAN frames. Failed to send %1").arg(label);
            emit errorMessage(msg); emit logMessage(msg);
        }
        return false;
    }
    if (!quiet) {
        emit logMessage(QStringLiteral("SLCAN TX %1 id=%2 dlc=%3 payload=%4 raw=%5")
                        .arg(remoteFrame ? QStringLiteral("RTR") : label,
                             formatCanId(frameId),
                             QString::number(payload.size()),
                             formatPayload(payload),
                             QString::fromLatin1(frame.trimmed())));
    }
    if (!slcanWriteRaw(frame, quiet)) {
        if (!quiet) {
            const QString msg = QStringLiteral("Failed to send %1 over SLCAN").arg(label);
            emit errorMessage(msg); emit logMessage(msg);
        }
        return false;
    }
    return true;
}

bool ODriveMotorController::candleWriteFrame(quint32 frameId, const QByteArray &payload, bool remoteFrame, bool quiet, const QString &label, bool extendedFrame)
{
    QJsonObject obj;
    obj.insert(QStringLiteral("cmd"), QStringLiteral("send"));
    obj.insert(QStringLiteral("id"), static_cast<int>(frameId));
    obj.insert(QStringLiteral("remote"), remoteFrame);
    obj.insert(QStringLiteral("extended"), extendedFrame);
    obj.insert(QStringLiteral("data"), QString::fromLatin1(payload.toHex()));

    if (!quiet) {
        emit logMessage(QStringLiteral("CANDLE TX %1 id=%2 dlc=%3 payload=%4")
                        .arg(remoteFrame ? QStringLiteral("RTR") : label,
                             formatCanId(frameId),
                             QString::number(payload.size()),
                             formatPayload(payload)));
    }
    return candleWriteCommand(QJsonDocument(obj).toJson(QJsonDocument::Compact) + '\n', quiet);
}

bool ODriveMotorController::candleWriteCommand(const QByteArray &jsonLine, bool quiet)
{
    if (!m_candleProcess || m_candleProcess->state() != QProcess::Running) {
        return false;
    }
    const qint64 written = m_candleProcess->write(jsonLine);
    if (written != jsonLine.size()) {
        if (!quiet) {
            emit errorMessage(QStringLiteral("Failed to write command to candle bridge."));
            emit logMessage(QStringLiteral("Failed to write command to candle bridge."));
        }
        return false;
    }
    if (!m_candleProcess->waitForBytesWritten(500)) {
        if (!quiet) {
            emit errorMessage(QStringLiteral("Timed out writing to candle bridge."));
            emit logMessage(QStringLiteral("Timed out writing to candle bridge."));
        }
        return false;
    }
    return true;
}

bool ODriveMotorController::slcanWriteRaw(const QByteArray &command, bool quiet)
{
    if (!m_serialPort || !m_serialPort->isOpen()) return false;
    const qint64 written = m_serialPort->write(command);
    if (written != command.size()) {
        if (!quiet) {
            const QString msg = QStringLiteral("SLCAN write failed: %1").arg(m_serialPort->errorString());
            emit errorMessage(msg); emit logMessage(msg);
        }
        return false;
    }
    if (!m_serialPort->waitForBytesWritten(100)) {
        if (!quiet) {
            const QString msg = QStringLiteral("SLCAN write timeout: %1").arg(m_serialPort->errorString());
            emit errorMessage(msg); emit logMessage(msg);
        }
        return false;
    }
    return true;
}

bool ODriveMotorController::slcanWriteCommand(const QByteArray &command,
                                              QByteArray *response,
                                              int timeoutMs,
                                              bool requireAck)
{
    QString errorMessage;
    const bool ok = runSlcanCommand(m_serialPort, command, response, &errorMessage, timeoutMs, requireAck);
    if (!ok && !errorMessage.isEmpty()) {
        emit logMessage(errorMessage);
    }
    return ok;
}

QByteArray ODriveMotorController::slcanBitrateCommand(int bitrate)
{
    switch (bitrate) {
    case 10000: return "S0\r"; case 20000: return "S1\r"; case 50000: return "S2\r"; case 100000: return "S3\r";
    case 125000: return "S4\r"; case 250000: return "S5\r"; case 500000: return "S6\r"; case 750000: return "S7\r"; case 1000000: return "S8\r";
    default: return QByteArray();
    }
}

QList<int> ODriveMotorController::slcanProbeBaudRates(int preferredBaudRate)
{
    QList<int> baudRates;
    const QList<int> defaults = {
        1000000,
        2000000,
        921600,
        460800,
        230400,
        115200
    };

    if (preferredBaudRate > 0) {
        baudRates << preferredBaudRate;
    }
    for (const int baudRate : defaults) {
        if (!baudRates.contains(baudRate)) {
            baudRates << baudRate;
        }
    }
    return baudRates;
}

QByteArray ODriveMotorController::encodeSlcanFrame(quint32 frameId, const QByteArray &payload, bool remoteFrame, bool extendedFrame)
{
    if (frameId > 0x1FFFFFFF || payload.size() > 8) {
        return QByteArray();
    }

    const char frameType = extendedFrame
            ? (remoteFrame ? 'R' : 'T')
            : (remoteFrame ? 'r' : 't');
    const int idLength = extendedFrame ? 8 : 3;

    QByteArray line;
    line.append(frameType);
    line.append(QByteArray::number(frameId, 16).toUpper().rightJustified(idLength, '0'));
    line.append(QByteArray::number(payload.size(), 16).toUpper());
    if (!remoteFrame) {
        line.append(payload.toHex().toUpper());
    }
    line.append('\r');
    return line;
}

bool ODriveMotorController::sendRawCanFrame(quint32 frameId,
                                           const QByteArray &payload,
                                           bool extendedFrame,
                                           bool remoteFrame,
                                           bool quiet)
{
    const QByteArray boundedPayload = payload.left(8);

    if (isCandlePlugin()) {
        if (!isCandleConnected()) {
            if (!quiet) {
                const QString msg = QStringLiteral("Candle is not connected.");
                emit errorMessage(msg);
                emit logMessage(msg);
            }
            return false;
        }
        return candleWriteFrame(frameId, boundedPayload, remoteFrame, quiet, QStringLiteral("RAW"), extendedFrame);
    }

    if (isSlcanPlugin()) {
        if (!isSlcanConnected()) {
            if (!quiet) {
                const QString msg = QStringLiteral("SLCAN is not connected.");
                emit errorMessage(msg);
                emit logMessage(msg);
            }
            return false;
        }
        return slcanWriteFrame(frameId, boundedPayload, remoteFrame, quiet, QStringLiteral("RAW"), extendedFrame);
    }

    if (!isCanBusConnected()) {
        if (!quiet) {
            const QString msg = QStringLiteral("CAN device is not connected.");
            emit errorMessage(msg);
            emit logMessage(msg);
        }
        return false;
    }

    QCanBusFrame frame(frameId, boundedPayload);
    frame.setExtendedFrameFormat(extendedFrame);
    frame.setFrameType(remoteFrame ? QCanBusFrame::RemoteRequestFrame : QCanBusFrame::DataFrame);
    if (!m_device->writeFrame(frame)) {
        if (!quiet) {
            const QString msg = QStringLiteral("Raw frame send failed: %1").arg(m_device->errorString());
            emit errorMessage(msg);
            emit logMessage(msg);
        }
        return false;
    }
    return true;
}

bool ODriveMotorController::processSlcanLine(const QByteArray &line)
{
    const QByteArray trimmedLine = line.trimmed();
    if (trimmedLine.isEmpty()) return true;
    if (trimmedLine.contains('\a')) { emit logMessage(QStringLiteral("SLCAN command error.")); return false; }
    char type = trimmedLine.at(0); if (type != 't' && type != 'r' && type != 'T' && type != 'R') {
        if (!trimmedLine.isEmpty() && trimmedLine != QByteArrayLiteral("\r") && trimmedLine != QByteArrayLiteral("\n")) {
            emit logMessage(QStringLiteral("SLCAN ignored line: %1").arg(QString::fromLatin1(trimmedLine)));
        }
        return true;
    }
    bool extended = (type == 'T' || type == 'R'); bool remote = (type == 'r' || type == 'R'); int idLen = extended ? 8 : 3;
    if (trimmedLine.size() < 1 + idLen + 1) return false;
    bool ok = false; quint32 frameId = trimmedLine.mid(1, idLen).toUInt(&ok, 16); if (!ok) return false;
    int len = QString::fromLatin1(trimmedLine.mid(1 + idLen, 1)).toInt(&ok, 16); if (!ok || len < 0 || len > 8) return false;
    QByteArray payload; if (!remote) { payload = QByteArray::fromHex(trimmedLine.mid(1 + idLen + 1, len * 2)); if (payload.size() != len) return false; }
    if (remote) {
        const QString rxLine = QStringLiteral("SLCAN RX RTR id=%1 dlc=%2 ascii=%3")
                .arg(formatCanId(frameId),
                     QString::number(len),
                     formatAscii(payload));
        emit rxMessage(rxLine);
        return true;
    }
    if (!processFrame(static_cast<quint8>((frameId >> 5) & 0x3F), frameId, payload)) {
        const QString rxLine = QStringLiteral("SLCAN RX DATA id=%1 dlc=%2 ascii=%3")
                .arg(formatCanId(frameId),
                     QString::number(len),
                     formatAscii(payload));
        emit rxMessage(rxLine);
    }
    return true;
}

bool ODriveMotorController::processCandleLine(const QByteArray &line)
{
    if (line.isEmpty()) {
        return true;
    }

    QJsonParseError error;
    const QJsonDocument doc = QJsonDocument::fromJson(line, &error);
    if (error.error != QJsonParseError::NoError || !doc.isObject()) {
        emit logMessage(QStringLiteral("candle raw: %1").arg(QString::fromUtf8(line)));
        return false;
    }

    const QJsonObject obj = doc.object();
    const QString event = obj.value(QStringLiteral("event")).toString();
    if (event == QStringLiteral("ready")) {
        setConnectedState(true);
        m_pollTimer.start();
        emit logMessage(QStringLiteral("candle ready: %1").arg(obj.value(QStringLiteral("channel")).toString()));
        return true;
    }
    if (event == QStringLiteral("log")) {
        emit logMessage(QStringLiteral("candle: %1").arg(obj.value(QStringLiteral("message")).toString()));
        return true;
    }
    if (event == QStringLiteral("error")) {
        const QString msg = obj.value(QStringLiteral("message")).toString();
        emit errorMessage(msg);
        emit logMessage(msg);
        return false;
    }
    if (event == QStringLiteral("rx")) {
        const quint32 frameId = static_cast<quint32>(obj.value(QStringLiteral("id")).toInt());
        const QByteArray payload = QByteArray::fromHex(obj.value(QStringLiteral("data")).toString().toLatin1());
        const bool remote = obj.value(QStringLiteral("remote")).toBool(false);
        if (remote) {
            const QString rxLine = QStringLiteral("CANDLE RX RTR id=%1 dlc=%2 ascii=%3")
                    .arg(formatCanId(frameId),
                         QString::number(payload.size()),
                         formatAscii(payload));
            emit rxMessage(rxLine);
            return true;
        }
        if (!processFrame(static_cast<quint8>((frameId >> 5) & 0x3F), frameId, payload)) {
            const QString rxLine = QStringLiteral("CANDLE RX DATA id=%1 dlc=%2 ascii=%3")
                    .arg(formatCanId(frameId),
                         QString::number(payload.size()),
                         formatAscii(payload));
            emit rxMessage(rxLine);
        }
        return true;
    }

    emit logMessage(QStringLiteral("candle unknown: %1").arg(QString::fromUtf8(line)));
    return true;
}

bool ODriveMotorController::processFrame(quint8 nodeId, quint32 frameId, const QByteArray &payload)
{
    quint32 commandId = frameId & 0x1F;
    AxisStatus &s = m_statusByNode[nodeId];
    s.lastMessage = QDateTime::currentDateTime();
    QString summary;
    switch (commandId) {
    case Heartbeat:
        if (payload.size() >= 7) {
            const QDateTime previousHeartbeat = s.lastHeartbeat;
            s.axisError = readUInt32(payload, 0);
            s.axisState = quint8(payload.at(4));
            s.procedureResult = quint8(payload.at(5));
            s.trajectoryDone = payload.at(6) != 0;
            s.lastHeartbeat = s.lastMessage;
            summary = QStringLiteral("Heartbeat node=%1 axis_state=%2 procedure=%3 trajectory_done=%4 axis_error=%5")
                    .arg(QString::number(nodeId),
                         axisStateName(s.axisState),
                         procedureResultName(s.procedureResult),
                         s.trajectoryDone ? QStringLiteral("true") : QStringLiteral("false"),
                         formatCanId(s.axisError));
            if (!previousHeartbeat.isValid() || previousHeartbeat.msecsTo(s.lastMessage) > 1500) {
                emit logMessage(QStringLiteral("ODrive heartbeat detected from node %1").arg(nodeId));
            }
        }
        break;
    case GetError:
        if (payload.size() >= 8) {
            s.activeErrors = readUInt32(payload, 0);
            s.disarmReason = readUInt32(payload, 4);
            s.axisError = s.activeErrors | s.disarmReason;
            summary = QStringLiteral("Get_Error node=%1 active=%2 disarm=%3 combined=%4")
                    .arg(QString::number(nodeId),
                         formatCanId(s.activeErrors),
                         formatCanId(s.disarmReason),
                         formatCanId(s.axisError));
        }
        break;
    case GetEncoderEstimates:
        if (payload.size() >= 8) {
            s.positionTurns = readFloat(payload, 0);
            s.velocityTurnsPerSecond = readFloat(payload, 4);
            summary = QStringLiteral("Get_Encoder_Estimates node=%1 pos=%2 turns vel=%3 turns/s")
                    .arg(QString::number(nodeId),
                         QString::number(s.positionTurns, 'f', 3),
                         QString::number(s.velocityTurnsPerSecond, 'f', 3));
        }
        break;
    case GetIq:
        if (payload.size() >= 8) {
            s.iqSetpointAmps = readFloat(payload, 0);
            s.iqMeasuredAmps = readFloat(payload, 4);
            summary = QStringLiteral("Get_Iq node=%1 setpoint=%2 A measured=%3 A")
                    .arg(QString::number(nodeId),
                         QString::number(s.iqSetpointAmps, 'f', 3),
                         QString::number(s.iqMeasuredAmps, 'f', 3));
        }
        break;
    case GetTemperature:
        if (payload.size() >= 8) {
            s.fetTemperatureCelsius = readFloat(payload, 0);
            s.motorTemperatureCelsius = readFloat(payload, 4);
            summary = QStringLiteral("Get_Temperature node=%1 fet=%2 C motor=%3 C")
                    .arg(QString::number(nodeId),
                         QString::number(s.fetTemperatureCelsius, 'f', 2),
                         QString::number(s.motorTemperatureCelsius, 'f', 2));
        }
        break;
    case GetBusVoltageCurrent:
        if (payload.size() >= 8) {
            s.busVoltageVolts = readFloat(payload, 0);
            s.busCurrentAmps = readFloat(payload, 4);
            summary = QStringLiteral("Get_Bus_Voltage_Current node=%1 bus=%2 V current=%3 A")
                    .arg(QString::number(nodeId),
                         QString::number(s.busVoltageVolts, 'f', 3),
                         QString::number(s.busCurrentAmps, 'f', 3));
        }
        break;
    case GetTorques:
        if (payload.size() >= 8) {
            s.torqueTargetNm = readFloat(payload, 0);
            s.torqueEstimateNm = readFloat(payload, 4);
            summary = QStringLiteral("Get_Torques node=%1 target=%2 Nm estimate=%3 Nm")
                    .arg(QString::number(nodeId),
                         QString::number(s.torqueTargetNm, 'f', 3),
                         QString::number(s.torqueEstimateNm, 'f', 3));
        }
        break;
    case GetPowers:
        if (payload.size() >= 8) {
            s.electricalPowerWatts = readFloat(payload, 0);
            s.mechanicalPowerWatts = readFloat(payload, 4);
            summary = QStringLiteral("Get_Powers node=%1 electrical=%2 W mechanical=%3 W")
                    .arg(QString::number(nodeId),
                         QString::number(s.electricalPowerWatts, 'f', 2),
                         QString::number(s.mechanicalPowerWatts, 'f', 2));
        }
        break;
    default: return false;
    }
    if (!summary.isEmpty()) {
        emit rxMessage(summary);
    }
    emit nodeStatusUpdated(nodeId); emit statusUpdated();
    return true;
}

quint32 ODriveMotorController::frameIdFor(quint8 nodeId, CommandId commandId) const { return (quint32(nodeId & 0x3F) << 5) | quint32(commandId); }

void ODriveMotorController::clearDevice()
{
    clearSerialPort();
    clearCandleProcess();
    if (!m_device) return;
    disconnect(m_device, nullptr, this, nullptr);
    if (m_device->state() != QCanBusDevice::UnconnectedState) m_device->disconnectDevice();
    delete m_device; m_device = nullptr;
}

void ODriveMotorController::clearSerialPort()
{
    if (!m_serialPort) return;
    disconnect(m_serialPort, nullptr, this, nullptr);
    if (m_serialPort->isOpen()) {
        m_serialPort->write("C\r", 2);
        m_serialPort->flush();
        m_serialPort->waitForBytesWritten(300);
        m_serialPort->clear();
        m_serialPort->close();
    }
    delete m_serialPort; m_serialPort = nullptr; m_slcanReadBuffer.clear();
}

void ODriveMotorController::clearCandleProcess()
{
    if (!m_candleProcess) {
        return;
    }
    disconnect(m_candleProcess, nullptr, this, nullptr);
    if (m_candleProcess->state() == QProcess::Running) {
        candleWriteCommand(QByteArrayLiteral("{\"cmd\":\"stop\"}\n"), true);
        m_candleProcess->waitForFinished(500);
        if (m_candleProcess->state() == QProcess::Running) {
            m_candleProcess->kill();
            m_candleProcess->waitForFinished(500);
        }
    }
    delete m_candleProcess;
    m_candleProcess = nullptr;
    m_candleReadBuffer.clear();
}

void ODriveMotorController::setConnectedState(bool connected)
{
    if (m_connected == connected) return;
    m_connected = connected;
    emit connectionChanged(m_connected);
    emit logMessage(m_connected ? zh("43414e20e993bee8b7afe5b7b2e5b0b1e7bbaa") : zh("43414e20e993bee8b7afe5b7b2e696ade5bc80"));
}

QString ODriveMotorController::commandName(CommandId commandId)
{
    switch (commandId) {
    case Heartbeat: return QObject::tr("Heartbeat"); case EstopCommand: return QObject::tr("Estop"); case GetError: return QObject::tr("Get_Error");
    case SetAxisState: return QObject::tr("Set_Axis_State"); case GetEncoderEstimates: return QObject::tr("Get_Encoder_Estimates");
    case SetControllerMode: return QObject::tr("Set_Controller_Mode"); case SetInputPos: return QObject::tr("Set_Input_Pos");
    case SetInputVel: return QObject::tr("Set_Input_Vel"); case SetInputTorque: return QObject::tr("Set_Input_Torque");
    case SetLimits: return QObject::tr("Set_Limits"); case GetIq: return QObject::tr("Get_Iq"); case GetTemperature: return QObject::tr("Get_Temperature");
    case GetBusVoltageCurrent: return QObject::tr("Get_Bus_Voltage_Current"); case ClearErrors: return QObject::tr("Clear_Errors");
    case GetTorques: return QObject::tr("Get_Torques"); case GetPowers: return QObject::tr("Get_Powers"); default: return QObject::tr("Unknown");
    }
}

int ODriveMotorController::expectedReplyLength(CommandId commandId)
{
    switch (commandId) { case Heartbeat: return 7; case GetError: case GetEncoderEstimates: case GetIq: case GetTemperature: case GetBusVoltageCurrent: case GetTorques: case GetPowers: return 8; default: return 0; }
}

void ODriveMotorController::appendUInt32(QByteArray &payload, quint32 value)
{
    quint32 le = qToLittleEndian(value); payload.append(reinterpret_cast<const char *>(&le), sizeof(le));
}

void ODriveMotorController::appendInt16(QByteArray &payload, qint16 value)
{
    qint16 le = qToLittleEndian(value); payload.append(reinterpret_cast<const char *>(&le), sizeof(le));
}

void ODriveMotorController::appendFloat(QByteArray &payload, float value)
{
    quint32 raw = 0; std::memcpy(&raw, &value, sizeof(value)); appendUInt32(payload, raw);
}

quint32 ODriveMotorController::readUInt32(const QByteArray &payload, int offset)
{
    quint32 value = 0; if (offset < 0 || payload.size() < offset + int(sizeof(value))) return value; std::memcpy(&value, payload.constData() + offset, sizeof(value)); return qFromLittleEndian(value);
}

float ODriveMotorController::readFloat(const QByteArray &payload, int offset)
{
    quint32 raw = readUInt32(payload, offset); float value = 0.0f; std::memcpy(&value, &raw, sizeof(value)); return value;
}
