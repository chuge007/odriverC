#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "odrivemotorcontroller.h"

#include <QByteArray>
#include <QComboBox>
#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QDialog>
#include <QDoubleSpinBox>
#include <QDir>
#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLibrary>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QCheckBox>
#include <QRegularExpression>
#include <QStringList>
#include <QSpinBox>
#include <QTextDocument>
#include <QTextEdit>
#include <QTimer>
#include <QVBoxLayout>
#include <QtGlobal>

namespace {

QString zh(const char *utf8Hex)
{
    return QString::fromUtf8(QByteArray::fromHex(utf8Hex));
}

QString pluginLabel(const QString &plugin)
{
    if (plugin == QStringLiteral("auto")) {
        return QStringLiteral("Auto Detect (auto)");
    }
    if (plugin == QStringLiteral("systeccan")) {
        return zh("535953205445432043414e202873797374656363616e29");
    }
    if (plugin == QStringLiteral("socketcan")) {
        return QStringLiteral("SocketCAN (socketcan)");
    }
    if (plugin == QStringLiteral("candle")) {
        return QStringLiteral("Candle USB CAN (candle)");
    }
    if (plugin == QStringLiteral("slcan")) {
        return QStringLiteral("SLCAN Serial (slcan)");
    }
    if (plugin == QStringLiteral("virtualcan")) {
        return zh("e8999ae68b9f2043414e2028e4bb85e6b58be8af952c207669727475616c63616e29");
    }
    if (plugin == QStringLiteral("peakcan")) {
        return QStringLiteral("PEAK CAN (peakcan)");
    }
    if (plugin == QStringLiteral("passthrucan")) {
        return QStringLiteral("J2534 PassThru CAN (passthrucan)");
    }
    if (plugin == QStringLiteral("tinycan")) {
        return QStringLiteral("Tiny-CAN (tinycan)");
    }
    if (plugin == QStringLiteral("vectorcan")) {
        return QStringLiteral("Vector CAN (vectorcan)");
    }
    return plugin;
}

QString currentPluginId(const QComboBox *combo)
{
    const QString dataValue = combo->currentData().toString().trimmed();
    if (!dataValue.isEmpty()) {
        return dataValue;
    }
    return combo->currentText().trimmed();
}

bool checkSystemCanDriver(QString *errorMessage = nullptr)
{
    QLibrary driver(QStringLiteral("usbcan64"));
    if (driver.load()) {
        driver.unload();
        if (errorMessage) {
            errorMessage->clear();
        }
        return true;
    }

    if (errorMessage) {
        const QString appDir = QDir::toNativeSeparators(QCoreApplication::applicationDirPath());
        *errorMessage = QStringLiteral(
                    "systeccan needs the vendor driver usbcan64.dll. "
                    "Install the matching USB-CAN driver, or place the 64-bit usbcan64.dll "
                    "and its dependent DLLs next to the executable in %1. Loader detail: %2")
                .arg(appDir, driver.errorString());
    }
    return false;
}

QString primaryButtonStyle()
{
    return QStringLiteral(
                "QPushButton {"
                " background: #f8f7f2;"
                " border: 1px solid #c9c3b8;"
                " border-radius: 8px;"
                " padding: 8px 18px;"
                " font-size: 24px;"
                "}"
                "QPushButton:hover { background: #f1ece3; }"
                "QPushButton:pressed { background: #e7e0d2; }"
                "QPushButton:disabled { color: #999999; background: #f3f3f3; }");
}

QString readoutStyle()
{
    return QStringLiteral(
                "QLabel {"
                " background: #ffffff;"
                " border: 1px solid #d5d5d5;"
                " border-radius: 4px;"
                " padding: 6px 10px;"
                " font-size: 20px;"
                " min-width: 150px;"
                "}");
}

QString topInputStyle()
{
    return QStringLiteral(
                "QLineEdit, QSpinBox {"
                " background: #ffffff;"
                " border: 1px solid #cfcfcf;"
                " border-radius: 4px;"
                " padding: 4px 8px;"
                " font-size: 22px;"
                " min-height: 38px;"
                "}"
                "QSpinBox::up-button, QSpinBox::down-button { width: 18px; }");
}

} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      ui(new Ui::MainWindow),
      m_controller(new ODriveMotorController(this)),
      m_advancedDialog(nullptr),
      m_rxEdit(nullptr),
      m_txIdEdit(nullptr),
      m_txDlcSpin(nullptr),
      m_txDataEdit(nullptr),
      m_txExtendedCheck(nullptr),
      m_txRemoteCheck(nullptr),
      m_txPeriodSpin(nullptr),
      m_txSendButton(nullptr),
      m_txToggleButton(nullptr),
      m_txTimer(nullptr),
      m_turnZeroTurns(0.0),
      m_driveZeroTurns(0.0),
      m_zeroPointInitialized(false)
{
    ui->setupUi(this);
    buildUi();
    populatePlugins();
    populateControlModes();
    populateInputModes();
    refreshCanInterfaces();
    updateConnectionEditorHints();
    syncConnectionEditorsFromAdvanced();
    updateStatusPanel();
    syncAxisReadouts();

    connect(m_controller, &ODriveMotorController::connectionChanged,
            this, &MainWindow::updateConnectionState);
    connect(m_controller, &ODriveMotorController::statusUpdated,
            this, &MainWindow::updateStatusPanel);
    connect(m_controller, &ODriveMotorController::nodeStatusUpdated,
            this, &MainWindow::handleNodeStatusUpdate);
    connect(m_controller, &ODriveMotorController::logMessage,
            this, &MainWindow::appendLog);
    connect(m_controller, &ODriveMotorController::rxMessage,
            this, &MainWindow::appendRxMessage);
    connect(m_controller, &ODriveMotorController::errorMessage,
            this, &MainWindow::appendLog);

    QTimer *uiStateTimer = new QTimer(this);
    uiStateTimer->setInterval(500);
    connect(uiStateTimer, &QTimer::timeout, this, [this]() {
        updateLinkStatusDisplay();
    });
    uiStateTimer->start();

    updateConnectionState(false);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::refreshCanInterfaces()
{
    const QString plugin = currentPluginId(m_pluginCombo);
    const QString previousInterface = m_interfaceCombo->currentText().trimmed();
    QString errorMessage;
    QStringList interfaces;

    qInfo().noquote() << "Refreshing CAN interfaces. plugin=" << plugin
                      << " previousInterface=" << previousInterface;

    if (plugin == QStringLiteral("systeccan") && !checkSystemCanDriver(&errorMessage)) {
        interfaces.clear();
    } else {
        interfaces = m_controller->availableInterfaces(plugin, &errorMessage);
    }

    qInfo().noquote() << "Refresh result. plugin=" << plugin
                      << " interfaces=" << interfaces.join(QStringLiteral(", "))
                      << " error=" << errorMessage;

    m_interfaceCombo->blockSignals(true);
    m_interfaceCombo->clear();
    m_interfaceCombo->addItems(interfaces);
    m_interfaceCombo->setEditText(previousInterface);
    if (!interfaces.isEmpty() && previousInterface.isEmpty()) {
        m_interfaceCombo->setCurrentIndex(0);
    }
    m_interfaceCombo->blockSignals(false);

    if (!errorMessage.isEmpty()) {
        appendLog(zh("e588b7e696b0e68ea5e58fa3e58897e8a1a8e68f90e7a4ba3a20253120285b25325d29")
                  .arg(errorMessage, plugin));
    }

    if (plugin == QStringLiteral("virtualcan")) {
        appendLog(zh("e5bd93e5898de98089e68ba9e79a84e698afe8999ae68b9f2043414eefbc8ce4b88de4bc9ae8bf9ee68ea5e79c9fe5ae9ee7a1ace4bbb6e38082"));
    }

    updateConnectionEditorHints();
    syncConnectionEditorsFromAdvanced();
}

void MainWindow::toggleConnection()
{
    if (m_controller->isConnected()) {
        qInfo() << "Disconnect requested from UI";
        stopProgram();
        m_controller->disconnectDevice();
        return;
    }

    updateTrackedNodes();

    if (currentPluginId(m_pluginCombo) == QStringLiteral("virtualcan")) {
        appendLog(zh("e5bd93e5898de4bdbfe794a8e8999ae68b9f2043414eefbc8ce680bbe7babfe695b0e68daee4bb85e69da5e887aae7a88be5ba8fe887aae8baabe38082"));
        updateProgramStatus(zh("e8999ae68b9f2043414e20e5b7b2e8bf9ee68ea5efbc8ce4b88de4bc9ae8bf9ee68ea5e79c9fe5ae9ee8aebee5a487"));
    }

    if (currentPluginId(m_pluginCombo) == QStringLiteral("systeccan")) {
        QString driverError;
        if (!checkSystemCanDriver(&driverError)) {
            qWarning().noquote() << "systeccan driver check failed:" << driverError;
            appendLog(driverError);
            updateProgramStatus(QStringLiteral("systeccan driver missing: usbcan64.dll"));
            return;
        }
    }

    qInfo().noquote() << "Connect requested. plugin=" << currentPluginId(m_pluginCombo)
                      << " interface=" << m_interfaceCombo->currentText().trimmed()
                      << " bitrate=" << m_bitrateSpin->value()
                      << " nodeId=" << m_nodeIdSpin->value();

    const bool connected = m_controller->connectDevice(currentPluginId(m_pluginCombo),
                                                       m_interfaceCombo->currentText().trimmed(),
                                                       m_bitrateSpin->value(),
                                                       m_serialBaudSpin->value(),
                                                       static_cast<quint8>(m_nodeIdSpin->value()));
    qInfo() << "connectDevice returned" << connected;
    if (connected) {
        updateTrackedNodes();
        updateProgramStatus(zh("e8bf9ee68ea5e68890e58a9fefbc8ce58786e5a487e68ea7e588b6e8bdace8bdaee5928ce9a9b1e8bdae"));
        m_controller->requestTrackedTelemetry();
        updateLinkStatusDisplay();
        QTimer::singleShot(1500, this, [this]() {
            updateLinkStatusDisplay();
            if (!m_controller->isConnected() || hasFreshTrackedHeartbeat()) {
                return;
            }

            appendLog(QStringLiteral("CAN adapter connected, but no ODrive heartbeat (0x01) was received within 1.5 s. Check bitrate, node ID, wiring, and power."));
            updateProgramStatus(QStringLiteral("CAN connected, but no ODrive heartbeat"));
            m_controller->probeAllNodes();
            QTimer::singleShot(600, this, [this]() {
                if (!m_controller->isConnected()) {
                    return;
                }

                const QList<quint8> responsiveNodeIds = m_controller->recentResponsiveNodeIds(1500);
                if (responsiveNodeIds.isEmpty()) {
                    appendLog(QStringLiteral("Probe result: no ODrive reply was seen on nodes 0..63. This usually means CAN bitrate mismatch, no bus power, wiring/termination issue, or the adapter is not actually on CAN mode."));
                    return;
                }

                QStringList nodeStrings;
                for (const quint8 nodeId : responsiveNodeIds) {
                    nodeStrings << QString::number(nodeId);
                }
                appendLog(QStringLiteral("Probe result: received ODrive replies from node(s): %1").arg(nodeStrings.join(QStringLiteral(", "))));
            });
        });
    }
}

void MainWindow::updateConnectionState(bool connected)
{
    m_connectButton->setText(connected ? QStringLiteral("disconnect") : QStringLiteral("connect"));

    m_pluginCombo->setEnabled(!connected);
    m_interfaceCombo->setEnabled(!connected);
    m_bitrateSpin->setEnabled(!connected);
    m_refreshInterfacesButton->setEnabled(!connected);
    m_ipEdit->setEnabled(!connected);
    m_openAdvancedButton->setEnabled(true);
    setMotionControlsEnabled(connected);

    if (!connected) {
        m_scanState = ScanState();
        updateProgramStatus(zh("e8afb7e58588e8bf9ee68ea5204f4472697665"));
        syncAxisReadouts();
        updateStatusPanel();
    }

    updateLinkStatusDisplay();
    updateConnectionEditorHints();
}

void MainWindow::updateStatusPanel()
{
    const ODriveMotorController::AxisStatus &status = m_controller->status();

    auto formatHex = [](quint32 value) {
        return QStringLiteral("0x%1").arg(value, 8, 16, QLatin1Char('0')).toUpper();
    };

    if (!m_controller->isConnected() || !status.lastMessage.isValid()) {
        const QString placeholder = QStringLiteral("--");
        m_axisErrorValue->setText(placeholder);
        m_activeErrorsValue->setText(placeholder);
        m_disarmReasonValue->setText(placeholder);
        m_axisStateValue->setText(placeholder);
        m_procedureResultValue->setText(placeholder);
        m_trajectoryDoneValue->setText(placeholder);
        m_positionValue->setText(placeholder);
        m_velocityValue->setText(placeholder);
        m_busVoltageValue->setText(placeholder);
        m_busCurrentValue->setText(placeholder);
        m_iqValue->setText(placeholder);
        m_temperatureValue->setText(placeholder);
        m_torqueValue->setText(placeholder);
        m_powerValue->setText(placeholder);
        m_lastUpdateValue->setText(placeholder);
        return;
    }

    m_axisErrorValue->setText(formatHex(status.axisError));
    m_activeErrorsValue->setText(formatHex(status.activeErrors));
    m_disarmReasonValue->setText(formatHex(status.disarmReason));
    m_axisStateValue->setText(QStringLiteral("%1 (%2)")
                              .arg(ODriveMotorController::axisStateName(status.axisState))
                              .arg(status.axisState));
    m_procedureResultValue->setText(QStringLiteral("%1 (%2)")
                                    .arg(ODriveMotorController::procedureResultName(status.procedureResult))
                                    .arg(status.procedureResult));
    m_trajectoryDoneValue->setText(status.trajectoryDone ? zh("e698af") : zh("e590a6"));
    m_positionValue->setText(zh("253120e59c88").arg(QString::number(status.positionTurns, 'f', 3)));
    m_velocityValue->setText(zh("253120e59c882fe7a792").arg(QString::number(status.velocityTurnsPerSecond, 'f', 3)));
    m_busVoltageValue->setText(QStringLiteral("%1 V").arg(QString::number(status.busVoltageVolts, 'f', 3)));
    m_busCurrentValue->setText(QStringLiteral("%1 A").arg(QString::number(status.busCurrentAmps, 'f', 3)));
    m_iqValue->setText(QStringLiteral("%1 / %2 A")
                       .arg(QString::number(status.iqSetpointAmps, 'f', 3),
                            QString::number(status.iqMeasuredAmps, 'f', 3)));
    m_temperatureValue->setText(zh("2531202f20253220e69184e6b08fe5baa6")
                                .arg(QString::number(status.fetTemperatureCelsius, 'f', 2),
                                     QString::number(status.motorTemperatureCelsius, 'f', 2)));
    m_torqueValue->setText(QStringLiteral("%1 / %2 Nm")
                           .arg(QString::number(status.torqueTargetNm, 'f', 3),
                                QString::number(status.torqueEstimateNm, 'f', 3)));
    m_powerValue->setText(QStringLiteral("%1 / %2 W")
                          .arg(QString::number(status.electricalPowerWatts, 'f', 2),
                               QString::number(status.mechanicalPowerWatts, 'f', 2)));
    m_lastUpdateValue->setText(status.lastMessage.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz")));
}

void MainWindow::appendLog(const QString &message)
{
    if (!m_logEdit) {
        return;
    }

    const QString timestamp = QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss"));
    const QString line = QStringLiteral("[%1] %2").arg(timestamp, message);
    m_logEdit->append(line);
    qInfo().noquote() << line;
}

void MainWindow::appendRxMessage(const QString &message)
{
    if (!m_rxEdit) {
        return;
    }

    const QString timestamp = QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss"));
    m_rxEdit->append(QStringLiteral("[%1] %2").arg(timestamp, message));
}

static bool parseTxId(const QString &text, quint32 *outId)
{
    if (!outId) return false;
    const QString trimmed = text.trimmed();
    if (trimmed.isEmpty()) return false;

    bool ok = false;
    quint32 value = 0;
    if (trimmed.startsWith(QStringLiteral("0x"), Qt::CaseInsensitive)
            || trimmed.contains(QRegularExpression(QStringLiteral("[A-Fa-f]")))) {
        value = trimmed.startsWith(QStringLiteral("0x"), Qt::CaseInsensitive)
                ? trimmed.mid(2).toUInt(&ok, 16)
                : trimmed.toUInt(&ok, 16);
    } else {
        value = trimmed.toUInt(&ok, 10);
    }
    if (!ok) return false;
    *outId = value;
    return true;
}

static QByteArray parseTxData(const QString &text)
{
    const QString trimmed = text.trimmed();
    if (trimmed.isEmpty()) return QByteArray();

    if (trimmed.startsWith(QStringLiteral("ascii:"), Qt::CaseInsensitive)) {
        return trimmed.mid(6).toUtf8();
    }
    if (trimmed.startsWith(QStringLiteral("hex:"), Qt::CaseInsensitive)) {
        QString hex = trimmed.mid(4);
        hex.remove(QRegularExpression(QStringLiteral("\\s+")));
        return QByteArray::fromHex(hex.toLatin1());
    }

    QString hex = trimmed;
    hex.remove(QRegularExpression(QStringLiteral("\\s+")));
    if (!hex.isEmpty()
            && (hex.size() % 2 == 0)
            && !hex.contains(QRegularExpression(QStringLiteral("[^0-9A-Fa-f]")))) {
        return QByteArray::fromHex(hex.toLatin1());
    }

    return trimmed.toUtf8();
}

void MainWindow::sendCustomTxOnce()
{
    if (!m_controller || !m_controller->isConnected()) {
        appendLog(QStringLiteral("Transmit: not connected"));
        return;
    }
    if (!m_txIdEdit || !m_txDlcSpin || !m_txDataEdit || !m_txExtendedCheck || !m_txRemoteCheck) {
        return;
    }

    quint32 frameId = 0;
    if (!parseTxId(m_txIdEdit->text(), &frameId)) {
        appendLog(QStringLiteral("Transmit: invalid ID"));
        return;
    }

    const bool extended = m_txExtendedCheck->isChecked();
    const bool remote = m_txRemoteCheck->isChecked();
    const int dlc = qBound(0, m_txDlcSpin->value(), 8);

    if (!extended && frameId > 0x7FF) {
        appendLog(QStringLiteral("Transmit: ID > 0x7FF requires Extended mode"));
        return;
    }
    if (frameId > 0x1FFFFFFF) {
        appendLog(QStringLiteral("Transmit: ID must be <= 0x1FFFFFFF"));
        return;
    }

    QByteArray payload;
    if (remote) {
        payload = QByteArray(dlc, '\0');
    } else {
        payload = parseTxData(m_txDataEdit->text()).left(8);
        if (payload.size() < dlc) {
            payload.append(QByteArray(dlc - payload.size(), '\0'));
        } else if (payload.size() > dlc) {
            payload = payload.left(dlc);
        }
    }

    if (!m_controller->sendRawCanFrame(frameId, payload, extended, remote, false)) {
        appendLog(QStringLiteral("Transmit: send failed"));
    }
}

void MainWindow::toggleCustomTx()
{
    if (!m_txTimer || !m_txToggleButton || !m_txPeriodSpin) {
        return;
    }

    if (m_txTimer->isActive()) {
        m_txTimer->stop();
        m_txToggleButton->setText(QStringLiteral("Start Repeat"));
        appendLog(QStringLiteral("Transmit: repeat stopped"));
        return;
    }

    const int interval = qMax(1, m_txPeriodSpin->value());
    if (interval <= 0) {
        appendLog(QStringLiteral("Transmit: set Repeat > 0 ms to start"));
        return;
    }
    m_txTimer->setInterval(interval);
    m_txTimer->start();
    m_txToggleButton->setText(QStringLiteral("Stop Repeat"));
    appendLog(QStringLiteral("Transmit: repeat started"));
}

void MainWindow::requestIdle()
{
    m_controller->requestIdle();
}

void MainWindow::requestMotorCalibration()
{
    m_controller->requestMotorCalibration();
}

void MainWindow::requestFullCalibration()
{
    m_controller->requestFullCalibration();
}

void MainWindow::requestClosedLoop()
{
    m_controller->requestClosedLoop();
}

void MainWindow::applyControllerMode()
{
    const ODriveMotorController::ControlMode controlMode =
            static_cast<ODriveMotorController::ControlMode>(m_controlModeCombo->currentData().toInt());
    const ODriveMotorController::InputMode inputMode =
            static_cast<ODriveMotorController::InputMode>(m_inputModeCombo->currentData().toInt());
    m_controller->setControllerMode(controlMode, inputMode);
}

void MainWindow::sendPositionCommand()
{
    const ODriveMotorController::InputMode inputMode =
            static_cast<ODriveMotorController::InputMode>(m_inputModeCombo->currentData().toInt());
    m_controller->setControllerMode(ODriveMotorController::PositionControl, inputMode);
    m_controller->setPosition(static_cast<float>(m_positionSpin->value()),
                              static_cast<float>(m_positionVelFfSpin->value()),
                              static_cast<float>(m_positionTorqueFfSpin->value()));
}

void MainWindow::sendVelocityCommand()
{
    const ODriveMotorController::InputMode inputMode =
            static_cast<ODriveMotorController::InputMode>(m_inputModeCombo->currentData().toInt());
    m_controller->setControllerMode(ODriveMotorController::VelocityControl, inputMode);
    m_controller->setVelocity(static_cast<float>(m_velocitySpin->value()),
                              static_cast<float>(m_velocityTorqueFfSpin->value()));
}

void MainWindow::sendTorqueCommand()
{
    const ODriveMotorController::InputMode inputMode =
            static_cast<ODriveMotorController::InputMode>(m_inputModeCombo->currentData().toInt());
    m_controller->setControllerMode(ODriveMotorController::TorqueControl, inputMode);
    m_controller->setTorque(static_cast<float>(m_torqueSpin->value()));
}

void MainWindow::applyLimits()
{
    m_controller->setLimits(static_cast<float>(m_velocityLimitSpin->value()),
                            static_cast<float>(m_currentLimitSpin->value()));
}

void MainWindow::refreshTelemetry()
{
    appendLog(zh("e5b7b2e4b8bbe58aa8e588b7e696b0e78ab6e68081"));
    m_controller->requestTrackedTelemetry();
}

void MainWindow::openAdvancedPanel()
{
    if (!m_advancedDialog) {
        return;
    }

    m_advancedDialog->show();
    m_advancedDialog->raise();
    m_advancedDialog->activateWindow();
}

void MainWindow::syncAxisReadouts()
{
    if (!m_controller->isConnected()) {
        m_turnPositionLabel->setText(QStringLiteral("-- mm"));
        m_drivePositionLabel->setText(QStringLiteral("-- mm"));
        return;
    }

    m_turnPositionLabel->setText(QStringLiteral("%1 mm")
                                 .arg(QString::number(axisPositionMm(turnNodeId(), true), 'f', 2)));
    m_drivePositionLabel->setText(QStringLiteral("%1 mm")
                                  .arg(QString::number(axisPositionMm(driveNodeId(), false), 'f', 2)));
}

void MainWindow::setZeroPoint()
{
    if (!m_controller->isConnected()) {
        QMessageBox::warning(this, zh("e69caae8bf9ee68ea5"), zh("e8afb7e58588e8bf9ee68ea5204f4472697665e38082"));
        return;
    }

    m_turnZeroTurns = m_controller->status(turnNodeId()).positionTurns;
    m_driveZeroTurns = m_controller->status(driveNodeId()).positionTurns;
    m_zeroPointInitialized = true;
    syncAxisReadouts();
    updateProgramStatus(zh("e99bb6e782b9e5b7b2e8aebee7bdae"));
    appendLog(zh("e5bd93e5898de8bdace8bdaee5928ce9a9b1e8bdaee4bd8de7bdaee5b7b2e8aeb0e4b8bae99bb6e782b9"));
}

void MainWindow::sendHome()
{
    if (!m_controller->isConnected()) {
        return;
    }

    if (!m_zeroPointInitialized) {
        setZeroPoint();
    }

    m_scanState = ScanState();
    commandAxisPosition(turnNodeId(), 0.0, m_homeSpeedSpin->value(), true);
    commandAxisPosition(driveNodeId(), 0.0, m_homeSpeedSpin->value(), false);
    updateProgramStatus(zh("e6ada3e59ca8e59b9ee58e9fe782b9"));
}

void MainWindow::resetMotionState()
{
    if (!m_controller->isConnected()) {
        return;
    }

    stopAllMotion(true);

    QList<quint8> nodeIds;
    nodeIds << static_cast<quint8>(m_nodeIdSpin->value());
    if (!nodeIds.contains(turnNodeId())) {
        nodeIds << turnNodeId();
    }
    if (!nodeIds.contains(driveNodeId())) {
        nodeIds << driveNodeId();
    }

    for (const quint8 nodeId : nodeIds) {
        m_controller->clearErrors(nodeId, false);
    }

    m_scanState = ScanState();
    updateProgramStatus(zh("e5b7b2e5a48de4bd8defbc8ce99499e8afafe5b7b2e6b885e999a4"));
}

void MainWindow::enableAxes()
{
    if (!m_controller->isConnected()) {
        return;
    }

    ensureAxisClosedLoop(turnNodeId());
    ensureAxisClosedLoop(driveNodeId());
    updateProgramStatus(zh("e6898be58aa8e68ea7e588b6e5b7b2e5bc80e5a78b"));
}

void MainWindow::startScanProgram()
{
    if (!m_controller->isConnected()) {
        QMessageBox::warning(this, zh("e69caae8bf9ee68ea5"), zh("e8afb7e58588e8bf9ee68ea5204f4472697665e38082"));
        return;
    }

    if (m_scanAxisLengthSpin->value() <= 0.0
            || m_scanSpeedSpin->value() <= 0.0
            || m_stepLengthSpin->value() <= 0.0) {
        QMessageBox::warning(this,
                             zh("e58f82e695b0e697a0e69588"),
                             zh("e689abe68f8fe8bdb4e995bfe38081e689abe68f8fe9809fe5baa6e38081e6ada5e8bf9be995bfe5baa6e983bde5bf85e9a1bbe5a4a7e4ba8e2030e38082"));
        return;
    }

    if (!m_zeroPointInitialized) {
        setZeroPoint();
    }

    m_scanState = ScanState();
    m_scanState.active = true;
    m_scanState.phase = ScanHoming;
    m_scanState.forward = true;
    m_scanState.maxStepMm = qMax(0.0, m_stepAxisLengthSpin->value());

    commandAxisPosition(turnNodeId(), 0.0, m_homeSpeedSpin->value(), true);
    commandAxisPosition(driveNodeId(), 0.0, m_homeSpeedSpin->value(), false);

    updateProgramStatus(zh("e689abe68f8fe58786e5a487e4b8adefbc8ce58588e59b9ee588b0e99bb6e782b9"));
    appendLog(zh("e887aae58aa8e689abe68f8fe5b7b2e5bc80e5a78b"));
}

void MainWindow::stopProgram()
{
    if (!m_controller->isConnected()) {
        m_scanState = ScanState();
        updateProgramStatus(zh("e8afb7e58588e8bf9ee68ea5204f4472697665"));
        return;
    }

    m_scanState = ScanState();
    stopAllMotion(true);
    updateProgramStatus(zh("e8bf90e58aa8e5b7b2e7bb93e69d9f"));
}

void MainWindow::stepAdvanceOnce()
{
    if (!m_controller->isConnected()) {
        return;
    }

    if (!m_zeroPointInitialized) {
        setZeroPoint();
    }

    const double limitMm = qMax(0.0, m_stepAxisLengthSpin->value());
    const double rawNextMm = axisPositionMm(turnNodeId(), true) + m_stepLengthSpin->value();
    const double nextMm = limitMm > 0.0 ? qMin(limitMm, rawNextMm) : rawNextMm;

    commandAxisPosition(turnNodeId(), nextMm, qMax(0.1, m_jogSpeedSpin->value()), true);
    updateProgramStatus(zh("e6ada5e8bf9be8bdb4e7a7bbe58aa8e588b0202531206d6d").arg(QString::number(nextMm, 'f', 2)));
}

void MainWindow::handleNodeStatusUpdate(quint8 nodeId)
{
    Q_UNUSED(nodeId);
    syncAxisReadouts();
    updateLinkStatusDisplay();
    if (m_scanState.active) {
        advanceScanStateMachine();
    }
}

void MainWindow::buildUi()
{
    setWindowTitle(zh("4f447269766520e5b08fe8bda6e68ea7e588b6e58fb0"));
    resize(1180, 760);

    QVBoxLayout *mainLayout = new QVBoxLayout(ui->centralWidget);
    mainLayout->setContentsMargins(16, 14, 16, 14);
    mainLayout->setSpacing(14);

    buildMainInterface(mainLayout);
    buildAdvancedDialog();
}

void MainWindow::buildMainInterface(QVBoxLayout *mainLayout)
{
    QGroupBox *connectionGroup = new QGroupBox(ui->centralWidget);
    connectionGroup->setTitle(QString());
    QHBoxLayout *connectionLayout = new QHBoxLayout(connectionGroup);
    connectionLayout->setContentsMargins(18, 16, 18, 16);
    connectionLayout->setSpacing(10);

    QLabel *ipLabel = new QLabel(QStringLiteral("Link :"), connectionGroup);
    ipLabel->setStyleSheet(QStringLiteral("font-size: 26px;"));
    m_ipEdit = new QLineEdit(connectionGroup);
    m_ipEdit->setMinimumWidth(440);
    m_ipEdit->setReadOnly(true);
    m_ipEdit->setFocusPolicy(Qt::NoFocus);
    m_ipEdit->setPlaceholderText(QStringLiteral("Communication settings are configured in Advanced"));
    m_ipEdit->setStyleSheet(topInputStyle());

    m_connectButton = new QPushButton(QStringLiteral("connect"), connectionGroup);
    m_connectButton->setMinimumHeight(44);
    m_connectButton->setStyleSheet(QStringLiteral("font-size: 20px; padding: 4px 16px;"));

    m_openAdvancedButton = new QPushButton(zh("e9ab98e7baa7"), connectionGroup);
    m_openAdvancedButton->setMinimumHeight(44);
    m_openAdvancedButton->setStyleSheet(QStringLiteral("font-size: 18px; padding: 4px 14px;"));

    m_linkStatusLabel = new QLabel(zh("e69caae8bf9ee68ea5"), connectionGroup);
    m_linkStatusLabel->setMinimumWidth(90);
    m_linkStatusLabel->setStyleSheet(QStringLiteral("font-size: 18px;"));

    connectionLayout->addWidget(ipLabel);
    connectionLayout->addWidget(m_ipEdit, 1);
    connectionLayout->addWidget(m_connectButton);
    connectionLayout->addWidget(m_openAdvancedButton);
    connectionLayout->addWidget(m_linkStatusLabel);

    QHBoxLayout *contentLayout = new QHBoxLayout;
    contentLayout->setSpacing(18);

    QFrame *leftPanel = new QFrame(ui->centralWidget);
    leftPanel->setFrameShape(QFrame::StyledPanel);
    QGridLayout *leftLayout = new QGridLayout(leftPanel);
    leftLayout->setContentsMargins(16, 16, 16, 16);
    leftLayout->setHorizontalSpacing(12);
    leftLayout->setVerticalSpacing(18);

    QLabel *turnLabel = new QLabel(zh("e8bdace8bdae3a"), leftPanel);
    QLabel *driveLabel = new QLabel(zh("e9a9b1e8bdae3a"), leftPanel);
    turnLabel->setStyleSheet(QStringLiteral("font-size: 20px;"));
    driveLabel->setStyleSheet(QStringLiteral("font-size: 20px;"));
    m_turnPositionLabel = createReadoutLabel();
    m_drivePositionLabel = createReadoutLabel();
    m_setZeroButton = new QPushButton(zh("e8aebee7bdaee99bb6e782b9"), leftPanel);
    m_homeSpeedSpin = new QDoubleSpinBox(leftPanel);
    m_homeSpeedSpin->setDecimals(2);
    m_homeSpeedSpin->setRange(0.10, 100000.0);
    m_homeSpeedSpin->setValue(50.0);
    m_homeSpeedSpin->setSuffix(QStringLiteral(" mm/s"));
    m_homeSpeedSpin->setStyleSheet(QStringLiteral("font-size: 18px;"));
    m_jogSpeedSpin = new QDoubleSpinBox(leftPanel);
    m_jogSpeedSpin->setDecimals(2);
    m_jogSpeedSpin->setRange(0.10, 100000.0);
    m_jogSpeedSpin->setValue(2.0);
    m_jogSpeedSpin->setSuffix(QStringLiteral(" mm/s"));
    m_jogSpeedSpin->setStyleSheet(QStringLiteral("font-size: 18px;"));

    leftLayout->addWidget(turnLabel, 0, 0);
    leftLayout->addWidget(m_turnPositionLabel, 0, 1);
    leftLayout->addWidget(new QLabel(QStringLiteral("mm"), leftPanel), 0, 2);
    leftLayout->addWidget(driveLabel, 1, 0);
    leftLayout->addWidget(m_drivePositionLabel, 1, 1);
    leftLayout->addWidget(new QLabel(QStringLiteral("mm"), leftPanel), 1, 2);
    leftLayout->addWidget(m_setZeroButton, 2, 0, 1, 2);
    leftLayout->addWidget(new QLabel(zh("e59b9ee58e9fe782b9e9809fe5baa63a")), 3, 0);
    leftLayout->addWidget(m_homeSpeedSpin, 3, 1, 1, 2);
    leftLayout->addWidget(new QLabel(zh("e782b9e58aa8e9809fe5baa63a")), 4, 0);
    leftLayout->addWidget(m_jogSpeedSpin, 4, 1, 1, 2);
    leftLayout->setRowStretch(5, 1);

    QFrame *centerPanel = new QFrame(ui->centralWidget);
    centerPanel->setFrameShape(QFrame::StyledPanel);
    QGridLayout *centerLayout = new QGridLayout(centerPanel);
    centerLayout->setContentsMargins(20, 20, 20, 20);
    centerLayout->setHorizontalSpacing(14);
    centerLayout->setVerticalSpacing(14);

    m_forwardButton = new QPushButton(zh("e5898de8bf9b"), centerPanel);
    m_backwardButton = new QPushButton(zh("e5908ee98080"), centerPanel);
    m_leftButton = new QPushButton(zh("e5b7a6e8bdac"), centerPanel);
    m_rightButton = new QPushButton(zh("e58fb3e8bdac"), centerPanel);
    m_enableButton = new QPushButton(zh("e5bc80e5a78b"), centerPanel);

    const QList<QPushButton *> directionButtons = {m_forwardButton, m_backwardButton, m_leftButton, m_rightButton, m_enableButton};
    for (QPushButton *button : directionButtons) {
        button->setMinimumSize(110, 64);
        button->setStyleSheet(primaryButtonStyle());
    }

    centerLayout->addWidget(m_forwardButton, 0, 1);
    centerLayout->addWidget(m_leftButton, 1, 0);
    centerLayout->addWidget(m_rightButton, 1, 2);
    centerLayout->addWidget(m_backwardButton, 2, 1);
    centerLayout->addWidget(m_enableButton, 3, 1);
    centerLayout->setColumnStretch(1, 1);
    centerLayout->setRowStretch(4, 1);

    QFrame *rightPanel = new QFrame(ui->centralWidget);
    rightPanel->setFrameShape(QFrame::StyledPanel);
    QGridLayout *rightLayout = new QGridLayout(rightPanel);
    rightLayout->setContentsMargins(16, 16, 16, 16);
    rightLayout->setHorizontalSpacing(12);
    rightLayout->setVerticalSpacing(16);

    m_scanAxisLengthSpin = new QDoubleSpinBox(rightPanel);
    m_scanAxisLengthSpin->setRange(0.0, 100000.0);
    m_scanAxisLengthSpin->setDecimals(2);
    m_scanAxisLengthSpin->setValue(200.0);
    m_scanAxisLengthSpin->setSuffix(QStringLiteral(" mm"));
    m_scanAxisLengthSpin->setStyleSheet(QStringLiteral("font-size: 18px;"));

    m_scanSpeedSpin = new QDoubleSpinBox(rightPanel);
    m_scanSpeedSpin->setRange(0.0, 100000.0);
    m_scanSpeedSpin->setDecimals(2);
    m_scanSpeedSpin->setValue(50.0);
    m_scanSpeedSpin->setSuffix(QStringLiteral(" mm/s"));
    m_scanSpeedSpin->setStyleSheet(QStringLiteral("font-size: 18px;"));

    m_stepAxisLengthSpin = new QDoubleSpinBox(rightPanel);
    m_stepAxisLengthSpin->setRange(0.0, 100000.0);
    m_stepAxisLengthSpin->setDecimals(2);
    m_stepAxisLengthSpin->setValue(100.0);
    m_stepAxisLengthSpin->setSuffix(QStringLiteral(" mm"));
    m_stepAxisLengthSpin->setStyleSheet(QStringLiteral("font-size: 18px;"));

    m_stepLengthSpin = new QDoubleSpinBox(rightPanel);
    m_stepLengthSpin->setRange(0.01, 100000.0);
    m_stepLengthSpin->setDecimals(2);
    m_stepLengthSpin->setValue(21.50);
    m_stepLengthSpin->setSuffix(QStringLiteral(" mm"));
    m_stepLengthSpin->setStyleSheet(QStringLiteral("font-size: 18px;"));

    m_scanModeCombo = new QComboBox(rightPanel);
    m_scanModeCombo->addItem(zh("e5bc93e5bda2"), BowMode);
    m_scanModeCombo->addItem(zh("e58d95e59091"), OneWayMode);
    m_scanModeCombo->setStyleSheet(QStringLiteral("font-size: 18px;"));

    m_scanStartButton = new QPushButton(zh("e5bc80e5a78b"), rightPanel);
    m_stopButton = new QPushButton(zh("e7bb93e69d9f"), rightPanel);
    m_homeButton = new QPushButton(zh("e59b9ee58e9f"), rightPanel);
    m_resetButton = new QPushButton(zh("e5a48de4bd8d"), rightPanel);
    QPushButton *stepStartButton = new QPushButton(zh("e5bc80e5a78b"), rightPanel);
    stepStartButton->setObjectName(QStringLiteral("stepStartButton"));

    const QList<QPushButton *> actionButtons = {m_scanStartButton, m_stopButton, m_homeButton, m_resetButton, stepStartButton};
    for (QPushButton *button : actionButtons) {
        button->setMinimumSize(118, 58);
        button->setStyleSheet(primaryButtonStyle());
    }

    rightLayout->addWidget(new QLabel(zh("e689abe68f8fe8bdb4e995bf3a")), 0, 0);
    rightLayout->addWidget(m_scanAxisLengthSpin, 0, 1);
    rightLayout->addWidget(new QLabel(zh("e689abe68f8fe9809fe5baa63a")), 0, 2);
    rightLayout->addWidget(m_scanSpeedSpin, 0, 3);
    rightLayout->addWidget(new QLabel(zh("e6ada5e8bf9be8bdb4e995bf3a")), 1, 0);
    rightLayout->addWidget(m_stepAxisLengthSpin, 1, 1);
    rightLayout->addWidget(new QLabel(zh("e689abe68f8fe6a8a1e5bc8f3a")), 1, 2);
    rightLayout->addWidget(m_scanModeCombo, 1, 3);
    rightLayout->addWidget(new QLabel(zh("e6ada5e8bf9be995bfe5baa63a")), 2, 0);
    rightLayout->addWidget(m_stepLengthSpin, 2, 1);
    rightLayout->addWidget(m_homeButton, 3, 0);
    rightLayout->addWidget(m_resetButton, 3, 1);
    rightLayout->addWidget(m_scanStartButton, 3, 2);
    rightLayout->addWidget(stepStartButton, 3, 3);
    rightLayout->addWidget(m_stopButton, 4, 3);
    rightLayout->setRowStretch(5, 1);

    m_programStatusLabel = new QLabel(zh("e8afb7e58588e8bf9ee68ea5204f4472697665"), ui->centralWidget);
    m_programStatusLabel->setStyleSheet(QStringLiteral("QLabel { padding: 8px 12px; background: #f8f4ea; border: 1px solid #ddd1bb; border-radius: 8px; font-size: 16px; }"));

    contentLayout->addWidget(leftPanel, 1);
    contentLayout->addWidget(centerPanel, 1);
    contentLayout->addWidget(rightPanel, 2);

    mainLayout->addWidget(connectionGroup);
    mainLayout->addLayout(contentLayout, 1);
    mainLayout->addWidget(m_programStatusLabel);

    QHBoxLayout *messageLayout = new QHBoxLayout;
    messageLayout->setSpacing(12);

    QGroupBox *rxGroup = new QGroupBox(zh("e68ea5e694b6e4bfa1e681af"), ui->centralWidget);
    QVBoxLayout *rxLayout = new QVBoxLayout(rxGroup);
    m_rxEdit = new QTextEdit(rxGroup);
    m_rxEdit->setReadOnly(true);
    m_rxEdit->document()->setMaximumBlockCount(400);
    rxLayout->addWidget(m_rxEdit);

    QGroupBox *logGroup = new QGroupBox(zh("e7b3bbe7bb9fe697a5e5bf97"), ui->centralWidget);
    QVBoxLayout *logLayout = new QVBoxLayout(logGroup);
    m_logEdit = new QTextEdit(logGroup);
    m_logEdit->setReadOnly(true);
    m_logEdit->document()->setMaximumBlockCount(400);
    logLayout->addWidget(m_logEdit);

    messageLayout->addWidget(rxGroup, 1);
    messageLayout->addWidget(logGroup, 1);
    mainLayout->addLayout(messageLayout, 1);

    connect(m_connectButton, &QPushButton::clicked,
            this, &MainWindow::toggleConnection);
    connect(m_openAdvancedButton, &QPushButton::clicked,
            this, &MainWindow::openAdvancedPanel);
    connect(m_setZeroButton, &QPushButton::clicked,
            this, &MainWindow::setZeroPoint);
    connect(m_homeButton, &QPushButton::clicked,
            this, &MainWindow::sendHome);
    connect(m_resetButton, &QPushButton::clicked,
            this, &MainWindow::resetMotionState);
    connect(m_enableButton, &QPushButton::clicked,
            this, &MainWindow::enableAxes);
    connect(m_scanStartButton, &QPushButton::clicked,
            this, &MainWindow::startScanProgram);
    connect(stepStartButton, &QPushButton::clicked,
            this, &MainWindow::stepAdvanceOnce);
    connect(m_stopButton, &QPushButton::clicked,
            this, &MainWindow::stopProgram);

    auto connectJogButton = [this](QPushButton *button, bool turnAxis, double direction) {
        connect(button, &QPushButton::pressed, this, [this, turnAxis, direction]() {
            if (!m_controller->isConnected()) {
                return;
            }

            if (m_scanState.active) {
                m_scanState = ScanState();
                updateProgramStatus(zh("e6898be58aa8e782b9e58aa8e5b7b2e4b8ade6ada2e887aae58aa8e689abe68f8f"));
            }

            const quint8 nodeId = turnAxis ? turnNodeId() : driveNodeId();
            commandAxisVelocity(nodeId, direction * qMax(0.1, m_jogSpeedSpin->value()), turnAxis);
        });

        connect(button, &QPushButton::released, this, [this, turnAxis]() {
            if (!m_controller->isConnected()) {
                return;
            }

            const quint8 nodeId = turnAxis ? turnNodeId() : driveNodeId();
            commandAxisVelocity(nodeId, 0.0, turnAxis);
            updateProgramStatus(zh("e782b9e58aa8e5819ce6ada2"));
        });
    };

    connectJogButton(m_forwardButton, false, 1.0);
    connectJogButton(m_backwardButton, false, -1.0);
    connectJogButton(m_leftButton, true, -1.0);
    connectJogButton(m_rightButton, true, 1.0);
}

void MainWindow::buildAdvancedDialog()
{
    m_advancedDialog = new QDialog(this);
    m_advancedDialog->setWindowTitle(zh("e9ab98e7baa7e99da2e69dbf"));
    m_advancedDialog->resize(1180, 860);

    QVBoxLayout *layout = new QVBoxLayout(m_advancedDialog);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(12);

    QGroupBox *communicationGroup = new QGroupBox(zh("e9809ae4bfa1e9858de7bdae"), m_advancedDialog);
    QGridLayout *communicationLayout = new QGridLayout(communicationGroup);

    m_pluginCombo = new QComboBox(communicationGroup);
    m_interfaceCombo = new QComboBox(communicationGroup);
    m_interfaceCombo->setEditable(true);
    m_bitrateSpin = new QSpinBox(communicationGroup);
    m_bitrateSpin->setRange(10000, 1000000);
    m_bitrateSpin->setSingleStep(10000);
    m_bitrateSpin->setValue(250000);
    m_bitrateSpin->setSuffix(QStringLiteral(" bps"));
    m_serialBaudSpin = new QSpinBox(communicationGroup);
    m_serialBaudSpin->setRange(1200, 5000000);
    m_serialBaudSpin->setSingleStep(100000);
    m_serialBaudSpin->setValue(1000000);
    m_serialBaudSpin->setSuffix(QStringLiteral(" bps"));
    m_refreshInterfacesButton = new QPushButton(zh("e588b7e696b0e68ea5e58fa3"), communicationGroup);

    communicationLayout->addWidget(new QLabel(zh("e68f92e4bbb6")), 0, 0);
    communicationLayout->addWidget(m_pluginCombo, 0, 1);
    communicationLayout->addWidget(new QLabel(zh("e68ea5e58fa3")), 0, 2);
    communicationLayout->addWidget(m_interfaceCombo, 0, 3);
    communicationLayout->addWidget(m_refreshInterfacesButton, 0, 4);
    communicationLayout->addWidget(new QLabel(zh("e6b3a2e789b9e78e87")), 1, 0);
    communicationLayout->addWidget(m_bitrateSpin, 1, 1);
    communicationLayout->addWidget(new QLabel(QStringLiteral("Serial Baud")), 1, 2);
    communicationLayout->addWidget(m_serialBaudSpin, 1, 3);
    communicationLayout->setColumnStretch(3, 1);

    QGroupBox *mappingGroup = new QGroupBox(zh("e88a82e782b9e4b88ee68da2e7ae97"), m_advancedDialog);
    QGridLayout *mappingLayout = new QGridLayout(mappingGroup);

    m_nodeIdSpin = new QSpinBox(mappingGroup);
    m_nodeIdSpin->setRange(0, 63);
    m_nodeIdSpin->setValue(16);
    m_turnNodeIdSpin = new QSpinBox(mappingGroup);
    m_turnNodeIdSpin->setRange(0, 63);
    m_turnNodeIdSpin->setValue(16);
    m_driveNodeIdSpin = new QSpinBox(mappingGroup);
    m_driveNodeIdSpin->setRange(0, 63);
    m_driveNodeIdSpin->setValue(1);

    m_turnScaleSpin = new QDoubleSpinBox(mappingGroup);
    m_turnScaleSpin->setDecimals(4);
    m_turnScaleSpin->setRange(0.0001, 100000.0);
    m_turnScaleSpin->setValue(1.0);
    m_turnScaleSpin->setSuffix(zh("206d6d2fe59c88"));
    m_driveScaleSpin = new QDoubleSpinBox(mappingGroup);
    m_driveScaleSpin->setDecimals(4);
    m_driveScaleSpin->setRange(0.0001, 100000.0);
    m_driveScaleSpin->setValue(1.0);
    m_driveScaleSpin->setSuffix(zh("206d6d2fe59c88"));

    mappingLayout->addWidget(new QLabel(zh("e8b083e8af95e88a82e782b9")), 0, 0);
    mappingLayout->addWidget(m_nodeIdSpin, 0, 1);
    mappingLayout->addWidget(new QLabel(zh("e8bdace8bdaee88a82e782b9")), 0, 2);
    mappingLayout->addWidget(m_turnNodeIdSpin, 0, 3);
    mappingLayout->addWidget(new QLabel(zh("e9a9b1e8bdaee88a82e782b9")), 0, 4);
    mappingLayout->addWidget(m_driveNodeIdSpin, 0, 5);
    mappingLayout->addWidget(new QLabel(zh("e8bdace8bdaee6af8fe59c88")), 1, 0);
    mappingLayout->addWidget(m_turnScaleSpin, 1, 1);
    mappingLayout->addWidget(new QLabel(zh("e9a9b1e8bdaee6af8fe59c88")), 1, 2);
    mappingLayout->addWidget(m_driveScaleSpin, 1, 3);
    mappingLayout->setColumnStretch(5, 1);

    layout->addWidget(communicationGroup);
    layout->addWidget(mappingGroup);

    QGroupBox *txGroup = new QGroupBox(QStringLiteral("Transmit"), m_advancedDialog);
    QGridLayout *txLayout = new QGridLayout(txGroup);

    m_txIdEdit = new QLineEdit(txGroup);
    m_txIdEdit->setPlaceholderText(QStringLiteral("0x123 or 291"));
    m_txIdEdit->setStyleSheet(topInputStyle());

    m_txDlcSpin = new QSpinBox(txGroup);
    m_txDlcSpin->setRange(0, 8);
    m_txDlcSpin->setValue(8);
    m_txDlcSpin->setStyleSheet(topInputStyle());

    m_txDataEdit = new QLineEdit(txGroup);
    m_txDataEdit->setPlaceholderText(QStringLiteral("hex: 01 02 03 or ascii: hello"));
    m_txDataEdit->setStyleSheet(topInputStyle());

    m_txExtendedCheck = new QCheckBox(QStringLiteral("Extended (29-bit)"), txGroup);
    m_txRemoteCheck = new QCheckBox(QStringLiteral("Remote (RTR)"), txGroup);

    m_txPeriodSpin = new QSpinBox(txGroup);
    m_txPeriodSpin->setRange(0, 600000);
    m_txPeriodSpin->setValue(0);
    m_txPeriodSpin->setSuffix(QStringLiteral(" ms"));
    m_txPeriodSpin->setStyleSheet(topInputStyle());
    m_txPeriodSpin->setToolTip(QStringLiteral("0 = send once, >0 = repeat interval"));

    m_txSendButton = new QPushButton(QStringLiteral("Send"), txGroup);
    m_txSendButton->setMinimumHeight(44);
    m_txSendButton->setStyleSheet(QStringLiteral("font-size: 18px; padding: 4px 14px;"));

    m_txToggleButton = new QPushButton(QStringLiteral("Start Repeat"), txGroup);
    m_txToggleButton->setMinimumHeight(44);
    m_txToggleButton->setStyleSheet(QStringLiteral("font-size: 18px; padding: 4px 14px;"));

    txLayout->addWidget(new QLabel(QStringLiteral("ID")), 0, 0);
    txLayout->addWidget(m_txIdEdit, 0, 1);
    txLayout->addWidget(new QLabel(QStringLiteral("DLC")), 0, 2);
    txLayout->addWidget(m_txDlcSpin, 0, 3);
    txLayout->addWidget(m_txExtendedCheck, 0, 4);

    txLayout->addWidget(new QLabel(QStringLiteral("Data")), 1, 0);
    txLayout->addWidget(m_txDataEdit, 1, 1, 1, 3);
    txLayout->addWidget(m_txRemoteCheck, 1, 4);

    txLayout->addWidget(new QLabel(QStringLiteral("Repeat")), 2, 0);
    txLayout->addWidget(m_txPeriodSpin, 2, 1);
    txLayout->addWidget(m_txSendButton, 2, 3);
    txLayout->addWidget(m_txToggleButton, 2, 4);
    txLayout->setColumnStretch(1, 1);

    layout->addWidget(txGroup);
    buildAdvancedContent(layout, m_advancedDialog);

    connect(m_pluginCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::refreshCanInterfaces);
    connect(m_refreshInterfacesButton, &QPushButton::clicked,
            this, &MainWindow::refreshCanInterfaces);
    connect(m_interfaceCombo, &QComboBox::currentTextChanged,
            this, &MainWindow::syncConnectionEditorsFromAdvanced);
    connect(m_bitrateSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, [this](int) { syncConnectionEditorsFromAdvanced(); });
    connect(m_serialBaudSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, [this](int) { syncConnectionEditorsFromAdvanced(); });

    m_txTimer = new QTimer(this);
    m_txTimer->setInterval(100);
    connect(m_txSendButton, &QPushButton::clicked, this, &MainWindow::sendCustomTxOnce);
    connect(m_txToggleButton, &QPushButton::clicked, this, &MainWindow::toggleCustomTx);
    connect(m_txTimer, &QTimer::timeout, this, &MainWindow::sendCustomTxOnce);

    auto nodeChanged = [this]() {
        m_controller->setDefaultNodeId(static_cast<quint8>(m_nodeIdSpin->value()));
        updateTrackedNodes();
        syncAxisReadouts();
        updateStatusPanel();
    };

    connect(m_nodeIdSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, [this, nodeChanged](int) { nodeChanged(); syncConnectionEditorsFromAdvanced(); });
    connect(m_turnNodeIdSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, [nodeChanged](int) { nodeChanged(); });
    connect(m_driveNodeIdSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, [nodeChanged](int) { nodeChanged(); });
    connect(m_turnScaleSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, [this](double) { syncAxisReadouts(); });
    connect(m_driveScaleSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, [this](double) { syncAxisReadouts(); });
}

void MainWindow::buildAdvancedContent(QVBoxLayout *mainLayout, QWidget *parent)
{
    QGroupBox *axisStateGroup = new QGroupBox(zh("e8bdb4e78ab6e68081e4b88ee4bf9de68aa4"), parent);
    QHBoxLayout *axisStateLayout = new QHBoxLayout(axisStateGroup);
    m_idleButton = new QPushButton(zh("e8bf9be585a5e7a9bae997b2"), axisStateGroup);
    m_motorCalibrationButton = new QPushButton(zh("e794b5e69cbae6a0a1e58786"), axisStateGroup);
    m_fullCalibrationButton = new QPushButton(zh("e695b4e5a597e6a0a1e58786"), axisStateGroup);
    m_closedLoopButton = new QPushButton(zh("e8bf9be585a5e997ade78eaf"), axisStateGroup);
    m_estopButton = new QPushButton(zh("e680a5e5819c"), axisStateGroup);
    m_clearErrorsButton = new QPushButton(zh("e6b885e999a4e99499e8afaf"), axisStateGroup);
    m_refreshStatusButton = new QPushButton(zh("e588b7e696b0e78ab6e68081"), axisStateGroup);

    axisStateLayout->addWidget(m_idleButton);
    axisStateLayout->addWidget(m_motorCalibrationButton);
    axisStateLayout->addWidget(m_fullCalibrationButton);
    axisStateLayout->addWidget(m_closedLoopButton);
    axisStateLayout->addWidget(m_estopButton);
    axisStateLayout->addWidget(m_clearErrorsButton);
    axisStateLayout->addWidget(m_refreshStatusButton);

    QHBoxLayout *middleLayout = new QHBoxLayout;
    middleLayout->setSpacing(12);

    QVBoxLayout *leftColumn = new QVBoxLayout;
    leftColumn->setSpacing(12);

    QGroupBox *modeGroup = new QGroupBox(zh("e68ea7e588b6e6a8a1e5bc8f"), parent);
    QGridLayout *modeLayout = new QGridLayout(modeGroup);
    m_controlModeCombo = new QComboBox(modeGroup);
    m_inputModeCombo = new QComboBox(modeGroup);
    m_applyModeButton = new QPushButton(zh("e5ba94e794a8e6a8a1e5bc8f"), modeGroup);

    modeLayout->addWidget(new QLabel(zh("e68ea7e588b6e696b9e5bc8f")), 0, 0);
    modeLayout->addWidget(m_controlModeCombo, 0, 1);
    modeLayout->addWidget(new QLabel(zh("e8be93e585a5e696b9e5bc8f")), 1, 0);
    modeLayout->addWidget(m_inputModeCombo, 1, 1);
    modeLayout->addWidget(m_applyModeButton, 0, 2, 2, 1);

    QGroupBox *positionGroup = new QGroupBox(zh("e4bd8de7bdaee68ea7e588b6"), parent);
    QGridLayout *positionLayout = new QGridLayout(positionGroup);
    m_positionSpin = new QDoubleSpinBox(positionGroup);
    m_positionSpin->setRange(-100000.0, 100000.0);
    m_positionSpin->setDecimals(3);
    m_positionSpin->setSuffix(zh("20e59c88"));
    m_positionVelFfSpin = new QDoubleSpinBox(positionGroup);
    m_positionVelFfSpin->setRange(-1000.0, 1000.0);
    m_positionVelFfSpin->setDecimals(3);
    m_positionVelFfSpin->setSuffix(zh("20e59c882fe7a792"));
    m_positionTorqueFfSpin = new QDoubleSpinBox(positionGroup);
    m_positionTorqueFfSpin->setRange(-100.0, 100.0);
    m_positionTorqueFfSpin->setDecimals(3);
    m_positionTorqueFfSpin->setSuffix(QStringLiteral(" Nm"));
    m_sendPositionButton = new QPushButton(zh("e58f91e98081e4bd8de7bdae"), positionGroup);

    positionLayout->addWidget(new QLabel(zh("e79baee6a087e4bd8de7bdae")), 0, 0);
    positionLayout->addWidget(m_positionSpin, 0, 1);
    positionLayout->addWidget(new QLabel(zh("e9809fe5baa6e5898de9a688")), 1, 0);
    positionLayout->addWidget(m_positionVelFfSpin, 1, 1);
    positionLayout->addWidget(new QLabel(zh("e58a9be79fa9e5898de9a688")), 2, 0);
    positionLayout->addWidget(m_positionTorqueFfSpin, 2, 1);
    positionLayout->addWidget(m_sendPositionButton, 0, 2, 3, 1);

    QGroupBox *velocityGroup = new QGroupBox(zh("e9809fe5baa6e68ea7e588b6"), parent);
    QGridLayout *velocityLayout = new QGridLayout(velocityGroup);
    m_velocitySpin = new QDoubleSpinBox(velocityGroup);
    m_velocitySpin->setRange(-1000.0, 1000.0);
    m_velocitySpin->setDecimals(3);
    m_velocitySpin->setSuffix(zh("20e59c882fe7a792"));
    m_velocityTorqueFfSpin = new QDoubleSpinBox(velocityGroup);
    m_velocityTorqueFfSpin->setRange(-100.0, 100.0);
    m_velocityTorqueFfSpin->setDecimals(3);
    m_velocityTorqueFfSpin->setSuffix(QStringLiteral(" Nm"));
    m_sendVelocityButton = new QPushButton(zh("e58f91e98081e9809fe5baa6"), velocityGroup);

    velocityLayout->addWidget(new QLabel(zh("e79baee6a087e9809fe5baa6")), 0, 0);
    velocityLayout->addWidget(m_velocitySpin, 0, 1);
    velocityLayout->addWidget(new QLabel(zh("e58a9be79fa9e5898de9a688")), 1, 0);
    velocityLayout->addWidget(m_velocityTorqueFfSpin, 1, 1);
    velocityLayout->addWidget(m_sendVelocityButton, 0, 2, 2, 1);

    QGroupBox *torqueGroup = new QGroupBox(zh("e58a9be79fa9e68ea7e588b6"), parent);
    QGridLayout *torqueLayout = new QGridLayout(torqueGroup);
    m_torqueSpin = new QDoubleSpinBox(torqueGroup);
    m_torqueSpin->setRange(-100.0, 100.0);
    m_torqueSpin->setDecimals(3);
    m_torqueSpin->setSuffix(QStringLiteral(" Nm"));
    m_sendTorqueButton = new QPushButton(zh("e58f91e98081e58a9be79fa9"), torqueGroup);

    torqueLayout->addWidget(new QLabel(zh("e79baee6a087e58a9be79fa9")), 0, 0);
    torqueLayout->addWidget(m_torqueSpin, 0, 1);
    torqueLayout->addWidget(m_sendTorqueButton, 0, 2);

    QGroupBox *limitGroup = new QGroupBox(zh("e8bf90e8a18ce99990e5b985"), parent);
    QGridLayout *limitLayout = new QGridLayout(limitGroup);
    m_velocityLimitSpin = new QDoubleSpinBox(limitGroup);
    m_velocityLimitSpin->setRange(0.0, 100000.0);
    m_velocityLimitSpin->setDecimals(3);
    m_velocityLimitSpin->setValue(20.0);
    m_velocityLimitSpin->setSuffix(zh("20e59c882fe7a792"));
    m_currentLimitSpin = new QDoubleSpinBox(limitGroup);
    m_currentLimitSpin->setRange(0.0, 1000.0);
    m_currentLimitSpin->setDecimals(3);
    m_currentLimitSpin->setValue(10.0);
    m_currentLimitSpin->setSuffix(QStringLiteral(" A"));
    m_applyLimitsButton = new QPushButton(zh("e5ba94e794a8e99990e5b985"), limitGroup);

    limitLayout->addWidget(new QLabel(zh("e9809fe5baa6e99990e5b985")), 0, 0);
    limitLayout->addWidget(m_velocityLimitSpin, 0, 1);
    limitLayout->addWidget(new QLabel(zh("e794b5e6b581e99990e5b985")), 1, 0);
    limitLayout->addWidget(m_currentLimitSpin, 1, 1);
    limitLayout->addWidget(m_applyLimitsButton, 0, 2, 2, 1);

    leftColumn->addWidget(modeGroup);
    leftColumn->addWidget(positionGroup);
    leftColumn->addWidget(velocityGroup);
    leftColumn->addWidget(torqueGroup);
    leftColumn->addWidget(limitGroup);
    leftColumn->addStretch(1);

    QGroupBox *statusGroup = new QGroupBox(zh("e78ab6e68081e79b91e68ea7"), parent);
    QGridLayout *statusLayout = new QGridLayout(statusGroup);

    int row = 0;
    statusLayout->addWidget(new QLabel(zh("e5bf83e8b7b3e99499e8afaf")), row, 0);
    m_axisErrorValue = createValueLabel();
    statusLayout->addWidget(m_axisErrorValue, row++, 1);
    statusLayout->addWidget(new QLabel(zh("e5bd93e5898de99499e8afaf")), row, 0);
    m_activeErrorsValue = createValueLabel();
    statusLayout->addWidget(m_activeErrorsValue, row++, 1);
    statusLayout->addWidget(new QLabel(zh("e5a4b1e883bde58e9fe59ba0")), row, 0);
    m_disarmReasonValue = createValueLabel();
    statusLayout->addWidget(m_disarmReasonValue, row++, 1);
    statusLayout->addWidget(new QLabel(zh("e8bdb4e78ab6e68081")), row, 0);
    m_axisStateValue = createValueLabel();
    statusLayout->addWidget(m_axisStateValue, row++, 1);
    statusLayout->addWidget(new QLabel(zh("e6b581e7a88be7bb93e69e9c")), row, 0);
    m_procedureResultValue = createValueLabel();
    statusLayout->addWidget(m_procedureResultValue, row++, 1);
    statusLayout->addWidget(new QLabel(zh("e8bda8e8bfb9e5ae8ce68890")), row, 0);
    m_trajectoryDoneValue = createValueLabel();
    statusLayout->addWidget(m_trajectoryDoneValue, row++, 1);
    statusLayout->addWidget(new QLabel(zh("e4bd8de7bdae")), row, 0);
    m_positionValue = createValueLabel();
    statusLayout->addWidget(m_positionValue, row++, 1);
    statusLayout->addWidget(new QLabel(zh("e9809fe5baa6")), row, 0);
    m_velocityValue = createValueLabel();
    statusLayout->addWidget(m_velocityValue, row++, 1);
    statusLayout->addWidget(new QLabel(zh("e6af8de7babfe794b5e58e8b")), row, 0);
    m_busVoltageValue = createValueLabel();
    statusLayout->addWidget(m_busVoltageValue, row++, 1);
    statusLayout->addWidget(new QLabel(zh("e6af8de7babfe794b5e6b581")), row, 0);
    m_busCurrentValue = createValueLabel();
    statusLayout->addWidget(m_busCurrentValue, row++, 1);
    statusLayout->addWidget(new QLabel(zh("497120e8aebee5ae9a2fe6b58be9878f")), row, 0);
    m_iqValue = createValueLabel();
    statusLayout->addWidget(m_iqValue, row++, 1);
    statusLayout->addWidget(new QLabel(zh("4645542fe794b5e69cbae6b8a9e5baa6")), row, 0);
    m_temperatureValue = createValueLabel();
    statusLayout->addWidget(m_temperatureValue, row++, 1);
    statusLayout->addWidget(new QLabel(zh("e79baee6a0872fe4bcb0e7ae97e58a9be79fa9")), row, 0);
    m_torqueValue = createValueLabel();
    statusLayout->addWidget(m_torqueValue, row++, 1);
    statusLayout->addWidget(new QLabel(zh("e794b5e58a9fe78e872fe69cbae6a2b0e58a9fe78e87")), row, 0);
    m_powerValue = createValueLabel();
    statusLayout->addWidget(m_powerValue, row++, 1);
    statusLayout->addWidget(new QLabel(zh("e69c80e5908ee69bb4e696b0e697b6e997b4")), row, 0);
    m_lastUpdateValue = createValueLabel();
    statusLayout->addWidget(m_lastUpdateValue, row++, 1);
    statusLayout->setColumnStretch(1, 1);

    middleLayout->addLayout(leftColumn, 1);
    middleLayout->addWidget(statusGroup, 1);

    mainLayout->addWidget(axisStateGroup);
    mainLayout->addLayout(middleLayout, 1);

    connect(m_idleButton, &QPushButton::clicked,
            this, &MainWindow::requestIdle);
    connect(m_motorCalibrationButton, &QPushButton::clicked,
            this, &MainWindow::requestMotorCalibration);
    connect(m_fullCalibrationButton, &QPushButton::clicked,
            this, &MainWindow::requestFullCalibration);
    connect(m_closedLoopButton, &QPushButton::clicked,
            this, &MainWindow::requestClosedLoop);
    connect(m_applyModeButton, &QPushButton::clicked,
            this, &MainWindow::applyControllerMode);
    connect(m_sendPositionButton, &QPushButton::clicked,
            this, &MainWindow::sendPositionCommand);
    connect(m_sendVelocityButton, &QPushButton::clicked,
            this, &MainWindow::sendVelocityCommand);
    connect(m_sendTorqueButton, &QPushButton::clicked,
            this, &MainWindow::sendTorqueCommand);
    connect(m_applyLimitsButton, &QPushButton::clicked,
            this, &MainWindow::applyLimits);
    connect(m_refreshStatusButton, &QPushButton::clicked,
            this, &MainWindow::refreshTelemetry);
    connect(m_estopButton, &QPushButton::clicked, this, [this]() {
        m_controller->estop();
    });
    connect(m_clearErrorsButton, &QPushButton::clicked, this, [this]() {
        m_controller->clearErrors(false);
    });
}

void MainWindow::populatePlugins()
{
    const QStringList plugins = m_controller->availablePlugins();
    const QString previousPlugin = currentPluginId(m_pluginCombo);
    const bool systemCanReady = checkSystemCanDriver();

    m_pluginCombo->blockSignals(true);
    m_pluginCombo->clear();
    for (const QString &plugin : plugins) {
        m_pluginCombo->addItem(pluginLabel(plugin), plugin);
    }

    const int autoIndex = m_pluginCombo->findData(QStringLiteral("auto"));
    const int systemCanIndex = m_pluginCombo->findData(QStringLiteral("systeccan"));
    const int candleIndex = m_pluginCombo->findData(QStringLiteral("candle"));
    const int slcanIndex = m_pluginCombo->findData(QStringLiteral("slcan"));
    const int socketCanIndex = m_pluginCombo->findData(QStringLiteral("socketcan"));
    const int peakCanIndex = m_pluginCombo->findData(QStringLiteral("peakcan"));
    const int passThruIndex = m_pluginCombo->findData(QStringLiteral("passthrucan"));
    const int tinyCanIndex = m_pluginCombo->findData(QStringLiteral("tinycan"));
    const int vectorCanIndex = m_pluginCombo->findData(QStringLiteral("vectorcan"));
    const int virtualCanIndex = m_pluginCombo->findData(QStringLiteral("virtualcan"));
    const int previousIndex = previousPlugin.isEmpty() ? -1 : m_pluginCombo->findData(previousPlugin);

    if (previousIndex >= 0) {
        m_pluginCombo->setCurrentIndex(previousIndex);
    } else if (autoIndex >= 0) {
        m_pluginCombo->setCurrentIndex(autoIndex);
    } else if (slcanIndex >= 0) {
        m_pluginCombo->setCurrentIndex(slcanIndex);
    } else if (candleIndex >= 0) {
        m_pluginCombo->setCurrentIndex(candleIndex);
    } else if (systemCanIndex >= 0 && systemCanReady) {
        m_pluginCombo->setCurrentIndex(systemCanIndex);
    } else if (socketCanIndex >= 0) {
        m_pluginCombo->setCurrentIndex(socketCanIndex);
    } else if (peakCanIndex >= 0) {
        m_pluginCombo->setCurrentIndex(peakCanIndex);
    } else if (passThruIndex >= 0) {
        m_pluginCombo->setCurrentIndex(passThruIndex);
    } else if (tinyCanIndex >= 0) {
        m_pluginCombo->setCurrentIndex(tinyCanIndex);
    } else if (vectorCanIndex >= 0) {
        m_pluginCombo->setCurrentIndex(vectorCanIndex);
    } else if (virtualCanIndex >= 0) {
        m_pluginCombo->setCurrentIndex(virtualCanIndex);
    } else if (systemCanIndex >= 0) {
        m_pluginCombo->setCurrentIndex(systemCanIndex);
    }
    m_pluginCombo->blockSignals(false);
}

void MainWindow::populateControlModes()
{
    m_controlModeCombo->clear();
    m_controlModeCombo->addItem(zh("e4bd8de7bdaee68ea7e588b6"), ODriveMotorController::PositionControl);
    m_controlModeCombo->addItem(zh("e9809fe5baa6e68ea7e588b6"), ODriveMotorController::VelocityControl);
    m_controlModeCombo->addItem(zh("e58a9be79fa9e68ea7e588b6"), ODriveMotorController::TorqueControl);
    m_controlModeCombo->addItem(zh("e794b5e58e8be68ea7e588b6"), ODriveMotorController::VoltageControl);
}

void MainWindow::populateInputModes()
{
    m_inputModeCombo->clear();
    m_inputModeCombo->addItem(zh("e9808fe4bca0e6a8a1e5bc8f"), ODriveMotorController::Passthrough);
    m_inputModeCombo->addItem(zh("e9809fe5baa6e6969ce59da1"), ODriveMotorController::VelRamp);
    m_inputModeCombo->addItem(zh("e4bd8de7bdaee6bba4e6b3a2"), ODriveMotorController::PosFilter);
    m_inputModeCombo->addItem(zh("e6a2afe5bda2e8bda8e8bfb9"), ODriveMotorController::TrapTraj);
    m_inputModeCombo->addItem(zh("e58a9be79fa9e6969ce59da1"), ODriveMotorController::TorqueRamp);
    m_inputModeCombo->addItem(zh("e69caae6bf80e6b4bb"), ODriveMotorController::Inactive);
    m_inputModeCombo->setCurrentIndex(0);
}

void MainWindow::setMotionControlsEnabled(bool enabled)
{
    const QList<QWidget *> widgets = {
        m_setZeroButton,
        m_homeSpeedSpin,
        m_jogSpeedSpin,
        m_forwardButton,
        m_backwardButton,
        m_leftButton,
        m_rightButton,
        m_enableButton,
        m_scanAxisLengthSpin,
        m_scanSpeedSpin,
        m_stepAxisLengthSpin,
        m_stepLengthSpin,
        m_scanModeCombo,
        m_homeButton,
        m_resetButton,
        m_scanStartButton,
        m_stopButton,
        m_controlModeCombo,
        m_inputModeCombo,
        m_applyModeButton,
        m_idleButton,
        m_motorCalibrationButton,
        m_fullCalibrationButton,
        m_closedLoopButton,
        m_estopButton,
        m_clearErrorsButton,
        m_refreshStatusButton,
        m_positionSpin,
        m_positionVelFfSpin,
        m_positionTorqueFfSpin,
        m_sendPositionButton,
        m_velocitySpin,
        m_velocityTorqueFfSpin,
        m_sendVelocityButton,
        m_torqueSpin,
        m_sendTorqueButton,
        m_velocityLimitSpin,
        m_currentLimitSpin,
        m_applyLimitsButton
    };

    for (QWidget *widget : widgets) {
        if (widget) {
            widget->setEnabled(enabled);
        }
    }
}

void MainWindow::updateTrackedNodes()
{
    QList<quint8> nodeIds;
    const QList<int> rawNodeIds = {
        m_nodeIdSpin ? m_nodeIdSpin->value() : 0,
        m_turnNodeIdSpin ? m_turnNodeIdSpin->value() : 0,
        m_driveNodeIdSpin ? m_driveNodeIdSpin->value() : 0
    };

    for (const int rawNodeId : rawNodeIds) {
        const quint8 nodeId = static_cast<quint8>(rawNodeId & 0x3F);
        if (!nodeIds.contains(nodeId)) {
            nodeIds << nodeId;
        }
    }

    m_controller->setDefaultNodeId(static_cast<quint8>(m_nodeIdSpin->value()));
    m_controller->setTrackedNodeIds(nodeIds);

    if (m_controller->isConnected()) {
        m_controller->requestTrackedTelemetry();
    }
}

void MainWindow::syncConnectionEditorsFromAdvanced()
{
    if (!m_ipEdit || !m_pluginCombo || !m_interfaceCombo || !m_bitrateSpin || !m_nodeIdSpin) {
        return;
    }

    const QString plugin = currentPluginId(m_pluginCombo);
    const QString interfaceName = m_interfaceCombo->currentText().trimmed();
    QString summary = pluginLabel(plugin);
    if (!interfaceName.isEmpty()) {
        summary += QStringLiteral(" | %1").arg(interfaceName);
    }
    summary += QStringLiteral(" | %1 bps | node %2")
            .arg(QString::number(m_bitrateSpin->value()),
                 QString::number(m_nodeIdSpin->value()));

    m_ipEdit->blockSignals(true);
    m_ipEdit->setText(summary);
    m_ipEdit->blockSignals(false);
}

void MainWindow::updateConnectionEditorHints()
{
    if (!m_pluginCombo || !m_ipEdit || !m_serialBaudSpin) {
        return;
    }

    const QString plugin = currentPluginId(m_pluginCombo);
    const bool serialTransport = (plugin == QStringLiteral("auto") || plugin == QStringLiteral("slcan"));

    if (plugin == QStringLiteral("candle")) {
        m_ipEdit->setToolTip(QStringLiteral("Communication settings are configured in Advanced. Current transport is Candle / gs_usb."));
    } else if (serialTransport) {
        m_ipEdit->setToolTip(QStringLiteral("Communication settings are configured in Advanced. Current transport is SLCAN over a virtual COM port."));
    } else {
        m_ipEdit->setToolTip(QStringLiteral("Communication settings are configured in Advanced."));
    }

    const QString transportTip = serialTransport
            ? QStringLiteral("USB serial transport baud for SLCAN probing. CANgaroo commonly uses 1000000; other adapters may use 2000000 or 115200.")
            : QStringLiteral("Serial transport baud is ignored for the selected plugin.");
    m_serialBaudSpin->setToolTip(transportTip);
    m_serialBaudSpin->setEnabled(!m_controller->isConnected() && serialTransport);
}

void MainWindow::updateLinkStatusDisplay()
{
    if (!m_linkStatusLabel) {
        return;
    }

    if (!m_controller->isConnected()) {
        m_linkStatusLabel->setText(zh("e69caae8bf9ee68ea5"));
        m_linkStatusLabel->setStyleSheet(QStringLiteral("QLabel { color: #9c2f2f; font-weight: 600; font-size: 18px; }"));
        return;
    }

    if (currentPluginId(m_pluginCombo) == QStringLiteral("virtualcan")) {
        m_linkStatusLabel->setText(zh("e8999ae68b9f2043414e20e5b7b2e8bf9ee68ea5"));
        m_linkStatusLabel->setStyleSheet(QStringLiteral("QLabel { color: #8a5600; font-weight: 600; font-size: 18px; }"));
        return;
    }

    if (hasFreshTrackedHeartbeat()) {
        m_linkStatusLabel->setText(zh("4f447269766520e59ca8e7babf"));
        m_linkStatusLabel->setStyleSheet(QStringLiteral("QLabel { color: #0b7a37; font-weight: 600; font-size: 18px; }"));
        return;
    }

    if (hasAnyTrackedNodeResponse()) {
        m_linkStatusLabel->setText(QStringLiteral("ODrive response, no heartbeat"));
        m_linkStatusLabel->setStyleSheet(QStringLiteral("QLabel { color: #ad6f00; font-weight: 600; font-size: 18px; }"));
        return;
    }

    m_linkStatusLabel->setText(zh("43414e20e5b7b2e8bf9ee68ea5"));
    m_linkStatusLabel->setStyleSheet(QStringLiteral("QLabel { color: #ad6f00; font-weight: 600; font-size: 18px; }"));
}

bool MainWindow::hasAnyTrackedNodeResponse() const
{
    const QDateTime now = QDateTime::currentDateTime();
    const QList<quint8> nodeIds = m_controller->trackedNodeIds();

    for (const quint8 nodeId : nodeIds) {
        const ODriveMotorController::AxisStatus &status = m_controller->status(nodeId);
        if (status.lastMessage.isValid() && status.lastMessage.msecsTo(now) <= 2000) {
            return true;
        }
    }

    return false;
}

bool MainWindow::hasFreshTrackedHeartbeat() const
{
    constexpr qint64 heartbeatTimeoutMs = 1500;

    const QDateTime now = QDateTime::currentDateTime();
    QList<quint8> nodeIds = m_controller->trackedNodeIds();
    if (nodeIds.isEmpty()) {
        nodeIds << m_controller->nodeId();
    }

    for (const quint8 nodeId : nodeIds) {
        const ODriveMotorController::AxisStatus &status = m_controller->status(nodeId);
        if (!status.lastHeartbeat.isValid()) {
            continue;
        }

        qint64 ageMs = status.lastHeartbeat.msecsTo(now);
        if (ageMs < 0) {
            ageMs = 0;
        }

        if (ageMs <= heartbeatTimeoutMs) {
            return true;
        }
    }

    return false;
}

void MainWindow::updateProgramStatus(const QString &message)
{
    if (m_programStatusLabel) {
        m_programStatusLabel->setText(message);
    }
}

void MainWindow::advanceScanStateMachine()
{
    if (!m_scanState.active) {
        return;
    }

    switch (m_scanState.phase) {
    case ScanHoming:
        if (axisReached(turnNodeId(), 0.0, true) && axisReached(driveNodeId(), 0.0, false)) {
            m_scanState.phase = ScanSweeping;
            m_scanState.forward = true;
            startScanSweep(m_scanAxisLengthSpin->value());
        }
        break;
    case ScanSweeping:
        if (axisReached(driveNodeId(), m_scanState.scanTargetMm, false)) {
            if (m_scanState.currentStepMm >= m_scanState.maxStepMm - 0.2) {
                m_scanState = ScanState();
                updateProgramStatus(zh("e689abe68f8fe5ae8ce68890"));
                appendLog(zh("e887aae58aa8e689abe68f8fe5ae8ce68890"));
                return;
            }

            m_scanState.stepTargetMm = qMin(m_scanState.maxStepMm,
                                            m_scanState.currentStepMm + m_stepLengthSpin->value());

            if (qAbs(m_scanState.stepTargetMm - m_scanState.currentStepMm) < 0.001) {
                m_scanState = ScanState();
                updateProgramStatus(zh("e689abe68f8fe5ae8ce68890"));
                appendLog(zh("e887aae58aa8e689abe68f8fe5ae8ce68890"));
                return;
            }

            m_scanState.phase = ScanStepping;
            commandAxisPosition(turnNodeId(),
                                m_scanState.stepTargetMm,
                                qMax(0.1, m_jogSpeedSpin->value()),
                                true);
            updateProgramStatus(zh("e6ada5e8bf9be8bdb4e5898de8bf9be588b0202531206d6d")
                                .arg(QString::number(m_scanState.stepTargetMm, 'f', 2)));
        }
        break;
    case ScanStepping:
        if (axisReached(turnNodeId(), m_scanState.stepTargetMm, true)) {
            m_scanState.currentStepMm = m_scanState.stepTargetMm;

            const ScanModeOption mode =
                    static_cast<ScanModeOption>(m_scanModeCombo->currentData().toInt());

            if (mode == BowMode) {
                m_scanState.forward = !m_scanState.forward;
                m_scanState.phase = ScanSweeping;
                startScanSweep(m_scanState.forward ? m_scanAxisLengthSpin->value() : 0.0);
            } else {
                m_scanState.phase = ScanReturning;
                commandAxisPosition(driveNodeId(), 0.0, m_scanSpeedSpin->value(), false);
                updateProgramStatus(zh("e689abe68f8fe8bdb4e8bf94e59b9ee8b5b7e782b9"));
            }
        }
        break;
    case ScanReturning:
        if (axisReached(driveNodeId(), 0.0, false)) {
            m_scanState.phase = ScanSweeping;
            m_scanState.forward = true;
            startScanSweep(m_scanAxisLengthSpin->value());
        }
        break;
    case ScanIdle:
    default:
        break;
    }
}

void MainWindow::startScanSweep(double targetMm)
{
    m_scanState.scanTargetMm = targetMm;
    commandAxisPosition(driveNodeId(), targetMm, m_scanSpeedSpin->value(), false);
    updateProgramStatus(zh("e689abe68f8fe8bdb4e7a7bbe58aa8e588b0202531206d6d").arg(QString::number(targetMm, 'f', 2)));
}

bool MainWindow::ensureAxisClosedLoop(quint8 nodeId)
{
    return m_controller->requestClosedLoop(nodeId);
}

bool MainWindow::commandAxisVelocity(quint8 nodeId, double speedMmPerSecond, bool turnAxis)
{
    if (!m_controller->isConnected()) {
        return false;
    }

    ensureAxisClosedLoop(nodeId);
    const double velocityLimitMmPerSecond = qMax(qAbs(speedMmPerSecond), 0.1);
    m_controller->setLimits(nodeId,
                            static_cast<float>(mmPerSecondToTurnsPerSecond(velocityLimitMmPerSecond, turnAxis)),
                            static_cast<float>(currentLimitAmps()));
    m_controller->setControllerMode(nodeId,
                                    ODriveMotorController::VelocityControl,
                                    ODriveMotorController::Passthrough);
    return m_controller->setVelocity(nodeId,
                                     static_cast<float>(mmPerSecondToTurnsPerSecond(speedMmPerSecond, turnAxis)),
                                     0.0f);
}

bool MainWindow::commandAxisPosition(quint8 nodeId,
                                     double targetMm,
                                     double speedMmPerSecond,
                                     bool turnAxis)
{
    if (!m_controller->isConnected()) {
        return false;
    }

    ensureAxisClosedLoop(nodeId);
    m_controller->setLimits(nodeId,
                            static_cast<float>(mmPerSecondToTurnsPerSecond(qMax(0.1, speedMmPerSecond), turnAxis)),
                            static_cast<float>(currentLimitAmps()));
    m_controller->setControllerMode(nodeId,
                                    ODriveMotorController::PositionControl,
                                    ODriveMotorController::Passthrough);
    return m_controller->setPosition(nodeId,
                                     static_cast<float>(axisAbsoluteTurns(nodeId, targetMm, turnAxis)),
                                     0.0f,
                                     0.0f);
}

void MainWindow::stopAllMotion(bool requestIdleState)
{
    if (!m_controller->isConnected()) {
        return;
    }

    QList<quint8> nodeIds;
    nodeIds << turnNodeId();
    if (!nodeIds.contains(driveNodeId())) {
        nodeIds << driveNodeId();
    }

    for (const quint8 nodeId : nodeIds) {
        m_controller->setControllerMode(nodeId,
                                        ODriveMotorController::VelocityControl,
                                        ODriveMotorController::Passthrough);
        m_controller->setVelocity(nodeId, 0.0f, 0.0f);
        if (requestIdleState) {
            m_controller->requestIdle(nodeId);
        }
    }
}

bool MainWindow::axisReached(quint8 nodeId, double targetMm, bool turnAxis, double toleranceMm) const
{
    return qAbs(axisPositionMm(nodeId, turnAxis) - targetMm) <= toleranceMm;
}

double MainWindow::axisPositionMm(quint8 nodeId, bool turnAxis) const
{
    const double turns = m_controller->status(nodeId).positionTurns;
    const double zeroTurns = turnAxis ? m_turnZeroTurns : m_driveZeroTurns;
    return turnsToMm(turns - zeroTurns, turnAxis);
}

double MainWindow::axisAbsoluteTurns(quint8 nodeId, double targetMm, bool turnAxis) const
{
    Q_UNUSED(nodeId);
    const double zeroTurns = turnAxis ? m_turnZeroTurns : m_driveZeroTurns;
    return zeroTurns + mmToTurns(targetMm, turnAxis);
}

double MainWindow::turnsToMm(double turns, bool turnAxis) const
{
    return turns * mmPerTurn(turnAxis);
}

double MainWindow::mmToTurns(double mm, bool turnAxis) const
{
    return mm / mmPerTurn(turnAxis);
}

double MainWindow::mmPerTurn(bool turnAxis) const
{
    const QDoubleSpinBox *spin = turnAxis ? m_turnScaleSpin : m_driveScaleSpin;
    return spin ? qMax(0.0001, spin->value()) : 1.0;
}

double MainWindow::mmPerSecondToTurnsPerSecond(double mmPerSecond, bool turnAxis) const
{
    return mmToTurns(mmPerSecond, turnAxis);
}

quint8 MainWindow::turnNodeId() const
{
    return static_cast<quint8>(m_turnNodeIdSpin ? (m_turnNodeIdSpin->value() & 0x3F) : 0);
}

quint8 MainWindow::driveNodeId() const
{
    return static_cast<quint8>(m_driveNodeIdSpin ? (m_driveNodeIdSpin->value() & 0x3F) : 1);
}

double MainWindow::currentLimitAmps() const
{
    return m_currentLimitSpin ? m_currentLimitSpin->value() : 10.0;
}

QLabel *MainWindow::createValueLabel()
{
    QLabel *label = new QLabel(QStringLiteral("--"));
    label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    label->setMinimumWidth(220);
    label->setStyleSheet(QStringLiteral("QLabel { background: #f5f5f5; border: 1px solid #d8d8d8; padding: 4px 6px; }"));
    return label;
}

QLabel *MainWindow::createReadoutLabel()
{
    QLabel *label = new QLabel(QStringLiteral("-- mm"));
    label->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    label->setStyleSheet(readoutStyle());
    return label;
}
