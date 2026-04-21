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
#include <QFileInfo>
#include <QFrame>
#include <QGuiApplication>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLibrary>
#include <QLineEdit>
#include <QMap>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPointer>
#include <QProcess>
#include <QPushButton>
#include <QCheckBox>
#include <QRegularExpression>
#include <QScrollArea>
#include <QScreen>
#include <QSettings>
#include <QStringList>
#include <QSpinBox>
#include <QTextDocument>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>
#include <QtGlobal>
#include <algorithm>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

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

QString appSettingsPath()
{
    return QDir(QCoreApplication::applicationDirPath())
            .filePath(QStringLiteral("odriver.ini"));
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

QString decodeBitfield(quint32 value,
                       const std::initializer_list<QPair<quint32, const char *>> &entries,
                       const QString &noneText = QStringLiteral("None"))
{
    if (value == 0) {
        return noneText;
    }

    QStringList parts;
    quint32 remaining = value;
    for (const auto &entry : entries) {
        if ((value & entry.first) == entry.first) {
            parts << QString::fromLatin1(entry.second);
            remaining &= ~entry.first;
        }
    }
    if (remaining != 0) {
        parts << QStringLiteral("UNKNOWN(0x%1)")
                     .arg(remaining, 0, 16)
                     .toUpper();
    }
    return parts.join(QStringLiteral(" | "));
}

QString describeAxisError(quint32 value)
{
    return decodeBitfield(value, {
                              qMakePair<quint32, const char *>(0x00000001u, "INVALID_STATE"),
                              qMakePair<quint32, const char *>(0x00000040u, "MOTOR_FAILED"),
                              qMakePair<quint32, const char *>(0x00000080u, "SENSORLESS_ESTIMATOR_FAILED"),
                              qMakePair<quint32, const char *>(0x00000100u, "ENCODER_FAILED"),
                              qMakePair<quint32, const char *>(0x00000200u, "CONTROLLER_FAILED"),
                              qMakePair<quint32, const char *>(0x00000800u, "WATCHDOG_TIMER_EXPIRED"),
                              qMakePair<quint32, const char *>(0x00001000u, "MIN_ENDSTOP_PRESSED"),
                              qMakePair<quint32, const char *>(0x00002000u, "MAX_ENDSTOP_PRESSED"),
                              qMakePair<quint32, const char *>(0x00004000u, "ESTOP_REQUESTED"),
                              qMakePair<quint32, const char *>(0x00020000u, "HOMING_WITHOUT_ENDSTOP"),
                              qMakePair<quint32, const char *>(0x00040000u, "OVER_TEMP"),
                              qMakePair<quint32, const char *>(0x00080000u, "UNKNOWN_POSITION")
                          });
}

QString describeOdriveError(quint32 value)
{
    return decodeBitfield(value, {
                              qMakePair<quint32, const char *>(0x00000001u, "CONTROL_ITERATION_MISSED"),
                              qMakePair<quint32, const char *>(0x00000002u, "DC_BUS_UNDER_VOLTAGE"),
                              qMakePair<quint32, const char *>(0x00000004u, "DC_BUS_OVER_VOLTAGE"),
                              qMakePair<quint32, const char *>(0x00000008u, "DC_BUS_OVER_REGEN_CURRENT"),
                              qMakePair<quint32, const char *>(0x00000010u, "DC_BUS_OVER_CURRENT"),
                              qMakePair<quint32, const char *>(0x00000020u, "BRAKE_DEADTIME_VIOLATION"),
                              qMakePair<quint32, const char *>(0x00000040u, "BRAKE_DUTY_CYCLE_NAN"),
                              qMakePair<quint32, const char *>(0x00000080u, "INVALID_BRAKE_RESISTANCE")
                          });
}

QString describeDisarmReason(quint32 value)
{
    if (value == 0) {
        return QStringLiteral("None");
    }

    return QStringLiteral("0x%1 (see ODrive disarm reason)")
            .arg(value, 8, 16, QLatin1Char('0'))
            .toUpper();
}

QString describeMotorError(quint32 value)
{
    return decodeBitfield(value, {
                              qMakePair<quint32, const char *>(0x00000001u, "PHASE_RESISTANCE_OUT_OF_RANGE"),
                              qMakePair<quint32, const char *>(0x00000002u, "PHASE_INDUCTANCE_OUT_OF_RANGE"),
                              qMakePair<quint32, const char *>(0x00000008u, "DRV_FAULT"),
                              qMakePair<quint32, const char *>(0x00000010u, "CONTROL_DEADLINE_MISSED"),
                              qMakePair<quint32, const char *>(0x00000080u, "MODULATION_MAGNITUDE"),
                              qMakePair<quint32, const char *>(0x00000100u, "CURRENT_SENSE_SATURATION"),
                              qMakePair<quint32, const char *>(0x00000400u, "CURRENT_LIMIT_VIOLATION"),
                              qMakePair<quint32, const char *>(0x00001000u, "MODULATION_IS_NAN"),
                              qMakePair<quint32, const char *>(0x00004000u, "MOTOR_THERMISTOR_OVER_TEMP"),
                              qMakePair<quint32, const char *>(0x00010000u, "FET_THERMISTOR_OVER_TEMP"),
                              qMakePair<quint32, const char *>(0x00020000u, "TIMER_UPDATE_MISSED"),
                              qMakePair<quint32, const char *>(0x00040000u, "CURRENT_MEASUREMENT_UNAVAILABLE"),
                              qMakePair<quint32, const char *>(0x00080000u, "CONTROLLER_FAILED"),
                              qMakePair<quint32, const char *>(0x00100000u, "I_BUS_OUT_OF_RANGE"),
                              qMakePair<quint32, const char *>(0x00200000u, "BRAKE_RESISTOR_DISARMED"),
                              qMakePair<quint32, const char *>(0x00400000u, "SYSTEM_LEVEL"),
                              qMakePair<quint32, const char *>(0x00800000u, "BAD_TIMING"),
                              qMakePair<quint32, const char *>(0x01000000u, "UNKNOWN_PHASE_ESTIMATE"),
                              qMakePair<quint32, const char *>(0x02000000u, "UNKNOWN_PHASE_VEL")
                          });
}

QString describeEncoderError(quint32 value)
{
    return decodeBitfield(value, {
                              qMakePair<quint32, const char *>(0x00000001u, "UNSTABLE_GAIN"),
                              qMakePair<quint32, const char *>(0x00000002u, "CPR_POLEPAIRS_MISMATCH"),
                              qMakePair<quint32, const char *>(0x00000004u, "NO_RESPONSE"),
                              qMakePair<quint32, const char *>(0x00000008u, "UNSUPPORTED_ENCODER_MODE"),
                              qMakePair<quint32, const char *>(0x00000010u, "ILLEGAL_HALL_STATE"),
                              qMakePair<quint32, const char *>(0x00000020u, "INDEX_NOT_FOUND_YET"),
                              qMakePair<quint32, const char *>(0x00000040u, "ABS_SPI_TIMEOUT"),
                              qMakePair<quint32, const char *>(0x00000080u, "ABS_SPI_COM_FAIL"),
                              qMakePair<quint32, const char *>(0x00000100u, "ABS_SPI_NOT_READY"),
                              qMakePair<quint32, const char *>(0x00000200u, "HALL_NOT_CALIBRATED_YET")
                          });
}

QString describeControllerError(quint32 value)
{
    return decodeBitfield(value, {
                              qMakePair<quint32, const char *>(0x00000001u, "OVERSPEED"),
                              qMakePair<quint32, const char *>(0x00000002u, "INVALID_INPUT_MODE"),
                              qMakePair<quint32, const char *>(0x00000004u, "UNSTABLE_GAIN"),
                              qMakePair<quint32, const char *>(0x00000008u, "INVALID_MIRROR_AXIS"),
                              qMakePair<quint32, const char *>(0x00000010u, "INVALID_LOAD_ENCODER"),
                              qMakePair<quint32, const char *>(0x00000020u, "INVALID_ESTIMATE"),
                              qMakePair<quint32, const char *>(0x00000040u, "INVALID_CIRCULAR_RANGE"),
                              qMakePair<quint32, const char *>(0x00000080u, "SPINOUT_DETECTED")
                          });
}

QString describeSensorlessError(quint32 value)
{
    return decodeBitfield(value, {
                              qMakePair<quint32, const char *>(0x00000001u, "UNSTABLE_GAIN"),
                              qMakePair<quint32, const char *>(0x00000002u, "UNKNOWN_CURRENT_MEASUREMENT"),
                              qMakePair<quint32, const char *>(0x00000004u, "UNKNOWN_CURRENT_COMMAND")
                          });
}

QString describeControlModeName(quint8 mode)
{
    switch (mode) {
    case ODriveMotorController::VoltageControl:
        return QStringLiteral("VoltageControl");
    case ODriveMotorController::TorqueControl:
        return QStringLiteral("TorqueControl");
    case ODriveMotorController::VelocityControl:
        return QStringLiteral("VelocityControl");
    case ODriveMotorController::PositionControl:
        return QStringLiteral("PositionControl");
    default:
        return QStringLiteral("Unknown(%1)").arg(mode);
    }
}

QString describeInputModeName(quint8 mode)
{
    switch (mode) {
    case ODriveMotorController::Inactive:
        return QStringLiteral("Inactive");
    case ODriveMotorController::Passthrough:
        return QStringLiteral("Passthrough");
    case ODriveMotorController::VelRamp:
        return QStringLiteral("VelRamp");
    case ODriveMotorController::PosFilter:
        return QStringLiteral("PosFilter");
    case ODriveMotorController::MixChannels:
        return QStringLiteral("MixChannels");
    case ODriveMotorController::TrapTraj:
        return QStringLiteral("TrapTraj");
    case ODriveMotorController::TorqueRamp:
        return QStringLiteral("TorqueRamp");
    case ODriveMotorController::Mirror:
        return QStringLiteral("Mirror");
    case ODriveMotorController::Tuning:
        return QStringLiteral("Tuning");
    default:
        return QStringLiteral("Unknown(%1)").arg(mode);
    }
}

void writeConsoleLine(const QString &line)
{
#ifdef Q_OS_WIN
    HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
    if (handle != INVALID_HANDLE_VALUE && handle != nullptr) {
        DWORD mode = 0;
        if (GetConsoleMode(handle, &mode)) {
            const std::wstring wide = line.toStdWString();
            DWORD written = 0;
            WriteConsoleW(handle, wide.c_str(), static_cast<DWORD>(wide.size()), &written, nullptr);
            WriteConsoleW(handle, L"\r\n", 2, &written, nullptr);
            return;
        }
    }
#endif
    qInfo().noquote() << line;
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
      m_txExtendedCheck(nullptr),
      m_txRemoteCheck(nullptr),
      m_txPeriodSpin(nullptr),
      m_txSendButton(nullptr),
      m_txToggleButton(nullptr),
      m_txStatusLabel(nullptr),
      m_txErrorCheck(nullptr),
      m_txTimer(nullptr),
      m_txResponseTimer(nullptr),
      m_logFlushTimer(nullptr),
      m_rxFlushTimer(nullptr),
      m_statusUpdateTimer(nullptr),
      m_settingsSaveTimer(nullptr),
      m_lastTxFrameId(0),
      m_lastTxNodeId(0),
      m_lastTxBoardIndex(-1),
      m_waitingForTxResponse(false)
{
    ui->setupUi(this);
    buildUi();
    populatePlugins();
    populateControlModes();
    populateInputModes();
    loadSettings();
    refreshCanInterfaces();
    updateConnectionEditorHints();
    syncConnectionEditorsFromAdvanced();
    updateStatusPanel();
    syncAxisReadouts();
    connectSettingsPersistence();

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
    saveSettings();

    m_waitingForTxResponse = false;
    m_scanState = ScanState();

    const QList<QTimer *> timers = {
        m_txTimer,
        m_txResponseTimer,
        m_logFlushTimer,
        m_rxFlushTimer,
        m_statusUpdateTimer,
        m_settingsSaveTimer
    };
    for (QTimer *timer : timers) {
        if (timer) {
            timer->stop();
        }
    }

    disconnectAllBoards();

    if (m_controller) {
        disconnect(m_controller, nullptr, this, nullptr);
        m_controller->disconnectDevice();
    }

    if (m_advancedDialog) {
        m_advancedDialog->close();
        delete m_advancedDialog;
        m_advancedDialog = nullptr;
    }

    m_logEdit = nullptr;
    m_rxEdit = nullptr;
    m_txStatusLabel = nullptr;
    m_programStatusLabel = nullptr;
    m_linkStatusLabel = nullptr;

    delete ui;
    ui = nullptr;
}

void MainWindow::refreshCanInterfaces()
{
    const QString plugin = currentPluginId(m_pluginCombo);
    QString errorMessage;
    QStringList interfaces;

    qInfo().noquote() << "Refreshing CAN interfaces. plugin=" << plugin
                      << " configuredBoardRows=" << connectedInterfaceCount();

    if (plugin == QStringLiteral("systeccan") && !checkSystemCanDriver(&errorMessage)) {
        interfaces.clear();
    } else {
        interfaces = m_controller->availableInterfaces(plugin, &errorMessage);
    }

    qInfo().noquote() << "Refresh result. plugin=" << plugin
                      << " interfaces=" << interfaces.join(QStringLiteral(", "))
                      << " error=" << errorMessage;
    m_availableInterfaces = interfaces;
    refreshBoardInterfaceOptions();

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
    if (hasAnyConnectedBoard()) {
        qInfo() << "Disconnect requested from UI";
        stopProgram();
        disconnectAllBoards();
        return;
    }

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

    const QList<int> boardIndices = configuredBoardIndices();
    if (boardIndices.isEmpty()) {
        appendLog(QStringLiteral("No board rows are configured in Advanced. Multiple boards may share the same CAN interface and are distinguished by Node ID."));
        updateProgramStatus(QStringLiteral("No configured board rows"));
        return;
    }

    bool anyConnected = false;
    const QString plugin = currentPluginId(m_pluginCombo);
    QMap<QString, ODriveMotorController *> controllersByInterface;

    for (const int boardIndex : boardIndices) {
        const QString interfaceName = boardInterfaceName(boardIndex);
        if (interfaceName.isEmpty()) {
            continue;
        }

        BoardRuntime &runtime = m_boardRuntimes[boardIndex];
        runtime.controller = nullptr;
        runtime.connected = false;
        runtime.ownsController = false;

        ODriveMotorController *controller = controllersByInterface.value(interfaceName, nullptr);
        const quint8 defaultNode = boardTurnNodeId(boardIndex);

        if (!controller) {
            controller = new ODriveMotorController(this);
            qInfo().noquote() << "Connect requested. board=" << boardIndex
                              << " plugin=" << plugin
                              << " interface=" << interfaceName
                              << " bitrate=" << m_bitrateSpin->value()
                              << " defaultNode=" << int(defaultNode);

            if (!controller->connectDevice(plugin,
                                           interfaceName,
                                           m_bitrateSpin->value(),
                                           m_serialBaudSpin->value(),
                                           defaultNode)) {
                appendLog(QStringLiteral("%1 connect failed").arg(boardLogPrefix(boardIndex)));
                controller->deleteLater();
                continue;
            }

            controllersByInterface.insert(interfaceName, controller);
            runtime.ownsController = true;
        } else {
            qInfo().noquote() << "Reusing connected interface for board=" << boardIndex
                              << " plugin=" << plugin
                              << " interface=" << interfaceName
                              << " defaultNode=" << int(defaultNode);
            appendLog(QStringLiteral("%1 using shared interface %2 (distinguished by Node ID)")
                      .arg(boardLogPrefix(boardIndex), interfaceName));
        }

        runtime.controller = controller;
        runtime.connected = true;
        connectBoardControllerSignals(boardIndex, controller);
        runtime.detectedNodes = m_boardConfigRows[boardIndex].detectedNodes;

        QList<quint8> trackedNodeIds = controller->trackedNodeIds();
        trackedNodeIds << boardTurnNodeId(boardIndex) << boardDriveNodeId(boardIndex);
        controller->setDefaultNodeId(defaultNode);
        controller->setTrackedNodeIds(trackedNodeIds);
        controller->requestTrackedTelemetry();

        anyConnected = true;
        updateBoardStatusLabel(boardIndex, zh("e5b7b2e8bf9ee68ea5"));
        QTimer::singleShot(300, this, [this, boardIndex]() { refreshBoardNodes(boardIndex, false); });
    }

    if (!anyConnected) {
        updateProgramStatus(QStringLiteral("No boards connected"));
        updateConnectionState(false);
        return;
    }

    updateProgramStatus(zh("e8bf9ee68ea5e68890e58a9fefbc8ce58786e5a487e68ea7e588b6e5a49ae59d97e69dbfe58da1"));
    updateConnectionStateFromBoards();
    updateDebugBoardCombo();
    updateDebugNodeCombo();
    updateLinkStatusDisplay();
}

void MainWindow::updateConnectionState(bool connected)
{
    m_connectButton->setText(connected ? QStringLiteral("disconnect") : QStringLiteral("connect"));
    if (m_advancedConnectButton) {
        m_advancedConnectButton->setText(connected ? QStringLiteral("disconnect") : QStringLiteral("connect"));
    }

    m_pluginCombo->setEnabled(!connected);
    m_bitrateSpin->setEnabled(!connected);
    if (m_interfaceCountCombo) {
        m_interfaceCountCombo->setEnabled(!connected);
    }
    m_refreshInterfacesButton->setEnabled(!connected);
    m_ipEdit->setEnabled(!connected);
    m_openAdvancedButton->setEnabled(true);
    if (m_rescanNodesButton) {
        m_rescanNodesButton->setEnabled(connected);
    }
    for (int i = 0; i < m_boardConfigRows.size(); ++i) {
        BoardConfigRow &row = m_boardConfigRows[i];
        const bool visible = i < connectedInterfaceCount();
        if (row.interfaceCombo) row.interfaceCombo->setEnabled(!connected && visible);
        if (row.scanNodesButton) row.scanNodesButton->setEnabled(visible);
        if (row.turnNodeCombo) row.turnNodeCombo->setEnabled(!connected && visible);
        if (row.driveNodeCombo) row.driveNodeCombo->setEnabled(!connected && visible);
        if (row.turnScaleSpin) row.turnScaleSpin->setEnabled(!connected && visible);
        if (row.driveScaleSpin) row.driveScaleSpin->setEnabled(!connected && visible);
    }
    setMotionControlsEnabled(connected);

    if (!connected) {
        m_scanState = ScanState();
        m_waitingForTxResponse = false;
        if (m_txTimer) {
            m_txTimer->stop();
        }
        if (m_txResponseTimer) {
            m_txResponseTimer->stop();
        }
        if (m_txToggleButton) {
            m_txToggleButton->setText(QStringLiteral("Send Repeat"));
        }
        if (m_txStatusLabel) {
            m_txStatusLabel->setText(QStringLiteral("Status: idle"));
            m_txStatusLabel->setStyleSheet(QStringLiteral("QLabel { color: #666666; font-weight: 600; }"));
        }
        updateProgramStatus(zh("e8afb7e58588e8bf9ee68ea5204f4472697665"));
        if (m_debugNodeCombo) {
            m_debugNodeCombo->clear();
            m_debugNodeCombo->setEnabled(false);
        }
        if (m_activeNodesLabel) {
            m_activeNodesLabel->setText(zh("e6a380e6b58be588b0e79a84e88a82e782b93a202d2d"));
        }
        syncAxisReadouts();
        updateStatusPanel();
    }

    updateLinkStatusDisplay();
    updateConnectionEditorHints();
}

void MainWindow::updateStatusPanel()
{
    ODriveMotorController *controller = currentDebugController();
    const quint8 nodeId = currentDebugNodeId();

    auto formatHex = [](quint32 value) {
        return QStringLiteral("0x%1").arg(value, 8, 16, QLatin1Char('0')).toUpper();
    };

    if (!controller || !controller->isConnected()) {
        const QString placeholder = QStringLiteral("--");
        m_axisErrorValue->setText(placeholder);
        m_axisErrorDetailValue->setText(placeholder);
        m_activeErrorsValue->setText(placeholder);
        m_activeErrorsDetailValue->setText(placeholder);
        m_disarmReasonValue->setText(placeholder);
        m_disarmReasonDetailValue->setText(placeholder);
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

    const ODriveMotorController::AxisStatus &status = controller->status(nodeId);
    if (!status.lastMessage.isValid()) {
        const QString placeholder = QStringLiteral("--");
        m_axisErrorValue->setText(placeholder);
        m_axisErrorDetailValue->setText(placeholder);
        m_activeErrorsValue->setText(placeholder);
        m_activeErrorsDetailValue->setText(placeholder);
        m_disarmReasonValue->setText(placeholder);
        m_disarmReasonDetailValue->setText(placeholder);
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
    m_axisErrorDetailValue->setText(describeAxisError(status.axisError));
    m_activeErrorsValue->setText(formatHex(status.activeErrors));
    m_activeErrorsDetailValue->setText(describeOdriveError(status.activeErrors));
    m_disarmReasonValue->setText(formatHex(status.disarmReason));
    m_disarmReasonDetailValue->setText(describeDisarmReason(status.disarmReason));
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

    if (message == m_lastLogMessage) {
        return;
    }
    m_lastLogMessage = message;

    const QString timestamp = QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss"));
    const QString line = QStringLiteral("[%1] %2").arg(timestamp, message);

    if (m_logFlushTimer) {
        m_pendingLogLines.append(line);
        if (!m_logFlushTimer->isActive()) {
            m_logFlushTimer->start();
        }
    } else {
        m_logEdit->appendPlainText(line);
        writeConsoleLine(line);
    }
}

void MainWindow::appendRxMessage(const QString &message)
{
    if (!m_rxEdit) {
        return;
    }

    if (message == m_lastRxMessage) {
        return;
    }
    m_lastRxMessage = message;

    const QString timestamp = QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss"));
    m_pendingRxLines.append(QStringLiteral("[%1] %2").arg(timestamp, message));
    if (m_rxFlushTimer && !m_rxFlushTimer->isActive()) {
        m_rxFlushTimer->start();
    }
}

void MainWindow::flushQueuedMessages()
{
    if (!m_logEdit || m_pendingLogLines.isEmpty()) {
        return;
    }

    const QStringList lines = m_pendingLogLines;
    m_pendingLogLines.clear();
    m_logEdit->appendPlainText(lines.join(QStringLiteral("\n")));
    for (const QString &line : lines) {
        writeConsoleLine(line);
    }
}

void MainWindow::flushQueuedRxMessages()
{
    if (!m_rxEdit || m_pendingRxLines.isEmpty()) {
        return;
    }

    m_rxEdit->appendPlainText(m_pendingRxLines.join(QStringLiteral("\n")));
    m_pendingRxLines.clear();
}

void MainWindow::scheduleStatusPanelUpdate()
{
    if (m_statusUpdateTimer && !m_statusUpdateTimer->isActive()) {
        m_statusUpdateTimer->start();
    }
}

void MainWindow::scheduleSaveSettings()
{
    if (m_settingsSaveTimer && !m_settingsSaveTimer->isActive()) {
        m_settingsSaveTimer->start();
    }
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

static bool parseTxByteText(const QString &text, char *outByte)
{
    if (!outByte) {
        return false;
    }

    const QString trimmed = text.trimmed();
    if (trimmed.isEmpty()) {
        *outByte = 0;
        return true;
    }

    bool ok = false;
    const int value = trimmed.toInt(&ok, 16);
    if (!ok || value < 0x00 || value > 0xFF) {
        return false;
    }

    *outByte = static_cast<char>(value);
    return true;
}

void MainWindow::sendCustomTxOnce()
{
    ODriveMotorController *controller = currentDebugController();
    const int boardIndex = currentDebugBoardIndex();
    if (!controller || !controller->isConnected()) {
        appendLog(QStringLiteral("Transmit: not connected"));
        if (m_txStatusLabel) {
            m_txStatusLabel->setText(QStringLiteral("Status: not connected"));
            m_txStatusLabel->setStyleSheet(QStringLiteral("QLabel { color: #9c2f2f; font-weight: 600; }"));
        }
        return;
    }
    if (!m_txIdEdit || !m_txDlcSpin || !m_txExtendedCheck || !m_txRemoteCheck) {
        return;
    }

    quint32 frameId = 0;
    if (!parseTxId(m_txIdEdit->text(), &frameId)) {
        appendLog(QStringLiteral("Transmit: invalid ID"));
        if (m_txStatusLabel) {
            m_txStatusLabel->setText(QStringLiteral("Status: invalid address"));
            m_txStatusLabel->setStyleSheet(QStringLiteral("QLabel { color: #9c2f2f; font-weight: 600; }"));
        }
        return;
    }

    const bool extended = m_txExtendedCheck->isChecked();
    const bool remote = m_txRemoteCheck->isChecked();
    const int dlc = qBound(0, m_txDlcSpin->value(), 8);

    if (!extended && frameId > 0x7FF) {
        appendLog(QStringLiteral("Transmit: ID > 0x7FF requires Extended mode"));
        if (m_txStatusLabel) {
            m_txStatusLabel->setText(QStringLiteral("Status: standard ID must be <= 0x7FF"));
            m_txStatusLabel->setStyleSheet(QStringLiteral("QLabel { color: #9c2f2f; font-weight: 600; }"));
        }
        return;
    }
    if (frameId > 0x1FFFFFFF) {
        appendLog(QStringLiteral("Transmit: ID must be <= 0x1FFFFFFF"));
        if (m_txStatusLabel) {
            m_txStatusLabel->setText(QStringLiteral("Status: ID must be <= 0x1FFFFFFF"));
            m_txStatusLabel->setStyleSheet(QStringLiteral("QLabel { color: #9c2f2f; font-weight: 600; }"));
        }
        return;
    }

    QByteArray payload;
    if (remote) {
        payload = QByteArray(dlc, '\0');
    } else {
        payload.resize(dlc);
        for (int i = 0; i < dlc; ++i) {
            char value = 0;
            const QLineEdit *edit = (i < m_txByteEdits.size()) ? m_txByteEdits.at(i) : nullptr;
            if (!parseTxByteText(edit ? edit->text() : QString(), &value)) {
                appendLog(QStringLiteral("Transmit: invalid payload byte at index %1").arg(i + 1));
                if (m_txStatusLabel) {
                    m_txStatusLabel->setText(QStringLiteral("Status: payload byte %1 is invalid").arg(i + 1));
                    m_txStatusLabel->setStyleSheet(QStringLiteral("QLabel { color: #9c2f2f; font-weight: 600; }"));
                }
                return;
            }
            payload[i] = value;
        }
    }

    if (!controller->sendRawCanFrame(frameId, payload, extended, remote, false)) {
        appendLog(QStringLiteral("Transmit: send failed"));
        if (m_txStatusLabel) {
            m_txStatusLabel->setText(QStringLiteral("Status: send failed"));
            m_txStatusLabel->setStyleSheet(QStringLiteral("QLabel { color: #9c2f2f; font-weight: 600; }"));
        }
        m_waitingForTxResponse = false;
        if (m_txResponseTimer) {
            m_txResponseTimer->stop();
        }
        return;
    }

    m_lastTxFrameId = frameId;
    m_lastTxNodeId = static_cast<quint8>((frameId >> 5) & 0x3F);
    m_lastTxBoardIndex = boardIndex;
    m_waitingForTxResponse = true;
    if (m_txStatusLabel) {
        m_txStatusLabel->setText(QStringLiteral("Status: sent, waiting for reply"));
        m_txStatusLabel->setStyleSheet(QStringLiteral("QLabel { color: #ad6f00; font-weight: 600; }"));
    }
    if (m_txResponseTimer) {
        m_txResponseTimer->start();
    }
}

void MainWindow::toggleCustomTx()
{
    if (!m_txTimer || !m_txToggleButton || !m_txPeriodSpin) {
        return;
    }

    if (m_txTimer->isActive()) {
        m_txTimer->stop();
        m_txToggleButton->setText(QStringLiteral("Send Repeat"));
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

void MainWindow::handleCustomTxRxMessage(const QString &message)
{
    if (!m_waitingForTxResponse || !m_txStatusLabel) {
        return;
    }

    const QString upperMessage = message.toUpper();
    if (m_lastTxBoardIndex >= 0) {
        const QString boardToken = boardLogPrefix(m_lastTxBoardIndex).toUpper();
        if (!upperMessage.contains(boardToken.toUpper())) {
            return;
        }
    }
    const QString frameIdToken = QStringLiteral("ID=%1")
            .arg(QStringLiteral("0X%1").arg(m_lastTxFrameId, 3, 16, QLatin1Char('0')).toUpper());
    const QString nodeToken = QStringLiteral("NODE=%1").arg(m_lastTxNodeId);

    if (!upperMessage.contains(frameIdToken) && !upperMessage.contains(nodeToken)) {
        return;
    }

    m_waitingForTxResponse = false;
    if (m_txResponseTimer) {
        m_txResponseTimer->stop();
    }
    m_txStatusLabel->setText(QStringLiteral("Status: reply received"));
    m_txStatusLabel->setStyleSheet(QStringLiteral("QLabel { color: #0b7a37; font-weight: 600; }"));
}

void MainWindow::handleCustomTxResponseTimeout()
{
    if (!m_waitingForTxResponse || !m_txStatusLabel) {
        return;
    }

    m_waitingForTxResponse = false;
    m_txStatusLabel->setText(QStringLiteral("Status: sent, no reply seen"));
    m_txStatusLabel->setStyleSheet(QStringLiteral("QLabel { color: #9c2f2f; font-weight: 600; }"));
}

void MainWindow::scanForActiveNodes()
{
    if (!m_debugNodeCombo || !m_activeNodesLabel) {
        return;
    }
    refreshCurrentBoardNodes();
}

void MainWindow::updateDetectedNodesList()
{
    const int boardIndex = currentDebugBoardIndex();
    if (boardIndex < 0 || boardIndex >= m_boardConfigRows.size() || !m_debugNodeCombo || !m_activeNodesLabel) {
        return;
    }

    const QList<quint8> activeNodes = m_boardConfigRows[boardIndex].detectedNodes;
    const quint8 previousNodeId = comboNodeValue(m_debugNodeCombo, 0);

    m_updatingDebugNodeCombo = true;
    m_debugNodeCombo->blockSignals(true);
    m_debugNodeCombo->clear();

    if (activeNodes.isEmpty()) {
        m_debugNodeCombo->setEnabled(false);
        m_debugNodeCombo->blockSignals(false);
        m_updatingDebugNodeCombo = false;
        m_activeNodesLabel->setText(zh("e6a380e6b58be588b0e79a84e88a82e782b9320e697a0"));
        return;
    }

    for (quint8 nodeId : activeNodes) {
        const QString nodeText = QStringLiteral("Node %1").arg(nodeId);
        m_debugNodeCombo->addItem(nodeText, nodeId);
    }

    m_debugNodeCombo->setEnabled(true);
    QStringList nodeStrings;
    for (quint8 nodeId : activeNodes) {
        nodeStrings << QString::number(nodeId);
    }
    m_activeNodesLabel->setText(QStringLiteral("%1: %2").arg(zh("e6a380e6b58be588b0e79a84e88a82e782b9"), nodeStrings.join(", ")));

    int currentNodeIndex = m_debugNodeCombo->findData(previousNodeId);
    if (currentNodeIndex < 0) {
        currentNodeIndex = m_debugNodeCombo->findData(boardTurnNodeId(boardIndex));
    }
    if (currentNodeIndex < 0) {
        currentNodeIndex = 0;
    }
    m_debugNodeCombo->setCurrentIndex(currentNodeIndex);
    m_debugNodeCombo->blockSignals(false);
    m_updatingDebugNodeCombo = false;
}

void MainWindow::switchToDetectedNode(quint8 nodeId)
{
    if (m_switchingDebugNode || m_updatingDebugNodeCombo || !m_debugNodeCombo) {
        return;
    }

    const int index = m_debugNodeCombo->findData(nodeId);
    if (index < 0) {
        return;
    }

    const quint8 previousNodeId = comboNodeValue(m_debugNodeCombo, 0);
    m_switchingDebugNode = true;

    if (m_debugNodeCombo->currentIndex() != index) {
        m_debugNodeCombo->blockSignals(true);
        m_debugNodeCombo->setCurrentIndex(index);
        m_debugNodeCombo->blockSignals(false);
    }

    updateDebugScaleSpin();

    if (previousNodeId != nodeId) {
        refreshTelemetry();
        appendLog(QStringLiteral("%1 Node %2").arg(zh("e5b7b2e5887e68da2e588b0"), QString::number(nodeId)));
    }
    updateProgramStatus(QStringLiteral("%1: %2").arg(zh("e5bd93e5898de68ea7e588b688a82e782b9"), QString::number(nodeId)));

    m_switchingDebugNode = false;
}

void MainWindow::requestIdle()
{
    if (ODriveMotorController *controller = currentDebugController()) {
        controller->requestIdle(currentDebugNodeId());
    }
}

void MainWindow::requestMotorCalibration()
{
    if (ODriveMotorController *controller = currentDebugController()) {
        controller->requestMotorCalibration(currentDebugNodeId());
    }
}

void MainWindow::requestFullCalibration()
{
    if (ODriveMotorController *controller = currentDebugController()) {
        controller->requestFullCalibration(currentDebugNodeId());
    }
}

void MainWindow::requestClosedLoop()
{
    if (ODriveMotorController *controller = currentDebugController()) {
        controller->requestClosedLoop(currentDebugNodeId());
    }
}

void MainWindow::applyControllerMode()
{
    const ODriveMotorController::ControlMode controlMode =
            static_cast<ODriveMotorController::ControlMode>(m_controlModeCombo->currentData().toInt());
    const ODriveMotorController::InputMode inputMode =
            static_cast<ODriveMotorController::InputMode>(m_inputModeCombo->currentData().toInt());
    if (ODriveMotorController *controller = currentDebugController()) {
        controller->setControllerMode(currentDebugNodeId(), controlMode, inputMode);
    }
}

void MainWindow::sendPositionCommand()
{
    ODriveMotorController *controller = currentDebugController();
    if (!controller) {
        return;
    }
    const ODriveMotorController::InputMode inputMode =
            static_cast<ODriveMotorController::InputMode>(m_inputModeCombo->currentData().toInt());
    controller->setControllerMode(currentDebugNodeId(), ODriveMotorController::PositionControl, inputMode);
    controller->setPosition(currentDebugNodeId(),
                            static_cast<float>(m_positionSpin->value()),
                            static_cast<float>(m_positionVelFfSpin->value()),
                            static_cast<float>(m_positionTorqueFfSpin->value()));
}

void MainWindow::sendVelocityCommand()
{
    ODriveMotorController *controller = currentDebugController();
    if (!controller) {
        return;
    }
    const ODriveMotorController::InputMode inputMode =
            static_cast<ODriveMotorController::InputMode>(m_inputModeCombo->currentData().toInt());
    controller->setControllerMode(currentDebugNodeId(), ODriveMotorController::VelocityControl, inputMode);
    controller->setVelocity(currentDebugNodeId(),
                            static_cast<float>(m_velocitySpin->value()),
                            static_cast<float>(m_velocityTorqueFfSpin->value()));
}

void MainWindow::sendTorqueCommand()
{
    ODriveMotorController *controller = currentDebugController();
    if (!controller) {
        return;
    }
    const ODriveMotorController::InputMode inputMode =
            static_cast<ODriveMotorController::InputMode>(m_inputModeCombo->currentData().toInt());
    controller->setControllerMode(currentDebugNodeId(), ODriveMotorController::TorqueControl, inputMode);
    controller->setTorque(currentDebugNodeId(), static_cast<float>(m_torqueSpin->value()));
}

void MainWindow::applyLimits()
{
    if (ODriveMotorController *controller = currentDebugController()) {
        controller->setLimits(currentDebugNodeId(),
                              static_cast<float>(m_velocityLimitSpin->value()),
                              static_cast<float>(m_currentLimitSpin->value()));
    }
}

void MainWindow::readPidParams()
{
    ODriveMotorController *controller = currentDebugController();
    if (!controller || !controller->isConnected()) {
        appendLog(zh("e8afbbe58f96504944e58f82e695b0e5a4b1e8b4a5efbc9ae69caae8bf9ee68ea5"));
        return;
    }

    const quint8 nodeId = currentDebugNodeId();
    
    // 使用Python辅助工具读取PID参数
    appendLog(zh("e6ada3e59ca8e8afbbe58f96504944e58f82e695b02e2e2e"));
    
    QProcess *process = new QProcess(this);
    process->setProgram(QStringLiteral("python"));
    process->setArguments({QStringLiteral("qt_pid_helper.py"), QStringLiteral("read"), QString::number(nodeId)});
    
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, process](int exitCode, QProcess::ExitStatus exitStatus) {
        process->deleteLater();
        
        if (exitCode != 0 || exitStatus != QProcess::NormalExit) {
            appendLog(zh("e8afbbe58f96504944e5a4b1e8b4a5efbc9ae8bf9be7a88be5bc82e5b8b8"));
            return;
        }
        
        const QByteArray output = process->readAllStandardOutput();
        QJsonDocument doc = QJsonDocument::fromJson(output);
        if (!doc.isObject()) {
            appendLog(zh("e8afbbe58f96504944e5a4b1e8b4a5efbc9ae8be93e587bae6a0bce5bc8fe99499e8afaf"));
            return;
        }
        
        QJsonObject obj = doc.object();
        if (!obj.value(QStringLiteral("success")).toBool()) {
            const QString error = obj.value(QStringLiteral("error")).toString();
            appendLog(QStringLiteral("PID %1: %2").arg(zh("e8afbbe58f96e5a4b1e8b4a5"), error));
            return;
        }
        
        const double posGain = obj.value(QStringLiteral("pos_gain")).toDouble();
        const double velGain = obj.value(QStringLiteral("vel_gain")).toDouble();
        const double velIntGain = obj.value(QStringLiteral("vel_integrator_gain")).toDouble();
        
        m_posGainSpin->setValue(posGain);
        m_velGainSpin->setValue(velGain);
        m_velIntegratorGainSpin->setValue(velIntGain);
        
        appendLog(QStringLiteral("PID %1: pos_gain=%2, vel_gain=%3, vel_integrator_gain=%4")
                  .arg(zh("e8afbbe58f96e68890e58a9f"))
                  .arg(QString::number(posGain, 'f', 3))
                  .arg(QString::number(velGain, 'f', 6))
                  .arg(QString::number(velIntGain, 'f', 6)));
    });
    
    process->start();
}

void MainWindow::applyPidParams()
{
    ODriveMotorController *controller = currentDebugController();
    if (!controller || !controller->isConnected()) {
        appendLog(zh("e5ba94e794a8504944e58f82e695b0e5a4b1e8b4a5efbc9ae69caae8bf9ee68ea5"));
        return;
    }

    const quint8 nodeId = currentDebugNodeId();
    const double posGain = m_posGainSpin->value();
    const double velGain = m_velGainSpin->value();
    const double velIntegratorGain = m_velIntegratorGainSpin->value();
    
    // 使用Python辅助工具设置PID参数
    appendLog(QStringLiteral("PID %1: pos_gain=%2, vel_gain=%3, vel_integrator_gain=%4")
              .arg(zh("e6ada3e59ca8e5ba94e794a8"))
              .arg(QString::number(posGain, 'f', 3))
              .arg(QString::number(velGain, 'f', 6))
              .arg(QString::number(velIntegratorGain, 'f', 6)));
    
    QProcess *process = new QProcess(this);
    process->setProgram(QStringLiteral("python"));
    // 使用固定格式，避免科学计数法
    QString posGainStr = QString::number(posGain, 'f', 6).remove(QRegularExpression("0+$")).remove(QRegularExpression("\\.$"));
    QString velGainStr = QString::number(velGain, 'f', 6).remove(QRegularExpression("0+$")).remove(QRegularExpression("\\.$"));
    QString velIntGainStr = QString::number(velIntegratorGain, 'f', 6).remove(QRegularExpression("0+$")).remove(QRegularExpression("\\.$"));
    
    process->setArguments({
        QStringLiteral("qt_pid_helper.py"),
        QStringLiteral("set"),
        QString::number(nodeId),
        posGainStr,
        velGainStr,
        velIntGainStr
    });
    
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, process](int exitCode, QProcess::ExitStatus exitStatus) {
        process->deleteLater();
        
        if (exitCode != 0 || exitStatus != QProcess::NormalExit) {
            appendLog(zh("e5ba94e794a8504944e5a4b1e8b4a5efbc9ae8bf9be7a88be5bc82e5b8b8"));
            return;
        }
        
        const QByteArray output = process->readAllStandardOutput();
        QJsonDocument doc = QJsonDocument::fromJson(output);
        if (!doc.isObject()) {
            appendLog(zh("e5ba94e794a8504944e5a4b1e8b4a5efbc9ae8be93e587bae6a0bce5bc8fe99499e8afaf"));
            return;
        }
        
        QJsonObject obj = doc.object();
        if (!obj.value(QStringLiteral("success")).toBool()) {
            const QString error = obj.value(QStringLiteral("error")).toString();
            appendLog(QStringLiteral("PID %1: %2").arg(zh("e5ba94e794a8e5a4b1e8b4a5"), error));
            return;
        }
        
        const double actualPos = obj.value(QStringLiteral("pos_gain")).toDouble();
        const double actualVel = obj.value(QStringLiteral("vel_gain")).toDouble();
        const double actualInt = obj.value(QStringLiteral("vel_integrator_gain")).toDouble();
        
        appendLog(QStringLiteral("PID %1: pos_gain=%2, vel_gain=%3, vel_integrator_gain=%4")
                  .arg(zh("e5ba94e794a8e68890e58a9f"))
                  .arg(QString::number(actualPos, 'f', 3))
                  .arg(QString::number(actualVel, 'f', 6))
                  .arg(QString::number(actualInt, 'f', 6)));
    });
    
    process->start();
}

void MainWindow::refreshTelemetry()
{
    appendLog(zh("e5b7b2e4b8bbe58aa8e588b7e696b0e78ab6e68081"));
    if (ODriveMotorController *controller = currentDebugController()) {
        controller->requestAllTelemetry(currentDebugNodeId(), true);
    }
}

void MainWindow::openAdvancedPanel()
{
    if (!m_advancedDialog) {
        appendLog(QStringLiteral("Advanced dialog is not initialized"));
        return;
    }

    if (!m_advancedDialog->isVisible()) {
        QRect targetGeometry = m_advancedDialog->geometry();
        if (!targetGeometry.isValid() || targetGeometry.isEmpty()) {
            targetGeometry = QRect(pos() + QPoint(40, 40), m_advancedDialog->size());
        }

        if (const QScreen *screen = QGuiApplication::screenAt(frameGeometry().center())) {
            const QRect available = screen->availableGeometry();
            const QSize dialogSize = m_advancedDialog->size();
            QPoint topLeft = frameGeometry().topRight() + QPoint(16, 0);

            if (topLeft.x() + dialogSize.width() > available.right()) {
                topLeft.setX(qMax(available.left(), frameGeometry().left() - dialogSize.width() - 16));
            }
            if (topLeft.y() + dialogSize.height() > available.bottom()) {
                topLeft.setY(qMax(available.top(), available.bottom() - dialogSize.height() + 1));
            }
            topLeft.setX(qMax(available.left(), topLeft.x()));
            topLeft.setY(qMax(available.top(), topLeft.y()));
            targetGeometry.moveTopLeft(topLeft);
        }

        m_advancedDialog->setGeometry(targetGeometry);
    }

    if (m_advancedDialog->isMinimized()) {
        m_advancedDialog->showNormal();
    } else {
        m_advancedDialog->show();
    }
    m_advancedDialog->raise();
    m_advancedDialog->activateWindow();
}

void MainWindow::syncAxisReadouts()
{
    const QList<int> boardIndices = connectedBoardIndices();
    if (boardIndices.isEmpty()) {
        m_frontLeftPositionLabel->setText(QStringLiteral("-- mm"));
        m_frontRightPositionLabel->setText(QStringLiteral("-- mm"));
        m_rearLeftPositionLabel->setText(QStringLiteral("-- mm"));
        m_rearRightPositionLabel->setText(QStringLiteral("-- mm"));
        return;
    }

    // 四轮映射：板子0转轮=前左，板子0驱轮=前右，板子1转轮=后左，板子1驱轮=后右
    QString frontLeft = QStringLiteral("-- mm");
    QString frontRight = QStringLiteral("-- mm");
    QString rearLeft = QStringLiteral("-- mm");
    QString rearRight = QStringLiteral("-- mm");
    
    for (const int boardIndex : boardIndices) {
        const double turnPos = axisPositionMm(boardIndex, boardTurnNodeId(boardIndex), true);
        const double drivePos = axisPositionMm(boardIndex, boardDriveNodeId(boardIndex), false);
        
        if (boardIndex == 0) {
            frontLeft = QStringLiteral("%1 mm").arg(QString::number(turnPos, 'f', 2));
            frontRight = QStringLiteral("%1 mm").arg(QString::number(drivePos, 'f', 2));
        } else if (boardIndex == 1) {
            rearLeft = QStringLiteral("%1 mm").arg(QString::number(turnPos, 'f', 2));
            rearRight = QStringLiteral("%1 mm").arg(QString::number(drivePos, 'f', 2));
        }
    }
    
    m_frontLeftPositionLabel->setText(frontLeft);
    m_frontRightPositionLabel->setText(frontRight);
    m_rearLeftPositionLabel->setText(rearLeft);
    m_rearRightPositionLabel->setText(rearRight);
}

void MainWindow::setZeroPoint()
{
    const QList<int> boardIndices = connectedBoardIndices();
    if (boardIndices.isEmpty()) {
        appendLog(zh("e8afb7e58588e8bf9ee68ea5204f4472697665"));
        updateProgramStatus(zh("e8afb7e58588e8bf9ee68ea5204f4472697665"));
        return;
    }

    for (const int boardIndex : boardIndices) {
        BoardRuntime &runtime = m_boardRuntimes[boardIndex];
        ODriveMotorController *controller = runtime.controller;
        if (!controller) {
            continue;
        }
        runtime.turnZeroTurns = controller->status(boardTurnNodeId(boardIndex)).positionTurns;
        runtime.driveZeroTurns = controller->status(boardDriveNodeId(boardIndex)).positionTurns;
        runtime.zeroPointInitialized = true;
    }
    syncAxisReadouts();
    updateProgramStatus(zh("e99bb6e782b9e5b7b2e8aebee7bdae"));
    appendLog(zh("e5bd93e5898de8bdace8bdaee5928ce9a9b1e8bdaee4bd8de7bdaee5b7b2e8aeb0e4b8bae99bb6e782b9"));
}

void MainWindow::sendHome()
{
    const QList<int> boardIndices = connectedBoardIndices();
    if (boardIndices.isEmpty()) {
        return;
    }

    bool needZeroPoint = false;
    for (const int boardIndex : boardIndices) {
        if (!m_boardRuntimes[boardIndex].zeroPointInitialized) {
            needZeroPoint = true;
            break;
        }
    }
    if (needZeroPoint) {
        setZeroPoint();
    }

    m_scanState = ScanState();
    for (const int boardIndex : boardIndices) {
        commandAxisPosition(boardIndex, boardTurnNodeId(boardIndex), 0.0, m_homeSpeedSpin->value(), true);
        commandAxisPosition(boardIndex, boardDriveNodeId(boardIndex), 0.0, m_homeSpeedSpin->value(), false);
    }
    updateProgramStatus(zh("e6ada3e59ca8e59b9ee58e9fe782b9"));
}

void MainWindow::resetMotionState()
{
    const QList<int> boardIndices = connectedBoardIndices();
    if (boardIndices.isEmpty()) {
        return;
    }

    stopAllMotion(true);
    for (const int boardIndex : boardIndices) {
        if (ODriveMotorController *controller = m_boardRuntimes[boardIndex].controller) {
            controller->clearErrors(boardTurnNodeId(boardIndex), false);
            if (boardDriveNodeId(boardIndex) != boardTurnNodeId(boardIndex)) {
                controller->clearErrors(boardDriveNodeId(boardIndex), false);
            }
        }
    }

    m_scanState = ScanState();
    updateProgramStatus(zh("e5b7b2e5a48de4bd8defbc8ce99499e8afafe5b7b2e6b885e999a4"));
}

void MainWindow::enableAxes()
{
    const QList<int> boardIndices = connectedBoardIndices();
    if (boardIndices.isEmpty()) {
        appendLog(zh("e8afb7e58588e8bf9ee68ea5204f4472697665"));
        updateProgramStatus(zh("e8afb7e58588e8bf9ee68ea5204f4472697665"));
        return;
    }

    const auto hex32 = [](quint32 value) {
        return QStringLiteral("0x%1").arg(value, 8, 16, QLatin1Char('0')).toUpper();
    };

    QList<QPair<int, int>> axisTargets;
    for (const int boardIndex : boardIndices) {
        QList<quint8> nodeIds;
        nodeIds << boardTurnNodeId(boardIndex);
        const quint8 driveNodeId = boardDriveNodeId(boardIndex);
        if (!nodeIds.contains(driveNodeId)) {
            nodeIds << driveNodeId;
        }

        for (const quint8 nodeId : nodeIds) {
            axisTargets.append(qMakePair(boardIndex, int(nodeId)));
        }
    }

    if (axisTargets.isEmpty()) {
        appendLog(QStringLiteral("Enable aborted: no axis targets found"));
        updateProgramStatus(QStringLiteral("No axis targets"));
        return;
    }

    appendLog(QStringLiteral("Enable all axes: entering IDLE, clearing errors, then closing loop"));
    updateProgramStatus(zh("e6ada3e59ca8e4bdbfe883bde585a8e983a8e794b5e69cba"));

    for (const auto &target : axisTargets) {
        const int boardIndex = target.first;
        const quint8 nodeId = static_cast<quint8>(target.second & 0x3F);
        ODriveMotorController *controller = m_boardRuntimes[boardIndex].controller;
        if (!controller || !controller->isConnected()) {
            continue;
        }

        controller->requestIdle(nodeId);
        controller->clearErrors(nodeId, false);
        controller->requestAllTelemetry(nodeId, false);
    }

    QTimer::singleShot(600, this, [this, axisTargets, hex32]() {
        QList<QPair<int, int>> calibrationTargets;

        for (const auto &target : axisTargets) {
            const int boardIndex = target.first;
            const quint8 nodeId = static_cast<quint8>(target.second & 0x3F);
            ODriveMotorController *controller = m_boardRuntimes[boardIndex].controller;
            if (!controller || !controller->isConnected()) {
                continue;
            }

            const ODriveMotorController::AxisStatus &status = controller->status(nodeId);
            const bool hasErrors = status.axisError != 0
                    || status.motorError != 0
                    || status.encoderError != 0
                    || status.controllerError != 0;
            const bool needsCalibration = (status.axisError & 0x00000001u) != 0
                    || status.motorError != 0
                    || status.encoderError != 0;

            if (hasErrors) {
                appendLog(QStringLiteral("%1 Node %2 enable precheck: axis=%3 motor=%4 encoder=%5 controller=%6")
                          .arg(boardLogPrefix(boardIndex))
                          .arg(nodeId)
                          .arg(hex32(status.axisError))
                          .arg(hex32(status.motorError))
                          .arg(hex32(status.encoderError))
                          .arg(hex32(status.controllerError)));
            }

            if (needsCalibration) {
                calibrationTargets.append(target);
            }
        }

        if (!calibrationTargets.isEmpty()) {
            appendLog(QStringLiteral("Enable detected %1 axis(es) needing full calibration before closed loop")
                      .arg(calibrationTargets.size()));
            for (const auto &target : calibrationTargets) {
                const int boardIndex = target.first;
                const quint8 nodeId = static_cast<quint8>(target.second & 0x3F);
                ODriveMotorController *controller = m_boardRuntimes[boardIndex].controller;
                if (!controller || !controller->isConnected()) {
                    continue;
                }

                appendLog(QStringLiteral("%1 Node %2 running full calibration during enable")
                          .arg(boardLogPrefix(boardIndex))
                          .arg(nodeId));
                controller->requestIdle(nodeId);
                controller->clearErrors(nodeId, false);
                controller->requestFullCalibration(nodeId);
            }
        }

        const int closeLoopDelayMs = calibrationTargets.isEmpty() ? 800 : 12500;
        QTimer::singleShot(closeLoopDelayMs, this, [this, axisTargets, hex32]() {
            for (const auto &target : axisTargets) {
                const int boardIndex = target.first;
                const quint8 nodeId = static_cast<quint8>(target.second & 0x3F);
                ODriveMotorController *controller = m_boardRuntimes[boardIndex].controller;
                if (!controller || !controller->isConnected()) {
                    continue;
                }

                controller->requestAllTelemetry(nodeId, false);
                controller->clearErrors(nodeId, false);
                const float safeVelLimit = (nodeId == 0) ? 5.0f : 20.0f;
                controller->setLimits(nodeId, safeVelLimit, static_cast<float>(currentLimitAmps()));
                ensureAxisClosedLoop(boardIndex, nodeId);
            }

            QTimer::singleShot(1200, this, [this, axisTargets, hex32]() {
                int successCount = 0;
                QStringList failedAxes;

                for (const auto &target : axisTargets) {
                    const int boardIndex = target.first;
                    const quint8 nodeId = static_cast<quint8>(target.second & 0x3F);
                    ODriveMotorController *controller = m_boardRuntimes[boardIndex].controller;
                    if (!controller || !controller->isConnected()) {
                        failedAxes << QStringLiteral("%1 Node %2 disconnected")
                                      .arg(boardLogPrefix(boardIndex))
                                      .arg(nodeId);
                        continue;
                    }

                    controller->requestAllTelemetry(nodeId, false);
                    const ODriveMotorController::AxisStatus &status = controller->status(nodeId);
                    const bool ok = status.axisState == ODriveMotorController::ClosedLoopControl
                            && status.axisError == 0
                            && status.motorError == 0
                            && status.encoderError == 0
                            && status.controllerError == 0;

                    if (ok) {
                        ++successCount;
                        continue;
                    }

                    failedAxes << QStringLiteral("%1 Node %2 state=%3 axis=%4 motor=%5 encoder=%6 controller=%7")
                                  .arg(boardLogPrefix(boardIndex))
                                  .arg(nodeId)
                                  .arg(status.axisState)
                                  .arg(hex32(status.axisError))
                                  .arg(hex32(status.motorError))
                                  .arg(hex32(status.encoderError))
                                  .arg(hex32(status.controllerError));
                }

                if (failedAxes.isEmpty()) {
                    const QString statusMessage = QStringLiteral("All %1 axis(es) entered closed loop without errors")
                            .arg(successCount);
                    appendLog(statusMessage);
                    updateProgramStatus(zh("e585a8e983a8e794b5e69cbae5b7b2e8bf9be585a5e997ade78eafefbc8ce697a0e99499e8afaf"));
                } else {
                    appendLog(QStringLiteral("Enable finished with %1/%2 axis(es) ready")
                              .arg(successCount)
                              .arg(axisTargets.size()));
                    for (const QString &line : failedAxes) {
                        appendLog(line);
                    }
                    updateProgramStatus(QStringLiteral("部分电机未进入闭环或仍有错误"));
                }

                scheduleStatusPanelUpdate();
            });
        });
    });
}

void MainWindow::startScanProgram()
{
    const QList<int> boardIndices = connectedBoardIndices();
    if (boardIndices.isEmpty()) {
        appendLog(zh("e8afb7e58588e8bf9ee68ea5204f4472697665"));
        updateProgramStatus(zh("e8afb7e58588e8bf9ee68ea5204f4472697665"));
        return;
    }

    if (m_scanAxisLengthSpin->value() <= 0.0
            || m_scanSpeedSpin->value() <= 0.0
            || m_stepLengthSpin->value() <= 0.0) {
        appendLog(zh("e58f82e695b0e697a0e69588efbc9ae689abe68f8fe8bdb4e995bfe38081e689abe68f8fe9809fe5baa6e38081e6ada5e8bf9be995bfe5baa6e983bde5bf85e9a1bbe5a4a7e4ba8e2030"));
        updateProgramStatus(zh("e58f82e695b0e697a0e69588"));
        return;
    }

    bool needZeroPoint = false;
    for (const int boardIndex : boardIndices) {
        if (!m_boardRuntimes[boardIndex].zeroPointInitialized) {
            needZeroPoint = true;
            break;
        }
    }
    if (needZeroPoint) {
        setZeroPoint();
    }

    m_scanState = ScanState();
    m_scanState.active = true;
    m_scanState.phase = ScanHoming;
    m_scanState.forward = true;
    m_scanState.maxStepMm = qMax(0.0, m_stepAxisLengthSpin->value());

    for (const int boardIndex : boardIndices) {
        commandAxisPosition(boardIndex, boardTurnNodeId(boardIndex), 0.0, m_homeSpeedSpin->value(), true);
        commandAxisPosition(boardIndex, boardDriveNodeId(boardIndex), 0.0, m_homeSpeedSpin->value(), false);
    }

    updateProgramStatus(zh("e689abe68f8fe58786e5a487e4b8adefbc8ce58588e59b9ee588b0e99bb6e782b9"));
    appendLog(zh("e887aae58aa8e689abe68f8fe5b7b2e5bc80e5a78b"));
}

void MainWindow::stopProgram()
{
    if (!hasAnyConnectedBoard()) {
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
    const QList<int> boardIndices = connectedBoardIndices();
    if (boardIndices.isEmpty()) {
        return;
    }

    bool needZeroPoint = false;
    for (const int boardIndex : boardIndices) {
        if (!m_boardRuntimes[boardIndex].zeroPointInitialized) {
            needZeroPoint = true;
            break;
        }
    }
    if (needZeroPoint) {
        setZeroPoint();
    }

    const double limitMm = qMax(0.0, m_stepAxisLengthSpin->value());
    const int firstBoardIndex = boardIndices.first();
    const double rawNextMm = axisPositionMm(firstBoardIndex, boardTurnNodeId(firstBoardIndex), true) + m_stepLengthSpin->value();
    const double nextMm = limitMm > 0.0 ? qMin(limitMm, rawNextMm) : rawNextMm;

    for (const int boardIndex : boardIndices) {
        commandAxisPosition(boardIndex,
                            boardTurnNodeId(boardIndex),
                            nextMm,
                            qMax(0.1, m_jogSpeedSpin->value()),
                            true);
    }
    updateProgramStatus(zh("e6ada5e8bf9be8bdb4e7a7bbe58aa8e588b0202531206d6d").arg(QString::number(nextMm, 'f', 2)));
}

void MainWindow::handleNodeStatusUpdate(quint8 nodeId)
{
    Q_UNUSED(nodeId);
    scheduleStatusPanelUpdate();
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

    m_logFlushTimer = new QTimer(this);
    m_logFlushTimer->setSingleShot(true);
    m_logFlushTimer->setInterval(80);
    connect(m_logFlushTimer, &QTimer::timeout, this, &MainWindow::flushQueuedMessages);

    m_rxFlushTimer = new QTimer(this);
    m_rxFlushTimer->setSingleShot(true);
    m_rxFlushTimer->setInterval(80);
    connect(m_rxFlushTimer, &QTimer::timeout, this, &MainWindow::flushQueuedRxMessages);

    m_statusUpdateTimer = new QTimer(this);
    m_statusUpdateTimer->setSingleShot(true);
    m_statusUpdateTimer->setInterval(100);
    connect(m_statusUpdateTimer, &QTimer::timeout, this, [this]() {
        updateStatusPanel();
        syncAxisReadouts();
        updateLinkStatusDisplay();
    });

    m_settingsSaveTimer = new QTimer(this);
    m_settingsSaveTimer->setSingleShot(true);
    m_settingsSaveTimer->setInterval(250);
    connect(m_settingsSaveTimer, &QTimer::timeout, this, [this]() { saveSettings(); });

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

    QLabel *frontLeftLabel = new QLabel(zh("e5898de8bdaee5b7a63a"), leftPanel);
    QLabel *frontRightLabel = new QLabel(zh("e5898de8bdaee58fb33a"), leftPanel);
    QLabel *rearLeftLabel = new QLabel(zh("e5908ee8bdaee5b7a63a"), leftPanel);
    QLabel *rearRightLabel = new QLabel(zh("e5908ee8bdaee58fb33a"), leftPanel);
    frontLeftLabel->setStyleSheet(QStringLiteral("font-size: 20px;"));
    frontRightLabel->setStyleSheet(QStringLiteral("font-size: 20px;"));
    rearLeftLabel->setStyleSheet(QStringLiteral("font-size: 20px;"));
    rearRightLabel->setStyleSheet(QStringLiteral("font-size: 20px;"));
    m_frontLeftPositionLabel = createReadoutLabel();
    m_frontRightPositionLabel = createReadoutLabel();
    m_rearLeftPositionLabel = createReadoutLabel();
    m_rearRightPositionLabel = createReadoutLabel();
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

    leftLayout->addWidget(frontLeftLabel, 0, 0);
    leftLayout->addWidget(m_frontLeftPositionLabel, 0, 1);
    leftLayout->addWidget(new QLabel(QStringLiteral("mm"), leftPanel), 0, 2);
    leftLayout->addWidget(frontRightLabel, 1, 0);
    leftLayout->addWidget(m_frontRightPositionLabel, 1, 1);
    leftLayout->addWidget(new QLabel(QStringLiteral("mm"), leftPanel), 1, 2);
    leftLayout->addWidget(rearLeftLabel, 2, 0);
    leftLayout->addWidget(m_rearLeftPositionLabel, 2, 1);
    leftLayout->addWidget(new QLabel(QStringLiteral("mm"), leftPanel), 2, 2);
    leftLayout->addWidget(rearRightLabel, 3, 0);
    leftLayout->addWidget(m_rearRightPositionLabel, 3, 1);
    leftLayout->addWidget(new QLabel(QStringLiteral("mm"), leftPanel), 3, 2);
    leftLayout->addWidget(m_setZeroButton, 4, 0, 1, 2);
    leftLayout->addWidget(new QLabel(zh("e59b9ee58e9fe782b9e9809fe5baa63a")), 5, 0);
    leftLayout->addWidget(m_homeSpeedSpin, 5, 1, 1, 2);
    leftLayout->addWidget(new QLabel(zh("e8a18ce9a9b6e9809fe5baa63a")), 6, 0);
    leftLayout->addWidget(m_jogSpeedSpin, 6, 1, 1, 2);
    leftLayout->setRowStretch(7, 1);

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
    m_enableButton = new QPushButton(zh("e4bdbfe883bd"), centerPanel);

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
    m_rxEdit = new QPlainTextEdit(rxGroup);
    m_rxEdit->setReadOnly(true);
    m_rxEdit->setUndoRedoEnabled(false);
    m_rxEdit->setLineWrapMode(QPlainTextEdit::NoWrap);
    m_rxEdit->document()->setMaximumBlockCount(400);
    rxLayout->addWidget(m_rxEdit);

    QGroupBox *logGroup = new QGroupBox(zh("e7b3bbe7bb9fe697a5e5bf97"), ui->centralWidget);
    QVBoxLayout *logLayout = new QVBoxLayout(logGroup);
    m_logEdit = new QPlainTextEdit(logGroup);
    m_logEdit->setReadOnly(true);
    m_logEdit->setUndoRedoEnabled(false);
    m_logEdit->setLineWrapMode(QPlainTextEdit::NoWrap);
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

    auto connectJogButton = [this](QPushButton *button, bool turning, double direction) {
        connect(button, &QPushButton::pressed, this, [this, turning, direction]() {
            const QList<int> boardIndices = connectedBoardIndices();
            if (boardIndices.isEmpty()) {
                return;
            }

            if (m_scanState.active) {
                m_scanState = ScanState();
                updateProgramStatus(zh("e6898be58aa8e782b9e58aa8e5b7b2e4b8ade6ada2e887aae58aa8e689abe68f8f"));
            }

            QStringList errorAxes;
            for (const int boardIndex : boardIndices) {
                QList<quint8> requiredNodeIds;
                requiredNodeIds << boardTurnNodeId(boardIndex);
                const quint8 driveNodeId = boardDriveNodeId(boardIndex);
                if (!requiredNodeIds.contains(driveNodeId)) {
                    requiredNodeIds << driveNodeId;
                }

                for (const quint8 requiredNodeId : requiredNodeIds) {
                    if (axisHasActiveErrors(boardIndex, requiredNodeId)) {
                        errorAxes << QStringLiteral("%1 Node %2")
                                      .arg(boardLogPrefix(boardIndex))
                                      .arg(requiredNodeId);
                    }
                }
            }

            if (!errorAxes.isEmpty()) {
                appendLog(QStringLiteral("Motion blocked: axes have active errors -> %1")
                          .arg(errorAxes.join(QStringLiteral(", "))));
                updateProgramStatus(QStringLiteral("小车运动已拒绝：存在带错误电机"));
                return;
            }

            struct WheelCommand
            {
                int boardIndex = -1;
                quint8 nodeId = 0;
                bool turnAxis = false;
                double speedMmPerSecond = 0.0;
                QString label;
            };

            const double targetSpeed = direction * qMax(0.1, m_jogSpeedSpin->value());
            QList<WheelCommand> wheelCommands;
            wheelCommands.reserve(boardIndices.size() * 2);

            for (const int boardIndex : boardIndices) {
                if (turning) {
                    wheelCommands.append(WheelCommand{boardIndex,
                                                      boardTurnNodeId(boardIndex),
                                                      true,
                                                      -targetSpeed,
                                                      QStringLiteral("%1:L").arg(boardDisplayName(boardIndex))});
                    wheelCommands.append(WheelCommand{boardIndex,
                                                      boardDriveNodeId(boardIndex),
                                                      false,
                                                      targetSpeed,
                                                      QStringLiteral("%1:R").arg(boardDisplayName(boardIndex))});
                } else {
                    wheelCommands.append(WheelCommand{boardIndex,
                                                      boardTurnNodeId(boardIndex),
                                                      true,
                                                      targetSpeed,
                                                      QStringLiteral("%1:L").arg(boardDisplayName(boardIndex))});
                    wheelCommands.append(WheelCommand{boardIndex,
                                                      boardDriveNodeId(boardIndex),
                                                      false,
                                                      targetSpeed,
                                                      QStringLiteral("%1:R").arg(boardDisplayName(boardIndex))});
                }
            }

            int successCount = 0;
            int failCount = 0;
            QStringList statusList;
            QMap<int, QStringList> boardStatusMap;

            QList<bool> preparedResults;
            preparedResults.reserve(wheelCommands.size());
            for (const WheelCommand &command : wheelCommands) {
                preparedResults.append(prepareAxisVelocityControl(command.boardIndex,
                                                                 command.nodeId,
                                                                 command.speedMmPerSecond,
                                                                 command.turnAxis));
            }

            for (int i = 0; i < wheelCommands.size(); ++i) {
                const WheelCommand &command = wheelCommands.at(i);
                const bool preparedOk = preparedResults.value(i, false);
                const bool ok = preparedOk
                        ? sendPreparedAxisVelocity(command.boardIndex,
                                                   command.nodeId,
                                                   command.speedMmPerSecond,
                                                   command.turnAxis)
                        : false;

                successCount += ok ? 1 : 0;
                failCount += ok ? 0 : 1;
                boardStatusMap[command.boardIndex] << QStringLiteral("%1%2")
                                                      .arg(command.turnAxis ? QStringLiteral("L") : QStringLiteral("R"))
                                                      .arg(ok ? QStringLiteral("OK") : QStringLiteral("FAIL"));
            }

            for (const int boardIndex : boardIndices) {
                const QStringList parts = boardStatusMap.value(boardIndex);
                const QString leftState = parts.value(0, QStringLiteral("LFAIL")).mid(1);
                const QString rightState = parts.value(1, QStringLiteral("RFAIL")).mid(1);
                statusList << QStringLiteral("%1:L%2 R%3")
                              .arg(boardDisplayName(boardIndex))
                              .arg(leftState)
                              .arg(rightState);
            }

            const int totalWheelGroups = wheelCommands.size();
            QString syncStatus = QString(zh("e5908ce6ada5") + " %1/%2 " + zh("e68890e58a9f"))
                    .arg(successCount)
                    .arg(totalWheelGroups);
            if (failCount > 0) {
                syncStatus += QStringLiteral(" [%1").arg(failCount) + zh("e5a4b1e8b4a5") + QStringLiteral("]");
            }
            updateProgramStatus(syncStatus + QStringLiteral(" - ") + statusList.join(QStringLiteral(" ")));
        });

        connect(button, &QPushButton::released, this, [this]() {
            const QList<int> boardIndices = connectedBoardIndices();
            if (boardIndices.isEmpty()) {
                return;
            }

            int stopCount = 0;
            for (const int boardIndex : boardIndices) {
                stopCount += stopAxisVelocity(boardIndex, boardTurnNodeId(boardIndex)) ? 1 : 0;
                const quint8 driveNodeId = boardDriveNodeId(boardIndex);
                if (driveNodeId != boardTurnNodeId(boardIndex)) {
                    stopCount += stopAxisVelocity(boardIndex, driveNodeId) ? 1 : 0;
                }
            }
            updateProgramStatus(QString(zh("e5819ce6ada2") + " %1").arg(stopCount));
        });
    };

    // 当前四轮映射：
    // 板0: turn=前左, drive=前右
    // 板1: turn=后左, drive=后右
    // 前进/后退：四轮同速；左右：左右轮差速
    connectJogButton(m_forwardButton, false, 1.0);
    connectJogButton(m_backwardButton, false, -1.0);
    connectJogButton(m_leftButton, true, -1.0);
    connectJogButton(m_rightButton, true, 1.0);
}

void MainWindow::buildAdvancedDialog()
{
    m_advancedDialog = new QDialog(nullptr, Qt::Window);
    m_advancedDialog->setAttribute(Qt::WA_DeleteOnClose, false);
    m_advancedDialog->setWindowTitle(zh("e9ab98e7baa7e99da2e69dbf"));
    const QRect availableGeometry = QGuiApplication::primaryScreen()
            ? QGuiApplication::primaryScreen()->availableGeometry()
            : QRect(0, 0, 1280, 900);
    m_advancedDialog->resize(qMax(900, int(availableGeometry.width() * 0.9)),
                             qMax(650, int(availableGeometry.height() * 0.88)));
    m_advancedDialog->move(availableGeometry.center() - QPoint(m_advancedDialog->width() / 2,
                                                               m_advancedDialog->height() / 2));

    QVBoxLayout *dialogLayout = new QVBoxLayout(m_advancedDialog);
    dialogLayout->setContentsMargins(8, 8, 8, 8);
    dialogLayout->setSpacing(0);

    QScrollArea *scrollArea = new QScrollArea(m_advancedDialog);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    dialogLayout->addWidget(scrollArea);

    QWidget *contentWidget = new QWidget(scrollArea);
    scrollArea->setWidget(contentWidget);

    QVBoxLayout *layout = new QVBoxLayout(contentWidget);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(12);

    QGroupBox *communicationGroup = new QGroupBox(zh("e9809ae4bfa1e9858de7bdae"), contentWidget);
    QGridLayout *communicationLayout = new QGridLayout(communicationGroup);

    m_pluginCombo = new QComboBox(communicationGroup);
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
    m_advancedConnectButton = new QPushButton(QStringLiteral("connect"), communicationGroup);
    m_advancedConnectButton->setMinimumHeight(40);
    m_advancedConnectButton->setStyleSheet(QStringLiteral("font-size: 18px; padding: 4px 14px;"));

    communicationLayout->addWidget(new QLabel(zh("e68f92e4bbb6")), 0, 0);
    communicationLayout->addWidget(m_pluginCombo, 0, 1);
    communicationLayout->addWidget(m_refreshInterfacesButton, 0, 2);
    communicationLayout->addWidget(m_advancedConnectButton, 0, 3);
    communicationLayout->addWidget(new QLabel(zh("e6b3a2e789b9e78e87")), 1, 0);
    communicationLayout->addWidget(m_bitrateSpin, 1, 1);
    communicationLayout->addWidget(new QLabel(QStringLiteral("Serial Baud")), 1, 2);
    communicationLayout->addWidget(m_serialBaudSpin, 1, 3);
    communicationLayout->setColumnStretch(3, 1);

    QGroupBox *interfaceConfigGroup = new QGroupBox(zh("e68ea5e58fa3e9858de7bdae"), contentWidget);
    QGridLayout *interfaceConfigLayout = new QGridLayout(interfaceConfigGroup);

    m_interfaceCountCombo = new QComboBox(interfaceConfigGroup);
    m_interfaceCountCombo->addItem(zh("3120e4b8aae68ea5e58fa3"), 1);
    m_interfaceCountCombo->addItem(zh("3220e4b8aae68ea5e58fa3"), 2);
    m_interfaceCountCombo->addItem(zh("3320e4b8aae68ea5e58fa3"), 3);
    m_interfaceCountCombo->addItem(zh("3420e4b8aae68ea5e58fa3"), 4);
    m_interfaceCountCombo->setCurrentIndex(0);

    interfaceConfigLayout->addWidget(new QLabel(zh("e8bf9ee68ea5e68ea5e58fa3e695b0"), interfaceConfigGroup), 0, 0);
    interfaceConfigLayout->addWidget(m_interfaceCountCombo, 0, 1);
    buildBoardConfigRows(interfaceConfigLayout, interfaceConfigGroup);
    interfaceConfigLayout->setColumnStretch(3, 1);

    QGroupBox *nodeSwitchGroup = new QGroupBox(zh("e88a82e782b9e58887e68da2"), contentWidget);
    QGridLayout *nodeSwitchLayout = new QGridLayout(nodeSwitchGroup);

    m_debugBoardCombo = new QComboBox(nodeSwitchGroup);
    m_debugNodeCombo = new QComboBox(nodeSwitchGroup);
    m_debugNodeCombo->setEnabled(false);
    m_debugScaleSpin = new QDoubleSpinBox(nodeSwitchGroup);
    m_debugScaleSpin->setDecimals(4);
    m_debugScaleSpin->setRange(0.0001, 100000.0);
    m_debugScaleSpin->setValue(1.0);
    m_debugScaleSpin->setSuffix(zh("206d6d2fe59c88"));
    m_activeNodesLabel = new QLabel(zh("e6a380e6b58be588b0e79a84e88a82e782b93a202d2d"), nodeSwitchGroup);
    m_activeNodesLabel->setStyleSheet(QStringLiteral("font-size: 16px;"));
    m_rescanNodesButton = new QPushButton(zh("e9878de696b0e689abe68f8f"), nodeSwitchGroup);
    m_rescanNodesButton->setMinimumHeight(40);
    m_rescanNodesButton->setStyleSheet(QStringLiteral("font-size: 16px; padding: 4px 12px;"));

    nodeSwitchLayout->addWidget(new QLabel(zh("e8b083e8af95e69dbfe5ad90"), nodeSwitchGroup), 0, 0);
    nodeSwitchLayout->addWidget(m_debugBoardCombo, 0, 1);
    nodeSwitchLayout->addWidget(new QLabel(zh("e8b083e8af95e88a82e782b9"), nodeSwitchGroup), 0, 2);
    nodeSwitchLayout->addWidget(m_debugNodeCombo, 0, 3);
    nodeSwitchLayout->addWidget(new QLabel(zh("e68da2e7ae97"), nodeSwitchGroup), 1, 0);
    nodeSwitchLayout->addWidget(m_debugScaleSpin, 1, 1);
    nodeSwitchLayout->addWidget(m_activeNodesLabel, 1, 2);
    nodeSwitchLayout->addWidget(m_rescanNodesButton, 1, 3);
    nodeSwitchLayout->setColumnStretch(2, 1);

    layout->addWidget(communicationGroup);
    layout->addWidget(interfaceConfigGroup);
    layout->addWidget(nodeSwitchGroup);

    QGroupBox *txGroup = new QGroupBox(QStringLiteral("Transmit"), contentWidget);
    QGridLayout *txLayout = new QGridLayout(txGroup);
    txLayout->setHorizontalSpacing(10);
    txLayout->setVerticalSpacing(8);

    m_txIdEdit = new QLineEdit(txGroup);
    m_txIdEdit->setPlaceholderText(QStringLiteral("207"));
    m_txIdEdit->setStyleSheet(topInputStyle());
    m_txIdEdit->setMaximumWidth(110);

    m_txDlcSpin = new QSpinBox(txGroup);
    m_txDlcSpin->setRange(0, 8);
    m_txDlcSpin->setValue(8);
    m_txDlcSpin->setStyleSheet(topInputStyle());
    m_txDlcSpin->setMaximumWidth(70);

    m_txExtendedCheck = new QCheckBox(QStringLiteral("Extended ID"), txGroup);
    m_txRemoteCheck = new QCheckBox(QStringLiteral("RTR"), txGroup);
    m_txErrorCheck = new QCheckBox(QStringLiteral("Error Frame"), txGroup);
    m_txErrorCheck->setEnabled(false);
    m_txErrorCheck->setToolTip(QStringLiteral("Raw Error Frame transmit is not supported in this tool."));

    m_txPeriodSpin = new QSpinBox(txGroup);
    m_txPeriodSpin->setRange(0, 600000);
    m_txPeriodSpin->setValue(1000);
    m_txPeriodSpin->setSuffix(QStringLiteral(" ms"));
    m_txPeriodSpin->setStyleSheet(topInputStyle());
    m_txPeriodSpin->setToolTip(QStringLiteral("0 = send once, >0 = repeat interval"));
    m_txPeriodSpin->setMaximumWidth(110);

    m_txSendButton = new QPushButton(QStringLiteral("Send Single"), txGroup);
    m_txSendButton->setMinimumHeight(44);
    m_txSendButton->setStyleSheet(QStringLiteral("font-size: 18px; padding: 4px 14px;"));

    m_txToggleButton = new QPushButton(QStringLiteral("Send Repeat"), txGroup);
    m_txToggleButton->setMinimumHeight(44);
    m_txToggleButton->setStyleSheet(QStringLiteral("font-size: 18px; padding: 4px 14px;"));

    m_txStatusLabel = new QLabel(QStringLiteral("Status: idle"), txGroup);
    m_txStatusLabel->setStyleSheet(QStringLiteral("QLabel { color: #666666; font-weight: 600; }"));

    QVBoxLayout *addressLayout = new QVBoxLayout;
    addressLayout->setSpacing(4);
    addressLayout->addWidget(new QLabel(QStringLiteral("Address"), txGroup));
    addressLayout->addWidget(m_txIdEdit);

    QVBoxLayout *dlcLayout = new QVBoxLayout;
    dlcLayout->setSpacing(4);
    dlcLayout->addWidget(new QLabel(QStringLiteral("DLC"), txGroup));
    dlcLayout->addWidget(m_txDlcSpin);

    QVBoxLayout *payloadLayoutHost = new QVBoxLayout;
    payloadLayoutHost->setSpacing(4);
    payloadLayoutHost->addWidget(new QLabel(QStringLiteral("Payload"), txGroup));

    QGridLayout *payloadLayout = new QGridLayout;
    payloadLayout->setHorizontalSpacing(6);
    payloadLayout->setVerticalSpacing(2);
    m_txByteEdits.clear();
    for (int i = 0; i < 8; ++i) {
        QLineEdit *byteEdit = new QLineEdit(txGroup);
        byteEdit->setMaxLength(2);
        byteEdit->setFixedWidth(42);
        byteEdit->setAlignment(Qt::AlignCenter);
        byteEdit->setPlaceholderText(QStringLiteral("00"));
        byteEdit->setText(QStringLiteral("00"));
        byteEdit->setStyleSheet(QStringLiteral(
                                    "QLineEdit {"
                                    " background: #ffffff;"
                                    " border: 1px solid #cfcfcf;"
                                    " border-radius: 4px;"
                                    " padding: 4px 2px;"
                                    " font-size: 20px;"
                                    " min-height: 36px;"
                                    "}"));
        payloadLayout->addWidget(byteEdit, 0, i);

        QLabel *indexLabel = new QLabel(QString::number(i + 1), txGroup);
        indexLabel->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
        payloadLayout->addWidget(indexLabel, 1, i);
        m_txByteEdits.append(byteEdit);
    }
    payloadLayoutHost->addLayout(payloadLayout);

    QHBoxLayout *flagsLayout = new QHBoxLayout;
    flagsLayout->setSpacing(18);
    flagsLayout->addWidget(m_txExtendedCheck);
    flagsLayout->addWidget(m_txRemoteCheck);
    flagsLayout->addWidget(m_txErrorCheck);
    flagsLayout->addStretch(1);

    QVBoxLayout *buttonLayout = new QVBoxLayout;
    buttonLayout->setSpacing(8);
    buttonLayout->addStretch(1);
    buttonLayout->addWidget(m_txSendButton);
    buttonLayout->addWidget(m_txToggleButton);
    buttonLayout->addStretch(1);

    QHBoxLayout *repeatLayout = new QHBoxLayout;
    repeatLayout->setSpacing(8);
    repeatLayout->addWidget(new QLabel(QStringLiteral("Send Repeat"), txGroup));
    repeatLayout->addWidget(m_txPeriodSpin);
    repeatLayout->addSpacing(18);
    repeatLayout->addWidget(m_txStatusLabel, 1);

    txLayout->addLayout(addressLayout, 0, 0);
    txLayout->addLayout(dlcLayout, 0, 1);
    txLayout->addLayout(payloadLayoutHost, 0, 2);
    txLayout->addLayout(buttonLayout, 0, 3);
    txLayout->addLayout(flagsLayout, 1, 2);
    txLayout->addLayout(repeatLayout, 2, 0, 1, 4);
    txLayout->setColumnStretch(2, 1);

    layout->addWidget(txGroup);
    buildAdvancedContent(layout, contentWidget);

    connect(m_pluginCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::refreshCanInterfaces);
    connect(m_refreshInterfacesButton, &QPushButton::clicked,
            this, &MainWindow::refreshCanInterfaces);
    connect(m_advancedConnectButton, &QPushButton::clicked,
            this, &MainWindow::toggleConnection);
    connect(m_bitrateSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, [this](int) { syncConnectionEditorsFromAdvanced(); });
    connect(m_serialBaudSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, [this](int) { syncConnectionEditorsFromAdvanced(); });
    connect(m_interfaceCountCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int) {
        updateBoardRowVisibility();
        updateDebugBoardCombo();
        syncConnectionEditorsFromAdvanced();
    });

    m_txTimer = new QTimer(this);
    m_txTimer->setInterval(100);
    m_txResponseTimer = new QTimer(this);
    m_txResponseTimer->setSingleShot(true);
    m_txResponseTimer->setInterval(1200);
    connect(m_txSendButton, &QPushButton::clicked, this, &MainWindow::sendCustomTxOnce);
    connect(m_txToggleButton, &QPushButton::clicked, this, &MainWindow::toggleCustomTx);
    connect(m_txTimer, &QTimer::timeout, this, &MainWindow::sendCustomTxOnce);
    connect(m_txResponseTimer, &QTimer::timeout, this, &MainWindow::handleCustomTxResponseTimeout);
    auto syncTxByteEditors = [this]() {
        const int dlc = m_txDlcSpin ? m_txDlcSpin->value() : 0;
        const bool remote = m_txRemoteCheck && m_txRemoteCheck->isChecked();
        for (int i = 0; i < m_txByteEdits.size(); ++i) {
            if (QLineEdit *edit = m_txByteEdits.at(i)) {
                edit->setEnabled(!remote && i < dlc);
            }
        }
    };
    connect(m_txDlcSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [syncTxByteEditors](int) { syncTxByteEditors(); });
    connect(m_txRemoteCheck, &QCheckBox::toggled, this, [syncTxByteEditors](bool) { syncTxByteEditors(); });
    syncTxByteEditors();

    connect(m_rescanNodesButton, &QPushButton::clicked,
            this, &MainWindow::scanForActiveNodes);
    connect(m_debugBoardCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int) {
        updateDebugNodeCombo();
        updateDebugScaleSpin();
        updateStatusPanel();
    });
    connect(m_debugNodeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int index) {
        if (m_updatingDebugNodeCombo || m_switchingDebugNode || index < 0 || m_debugNodeCombo->count() <= 0) {
            return;
        }
        const quint8 nodeId = m_debugNodeCombo->currentData().toUInt();
        switchToDetectedNode(nodeId);
    });
    connect(m_debugScaleSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, [this](double value) {
        const int boardIndex = currentDebugBoardIndex();
        if (boardIndex < 0 || boardIndex >= m_boardConfigRows.size()) {
            return;
        }
        if (currentDebugNodeId() == boardTurnNodeId(boardIndex) && m_boardConfigRows[boardIndex].turnScaleSpin) {
            m_boardConfigRows[boardIndex].turnScaleSpin->setValue(value);
        } else if (currentDebugNodeId() == boardDriveNodeId(boardIndex) && m_boardConfigRows[boardIndex].driveScaleSpin) {
            m_boardConfigRows[boardIndex].driveScaleSpin->setValue(value);
        }
        syncAxisReadouts();
        scheduleSaveSettings();
    });

    updateBoardRowVisibility();
    updateDebugBoardCombo();
    updateDebugNodeCombo();
    updateDebugScaleSpin();
}

void MainWindow::buildBoardConfigRows(QGridLayout *layout, QWidget *parent)
{
    constexpr int maxBoards = 4;
    m_boardConfigRows.resize(maxBoards);
    m_boardRuntimes.resize(maxBoards);

    layout->addWidget(new QLabel(zh("e69dbfe5ad90")), 1, 0);
    layout->addWidget(new QLabel(zh("e68ea5e58fa3")), 1, 1);
    layout->addWidget(new QLabel(zh("e88a82e782b9e689abe68f8f")), 1, 2);
    layout->addWidget(new QLabel(zh("e5b7a6e8bdaee88a82e782b9")), 1, 3);
    layout->addWidget(new QLabel(zh("e58fb3e8bdaee88a82e782b9")), 1, 4);
    layout->addWidget(new QLabel(zh("e5b7a6e8bdaee68da2e7ae97")), 1, 5);
    layout->addWidget(new QLabel(zh("e58fb3e8bdaee68da2e7ae97")), 1, 6);
    layout->addWidget(new QLabel(zh("e78ab6e68081")), 1, 7);

    // 板子名称映射：板子0=前轮，板子1=后轮
    const QStringList boardNames = {zh("e5898de8bdae"), zh("e5908ee8bdae"), zh("e69dbf33"), zh("e69dbf34")};
    
    for (int boardIndex = 0; boardIndex < maxBoards; ++boardIndex) {
        BoardConfigRow &row = m_boardConfigRows[boardIndex];
        const QString boardName = boardIndex < boardNames.size() ? boardNames[boardIndex] : zh("e69dbf2531").arg(boardIndex + 1);
        row.label = new QLabel(boardName, parent);
        row.interfaceCombo = new QComboBox(parent);
        row.interfaceCombo->setMinimumWidth(150);
        row.scanNodesButton = new QPushButton(zh("e689abe68f8f"), parent);
        row.turnNodeCombo = new QComboBox(parent);
        row.driveNodeCombo = new QComboBox(parent);
        row.turnScaleSpin = new QDoubleSpinBox(parent);
        row.turnScaleSpin->setDecimals(4);
        row.turnScaleSpin->setRange(0.0001, 100000.0);
        row.turnScaleSpin->setValue(1.0);
        row.turnScaleSpin->setSuffix(zh("206d6d2fe59c88"));
        row.driveScaleSpin = new QDoubleSpinBox(parent);
        row.driveScaleSpin->setDecimals(4);
        row.driveScaleSpin->setRange(0.0001, 100000.0);
        row.driveScaleSpin->setValue(1.0);
        row.driveScaleSpin->setSuffix(zh("206d6d2fe59c88"));
        row.statusLabel = new QLabel(QStringLiteral("--"), parent);
        row.statusLabel->setMinimumWidth(120);

        const int gridRow = boardIndex + 2;
        layout->addWidget(row.label, gridRow, 0);
        layout->addWidget(row.interfaceCombo, gridRow, 1);
        layout->addWidget(row.scanNodesButton, gridRow, 2);
        layout->addWidget(row.turnNodeCombo, gridRow, 3);
        layout->addWidget(row.driveNodeCombo, gridRow, 4);
        layout->addWidget(row.turnScaleSpin, gridRow, 5);
        layout->addWidget(row.driveScaleSpin, gridRow, 6);
        layout->addWidget(row.statusLabel, gridRow, 7);

        connect(row.interfaceCombo, &QComboBox::currentTextChanged, this, [this, boardIndex](const QString &) {
            m_boardConfigRows[boardIndex].preferredInterfaceName = m_boardConfigRows[boardIndex].interfaceCombo
                    ? m_boardConfigRows[boardIndex].interfaceCombo->currentText().trimmed()
                    : QString();
            m_boardConfigRows[boardIndex].detectedNodes.clear();
            updateBoardNodeCombos(boardIndex);
            updateBoardStatusLabel(boardIndex);
            updateDebugBoardCombo();
            syncConnectionEditorsFromAdvanced();
            scheduleSaveSettings();
        });
        connect(row.scanNodesButton, &QPushButton::clicked, this, [this, boardIndex]() {
            refreshBoardNodes(boardIndex, true);
        });
        connect(row.turnNodeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this, boardIndex](int) {
            m_boardConfigRows[boardIndex].preferredTurnNodeId = boardTurnNodeId(boardIndex);
            updateDebugScaleSpin();
            syncAxisReadouts();
            scheduleSaveSettings();
        });
        connect(row.driveNodeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this, boardIndex](int) {
            m_boardConfigRows[boardIndex].preferredDriveNodeId = boardDriveNodeId(boardIndex);
            updateDebugScaleSpin();
            syncAxisReadouts();
            scheduleSaveSettings();
        });
        connect(row.turnScaleSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](double) {
            updateDebugScaleSpin();
            syncAxisReadouts();
            scheduleSaveSettings();
        });
        connect(row.driveScaleSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](double) {
            updateDebugScaleSpin();
            syncAxisReadouts();
            scheduleSaveSettings();
        });
    }
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
    m_diagnoseFixButton = new QPushButton(zh("e8af8ae696ade4bfaee5a48d"), axisStateGroup);
    m_initializeMachineButton = new QPushButton(zh("e695b4e69cbae58897e5a78b"), axisStateGroup);

    axisStateLayout->addWidget(m_idleButton);
    axisStateLayout->addWidget(m_motorCalibrationButton);
    axisStateLayout->addWidget(m_fullCalibrationButton);
    axisStateLayout->addWidget(m_closedLoopButton);
    axisStateLayout->addWidget(m_estopButton);
    axisStateLayout->addWidget(m_clearErrorsButton);
    axisStateLayout->addWidget(m_refreshStatusButton);
    axisStateLayout->addWidget(m_diagnoseFixButton);
    axisStateLayout->addWidget(m_initializeMachineButton);

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

    QGroupBox *pidGroup = new QGroupBox(zh("504944e58f82e695b0e9858de7bdae"), parent);
    QGridLayout *pidLayout = new QGridLayout(pidGroup);
    m_posGainSpin = new QDoubleSpinBox(pidGroup);
    m_posGainSpin->setRange(1.0, 5.0);
    m_posGainSpin->setDecimals(3);
    m_posGainSpin->setValue(1.0);
    m_posGainSpin->setSingleStep(0.1);
    m_velGainSpin = new QDoubleSpinBox(pidGroup);
    m_velGainSpin->setRange(0.02, 0.1);
    m_velGainSpin->setDecimals(6);
    m_velGainSpin->setValue(0.02);
    m_velGainSpin->setSingleStep(0.001);
    m_velIntegratorGainSpin = new QDoubleSpinBox(pidGroup);
    m_velIntegratorGainSpin->setRange(0.0, 1.0);
    m_velIntegratorGainSpin->setDecimals(6);
    m_velIntegratorGainSpin->setValue(0.0);
    m_velIntegratorGainSpin->setSingleStep(0.001);
    m_readPidButton = new QPushButton(zh("e8afbbe58f96"), pidGroup);
    m_applyPidButton = new QPushButton(zh("e5ba94e794a8"), pidGroup);

    pidLayout->addWidget(new QLabel(zh("e4bd8de7bdaee5a29ee79b8a20283120207e203529")), 0, 0);
    pidLayout->addWidget(m_posGainSpin, 0, 1);
    pidLayout->addWidget(m_readPidButton, 0, 2);
    pidLayout->addWidget(new QLabel(zh("e9809fe5baa6e5a29ee79b8a2028302e30322020207e2020302e3129")), 1, 0);
    pidLayout->addWidget(m_velGainSpin, 1, 1);
    pidLayout->addWidget(m_applyPidButton, 1, 2);
    pidLayout->addWidget(new QLabel(zh("e9809fe5baa6e7a7afe58886e5a29ee79b8a20283029")), 2, 0);
    pidLayout->addWidget(m_velIntegratorGainSpin, 2, 1);

    leftColumn->addWidget(modeGroup);
    leftColumn->addWidget(positionGroup);
    leftColumn->addWidget(velocityGroup);
    leftColumn->addWidget(torqueGroup);
    leftColumn->addWidget(limitGroup);
    leftColumn->addWidget(pidGroup);
    leftColumn->addStretch(1);

    QGroupBox *statusGroup = new QGroupBox(zh("e78ab6e68081e79b91e68ea7"), parent);
    QGridLayout *statusLayout = new QGridLayout(statusGroup);

    int row = 0;
    statusLayout->addWidget(new QLabel(zh("e5bf83e8b7b3e99499e8afaf")), row, 0);
    m_axisErrorValue = createValueLabel();
    statusLayout->addWidget(m_axisErrorValue, row++, 1);
    statusLayout->addWidget(new QLabel(QStringLiteral("Axis error detail")), row, 0);
    m_axisErrorDetailValue = createValueLabel();
    m_axisErrorDetailValue->setWordWrap(true);
    statusLayout->addWidget(m_axisErrorDetailValue, row++, 1);
    statusLayout->addWidget(new QLabel(zh("e5bd93e5898de99499e8afaf")), row, 0);
    m_activeErrorsValue = createValueLabel();
    statusLayout->addWidget(m_activeErrorsValue, row++, 1);
    statusLayout->addWidget(new QLabel(QStringLiteral("ODrive error detail")), row, 0);
    m_activeErrorsDetailValue = createValueLabel();
    m_activeErrorsDetailValue->setWordWrap(true);
    statusLayout->addWidget(m_activeErrorsDetailValue, row++, 1);
    statusLayout->addWidget(new QLabel(zh("e5a4b1e883bde58e9fe59ba0")), row, 0);
    m_disarmReasonValue = createValueLabel();
    statusLayout->addWidget(m_disarmReasonValue, row++, 1);
    statusLayout->addWidget(new QLabel(QStringLiteral("Disarm detail")), row, 0);
    m_disarmReasonDetailValue = createValueLabel();
    m_disarmReasonDetailValue->setWordWrap(true);
    statusLayout->addWidget(m_disarmReasonDetailValue, row++, 1);
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
    connect(m_readPidButton, &QPushButton::clicked,
            this, &MainWindow::readPidParams);
    connect(m_applyPidButton, &QPushButton::clicked,
            this, &MainWindow::applyPidParams);
    connect(m_refreshStatusButton, &QPushButton::clicked,
            this, &MainWindow::refreshTelemetry);
    connect(m_estopButton, &QPushButton::clicked, this, [this]() {
        if (ODriveMotorController *controller = currentDebugController()) {
            controller->estop(currentDebugNodeId());
        }
    });
    connect(m_clearErrorsButton, &QPushButton::clicked, this, [this]() {
        if (ODriveMotorController *controller = currentDebugController()) {
            controller->clearErrors(currentDebugNodeId(), false);
        }
    });
    connect(m_diagnoseFixButton, &QPushButton::clicked,
            this, &MainWindow::runDiagnoseFix);
    connect(m_initializeMachineButton, &QPushButton::clicked,
            this, &MainWindow::initializeMachine);
}

void MainWindow::runDiagnoseFix()
{
    ODriveMotorController *controller = currentDebugController();
    if (!controller || !controller->isConnected()) {
        appendLog(zh("e8af8ae696ade4bfaee5a48de5a4b1e8b4a5efbc9ae69caae8bf9ee68ea5"));
        qInfo() << "Diagnose failed: not connected";
        return;
    }

    const int boardIndex = currentDebugBoardIndex();
    const quint8 nodeId = currentDebugNodeId();
    QPointer<ODriveMotorController> controllerGuard(controller);
    controller->requestAllTelemetry(nodeId, false);

    QTimer::singleShot(250, this, [this, controllerGuard, boardIndex, nodeId]() {
        if (!controllerGuard || !controllerGuard->isConnected()) {
            appendLog(QStringLiteral("%1 Node %2 diagnose cancelled: controller no longer available")
                      .arg(boardDisplayName(boardIndex))
                      .arg(nodeId));
            return;
        }

        ODriveMotorController *controller = controllerGuard.data();
        const ODriveMotorController::AxisStatus &status = controller->status(nodeId);

        auto hex32 = [](quint32 value) {
            return QStringLiteral("0x%1").arg(value, 8, 16, QLatin1Char('0')).toUpper();
        };

        const QString severeTag = zh("e38090e4b8a5e9878de38091");
        const QString warningTag = zh("e38090e8ada6e5918ae38091");
        const QString okTag = zh("e38090e6ada3e5b8b8e38091");

        auto severityPrefix = [&](const QString &line) -> QString {
            if (line.startsWith(severeTag)) {
                return QStringLiteral("critical");
            }
            if (line.startsWith(warningTag)) {
                return QStringLiteral("warning");
            }
            return QStringLiteral("info");
        };

        QStringList report;
        QStringList findings;
        QStringList recommendations;
        int criticalCount = 0;
        int warningCount = 0;

        auto addFinding = [&](const QString &line) {
            findings << line;
            const QString severity = severityPrefix(line);
            if (severity == QStringLiteral("critical")) {
                ++criticalCount;
            } else if (severity == QStringLiteral("warning")) {
                ++warningCount;
            }
        };

        auto addRecommendation = [&](const QString &line) {
            if (!recommendations.contains(line)) {
                recommendations << line;
            }
        };

        report << QStringLiteral("========== %1 ==========").arg(zh("e8af8ae696ade68aa5e5918a"));
        report << QStringLiteral("%1: %2").arg(zh("e69dbfe5ad90"), boardDisplayName(boardIndex));
        report << QStringLiteral("%1: %2").arg(zh("e68ea5e58fa3"), boardInterfaceName(boardIndex));
        report << QStringLiteral("%1: %2").arg(zh("e88a82e782b9"), nodeId);
        report << QStringLiteral("%1: %2")
                     .arg(zh("e697b6e997b4"),
                          QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz")));
        report << QString();

        report << QStringLiteral("[1] %1").arg(zh("e59fbae7a180e78ab6e68081"));
        report << QStringLiteral("Axis State: %1 (%2)")
                     .arg(ODriveMotorController::axisStateName(status.axisState))
                     .arg(status.axisState);
        report << QStringLiteral("Procedure Result: %1 (%2)")
                     .arg(ODriveMotorController::procedureResultName(status.procedureResult))
                     .arg(status.procedureResult);
        report << QStringLiteral("Trajectory Done: %1")
                     .arg(status.trajectoryDone ? QStringLiteral("true") : QStringLiteral("false"));
        report << QStringLiteral("Last Heartbeat: %1")
                     .arg(status.lastHeartbeat.isValid()
                              ? status.lastHeartbeat.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz"))
                              : QStringLiteral("--"));
        report << QStringLiteral("Last Message: %1")
                     .arg(status.lastMessage.isValid()
                              ? status.lastMessage.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz"))
                              : QStringLiteral("--"));
        report << QString();

        report << QStringLiteral("[2] %1").arg(zh("e99499e8afafe4b88ee4bf9de68aa4"));
        report << QStringLiteral("%1: %2 -> %3").arg(zh("e8bdb4e99499e8afaf"), hex32(status.axisError), describeAxisError(status.axisError));
        report << QStringLiteral("%1: %2 -> %3").arg(zh("4f4472697665e7b3bbe7bb9fe99499e8afaf"), hex32(status.activeErrors), describeOdriveError(status.activeErrors));
        report << QStringLiteral("%1: %2 -> %3").arg(QStringLiteral("Disarm Reason"), hex32(status.disarmReason), describeDisarmReason(status.disarmReason));
        report << QStringLiteral("%1: %2 -> %3").arg(zh("e794b5e69cbae99499e8afaf"), hex32(status.motorError), describeMotorError(status.motorError));
        report << QStringLiteral("%1: %2 -> %3").arg(zh("e7bc96e7a081e599a8e99499e8afaf"), hex32(status.encoderError), describeEncoderError(status.encoderError));
        report << QStringLiteral("%1: %2 -> %3").arg(zh("e68ea7e588b6e599a8e99499e8afaf"), hex32(status.controllerError), describeControllerError(status.controllerError));
        report << QStringLiteral("%1: %2 -> %3").arg(zh("e697a0e6849fe4bcb0e7ae97e599a8e99499e8afaf"), hex32(status.sensorlessError), describeSensorlessError(status.sensorlessError));
        report << QString();

        report << QStringLiteral("[3] %1").arg(zh("e5ae9ee697b6e58f8de9a688"));
        report << QStringLiteral("Position: %1 turns / %2 mm")
                     .arg(QString::number(status.positionTurns, 'f', 4),
                          QString::number(turnsToMm(status.positionTurns, nodeId == boardTurnNodeId(boardIndex)), 'f', 3));
        report << QStringLiteral("Velocity: %1 turns/s").arg(QString::number(status.velocityTurnsPerSecond, 'f', 4));
        report << QStringLiteral("Iq Setpoint: %1 A").arg(QString::number(status.iqSetpointAmps, 'f', 4));
        report << QStringLiteral("Iq Measured: %1 A").arg(QString::number(status.iqMeasuredAmps, 'f', 4));
        report << QStringLiteral("Bus Voltage: %1 V").arg(QString::number(status.busVoltageVolts, 'f', 4));
        report << QStringLiteral("Bus Current: %1 A").arg(QString::number(status.busCurrentAmps, 'f', 4));
        report << QStringLiteral("Torque Target: %1 Nm").arg(QString::number(status.torqueTargetNm, 'f', 4));
        report << QStringLiteral("Torque Estimate: %1 Nm").arg(QString::number(status.torqueEstimateNm, 'f', 4));
        report << QStringLiteral("Electrical Power: %1 W").arg(QString::number(status.electricalPowerWatts, 'f', 3));
        report << QStringLiteral("Mechanical Power: %1 W").arg(QString::number(status.mechanicalPowerWatts, 'f', 3));
        report << QStringLiteral("FET Temperature: %1 C").arg(QString::number(status.fetTemperatureCelsius, 'f', 3));
        report << QStringLiteral("Motor Temperature: %1 C").arg(QString::number(status.motorTemperatureCelsius, 'f', 3));
        report << QString();

        report << QStringLiteral("[4] %1").arg(zh("e7bc96e7a081e599a82fe4bcb0e7ae97e599a8"));
        report << QStringLiteral("Encoder Shadow Count: %1").arg(status.encoderShadowCount);
        report << QStringLiteral("Encoder Count In CPR: %1").arg(status.encoderCountInCPR);
        report << QStringLiteral("Sensorless Pos Estimate: %1").arg(QString::number(status.sensorlessPosEstimate, 'f', 4));
        report << QStringLiteral("Sensorless Vel Estimate: %1").arg(QString::number(status.sensorlessVelEstimate, 'f', 4));
        report << QString();

        report << QStringLiteral("[5] %1").arg(zh("e5bd93e5898de68ea7e588b6e9858de7bdae"));
        report << QStringLiteral("Control Mode: %1 (%2)")
                     .arg(describeControlModeName(status.currentControlMode))
                     .arg(status.currentControlMode);
        report << QStringLiteral("Input Mode: %1 (%2)")
                     .arg(describeInputModeName(status.currentInputMode))
                     .arg(status.currentInputMode);
        report << QStringLiteral("Velocity Limit: %1 turns/s").arg(QString::number(status.currentVelocityLimit, 'f', 4));
        report << QStringLiteral("Current Limit: %1 A").arg(QString::number(status.currentCurrentLimit, 'f', 4));
        report << QStringLiteral("Traj Vel Limit: %1").arg(QString::number(status.trajVelLimit, 'f', 4));
        report << QStringLiteral("Traj Accel Limit: %1").arg(QString::number(status.trajAccelLimit, 'f', 4));
        report << QStringLiteral("Traj Decel Limit: %1").arg(QString::number(status.trajDecelLimit, 'f', 4));
        report << QStringLiteral("Traj Inertia: %1").arg(QString::number(status.trajInertia, 'f', 4));
        report << QStringLiteral("Pos Gain: %1").arg(QString::number(status.posGain, 'f', 6));
        report << QStringLiteral("Vel Gain: %1").arg(QString::number(status.velGain, 'f', 6));
        report << QStringLiteral("Vel Integrator Gain: %1").arg(QString::number(status.velIntegratorGain, 'f', 6));
        report << QString();

        report << QStringLiteral("[6] %1").arg(zh("e8af8ae696ade7bb93e8aeba"));

        if (status.axisError == 0 && status.activeErrors == 0 && status.motorError == 0
                && status.encoderError == 0 && status.controllerError == 0 && status.sensorlessError == 0) {
            addFinding(okTag + zh("e697a0e6b4bbe58aa8e69585e99a9ce4bd8d"));
        } else {
            if (status.axisError != 0) {
                addFinding(severeTag + zh("e8bdb4e99499e8afafefbc9a2531").arg(describeAxisError(status.axisError)));
                addRecommendation(zh("e58588e6b885e99499efbc8ce5868de7a1aee8aea42072657175657374656420737461746520e4b88ee5bd93e5898de8bdb4e78ab6e68081e698afe590a6e4b880e887b4"));
            }
            if (status.activeErrors != 0) {
                addFinding(severeTag + zh("4f4472697665e7b3bbe7bb9fe99499e8afafefbc9a2531").arg(describeOdriveError(status.activeErrors)));
                addRecommendation(zh("e6a380e69fa5e6af8de7babfe4be9be794b5e38081e588b6e58aa8e794b5e998bbe38081e680bbe7babfe7a8b3e5ae9ae680a7"));
            }
            if (status.motorError != 0) {
                addFinding(severeTag + zh("e794b5e69cbae99499e8afafefbc9a2531").arg(describeMotorError(status.motorError)));
                addRecommendation(zh("e6a380e69fa5e794b5e69cbae58f82e695b0e38081e79bb8e7babfe8bf9ee68ea5e38081e4be9be794b5e4b88ee6b8a9e5baa6"));
            }
            if (status.encoderError != 0) {
                addFinding(severeTag + zh("e7bc96e7a081e599a8e99499e8afafefbc9a2531").arg(describeEncoderError(status.encoderError)));
                addRecommendation(zh("e6a380e69fa5e7bc96e7a081e599a8e8bf9ee7babfe38081435052e38081e69e81e5afb9e695b0e4b88ee6a0a1e58786e78ab6e68081"));
            }
            if (status.controllerError != 0) {
                addFinding(severeTag + zh("e68ea7e588b6e599a8e99499e8afafefbc9a2531").arg(describeControllerError(status.controllerError)));
                addRecommendation(zh("e6a380e69fa5e68ea7e588b6e6a8a1e5bc8fe38081e8be93e585a5e6a8a1e5bc8fe38081e9809fe5baa6e99990e588b6e38081e794b5e6b581e99990e588b6e4b88ee58f8de9a688e696b9e59091"));
            }
            if (status.sensorlessError != 0) {
                addFinding(warningTag + zh("e697a0e6849fe4bcb0e7ae97e599a8e99499e8afafefbc9a2531").arg(describeSensorlessError(status.sensorlessError)));
            }
        }

        if (status.axisState == ODriveMotorController::ClosedLoopControl) {
            addFinding(okTag + zh("e5bd93e5898de88a82e782b9e5b7b2e8bf9be585a5e997ade78eafe68ea7e588b6"));
        } else {
            addFinding(warningTag + zh("e5bd93e5898de88a82e782b9e69caae59ca8e997ade78eafe68ea7e588b6efbc8ce78ab6e68081e4b8ba202531")
                           .arg(ODriveMotorController::axisStateName(status.axisState)));
            addRecommendation(zh("e88ba5e99c80e8a681e8bf90e8a18ce794b5e69cbaefbc8ce8afb7e7a1aee8aea4e6a0a1e58786e5ae8ce68890e5908ee9878de696b0e8bf9be585a5e997ade78eaf"));
        }

        if (!status.lastHeartbeat.isValid()) {
            addFinding(warningTag + zh("e5b09ae69caae694b6e588b0e69c89e69588e5bf83e8b7b3efbc8ce9809ae4bfa1e58fafe883bde4b88de7a8b3e5ae9ae68896e88a82e782b9e69caae5938de5ba94"));
            addRecommendation(zh("e7a1aee8aea42043414e20e9809ae4bfa1e38081e88a82e782b920494420e4b88ee5b883e7babf"));
        }

        if (status.busVoltageVolts < 10.0f) {
            addFinding(severeTag + zh("e6af8de7babfe794b5e58e8be8bf87e4bd8eefbc8ce5ad98e59ca8e6aca0e58e8be9a38ee999a9"));
            addRecommendation(zh("e6a380e69fa5e794b5e6ba90e38081e794b5e6b1a0e38081e794b5e6ba90e7babfe58e8be9998de4b88ee68ea5e68f92e4bbb6"));
        } else if (status.busVoltageVolts < 20.0f) {
            addFinding(warningTag + zh("e6af8de7babfe794b5e58e8be5818fe4bd8eefbc8ce5bbbae8aeaee7a1aee8aea4e4be9be794b5e7a8b3e5ae9ae680a7"));
        } else {
            addFinding(okTag + zh("e6af8de7babfe794b5e58e8be5a484e4ba8ee58fafe794a8e88c83e59bb4"));
        }

        if (status.fetTemperatureCelsius > 85.0f || status.motorTemperatureCelsius > 85.0f) {
            addFinding(severeTag + zh("e6b8a9e5baa6e8bf87e9ab98efbc8ce5ad98e59ca8e8bf87e6b8a9e4bf9de68aa4e9a38ee999a9"));
            addRecommendation(zh("e9998de4bd8ee794b5e6b5812fe8b49fe8bdbdefbc8ce6a380e69fa5e695a3e783ade4b88ee69cbae6a2b0e998bbe58a9b"));
        } else if (status.fetTemperatureCelsius > 70.0f || status.motorTemperatureCelsius > 70.0f) {
            addFinding(warningTag + zh("e6b8a9e5baa6e5818fe9ab98efbc8ce5bbbae8aeaee585b3e6b3a8e995bfe697b6e997b4e8bf90e8a18ce7a8b3e5ae9ae680a7"));
        }

        if (status.currentControlMode == ODriveMotorController::VelocityControl
                && status.currentInputMode != ODriveMotorController::Passthrough) {
            addFinding(warningTag + zh("e5bd93e5898de4b8bae9809fe5baa6e68ea7e588b6efbc8ce4bd86e8be93e585a5e6a8a1e5bc8fe4b88de698afe79bb4e9809ae6a8a1e5bc8f"));
        }

        if (status.currentControlMode == ODriveMotorController::TorqueControl
                && qFuzzyIsNull(status.velocityTurnsPerSecond) && !qFuzzyIsNull(status.torqueTargetNm)
                && status.controllerError != 0) {
            addFinding(warningTag + zh("e79baee6a087e58a9be79fa9e99d9ee99bb6e4bd86e9809fe5baa6e4b8bae99bb6e4b894e68ea7e588b6e599a8e68aa5e99499efbc8ce99c80e68e92e69fa5e997ade78eafe78ab6e680812fe99990e6b5812fe6aca0e58e8b"));
        }

        if (status.currentVelocityLimit <= 0.0f) {
            addFinding(severeTag + zh("e9809fe5baa6e99990e588b6e4b8ba30efbc8ce5bd93e5898de88a82e782b9e697a0e6b395e6ada3e5b8b8e8bf90e8a18c"));
            addRecommendation(zh("e8aebee7bdaee59088e79086e79a842076656c6f63697479206c696d697420e5908ee5868de6b58be8af95"));
        }

        if (status.currentCurrentLimit <= 0.0f) {
            addFinding(severeTag + zh("e794b5e6b581e99990e588b6e4b8ba30efbc8ce5bd93e5898de88a82e782b9e697a0e6b395e8be93e587bae69c89e69588e9a9b1e58aa8e58a9b"));
            addRecommendation(zh("e8aebee7bdaee59088e79086e79a842063757272656e74206c696d6974"));
        }

        if (nodeId == 0) {
            report << QStringLiteral("[6.1] %1").arg(zh("e88a82e782b930e4b893e9a1b9e6a380e69fa5"));

            if (status.currentVelocityLimit > 8.0f) {
                addFinding(warningTag + zh("e88a82e782b9302076656c6f63697479206c696d697420e5bd93e5898de4b8ba202531207475726e732f73efbc8ce5818fe9ab98efbc8ce5aeb9e69893e587bae78eb0e9809fe5baa6e8bf87e5bfabe68896e99c87e58aa8")
                               .arg(QString::number(status.currentVelocityLimit, 'f', 4)));
                addRecommendation(zh("e88a82e782b930e5bbbae8aeaee58588e9998de588b0e4bf9de5ae88e9809fe5baa6e99990e588b6e5908ee5868de9aa8ce8af81"));
            } else if (status.currentVelocityLimit > 0.0f) {
                addFinding(okTag + zh("e88a82e782b9302076656c6f63697479206c696d697420e5a484e4ba8ee79bb8e5afb9e4bf9de5ae88e88c83e59bb4"));
            }

            if (status.velGain > 0.03f) {
                addFinding(warningTag + zh("e88a82e782b9302076656c5f6761696e3d253120e5818fe9ab98efbc8ce58fafe883bde5bc95e58f91e68cafe88da1")
                               .arg(QString::number(status.velGain, 'f', 6)));
            } else if (status.velGain > 0.0f && status.velGain < 0.005f) {
                addFinding(warningTag + zh("e88a82e782b9302076656c5f6761696e3d253120e5818fe4bd8eefbc8ce58fafe883bde5afbce887b4e5938de5ba94e58f91e8bdaf")
                               .arg(QString::number(status.velGain, 'f', 6)));
            } else if (status.velGain > 0.0f) {
                addFinding(okTag + zh("e88a82e782b9302076656c5f6761696e20e5a484e4ba8ee58fafe68ea5e58f97e88c83e59bb4"));
            }

            if (status.velIntegratorGain > 0.2f) {
                addFinding(warningTag + zh("e88a82e782b9302076656c5f696e7465677261746f725f6761696e3d253120e5818fe9ab98efbc8ce58fafe883bde58aa0e589a7e99c87e88da1")
                               .arg(QString::number(status.velIntegratorGain, 'f', 6)));
            } else if (status.velIntegratorGain >= 0.0f && status.velIntegratorGain < 0.02f) {
                addFinding(warningTag + zh("e88a82e782b9302076656c5f696e7465677261746f725f6761696e3d253120e5818fe4bd8eefbc8ce58fafe883bde5afbce887b4e7a8b3e68081e8afafe5b7ae")
                               .arg(QString::number(status.velIntegratorGain, 'f', 6)));
            }

            if (status.encoderError == 0 && status.encoderCountInCPR == 0 && status.axisState != ODriveMotorController::Idle) {
                addFinding(warningTag + zh("e88a82e782b930e7bc96e7a081e599a820436f756e7420496e2043505220e4b8ba30efbc8ce99c80e7a1aee8aea4e7bc96e7a081e599a8e58f8de9a688e698afe590a6e79c9fe5ae9ee69bb4e696b0"));
                addRecommendation(zh("e6a380e69fa5e88a82e782b930e7bc96e7a081e599a8e4be9be794b5e38081e4bfa1e58fb7e7babfe58f8a435052e9858de7bdae"));
            }

            if (status.axisError & 0x00000001u) {
                addFinding(severeTag + zh("e88a82e782b930e587bae78eb020494e56414c49445f5354415445efbc8ce5b8b8e8a781e58e9fe59ba0e698afe69caae6a0a1e58786e38081e7bc96e7a081e599a8e69caae58786e5a487e5a5bde68896e997ade78eafe5898de69c89e99481e5ad98e99499e8afaf"));
                addRecommendation(zh("e88a82e782b930e9878de782b9e6a380e69fa5e6a0a1e58786e78ab6e68081e38081e7bc96e7a081e599a8e78ab6e68081e5928ce6b885e99499e9a1bae5ba8f"));
            }

            if (status.axisError & 0x00000200u || status.controllerError != 0) {
                addFinding(severeTag + zh("e88a82e782b930e5ad98e59ca8e68ea7e588b6e599a8e79bb8e585b3e5bc82e5b8b8efbc8ce99c80e9878de782b9e6a0b8e5afb9e6a8a1e5bc8fe38081e99990e580bce38081e58f8de9a688e696b9e59091e5928ce5a29ee79b8a"));
            }

            if (qAbs(status.velocityTurnsPerSecond) > status.currentVelocityLimit * 1.2f
                    && status.currentVelocityLimit > 0.0f) {
                addFinding(severeTag + zh("e88a82e782b930e5ae9ee99985e9809fe5baa6e8b685e8bf87e5bd93e5898d2076656c6f63697479206c696d6974efbc8ce99c80e68e92e69fa5e58f82e695b0e38081e58f8de9a688e696b9e59091e68896e68ea7e588b6e5bc82e5b8b8"));
                addRecommendation(zh("e6a380e69fa5e9809fe5baa6e58f8de9a688e696b9e59091e38081e7bc96e7a081e599a8e6a087e5ae9ae4b88e206f766572737065656420e59cbae699af"));
            }

            if (!qFuzzyIsNull(status.torqueTargetNm) && qFuzzyIsNull(status.torqueEstimateNm)
                    && status.axisState == ODriveMotorController::ClosedLoopControl) {
                addFinding(warningTag + zh("e88a82e782b930e5ad98e59ca8e79baee6a087e58a9be79fa9e4bd86e4bcb0e7ae97e58a9be79fa9e68ea5e8bf9130efbc8ce99c80e7a1aee8aea4e9a9b1e58aa8e8be93e587bae4b88ee69cbae6a2b0e8b49fe8bdbd"));
            }

            report << zh("e88a82e782b930e9878de782b9e585b3e6b3a8efbc9a43414e204944202f20e7bc96e7a081e599a8e58f8de9a688202f20e997ade78eafe78ab6e68081202f2076656c6f63697479206c696d6974202f2076656c5f6761696e202f2076656c5f696e7465677261746f725f6761696e202f20e99c87e58aa8e9a38ee999a9");
            report << QString();
        }

        if (findings.isEmpty()) {
            addFinding(okTag + zh("e697a0e5bc82e5b8b8e7bb93e8aeba"));
        }

        report << QStringLiteral("%1: %2").arg(zh("e4b8a5e9878de997aee9a298e695b0"), criticalCount);
        report << QStringLiteral("%1: %2").arg(zh("e8ada6e5918ae695b0"), warningCount);
        report << findings;
        report << QString();
        report << QStringLiteral("[7] %1").arg(zh("e5bbbae8aeaee58aa8e4bd9c"));

        addRecommendation(zh("e58588e588b7e696b0e78ab6e68081efbc8ce7a1aee8aea4e99499e8afafe698afe590a6e68c81e7bbade5ad98e59ca8"));
        if (criticalCount > 0 || warningCount > 0) {
            addRecommendation(zh("e88ba5e5ad98e59ca8e99499e8afafefbc8ce58588e6b885e99499e5908ee9878de696b0e8afbbe58f96"));
            addRecommendation(zh("e88ba5e69caae8bf9be997ade78eafefbc8ce7a1aee8aea4e5b7b2e6a0a1e58786e5908ee5868de5b09de8af95e8bf9be585a5e997ade78eaf"));
            addRecommendation(zh("e88ba5e4b8bae7bc96e7a081e599a82fe68ea7e588b6e599a8e99499e8afafefbc8ce4bc98e58588e6a380e69fa520435052e38081e68ea7e588b6e6a8a1e5bc8fe38081e8be93e585a5e6a8a1e5bc8fe38081e9809fe5baa62fe794b5e6b581e99990e588b6"));
            addRecommendation(zh("e88ba5e4b8bae6aca0e58e8be68896e68e89e997ade78eafefbc8ce4bc98e58588e6a380e69fa5e4be9be794b5e38081e7babfe7bc86e5928ce680bbe7babfe7a8b3e5ae9ae680a7"));
        }

        for (int i = 0; i < recommendations.size(); ++i) {
            report << QStringLiteral("%1. %2").arg(i + 1).arg(recommendations.at(i));
        }

        const QString reportText = report.join(QStringLiteral("\n"));
        appendLog(QStringLiteral("[%1 Node %2] %3")
                  .arg(boardDisplayName(boardIndex))
                  .arg(nodeId)
                  .arg(zh("e5b7b2e7949fe68890e8af8ae696ade68aa5e5918a")));
        appendLog(reportText);
        
        // 自动修复逻辑
        if (criticalCount > 0 || warningCount > 0) {
            appendLog(QStringLiteral("========== %1 ==========").arg(zh("e5bc80e5a78be887aae58aa8e4bfaee5a48d")));
            
            QTimer::singleShot(500, this, [this, controllerGuard, nodeId, status, criticalCount, warningCount]() {
                if (!controllerGuard || !controllerGuard->isConnected()) {
                    appendLog(QStringLiteral("Diagnose auto-fix cancelled for node %1: controller unavailable").arg(nodeId));
                    return;
                }

                ODriveMotorController *controller = controllerGuard.data();

                // 步骤1：清除错误
                if (status.axisError != 0 || status.activeErrors != 0 || status.motorError != 0
                    || status.encoderError != 0 || status.controllerError != 0) {
                    appendLog(zh("e6ada5e9aaa431efbc9ae6b885e999a4e99499e8afaf"));
                    controller->clearErrors(nodeId, false);
                }

                QTimer::singleShot(800, this, [this, controllerGuard, nodeId, status, criticalCount, warningCount]() {
                    if (!controllerGuard || !controllerGuard->isConnected()) {
                        appendLog(QStringLiteral("Diagnose auto-fix cancelled for node %1: controller unavailable").arg(nodeId));
                        return;
                    }

                    ODriveMotorController *controller = controllerGuard.data();

                    // 步骤2：检查并修复速度限制
                    bool needsLimitFix = false;
                    float safeVelLimit = status.currentVelocityLimit;
                    float safeCurrentLimit = status.currentCurrentLimit;
                    
                    if (status.currentVelocityLimit <= 0.0f || status.currentVelocityLimit > 20.0f) {
                        appendLog(zh("e6ada5e9aaa432efbc9ae8aebee7bdaee9809fe5baa6e99990e588b6"));
                        safeVelLimit = (nodeId == 0) ? 5.0f : 20.0f;
                        needsLimitFix = true;
                    }
                    
                    // 步骤3：检查并修复电流限制
                    if (status.currentCurrentLimit <= 0.0f) {
                        appendLog(zh("e6ada5e9aaa433efbc9ae8aebee7bdaee794b5e6b581e99990e588b6"));
                        safeCurrentLimit = 10.0f;
                        needsLimitFix = true;
                    }
                    
                    if (needsLimitFix) {
                        controller->setLimits(nodeId, safeVelLimit, safeCurrentLimit);
                    }
                    
                    QTimer::singleShot(800, this, [this, controllerGuard, nodeId, status, criticalCount, warningCount]() {
                        if (!controllerGuard || !controllerGuard->isConnected()) {
                            appendLog(QStringLiteral("Diagnose auto-fix cancelled for node %1: controller unavailable").arg(nodeId));
                            return;
                        }

                        ODriveMotorController *controller = controllerGuard.data();

                        // 重新读取状态
                        controller->requestAllTelemetry(nodeId, false);

                        QTimer::singleShot(500, this, [this, controllerGuard, nodeId, status, criticalCount, warningCount]() {
                            if (!controllerGuard || !controllerGuard->isConnected()) {
                                appendLog(QStringLiteral("Diagnose auto-fix cancelled for node %1: controller unavailable").arg(nodeId));
                                return;
                            }

                            ODriveMotorController *controller = controllerGuard.data();
                            const ODriveMotorController::AxisStatus &currentStatus = controller->status(nodeId);
                            
                            // 步骤4：检查是否需要校准
                            if (status.axisError & 0x00000001u) {
                                // INVALID_STATE错误，可能需要重新校准
                                appendLog(zh("e6ada5e9aaa434efbc9ae6a380e6b58be588b0494e56414c49445f5354415445e99499e8afafefbc8ce5b09de8af95e58588e8bf9be585a5e7a9bae997b2"));
                                controller->requestIdle(nodeId);
                                
                                QTimer::singleShot(1000, this, [this, controllerGuard, nodeId]() {
                                    if (!controllerGuard || !controllerGuard->isConnected()) {
                                        appendLog(QStringLiteral("Diagnose auto-fix cancelled for node %1: controller unavailable").arg(nodeId));
                                        return;
                                    }

                                    ODriveMotorController *controller = controllerGuard.data();
                                    controller->clearErrors(nodeId, false);

                                    QTimer::singleShot(800, this, [this, controllerGuard, nodeId]() {
                                        if (!controllerGuard || !controllerGuard->isConnected()) {
                                            appendLog(QStringLiteral("Diagnose auto-fix cancelled for node %1: controller unavailable").arg(nodeId));
                                            return;
                                        }

                                        ODriveMotorController *controller = controllerGuard.data();

                                        // 检查是否已校准
                                        controller->requestAllTelemetry(nodeId, false);

                                        QTimer::singleShot(500, this, [this, controllerGuard, nodeId]() {
                                            if (!controllerGuard || !controllerGuard->isConnected()) {
                                                appendLog(QStringLiteral("Diagnose auto-fix cancelled for node %1: controller unavailable").arg(nodeId));
                                                return;
                                            }

                                            ODriveMotorController *controller = controllerGuard.data();
                                            const ODriveMotorController::AxisStatus &checkStatus = controller->status(nodeId);
                                            
                                            // 如果编码器未就绪或电机未校准，需要完整校准
                                            if (checkStatus.encoderError != 0 || checkStatus.motorError != 0) {
                                                appendLog(zh("e6ada5e9aaa435efbc9ae6a380e6b58be588b0e7bc96e7a081e599a8e68896e794b5e69cbae99499e8afafefbc8ce68980e4bba5e99c80e8a681e5ae8ce695b4e6a0a1e58786"));
                                                appendLog(zh("e8afb7e6898be58aa8e8bf90e8a18ce3808ae695b4e69cbae5889de5a78be58c96e3808be68c89e992ae"));
                                                appendLog(QStringLiteral("========== %1 ==========").arg(zh("e4bfaee5a48de5ae8ce68890")));
                                                updateStatusPanel();
                                                return;
                                            }
                                            
                                            appendLog(zh("e6ada5e9aaa435efbc9ae5b09de8af95e8bf9be585a5e997ade78eaf"));
                                            controller->requestClosedLoop(nodeId);
                                            
                                            QTimer::singleShot(1500, this, [this, controllerGuard, nodeId]() {
                                                if (!controllerGuard || !controllerGuard->isConnected()) {
                                                    appendLog(QStringLiteral("Diagnose auto-fix cancelled for node %1: controller unavailable").arg(nodeId));
                                                    return;
                                                }

                                                ODriveMotorController *controller = controllerGuard.data();
                                                controller->requestAllTelemetry(nodeId, false);
                                                QTimer::singleShot(300, this, [this, controllerGuard, nodeId]() {
                                                    if (!controllerGuard || !controllerGuard->isConnected()) {
                                                        appendLog(QStringLiteral("Diagnose auto-fix cancelled for node %1: controller unavailable").arg(nodeId));
                                                        return;
                                                    }

                                                    ODriveMotorController *controller = controllerGuard.data();
                                                    const ODriveMotorController::AxisStatus &finalStatus = controller->status(nodeId);
                                                    if (finalStatus.axisError == 0 && finalStatus.axisState == 8) {
                                                        appendLog(zh("e4bfaee5a48de68890e58a9fefbc8ce794b5e69cbae5b7b2e8bf9be585a5e997ade78eaf"));
                                                    } else {
                                                        appendLog(zh("e4bfaee5a48de983a8e58886e68890e58a9fefbc8ce4bd86e4bb8de69c89e5bc82e5b8b8efbc8ce8afb7e6898be58aa8e8bf90e8a18ce3808ae695b4e69cbae5889de5a78be58c96e3808be68c89e992ae"));
                                                    }
                                                    appendLog(QStringLiteral("========== %1 ==========").arg(zh("e4bfaee5a48de5ae8ce68890")));
                                                    updateStatusPanel();
                                                });
                                            });
                                        });
                                    });
                                });
                            } else if (currentStatus.axisState != ODriveMotorController::ClosedLoopControl) {
                                // 步骤5：如果不在闭环状态，尝试进入闭环
                                appendLog(zh("e6ada5e9aaa435efbc9ae5b09de8af95e8bf9be585a5e997ade78eaf"));
                                controller->requestClosedLoop(nodeId);
                                
                                QTimer::singleShot(1500, this, [this, controllerGuard, nodeId, criticalCount]() {
                                    if (!controllerGuard || !controllerGuard->isConnected()) {
                                        appendLog(QStringLiteral("Diagnose auto-fix cancelled for node %1: controller unavailable").arg(nodeId));
                                        return;
                                    }

                                    ODriveMotorController *controller = controllerGuard.data();
                                    controller->requestAllTelemetry(nodeId, false);
                                    QTimer::singleShot(300, this, [this, controllerGuard, nodeId, criticalCount]() {
                                        if (!controllerGuard || !controllerGuard->isConnected()) {
                                            appendLog(QStringLiteral("Diagnose auto-fix cancelled for node %1: controller unavailable").arg(nodeId));
                                            return;
                                        }

                                        ODriveMotorController *controller = controllerGuard.data();
                                        const ODriveMotorController::AxisStatus &finalStatus = controller->status(nodeId);
                                        
                                        // 判断修复是否成功
                                        bool hasErrors = (finalStatus.axisError != 0 || finalStatus.motorError != 0 
                                                         || finalStatus.encoderError != 0 || finalStatus.controllerError != 0);
                                        bool inClosedLoop = (finalStatus.axisState == 8);
                                        
                                        if (!hasErrors && inClosedLoop) {
                                            appendLog(zh("e4bfaee5a48de68890e58a9fefbc8ce794b5e69cbae5b7b2e8bf9be585a5e997ade78eaf"));
                                        } else if (!hasErrors && !inClosedLoop) {
                                            // 无错误但未在闭环 - 这是正常的，可能只是在空闲状态
                                            if (criticalCount == 0) {
                                                appendLog(zh("e4bfaee5a48de68890e58a9fefbc8ce99499e8afafe5b7b2e6b885e999a4efbc8ce78ab6e68081e6ada3e5b8b8"));
                                            } else {
                                                appendLog(zh("e4bfaee5a48de983a8e58886e68890e58a9fefbc8ce4bd86e69caae8bf9be585a5e997ade78eafefbc8ce8afb7e6898be58aa8e782b9e587bbe3808ae5bc80e5a78be3808be68c89e992ae"));
                                            }
                                        } else {
                                            appendLog(zh("e4bfaee5a48de983a8e58886e68890e58a9fefbc8ce4bd86e4bb8de69c89e5bc82e5b8b8efbc8ce8afb7e6898be58aa8e8bf90e8a18ce3808ae695b4e69cbae5889de5a78be58c96e3808be68c89e992ae"));
                                        }
                                        appendLog(QStringLiteral("========== %1 ==========").arg(zh("e4bfaee5a48de5ae8ce68890")));
                                        updateStatusPanel();
                                    });
                                });
                            } else {
                                // 已经在闭环且无严重错误
                                appendLog(zh("e4bfaee5a48de68890e58a9fefbc8ce78ab6e68081e6ada3e5b8b8"));
                                appendLog(QStringLiteral("========== %1 ==========").arg(zh("e4bfaee5a48de5ae8ce68890")));
                                updateStatusPanel();
                            }
                        });
                    });
                });
            });
        }
    });
}

void MainWindow::initializeMachine()
{
    ODriveMotorController *controller = currentDebugController();
    if (!controller || !controller->isConnected()) {
        appendLog(zh("e695b4e69cbae5889de5a78be58c96e5a4b1e8b4a5efbc9ae69caae8bf9ee68ea5"));
        qInfo() << "Machine initialization failed: not connected";
        return;
    }

    const quint8 nodeId = currentDebugNodeId();
    QPointer<ODriveMotorController> controllerGuard(controller);
    appendLog(QStringLiteral("========== %1 ==========").arg(zh("e5bc80e5a78be695b4e69cbae5889de5a78be58c96")));
    qInfo() << "========== Starting Machine Initialization ==========";

    // 步骤1：进入空闲状态
    appendLog(zh("e6ada5e9aaa431efbc9ae8bf9be585a5e7a9bae997b2e78ab6e68081"));
    qInfo() << "Step 1: Entering IDLE state";
    controller->requestIdle(nodeId);

    QTimer::singleShot(800, this, [this, controllerGuard, nodeId]() {
        if (!controllerGuard || !controllerGuard->isConnected()) {
            appendLog(QStringLiteral("Initialization cancelled for node %1: controller unavailable").arg(nodeId));
            return;
        }

        ODriveMotorController *controller = controllerGuard.data();
        // 步骤2：清除错误
        appendLog(zh("e6ada5e9aaa432efbc9ae6b885e999a4e99499e8afaf"));
        qInfo() << "Step 2: Clearing errors";
        controller->clearErrors(nodeId, false);

        QTimer::singleShot(800, this, [this, controllerGuard, nodeId]() {
            if (!controllerGuard || !controllerGuard->isConnected()) {
                appendLog(QStringLiteral("Initialization cancelled for node %1: controller unavailable").arg(nodeId));
                return;
            }

            ODriveMotorController *controller = controllerGuard.data();

            // 步骤3：检查当前状态
            controller->requestAllTelemetry(nodeId, false);

            QTimer::singleShot(500, this, [this, controllerGuard, nodeId]() {
                if (!controllerGuard || !controllerGuard->isConnected()) {
                    appendLog(QStringLiteral("Initialization cancelled for node %1: controller unavailable").arg(nodeId));
                    return;
                }

                ODriveMotorController *controller = controllerGuard.data();
                const ODriveMotorController::AxisStatus &preStatus = controller->status(nodeId);
                
                // 检查母线电压
                if (preStatus.busVoltageVolts < 10.0f) {
                    appendLog(zh("e5889de5a78be58c96e5a4b1e8b4a5efbc9ae6af8de7babfe794b5e58e8be8bf87e4bd8eefbc8ce8afb7e6a380e69fa5e4be9be794b5"));
                    qInfo() << "Initialization failed: bus voltage too low";
                    appendLog(QStringLiteral("========== %1 ==========").arg(zh("e5889de5a78be58c96e5a4b1e8b4a5")));
                    updateStatusPanel();
                    return;
                }
                
                appendLog(zh("e6ada5e9aaa433efbc9ae689a7e8a18ce5ae8ce695b4e6a0a1e58786efbc88e9a284e8aea1e99c8012e7a792efbc89"));
                qInfo() << "Step 3: Running full calibration (estimated 12 seconds)";
                controller->requestFullCalibration(nodeId);

                QTimer::singleShot(12000, this, [this, controllerGuard, nodeId]() {
                    if (!controllerGuard || !controllerGuard->isConnected()) {
                        appendLog(QStringLiteral("Initialization cancelled for node %1: controller unavailable").arg(nodeId));
                        return;
                    }

                    ODriveMotorController *controller = controllerGuard.data();

                    // 步骤4：检查校准结果
                    controller->requestAllTelemetry(nodeId, false);

                    QTimer::singleShot(500, this, [this, controllerGuard, nodeId]() {
                        if (!controllerGuard || !controllerGuard->isConnected()) {
                            appendLog(QStringLiteral("Initialization cancelled for node %1: controller unavailable").arg(nodeId));
                            return;
                        }

                        ODriveMotorController *controller = controllerGuard.data();
                        const ODriveMotorController::AxisStatus &status = controller->status(nodeId);

                        if (status.axisError == 0 && status.motorError == 0 && status.encoderError == 0) {
                            appendLog(zh("e6ada5e9aaa434efbc9ae6a0a1e58786e68890e58a9fefbc8ce8bf9be585a5e997ade78eaf"));
                            qInfo() << "Step 4: Calibration successful, entering closed loop";
                            
                            // 设置安全的限制
                            const float safeVelLimit = (nodeId == 0) ? 5.0f : 20.0f;
                            controller->setLimits(nodeId, safeVelLimit, 10.0f);
                            
                            QTimer::singleShot(500, this, [this, controllerGuard, nodeId]() {
                                if (!controllerGuard || !controllerGuard->isConnected()) {
                                    appendLog(QStringLiteral("Initialization cancelled for node %1: controller unavailable").arg(nodeId));
                                    return;
                                }

                                ODriveMotorController *controller = controllerGuard.data();
                                controller->requestClosedLoop(nodeId);

                                QTimer::singleShot(1500, this, [this, controllerGuard, nodeId]() {
                                    if (!controllerGuard || !controllerGuard->isConnected()) {
                                        appendLog(QStringLiteral("Initialization cancelled for node %1: controller unavailable").arg(nodeId));
                                        return;
                                    }

                                    ODriveMotorController *controller = controllerGuard.data();
                                    const ODriveMotorController::AxisStatus &finalStatus = controller->status(nodeId);

                                    if (finalStatus.axisError == 0 && finalStatus.axisState == 8) {
                                        appendLog(zh("e695b4e69cbae5889de5a78be58c96e5ae8ce68890efbc8ce794b5e69cbae5b7b2e5b0b1e7bbaa"));
                                        qInfo() << "Machine initialization complete! Motor is ready";
                                    } else if (finalStatus.axisError != 0) {
                                        appendLog(zh("e5889de5a78be58c96e5ae8ce68890e4bd86e4bb8de69c89e99499e8afaf3a20302531")
                                                  .arg(QString::number(finalStatus.axisError, 16).toUpper()));
                                        appendLog(zh("e8afb7e6a380e69fa5e99499e8afafe4bba3e7a081e5928ce7a1ace4bbb6e8bf9ee68ea5"));
                                        qInfo() << "Initialization complete but has errors:" << QString::number(finalStatus.axisError, 16);
                                    } else {
                                        appendLog(zh("e5889de5a78be58c96e5ae8ce68890e4bd86e69caae8bf9be585a5e997ade78eafefbc8ce5bd93e5898de78ab6e680813a2531")
                                                  .arg(finalStatus.axisState));
                                        qInfo() << "Initialization complete but not in closed loop, state:" << finalStatus.axisState;
                                    }

                                    appendLog(QStringLiteral("========== %1 ==========").arg(zh("e5889de5a78be58c96e5ae8ce68890")));
                                    qInfo() << "========== Initialization Complete ==========";
                                    updateStatusPanel();
                                });
                            });
                        } else {
                            appendLog(zh("e6a0a1e58786e5a4b1e8b4a5efbc9a"));
                            if (status.axisError != 0) {
                                appendLog(zh("20202de8bdb4e99499e8afaf3a20302531").arg(QString::number(status.axisError, 16).toUpper()));
                            }
                            if (status.motorError != 0) {
                                appendLog(zh("20202de794b5e69cbae99499e8afaf3a20302531").arg(QString::number(status.motorError, 16).toUpper()));
                            }
                            if (status.encoderError != 0) {
                                appendLog(zh("20202de7bc96e7a081e599a8e99499e8afaf3a20302531").arg(QString::number(status.encoderError, 16).toUpper()));
                            }
                            qInfo() << "Calibration failed - axis:" << QString::number(status.axisError, 16)
                                    << "motor:" << QString::number(status.motorError, 16)
                                    << "encoder:" << QString::number(status.encoderError, 16);
                            appendLog(zh("e8afb7e6a380e69fa5efbc9a"));
                            appendLog(zh("20201. e7a1ace4bbb6e8bf9ee68ea5e698afe590a6e6ada3e7a1ae"));
                            appendLog(zh("20202. e7bc96e7a081e599a8e8bf9ee7babfe698afe590a6e6ada3e7a1ae"));
                            appendLog(zh("20203. e794b5e69cbae79bb8e7babfe698afe590a6e6ada3e7a1ae"));
                            appendLog(zh("20204. e4be9be794b5e698afe590a6e7a8b3e5ae9a"));
                            appendLog(QStringLiteral("========== %1 ==========").arg(zh("e5889de5a78be58c96e5a4b1e8b4a5")));
                            qInfo() << "========== Initialization Failed ==========";
                            updateStatusPanel();
                        }
                    });
                });
            });
        });
    });
}

void MainWindow::loadSettings()
{
    QSettings settings(appSettingsPath(), QSettings::IniFormat);

    if (settings.contains(QStringLiteral("main/geometry"))) {
        restoreGeometry(settings.value(QStringLiteral("main/geometry")).toByteArray());
    }
    if (m_advancedDialog && settings.contains(QStringLiteral("advanced/geometry"))) {
        m_advancedDialog->restoreGeometry(settings.value(QStringLiteral("advanced/geometry")).toByteArray());
    }

    auto setComboByData = [](QComboBox *combo, const QVariant &value) {
        if (!combo || !value.isValid()) {
            return;
        }
        const int index = combo->findData(value);
        if (index >= 0) {
            combo->setCurrentIndex(index);
        }
    };

    if (m_pluginCombo) {
        m_pluginCombo->blockSignals(true);
        setComboByData(m_pluginCombo, settings.value(QStringLiteral("comm/plugin")));
        m_pluginCombo->blockSignals(false);
    }
    if (m_bitrateSpin) m_bitrateSpin->setValue(settings.value(QStringLiteral("comm/bitrate"), m_bitrateSpin->value()).toInt());
    if (m_serialBaudSpin) m_serialBaudSpin->setValue(settings.value(QStringLiteral("comm/serial_baud"), m_serialBaudSpin->value()).toInt());
    if (m_interfaceCountCombo) {
        setComboByData(m_interfaceCountCombo, settings.value(QStringLiteral("comm/interface_count"), 1));
    }
    for (int boardIndex = 0; boardIndex < m_boardConfigRows.size(); ++boardIndex) {
        BoardConfigRow &row = m_boardConfigRows[boardIndex];
        row.preferredInterfaceName = settings.value(QStringLiteral("boards/%1/interface").arg(boardIndex),
                                                    row.preferredInterfaceName).toString();
        row.preferredTurnNodeId = settings.value(QStringLiteral("boards/%1/turn_node").arg(boardIndex),
                                                 row.preferredTurnNodeId).toInt();
        row.preferredDriveNodeId = settings.value(QStringLiteral("boards/%1/drive_node").arg(boardIndex),
                                                  row.preferredDriveNodeId).toInt();
        if (row.turnScaleSpin) {
            row.turnScaleSpin->setValue(settings.value(QStringLiteral("boards/%1/turn_scale").arg(boardIndex),
                                                       row.turnScaleSpin->value()).toDouble());
        }
        if (row.driveScaleSpin) {
            row.driveScaleSpin->setValue(settings.value(QStringLiteral("boards/%1/drive_scale").arg(boardIndex),
                                                        row.driveScaleSpin->value()).toDouble());
        }
    }
    updateBoardRowVisibility();

    if (m_homeSpeedSpin) m_homeSpeedSpin->setValue(settings.value(QStringLiteral("motion/home_speed"), m_homeSpeedSpin->value()).toDouble());
    if (m_jogSpeedSpin) {
        const QVariant driveSpeedValue = settings.contains(QStringLiteral("motion/drive_speed"))
                ? settings.value(QStringLiteral("motion/drive_speed"))
                : settings.value(QStringLiteral("motion/jog_speed"), m_jogSpeedSpin->value());
        m_jogSpeedSpin->setValue(driveSpeedValue.toDouble());
    }
    if (m_scanAxisLengthSpin) m_scanAxisLengthSpin->setValue(settings.value(QStringLiteral("motion/scan_axis_length"), m_scanAxisLengthSpin->value()).toDouble());
    if (m_scanSpeedSpin) m_scanSpeedSpin->setValue(settings.value(QStringLiteral("motion/scan_speed"), m_scanSpeedSpin->value()).toDouble());
    if (m_stepAxisLengthSpin) m_stepAxisLengthSpin->setValue(settings.value(QStringLiteral("motion/step_axis_length"), m_stepAxisLengthSpin->value()).toDouble());
    if (m_stepLengthSpin) m_stepLengthSpin->setValue(settings.value(QStringLiteral("motion/step_length"), m_stepLengthSpin->value()).toDouble());

    setComboByData(m_scanModeCombo, settings.value(QStringLiteral("motion/scan_mode")));
    setComboByData(m_controlModeCombo, settings.value(QStringLiteral("control/control_mode")));
    setComboByData(m_inputModeCombo, settings.value(QStringLiteral("control/input_mode")));

    if (m_positionSpin) m_positionSpin->setValue(settings.value(QStringLiteral("control/position"), m_positionSpin->value()).toDouble());
    if (m_positionVelFfSpin) m_positionVelFfSpin->setValue(settings.value(QStringLiteral("control/position_vel_ff"), m_positionVelFfSpin->value()).toDouble());
    if (m_positionTorqueFfSpin) m_positionTorqueFfSpin->setValue(settings.value(QStringLiteral("control/position_torque_ff"), m_positionTorqueFfSpin->value()).toDouble());
    if (m_velocitySpin) m_velocitySpin->setValue(settings.value(QStringLiteral("control/velocity"), m_velocitySpin->value()).toDouble());
    if (m_velocityTorqueFfSpin) m_velocityTorqueFfSpin->setValue(settings.value(QStringLiteral("control/velocity_torque_ff"), m_velocityTorqueFfSpin->value()).toDouble());
    if (m_torqueSpin) m_torqueSpin->setValue(settings.value(QStringLiteral("control/torque"), m_torqueSpin->value()).toDouble());
    if (m_velocityLimitSpin) m_velocityLimitSpin->setValue(settings.value(QStringLiteral("control/velocity_limit"), m_velocityLimitSpin->value()).toDouble());
    if (m_currentLimitSpin) m_currentLimitSpin->setValue(settings.value(QStringLiteral("control/current_limit"), m_currentLimitSpin->value()).toDouble());
    if (m_debugBoardCombo) m_debugBoardCombo->setCurrentIndex(settings.value(QStringLiteral("debug/board_index"), 0).toInt());
    if (m_debugNodeCombo) {
        m_debugNodeCombo->setProperty("saved_node_id", settings.value(QStringLiteral("debug/node_id"), QVariant()));
    }
    if (m_debugScaleSpin) m_debugScaleSpin->setValue(settings.value(QStringLiteral("debug/scale"), m_debugScaleSpin->value()).toDouble());

    if (m_txIdEdit) m_txIdEdit->setText(settings.value(QStringLiteral("tx/id"), m_txIdEdit->text()).toString());
    if (m_txDlcSpin) m_txDlcSpin->setValue(settings.value(QStringLiteral("tx/dlc"), m_txDlcSpin->value()).toInt());
    for (int i = 0; i < m_txByteEdits.size(); ++i) {
        QLineEdit *edit = m_txByteEdits.at(i);
        if (edit) {
            edit->setText(settings.value(QStringLiteral("tx/byte_%1").arg(i), edit->text()).toString());
        }
    }
    if (m_txExtendedCheck) m_txExtendedCheck->setChecked(settings.value(QStringLiteral("tx/extended"), false).toBool());
    if (m_txRemoteCheck) m_txRemoteCheck->setChecked(settings.value(QStringLiteral("tx/remote"), false).toBool());
    if (m_txPeriodSpin) m_txPeriodSpin->setValue(settings.value(QStringLiteral("tx/period"), m_txPeriodSpin->value()).toInt());
}

void MainWindow::saveSettings() const
{
    QSettings settings(appSettingsPath(), QSettings::IniFormat);

    settings.setValue(QStringLiteral("main/geometry"), saveGeometry());
    if (m_advancedDialog) {
        settings.setValue(QStringLiteral("advanced/geometry"), m_advancedDialog->saveGeometry());
    }

    if (m_pluginCombo) settings.setValue(QStringLiteral("comm/plugin"), currentPluginId(m_pluginCombo));
    if (m_bitrateSpin) settings.setValue(QStringLiteral("comm/bitrate"), m_bitrateSpin->value());
    if (m_serialBaudSpin) settings.setValue(QStringLiteral("comm/serial_baud"), m_serialBaudSpin->value());
    if (m_interfaceCountCombo) settings.setValue(QStringLiteral("comm/interface_count"), connectedInterfaceCount());
    for (int boardIndex = 0; boardIndex < m_boardConfigRows.size(); ++boardIndex) {
        const BoardConfigRow &row = m_boardConfigRows.at(boardIndex);
        if (row.interfaceCombo) settings.setValue(QStringLiteral("boards/%1/interface").arg(boardIndex), row.interfaceCombo->currentText().trimmed());
        if (row.turnNodeCombo) settings.setValue(QStringLiteral("boards/%1/turn_node").arg(boardIndex), comboNodeValue(row.turnNodeCombo, static_cast<quint8>(row.preferredTurnNodeId)));
        if (row.driveNodeCombo) settings.setValue(QStringLiteral("boards/%1/drive_node").arg(boardIndex), comboNodeValue(row.driveNodeCombo, static_cast<quint8>(row.preferredDriveNodeId)));
        if (row.turnScaleSpin) settings.setValue(QStringLiteral("boards/%1/turn_scale").arg(boardIndex), row.turnScaleSpin->value());
        if (row.driveScaleSpin) settings.setValue(QStringLiteral("boards/%1/drive_scale").arg(boardIndex), row.driveScaleSpin->value());
    }
    if (m_debugBoardCombo) settings.setValue(QStringLiteral("debug/board_index"), m_debugBoardCombo->currentIndex());
    if (m_debugNodeCombo) settings.setValue(QStringLiteral("debug/node_id"), comboNodeValue(m_debugNodeCombo, 0));
    if (m_debugScaleSpin) settings.setValue(QStringLiteral("debug/scale"), m_debugScaleSpin->value());

    if (m_homeSpeedSpin) settings.setValue(QStringLiteral("motion/home_speed"), m_homeSpeedSpin->value());
    if (m_jogSpeedSpin) {
        settings.setValue(QStringLiteral("motion/drive_speed"), m_jogSpeedSpin->value());
        settings.remove(QStringLiteral("motion/jog_speed"));
    }
    if (m_scanAxisLengthSpin) settings.setValue(QStringLiteral("motion/scan_axis_length"), m_scanAxisLengthSpin->value());
    if (m_scanSpeedSpin) settings.setValue(QStringLiteral("motion/scan_speed"), m_scanSpeedSpin->value());
    if (m_stepAxisLengthSpin) settings.setValue(QStringLiteral("motion/step_axis_length"), m_stepAxisLengthSpin->value());
    if (m_stepLengthSpin) settings.setValue(QStringLiteral("motion/step_length"), m_stepLengthSpin->value());
    if (m_scanModeCombo) settings.setValue(QStringLiteral("motion/scan_mode"), m_scanModeCombo->currentData());

    if (m_controlModeCombo) settings.setValue(QStringLiteral("control/control_mode"), m_controlModeCombo->currentData());
    if (m_inputModeCombo) settings.setValue(QStringLiteral("control/input_mode"), m_inputModeCombo->currentData());
    if (m_positionSpin) settings.setValue(QStringLiteral("control/position"), m_positionSpin->value());
    if (m_positionVelFfSpin) settings.setValue(QStringLiteral("control/position_vel_ff"), m_positionVelFfSpin->value());
    if (m_positionTorqueFfSpin) settings.setValue(QStringLiteral("control/position_torque_ff"), m_positionTorqueFfSpin->value());
    if (m_velocitySpin) settings.setValue(QStringLiteral("control/velocity"), m_velocitySpin->value());
    if (m_velocityTorqueFfSpin) settings.setValue(QStringLiteral("control/velocity_torque_ff"), m_velocityTorqueFfSpin->value());
    if (m_torqueSpin) settings.setValue(QStringLiteral("control/torque"), m_torqueSpin->value());
    if (m_velocityLimitSpin) settings.setValue(QStringLiteral("control/velocity_limit"), m_velocityLimitSpin->value());
    if (m_currentLimitSpin) settings.setValue(QStringLiteral("control/current_limit"), m_currentLimitSpin->value());

    if (m_txIdEdit) settings.setValue(QStringLiteral("tx/id"), m_txIdEdit->text());
    if (m_txDlcSpin) settings.setValue(QStringLiteral("tx/dlc"), m_txDlcSpin->value());
    for (int i = 0; i < m_txByteEdits.size(); ++i) {
        const QLineEdit *edit = m_txByteEdits.at(i);
        if (edit) {
            settings.setValue(QStringLiteral("tx/byte_%1").arg(i), edit->text());
        }
    }
    if (m_txExtendedCheck) settings.setValue(QStringLiteral("tx/extended"), m_txExtendedCheck->isChecked());
    if (m_txRemoteCheck) settings.setValue(QStringLiteral("tx/remote"), m_txRemoteCheck->isChecked());
    if (m_txPeriodSpin) settings.setValue(QStringLiteral("tx/period"), m_txPeriodSpin->value());
}

void MainWindow::connectSettingsPersistence()
{
    const QList<QDoubleSpinBox *> doubleSpins = {
        m_homeSpeedSpin, m_jogSpeedSpin, m_scanAxisLengthSpin, m_scanSpeedSpin,
        m_stepAxisLengthSpin, m_stepLengthSpin,
        m_positionSpin, m_positionVelFfSpin, m_positionTorqueFfSpin,
        m_velocitySpin, m_velocityTorqueFfSpin, m_torqueSpin,
        m_velocityLimitSpin, m_currentLimitSpin
    };
    for (QDoubleSpinBox *spin : doubleSpins) {
        if (spin) connect(spin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](double) { scheduleSaveSettings(); });
    }

    const QList<QSpinBox *> spins = {
        m_bitrateSpin, m_serialBaudSpin, m_txDlcSpin, m_txPeriodSpin
    };
    for (QSpinBox *spin : spins) {
        if (spin) connect(spin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int) { scheduleSaveSettings(); });
    }

    const QList<QComboBox *> combos = {
        m_pluginCombo, m_interfaceCountCombo, m_debugBoardCombo, m_debugNodeCombo,
        m_scanModeCombo, m_controlModeCombo, m_inputModeCombo
    };
    for (QComboBox *combo : combos) {
        if (combo) connect(combo, &QComboBox::currentTextChanged, this, [this](const QString &) { scheduleSaveSettings(); });
    }

    QList<QLineEdit *> edits;
    edits << m_txIdEdit;
    for (QLineEdit *edit : m_txByteEdits) {
        edits << edit;
    }
    for (QLineEdit *edit : edits) {
        if (edit) connect(edit, &QLineEdit::textChanged, this, [this](const QString &) { scheduleSaveSettings(); });
    }

    const QList<QCheckBox *> checks = { m_txExtendedCheck, m_txRemoteCheck };
    for (QCheckBox *check : checks) {
        if (check) connect(check, &QCheckBox::toggled, this, [this](bool) { scheduleSaveSettings(); });
    }
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

void MainWindow::refreshBoardInterfaceOptions()
{
    for (int boardIndex = 0; boardIndex < m_boardConfigRows.size(); ++boardIndex) {
        BoardConfigRow &row = m_boardConfigRows[boardIndex];
        if (!row.interfaceCombo) {
            continue;
        }

        const QString preferred = row.interfaceCombo->currentText().trimmed().isEmpty()
                ? row.preferredInterfaceName
                : row.interfaceCombo->currentText().trimmed();
        row.interfaceCombo->blockSignals(true);
        row.interfaceCombo->clear();
        row.interfaceCombo->addItems(m_availableInterfaces);
        const int preferredIndex = row.interfaceCombo->findText(preferred);
        if (preferredIndex >= 0) {
            row.interfaceCombo->setCurrentIndex(preferredIndex);
        }
        row.interfaceCombo->blockSignals(false);
    }
}

void MainWindow::updateBoardRowVisibility()
{
    const int visibleCount = connectedInterfaceCount();
    for (int boardIndex = 0; boardIndex < m_boardConfigRows.size(); ++boardIndex) {
        const bool visible = boardIndex < visibleCount;
        BoardConfigRow &row = m_boardConfigRows[boardIndex];
        if (row.label) row.label->setVisible(visible);
        if (row.interfaceCombo) row.interfaceCombo->setVisible(visible);
        if (row.scanNodesButton) row.scanNodesButton->setVisible(visible);
        if (row.turnNodeCombo) row.turnNodeCombo->setVisible(visible);
        if (row.driveNodeCombo) row.driveNodeCombo->setVisible(visible);
        if (row.turnScaleSpin) row.turnScaleSpin->setVisible(visible);
        if (row.driveScaleSpin) row.driveScaleSpin->setVisible(visible);
        if (row.statusLabel) row.statusLabel->setVisible(visible);
    }
}

void MainWindow::updateBoardNodeCombos(int boardIndex)
{
    if (boardIndex < 0 || boardIndex >= m_boardConfigRows.size()) {
        return;
    }

    BoardConfigRow &row = m_boardConfigRows[boardIndex];
    auto updateCombo = [](QComboBox *combo, const QList<quint8> &nodes, int preferredNodeId) {
        if (!combo) {
            return;
        }
        combo->blockSignals(true);
        combo->clear();
        QList<quint8> values = nodes;
        const quint8 preferred = static_cast<quint8>(preferredNodeId & 0x3F);
        if (!values.contains(preferred)) {
            values << preferred;
        }
        std::sort(values.begin(), values.end());
        values.erase(std::unique(values.begin(), values.end()), values.end());
        for (const quint8 nodeId : values) {
            combo->addItem(QStringLiteral("Node %1").arg(nodeId), nodeId);
        }
        const int preferredIndex = combo->findData(preferred);
        if (preferredIndex >= 0) {
            combo->setCurrentIndex(preferredIndex);
        }
        combo->blockSignals(false);
    };

    updateCombo(row.turnNodeCombo, row.detectedNodes, row.preferredTurnNodeId);
    updateCombo(row.driveNodeCombo, row.detectedNodes, row.preferredDriveNodeId);
    
    // 启用节点选择下拉框
    if (row.turnNodeCombo) {
        row.turnNodeCombo->setEnabled(true);
    }
    if (row.driveNodeCombo) {
        row.driveNodeCombo->setEnabled(true);
    }
}

void MainWindow::updateAllBoardNodeCombos()
{
    for (int boardIndex = 0; boardIndex < m_boardConfigRows.size(); ++boardIndex) {
        updateBoardNodeCombos(boardIndex);
    }
}

void MainWindow::updateBoardStatusLabel(int boardIndex, const QString &statusText)
{
    if (boardIndex < 0 || boardIndex >= m_boardConfigRows.size()) {
        return;
    }

    BoardConfigRow &row = m_boardConfigRows[boardIndex];
    if (!row.statusLabel) {
        return;
    }

    if (!statusText.isEmpty()) {
        row.statusLabel->setText(statusText);
        return;
    }

    // 如果已连接，显示详细状态
    if (boardIndex < m_boardRuntimes.size() && m_boardRuntimes[boardIndex].connected) {
        ODriveMotorController *ctrl = m_boardRuntimes[boardIndex].controller;
        if (ctrl) {
            quint8 turnNode = boardTurnNodeId(boardIndex);
            quint8 driveNode = boardDriveNodeId(boardIndex);
            const QDateTime now = QDateTime::currentDateTime();
            const auto hasFreshHeartbeat = [&now](const ODriveMotorController::AxisStatus &status) {
                if (!status.lastHeartbeat.isValid()) {
                    return false;
                }

                qint64 ageMs = status.lastHeartbeat.msecsTo(now);
                if (ageMs < 0) {
                    ageMs = 0;
                }
                return ageMs <= 1500;
            };
            
            // 获取转轴和驱轴的状态
            QString turnStatus = QStringLiteral("--");
            QString driveStatus = QStringLiteral("--");
            
            // 转轴状态
            const ODriveMotorController::AxisStatus &turnAxisStatus = ctrl->status(turnNode);
            if (hasFreshHeartbeat(turnAxisStatus)) {
                turnStatus = QString("T:S%1 V%2")
                                 .arg(turnAxisStatus.axisState)
                                 .arg(turnAxisStatus.velocityTurnsPerSecond, 0, 'f', 1);
            }
            
            // 驱轴状态
            const ODriveMotorController::AxisStatus &driveAxisStatus = ctrl->status(driveNode);
            if (driveNode != turnNode && hasFreshHeartbeat(driveAxisStatus)) {
                driveStatus = QString(" D:S%1 V%2")
                                  .arg(driveAxisStatus.axisState)
                                  .arg(driveAxisStatus.velocityTurnsPerSecond, 0, 'f', 1);
            }
            
            row.statusLabel->setText(turnStatus + driveStatus);
        } else {
            row.statusLabel->setText(zh("e5b7b2e8bf9ee68ea5"));
        }
        return;
    }

    if (!row.detectedNodes.isEmpty()) {
        QString nodeList;
        for (int i = 0; i < row.detectedNodes.size() && i < 4; ++i) {
            if (i > 0) nodeList += QStringLiteral(",");
            nodeList += QString::number(row.detectedNodes[i]);
        }
        row.statusLabel->setText(zh("e88a82e782b93a20") + nodeList);
        return;
    }

    row.statusLabel->setText(QStringLiteral("--"));
}

void MainWindow::updateDebugBoardCombo()
{
    if (!m_debugBoardCombo) {
        return;
    }

    const QVariant previousData = m_debugBoardCombo->currentData();
    m_debugBoardCombo->blockSignals(true);
    m_debugBoardCombo->clear();
    for (int boardIndex = 0; boardIndex < connectedInterfaceCount() && boardIndex < m_boardConfigRows.size(); ++boardIndex) {
        const QString interfaceName = boardInterfaceName(boardIndex);
        if (!interfaceName.isEmpty()) {
            m_debugBoardCombo->addItem(QStringLiteral("%1 (%2)").arg(boardDisplayName(boardIndex), interfaceName), boardIndex);
        }
    }
    int index = m_debugBoardCombo->findData(previousData);
    if (index < 0 && m_debugBoardCombo->count() > 0) {
        index = 0;
    }
    if (index >= 0) {
        m_debugBoardCombo->setCurrentIndex(index);
    }
    m_debugBoardCombo->blockSignals(false);
}

void MainWindow::updateDebugNodeCombo()
{
    if (!m_debugNodeCombo) {
        return;
    }

    const int boardIndex = currentDebugBoardIndex();
    m_updatingDebugNodeCombo = true;
    m_debugNodeCombo->blockSignals(true);
    m_debugNodeCombo->clear();

    if (boardIndex >= 0 && boardIndex < m_boardConfigRows.size()) {
        const QList<quint8> nodes = m_boardConfigRows[boardIndex].detectedNodes;
        for (const quint8 nodeId : nodes) {
            m_debugNodeCombo->addItem(QStringLiteral("Node %1").arg(nodeId), nodeId);
        }

        const QVariant savedNode = m_debugNodeCombo->property("saved_node_id");
        int index = savedNode.isValid() ? m_debugNodeCombo->findData(savedNode) : -1;
        if (index < 0) {
            index = m_debugNodeCombo->findData(boardTurnNodeId(boardIndex));
        }
        if (index < 0 && m_debugNodeCombo->count() > 0) {
            index = 0;
        }
        if (index >= 0) {
            m_debugNodeCombo->setCurrentIndex(index);
        }
        m_debugNodeCombo->setEnabled(m_debugNodeCombo->count() > 0);

        QStringList nodeStrings;
        for (const quint8 nodeId : nodes) {
            nodeStrings << QString::number(nodeId);
        }
        if (m_activeNodesLabel) {
            m_activeNodesLabel->setText(nodes.isEmpty()
                                        ? zh("e6a380e6b58be588b0e79a84e88a82e782b9320e697a0")
                                        : QStringLiteral("%1: %2").arg(zh("e6a380e6b58be588b0e79a84e88a82e782b9"), nodeStrings.join(", ")));
        }
    } else {
        m_debugNodeCombo->setEnabled(false);
        if (m_activeNodesLabel) {
            m_activeNodesLabel->setText(zh("e6a380e6b58be588b0e79a84e88a82e782b93a202d2d"));
        }
    }

    m_debugNodeCombo->blockSignals(false);
    m_updatingDebugNodeCombo = false;
}

void MainWindow::updateDebugScaleSpin()
{
    if (!m_debugScaleSpin) {
        return;
    }

    const int boardIndex = currentDebugBoardIndex();
    if (boardIndex < 0 || boardIndex >= m_boardConfigRows.size()) {
        m_debugScaleSpin->setEnabled(false);
        return;
    }

    m_debugScaleSpin->setEnabled(true);
    const quint8 nodeId = currentDebugNodeId();
    const BoardConfigRow &row = m_boardConfigRows[boardIndex];
    m_debugScaleSpin->blockSignals(true);
    if (nodeId == boardTurnNodeId(boardIndex) && row.turnScaleSpin) {
        m_debugScaleSpin->setValue(row.turnScaleSpin->value());
    } else if (nodeId == boardDriveNodeId(boardIndex) && row.driveScaleSpin) {
        m_debugScaleSpin->setValue(row.driveScaleSpin->value());
    }
    m_debugScaleSpin->blockSignals(false);
}

void MainWindow::refreshCurrentBoardNodes()
{
    refreshBoardNodes(currentDebugBoardIndex(), true);
}

void MainWindow::refreshBoardNodes(int boardIndex, bool temporaryConnectionAllowed)
{
    if (boardIndex < 0 || boardIndex >= m_boardConfigRows.size()) {
        return;
    }

    const QString interfaceName = boardInterfaceName(boardIndex);
    if (interfaceName.isEmpty()) {
        updateBoardStatusLabel(boardIndex, zh("e8afb7e98089e68ba9e68ea5e58fa3"));
        return;
    }

    ODriveMotorController *controller = nullptr;
    bool temporaryController = false;
    if (boardIndex < m_boardRuntimes.size() && m_boardRuntimes[boardIndex].connected) {
        controller = m_boardRuntimes[boardIndex].controller;
    } else if (temporaryConnectionAllowed) {
        controller = new ODriveMotorController(this);
        temporaryController = true;
        if (!controller->connectDevice(currentPluginId(m_pluginCombo),
                                       interfaceName,
                                       m_bitrateSpin->value(),
                                       m_serialBaudSpin->value(),
                                       static_cast<quint8>(m_boardConfigRows[boardIndex].preferredTurnNodeId & 0x3F))) {
            appendLog(QStringLiteral("%1 %2").arg(boardLogPrefix(boardIndex), zh("e88a82e782b9e689abe68f8fe8bf9ee68ea5e5a4b1e8b4a5")));
            controller->deleteLater();
            updateBoardStatusLabel(boardIndex, zh("e689abe68f8fe5a4b1e8b4a5"));
            return;
        }
    }

    if (!controller) {
        return;
    }

    if (!temporaryController) {
        for (int otherBoard = 0; otherBoard < boardIndex && otherBoard < m_boardRuntimes.size(); ++otherBoard) {
            if (!m_boardRuntimes[otherBoard].connected) {
                continue;
            }
            if (m_boardRuntimes[otherBoard].controller != controller) {
                continue;
            }

            appendLog(QStringLiteral("%1 using shared interface scan handled by %2")
                      .arg(boardLogPrefix(boardIndex), boardDisplayName(otherBoard)));
            updateBoardStatusLabel(boardIndex, QStringLiteral("shared scan"));
            return;
        }
    }

    appendLog(QStringLiteral("%1 %2").arg(boardLogPrefix(boardIndex), zh("e6ada3e59ca8e689abe68f8fe88a82e782b92e2e2e")));
    updateBoardStatusLabel(boardIndex, zh("e689abe68f8fe4b8ad"));
    QPointer<ODriveMotorController> controllerGuard(controller);
    controller->probeAllNodes();
    QTimer::singleShot(1500, this, [this, boardIndex, controllerGuard, temporaryController]() {
        if (!controllerGuard) {
            updateBoardStatusLabel(boardIndex, zh("e689abe68f8fe5b7b2e58f96e6b688"));
            return;
        }

        ODriveMotorController *controller = controllerGuard.data();
        QList<quint8> nodes = controller->recentResponsiveNodeIds(2000);
        std::sort(nodes.begin(), nodes.end());

        QList<int> affectedBoards;
        if (temporaryController) {
            affectedBoards << boardIndex;
        } else {
            for (int runtimeIndex = 0; runtimeIndex < m_boardRuntimes.size(); ++runtimeIndex) {
                if (m_boardRuntimes[runtimeIndex].connected
                        && m_boardRuntimes[runtimeIndex].controller == controller) {
                    affectedBoards << runtimeIndex;
                }
            }
            if (!affectedBoards.contains(boardIndex)) {
                affectedBoards << boardIndex;
            }
            std::sort(affectedBoards.begin(), affectedBoards.end());
            affectedBoards.erase(std::unique(affectedBoards.begin(), affectedBoards.end()), affectedBoards.end());
        }

        for (const int affectedBoard : affectedBoards) {
            if (affectedBoard < 0 || affectedBoard >= m_boardConfigRows.size()) {
                continue;
            }

            m_boardConfigRows[affectedBoard].detectedNodes = nodes;
            if (affectedBoard < m_boardRuntimes.size()
                    && m_boardRuntimes[affectedBoard].connected
                    && m_boardRuntimes[affectedBoard].controller == controller) {
                m_boardRuntimes[affectedBoard].detectedNodes = nodes;
            }

            updateBoardNodeCombos(affectedBoard);
            updateBoardStatusLabel(affectedBoard);
        }

        updateDebugBoardCombo();
        updateDebugNodeCombo();
        updateDebugScaleSpin();

        for (const int affectedBoard : affectedBoards) {
            if (affectedBoard < 0 || affectedBoard >= m_boardConfigRows.size()) {
                continue;
            }

            if (nodes.isEmpty()) {
                appendLog(QStringLiteral("%1 %2").arg(boardLogPrefix(affectedBoard), zh("e69caae6a380e6b58be588b0e88a82e782b9")));
            } else {
                QStringList nodeStrings;
                for (const quint8 nodeId : nodes) {
                    nodeStrings << QString::number(nodeId);
                }
                appendLog(QStringLiteral("%1 %2")
                          .arg(boardLogPrefix(affectedBoard),
                               zh("e6a380e6b58be588b0e88a82e782b93a202532").arg(nodeStrings.join(", "))));
            }
        }

        if (temporaryController) {
            controller->disconnectDevice();
            controller->deleteLater();
        }
    });
}

void MainWindow::connectBoardControllerSignals(int boardIndex, ODriveMotorController *controller)
{
    if (!controller) {
        return;
    }

    for (int otherBoard = 0; otherBoard < m_boardRuntimes.size(); ++otherBoard) {
        if (otherBoard != boardIndex && m_boardRuntimes[otherBoard].controller == controller) {
            return;
        }
    }

    const auto controllerPrefix = [this, controller, boardIndex]() {
        QStringList boardNames;
        QString interfaceName;

        for (int runtimeIndex = 0; runtimeIndex < m_boardRuntimes.size(); ++runtimeIndex) {
            if (m_boardRuntimes[runtimeIndex].controller != controller) {
                continue;
            }
            boardNames << boardDisplayName(runtimeIndex);
            if (interfaceName.isEmpty()) {
                interfaceName = boardInterfaceName(runtimeIndex);
            }
        }

        if (boardNames.isEmpty()) {
            boardNames << boardDisplayName(boardIndex);
        }
        if (interfaceName.isEmpty()) {
            interfaceName = boardInterfaceName(boardIndex);
        }

        const QString boardLabel = boardNames.join(QStringLiteral("/"));
        return interfaceName.isEmpty()
                ? QStringLiteral("[%1]").arg(boardLabel)
                : QStringLiteral("[%1 %2]").arg(boardLabel, interfaceName);
    };

    connect(controller, &ODriveMotorController::connectionChanged, this, [this, controller](bool connected) {
        for (int runtimeIndex = 0; runtimeIndex < m_boardRuntimes.size(); ++runtimeIndex) {
            if (m_boardRuntimes[runtimeIndex].controller != controller) {
                continue;
            }
            m_boardRuntimes[runtimeIndex].connected = connected;
            updateBoardStatusLabel(runtimeIndex, connected ? zh("e5b7b2e8bf9ee68ea5") : zh("e69caae8bf9ee68ea5"));
        }
        updateConnectionStateFromBoards();
    });
    connect(controller, &ODriveMotorController::statusUpdated, this, &MainWindow::scheduleStatusPanelUpdate);
    connect(controller, &ODriveMotorController::nodeStatusUpdated, this, [this](quint8 nodeId) {
        handleNodeStatusUpdate(nodeId);
    });
    connect(controller, &ODriveMotorController::logMessage, this, [this, controllerPrefix](const QString &message) {
        appendLog(QStringLiteral("%1 %2").arg(controllerPrefix(), message));
    });
    connect(controller, &ODriveMotorController::rxMessage, this, [this, controllerPrefix](const QString &message) {
        const QString tagged = QStringLiteral("%1 %2").arg(controllerPrefix(), message);
        appendRxMessage(tagged);
        handleCustomTxRxMessage(tagged);
    });
    connect(controller, &ODriveMotorController::errorMessage, this, [this, controllerPrefix](const QString &message) {
        appendLog(QStringLiteral("%1 %2").arg(controllerPrefix(), message));
    });
}

void MainWindow::disconnectAllBoards()
{
    QList<ODriveMotorController *> ownedControllers;

    for (int boardIndex = 0; boardIndex < m_boardRuntimes.size(); ++boardIndex) {
        BoardRuntime &runtime = m_boardRuntimes[boardIndex];
        runtime.connected = false;
        runtime.zeroPointInitialized = false;
        if (runtime.controller && runtime.ownsController && !ownedControllers.contains(runtime.controller)) {
            ownedControllers << runtime.controller;
        }
        runtime.controller = nullptr;
        runtime.ownsController = false;
        updateBoardStatusLabel(boardIndex);
    }

    for (ODriveMotorController *controller : ownedControllers) {
        controller->disconnectDevice();
        controller->deleteLater();
    }

    updateConnectionState(false);
}

void MainWindow::updateConnectionStateFromBoards()
{
    updateConnectionState(hasAnyConnectedBoard());
}

void MainWindow::syncConnectionEditorsFromAdvanced()
{
    if (!m_ipEdit || !m_pluginCombo || !m_bitrateSpin) {
        return;
    }

    const QString plugin = currentPluginId(m_pluginCombo);
    QString summary = pluginLabel(plugin);
    QStringList interfaceNames;
    for (const int boardIndex : configuredBoardIndices()) {
        const QString interfaceName = boardInterfaceName(boardIndex);
        if (!interfaceName.isEmpty()) {
            interfaceNames << interfaceName;
        }
    }
    if (!interfaceNames.isEmpty()) {
        summary += QStringLiteral(" | %1").arg(interfaceNames.join(", "));
    }
    summary += QStringLiteral(" | %1 bps | %2 board row(s)")
            .arg(QString::number(m_bitrateSpin->value()),
                 QString::number(interfaceNames.size()));

    m_ipEdit->blockSignals(true);
    m_ipEdit->setText(summary);
    m_ipEdit->blockSignals(false);
}

bool MainWindow::hasAnyConnectedBoard() const
{
    return !connectedBoardIndices().isEmpty();
}

QList<int> MainWindow::configuredBoardIndices() const
{
    QList<int> result;
    const int count = qMin(connectedInterfaceCount(), m_boardConfigRows.size());
    for (int boardIndex = 0; boardIndex < count; ++boardIndex) {
        if (!boardInterfaceName(boardIndex).isEmpty()) {
            result << boardIndex;
        }
    }
    return result;
}

QList<int> MainWindow::connectedBoardIndices() const
{
    QList<int> result;
    for (int boardIndex = 0; boardIndex < m_boardRuntimes.size(); ++boardIndex) {
        const BoardRuntime &runtime = m_boardRuntimes[boardIndex];
        if (runtime.connected && runtime.controller && runtime.controller->isConnected()) {
            result << boardIndex;
        }
    }
    return result;
}

QString MainWindow::boardInterfaceName(int boardIndex) const
{
    if (boardIndex < 0 || boardIndex >= m_boardConfigRows.size()) {
        return QString();
    }
    const BoardConfigRow &row = m_boardConfigRows[boardIndex];
    if (row.interfaceCombo) {
        return row.interfaceCombo->currentText().trimmed();
    }
    return row.preferredInterfaceName;
}

QString MainWindow::boardDisplayName(int boardIndex) const
{
    return zh("e69dbf2531").arg(boardIndex + 1);
}

QString MainWindow::boardLogPrefix(int boardIndex) const
{
    const QString interfaceName = boardInterfaceName(boardIndex);
    if (interfaceName.isEmpty()) {
        return QStringLiteral("[%1]").arg(boardDisplayName(boardIndex));
    }
    return QStringLiteral("[%1 %2]").arg(boardDisplayName(boardIndex), interfaceName);
}

int MainWindow::currentDebugBoardIndex() const
{
    if (!m_debugBoardCombo || m_debugBoardCombo->count() <= 0) {
        return -1;
    }
    return m_debugBoardCombo->currentData().toInt();
}

quint8 MainWindow::currentDebugNodeId() const
{
    const int boardIndex = currentDebugBoardIndex();
    return comboNodeValue(m_debugNodeCombo, boardIndex >= 0 ? boardTurnNodeId(boardIndex) : 0);
}

ODriveMotorController *MainWindow::currentDebugController() const
{
    const int boardIndex = currentDebugBoardIndex();
    if (boardIndex < 0 || boardIndex >= m_boardRuntimes.size()) {
        return nullptr;
    }
    return m_boardRuntimes[boardIndex].controller;
}

int MainWindow::boardIndexForController(const ODriveMotorController *controller) const
{
    for (int boardIndex = 0; boardIndex < m_boardRuntimes.size(); ++boardIndex) {
        if (m_boardRuntimes[boardIndex].controller == controller) {
            return boardIndex;
        }
    }
    return -1;
}

quint8 MainWindow::comboNodeValue(const QComboBox *combo, quint8 fallback) const
{
    if (!combo || combo->count() <= 0) {
        return fallback;
    }
    return static_cast<quint8>(combo->currentData().toUInt() & 0x3F);
}

quint8 MainWindow::boardTurnNodeId(int boardIndex) const
{
    if (boardIndex < 0 || boardIndex >= m_boardConfigRows.size()) {
        return 0;
    }
    const BoardConfigRow &row = m_boardConfigRows[boardIndex];
    return comboNodeValue(row.turnNodeCombo, static_cast<quint8>(row.preferredTurnNodeId & 0x3F));
}

quint8 MainWindow::boardDriveNodeId(int boardIndex) const
{
    if (boardIndex < 0 || boardIndex >= m_boardConfigRows.size()) {
        return 1;
    }
    const BoardConfigRow &row = m_boardConfigRows[boardIndex];
    return comboNodeValue(row.driveNodeCombo, static_cast<quint8>(row.preferredDriveNodeId & 0x3F));
}

double MainWindow::boardScale(int boardIndex, bool turnAxis) const
{
    if (boardIndex < 0 || boardIndex >= m_boardConfigRows.size()) {
        return 1.0;
    }
    const BoardConfigRow &row = m_boardConfigRows[boardIndex];
    const QDoubleSpinBox *spin = turnAxis ? row.turnScaleSpin : row.driveScaleSpin;
    return spin ? qMax(0.0001, spin->value()) : 1.0;
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
    m_serialBaudSpin->setEnabled(!hasAnyConnectedBoard() && serialTransport);
}

void MainWindow::updateLinkStatusDisplay()
{
    if (!m_linkStatusLabel) {
        return;
    }

    if (!hasAnyConnectedBoard()) {
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
    for (const int boardIndex : connectedBoardIndices()) {
        const BoardRuntime &runtime = m_boardRuntimes[boardIndex];
        if (!runtime.controller) {
            continue;
        }
        const QList<quint8> nodeIds = runtime.controller->trackedNodeIds();
        for (const quint8 nodeId : nodeIds) {
            const ODriveMotorController::AxisStatus &status = runtime.controller->status(nodeId);
            if (status.lastMessage.isValid() && status.lastMessage.msecsTo(now) <= 2000) {
                return true;
            }
        }
    }

    return false;
}

bool MainWindow::hasFreshTrackedHeartbeat() const
{
    constexpr qint64 heartbeatTimeoutMs = 1500;

    const QDateTime now = QDateTime::currentDateTime();
    for (const int boardIndex : connectedBoardIndices()) {
        const BoardRuntime &runtime = m_boardRuntimes[boardIndex];
        if (!runtime.controller) {
            continue;
        }
        QList<quint8> nodeIds = runtime.controller->trackedNodeIds();
        if (nodeIds.isEmpty()) {
            nodeIds << boardTurnNodeId(boardIndex);
            if (!nodeIds.contains(boardDriveNodeId(boardIndex))) {
                nodeIds << boardDriveNodeId(boardIndex);
            }
        }
        for (const quint8 nodeId : nodeIds) {
            const ODriveMotorController::AxisStatus &status = runtime.controller->status(nodeId);
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

    const QList<int> boardIndices = connectedBoardIndices();
    if (boardIndices.isEmpty()) {
        return;
    }

    const auto allBoardsReached = [this, &boardIndices](double targetMm, bool turnAxis) {
        for (const int boardIndex : boardIndices) {
            const quint8 nodeId = turnAxis ? boardTurnNodeId(boardIndex) : boardDriveNodeId(boardIndex);
            if (!axisReached(boardIndex, nodeId, targetMm, turnAxis)) {
                return false;
            }
        }
        return true;
    };

    switch (m_scanState.phase) {
    case ScanHoming:
        if (allBoardsReached(0.0, true) && allBoardsReached(0.0, false)) {
            m_scanState.phase = ScanSweeping;
            m_scanState.forward = true;
            startScanSweep(m_scanAxisLengthSpin->value());
        }
        break;
    case ScanSweeping:
        if (allBoardsReached(m_scanState.scanTargetMm, false)) {
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
            for (const int boardIndex : boardIndices) {
                commandAxisPosition(boardIndex,
                                    boardTurnNodeId(boardIndex),
                                    m_scanState.stepTargetMm,
                                    qMax(0.1, m_jogSpeedSpin->value()),
                                    true);
            }
            updateProgramStatus(zh("e6ada5e8bf9be8bdb4e5898de8bf9be588b0202531206d6d")
                                .arg(QString::number(m_scanState.stepTargetMm, 'f', 2)));
        }
        break;
    case ScanStepping:
        if (allBoardsReached(m_scanState.stepTargetMm, true)) {
            m_scanState.currentStepMm = m_scanState.stepTargetMm;

            const ScanModeOption mode =
                    static_cast<ScanModeOption>(m_scanModeCombo->currentData().toInt());

            if (mode == BowMode) {
                m_scanState.forward = !m_scanState.forward;
                m_scanState.phase = ScanSweeping;
                startScanSweep(m_scanState.forward ? m_scanAxisLengthSpin->value() : 0.0);
            } else {
                m_scanState.phase = ScanReturning;
                for (const int boardIndex : boardIndices) {
                    commandAxisPosition(boardIndex,
                                        boardDriveNodeId(boardIndex),
                                        0.0,
                                        m_scanSpeedSpin->value(),
                                        false);
                }
                updateProgramStatus(zh("e689abe68f8fe8bdb4e8bf94e59b9ee8b5b7e782b9"));
            }
        }
        break;
    case ScanReturning:
        if (allBoardsReached(0.0, false)) {
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
    for (const int boardIndex : connectedBoardIndices()) {
        commandAxisPosition(boardIndex,
                            boardDriveNodeId(boardIndex),
                            targetMm,
                            m_scanSpeedSpin->value(),
                            false);
    }
    updateProgramStatus(zh("e689abe68f8fe8bdb4e7a7bbe58aa8e588b0202531206d6d").arg(QString::number(targetMm, 'f', 2)));
}

bool MainWindow::ensureAxisClosedLoop(int boardIndex, quint8 nodeId)
{
    if (boardIndex < 0 || boardIndex >= m_boardRuntimes.size() || !m_boardRuntimes[boardIndex].controller) {
        return false;
    }

    ODriveMotorController *controller = m_boardRuntimes[boardIndex].controller;
    const ODriveMotorController::AxisStatus &status = controller->status(nodeId);

    if (status.axisState == ODriveMotorController::ClosedLoopControl && status.axisError == 0) {
        return true;
    }

    if (status.axisError != 0 || status.motorError != 0 || status.encoderError != 0 || status.controllerError != 0) {
        logMotionIssueOnce(QStringLiteral("closed-loop/%1/%2").arg(boardIndex).arg(nodeId),
                           QStringLiteral("%1 Node %2 has active errors, skipping forced closed-loop re-entry")
                           .arg(boardLogPrefix(boardIndex))
                           .arg(nodeId));
        return false;
    }

    return controller->requestClosedLoop(nodeId);
}

bool MainWindow::prepareAxisVelocityControl(int boardIndex,
                                            quint8 nodeId,
                                            double speedMmPerSecond,
                                            bool turnAxis)
{
    if (boardIndex < 0 || boardIndex >= m_boardRuntimes.size() || nodeId > 63) {
        return false;
    }

    ODriveMotorController *controller = m_boardRuntimes[boardIndex].controller;
    if (!controller || !controller->isConnected()) {
        return false;
    }

    const QString axisKey = motionAxisKey(boardIndex, nodeId, turnAxis);
    const double mmPerTurnValue = mmPerTurn(boardIndex, turnAxis);
    if (mmPerTurnValue <= 0.0) {
        logMotionIssueOnce(axisKey + QStringLiteral("/scale"),
                           QStringLiteral("%1 invalid scale for node %2")
                           .arg(boardLogPrefix(boardIndex))
                           .arg(nodeId));
        return false;
    }

    const bool stopCommand = qAbs(speedMmPerSecond) < 0.0001;
    const ODriveMotorController::AxisStatus status = controller->status(nodeId);
    const bool hasErrors =
            status.axisError != 0 || status.motorError != 0 || status.encoderError != 0 || status.controllerError != 0;
    const bool alreadyClosedLoop =
            status.axisState == ODriveMotorController::ClosedLoopControl && status.axisError == 0;

    if (hasErrors) {
        logMotionIssueOnce(axisKey + QStringLiteral("/errors"),
                           QStringLiteral("%1 Node %2 motion command blocked by active errors")
                           .arg(boardLogPrefix(boardIndex))
                           .arg(nodeId));
        return false;
    }

    if (stopCommand) {
        return true;
    }

    if (!alreadyClosedLoop && !ensureAxisClosedLoop(boardIndex, nodeId)) {
        logMotionIssueOnce(axisKey + QStringLiteral("/closed-loop-failed"),
                           QStringLiteral("%1 Node %2 failed to enter closed loop for jog command")
                           .arg(boardLogPrefix(boardIndex))
                           .arg(nodeId));
        return false;
    }

    const float targetTurnsPerSecond =
            static_cast<float>(mmPerSecondToTurnsPerSecond(boardIndex, speedMmPerSecond, turnAxis));
    const float velocityLimitTurnsPerSecond =
            qMax<float>(qAbs(targetTurnsPerSecond) * 1.5f, 1.0f);

    const bool modeMatches =
            status.currentControlMode == ODriveMotorController::VelocityControl
            && status.currentInputMode == ODriveMotorController::Passthrough;
    if (!modeMatches) {
        controller->setControllerMode(nodeId,
                                      ODriveMotorController::VelocityControl,
                                      ODriveMotorController::Passthrough);
    }

    const float currentLimit = static_cast<float>(currentLimitAmps());
    const bool limitMatches =
            qAbs(status.currentVelocityLimit - velocityLimitTurnsPerSecond) < 0.01f
            && qAbs(status.currentCurrentLimit - currentLimit) < 0.01f;
    if (!limitMatches) {
        controller->setLimits(nodeId, velocityLimitTurnsPerSecond, currentLimit);
    }

    return true;
}

bool MainWindow::sendPreparedAxisVelocity(int boardIndex,
                                          quint8 nodeId,
                                          double speedMmPerSecond,
                                          bool turnAxis)
{
    if (boardIndex < 0 || boardIndex >= m_boardRuntimes.size() || nodeId > 63) {
        return false;
    }

    ODriveMotorController *controller = m_boardRuntimes[boardIndex].controller;
    if (!controller || !controller->isConnected()) {
        return false;
    }

    if (qAbs(speedMmPerSecond) < 0.0001) {
        return stopAxisVelocity(boardIndex, nodeId);
    }

    const QString axisKey = motionAxisKey(boardIndex, nodeId, turnAxis);
    const float targetTurnsPerSecond =
            static_cast<float>(mmPerSecondToTurnsPerSecond(boardIndex, speedMmPerSecond, turnAxis));
    const bool ok = controller->setVelocity(nodeId, targetTurnsPerSecond, 0.0f);
    if (!ok) {
        logMotionIssueOnce(axisKey + QStringLiteral("/set-velocity-failed"),
                           QStringLiteral("%1 Node %2 failed to send jog velocity command")
                           .arg(boardLogPrefix(boardIndex))
                           .arg(nodeId));
    }
    return ok;
}

bool MainWindow::commandWheelVelocity(int boardIndex, bool leftWheel, double speedMmPerSecond)
{
    const quint8 nodeId = leftWheel ? boardTurnNodeId(boardIndex) : boardDriveNodeId(boardIndex);
    return commandAxisVelocity(boardIndex, nodeId, speedMmPerSecond, leftWheel);
}

bool MainWindow::commandAxisVelocity(int boardIndex, quint8 nodeId, double speedMmPerSecond, bool turnAxis)
{
    if (!prepareAxisVelocityControl(boardIndex, nodeId, speedMmPerSecond, turnAxis)) {
        return false;
    }
    return sendPreparedAxisVelocity(boardIndex, nodeId, speedMmPerSecond, turnAxis);
}

bool MainWindow::commandAxisPosition(int boardIndex,
                                     quint8 nodeId,
                                     double targetMm,
                                     double speedMmPerSecond,
                                     bool turnAxis)
{
    if (boardIndex < 0 || boardIndex >= m_boardRuntimes.size() || nodeId > 63) {
        return false;
    }
    ODriveMotorController *controller = m_boardRuntimes[boardIndex].controller;
    if (!controller || !controller->isConnected()) {
        return false;
    }

    const ODriveMotorController::AxisStatus &status = controller->status(nodeId);
    const bool hasErrors =
            status.axisError != 0 || status.motorError != 0 || status.encoderError != 0 || status.controllerError != 0;
    if (hasErrors) {
        appendLog(QStringLiteral("%1 Node %2 position command blocked by active errors")
                  .arg(boardLogPrefix(boardIndex))
                  .arg(nodeId));
        return false;
    }

    if (!ensureAxisClosedLoop(boardIndex, nodeId)) {
        appendLog(QStringLiteral("%1 Node %2 failed to enter closed loop for position command")
                  .arg(boardLogPrefix(boardIndex))
                  .arg(nodeId));
        return false;
    }

    controller->setLimits(nodeId,
                          static_cast<float>(mmPerSecondToTurnsPerSecond(boardIndex, qMax(0.1, speedMmPerSecond), turnAxis)),
                          static_cast<float>(currentLimitAmps()));
    controller->setControllerMode(nodeId,
                                  ODriveMotorController::PositionControl,
                                  ODriveMotorController::Passthrough);
    return controller->setPosition(nodeId,
                                   static_cast<float>(axisAbsoluteTurns(boardIndex, nodeId, targetMm, turnAxis)),
                                   0.0f,
                                   0.0f);
}

void MainWindow::stopAllMotion(bool requestIdleState)
{
    for (const int boardIndex : connectedBoardIndices()) {
        ODriveMotorController *controller = m_boardRuntimes[boardIndex].controller;
        if (!controller) {
            continue;
        }

        QList<quint8> nodeIds;
        nodeIds << boardTurnNodeId(boardIndex);
        if (!nodeIds.contains(boardDriveNodeId(boardIndex))) {
            nodeIds << boardDriveNodeId(boardIndex);
        }

        for (const quint8 nodeId : nodeIds) {
            stopAxisVelocity(boardIndex, nodeId);
            if (requestIdleState) {
                controller->requestIdle(nodeId);
            }
        }
    }
}

bool MainWindow::axisReached(int boardIndex, quint8 nodeId, double targetMm, bool turnAxis, double toleranceMm) const
{
    return qAbs(axisPositionMm(boardIndex, nodeId, turnAxis) - targetMm) <= toleranceMm;
}

double MainWindow::axisPositionMm(int boardIndex, quint8 nodeId, bool turnAxis) const
{
    if (boardIndex < 0 || boardIndex >= m_boardRuntimes.size() || !m_boardRuntimes[boardIndex].controller) {
        return 0.0;
    }
    const double turns = m_boardRuntimes[boardIndex].controller->status(nodeId).positionTurns;
    const double zeroTurns = turnAxis ? m_boardRuntimes[boardIndex].turnZeroTurns : m_boardRuntimes[boardIndex].driveZeroTurns;
    return (turns - zeroTurns) * mmPerTurn(boardIndex, turnAxis);
}

double MainWindow::axisAbsoluteTurns(int boardIndex, quint8 nodeId, double targetMm, bool turnAxis) const
{
    Q_UNUSED(nodeId);
    if (boardIndex < 0 || boardIndex >= m_boardRuntimes.size()) {
        return 0.0;
    }
    const double zeroTurns = turnAxis ? m_boardRuntimes[boardIndex].turnZeroTurns : m_boardRuntimes[boardIndex].driveZeroTurns;
    return zeroTurns + (targetMm / mmPerTurn(boardIndex, turnAxis));
}

double MainWindow::turnsToMm(double turns, bool turnAxis) const
{
    const int boardIndex = currentDebugBoardIndex();
    return turns * mmPerTurn(boardIndex, turnAxis);
}

double MainWindow::mmToTurns(double mm, bool turnAxis) const
{
    const int boardIndex = currentDebugBoardIndex();
    return mm / mmPerTurn(boardIndex, turnAxis);
}

double MainWindow::mmPerTurn(int boardIndex, bool turnAxis) const
{
    return boardScale(boardIndex, turnAxis);
}

double MainWindow::mmPerSecondToTurnsPerSecond(int boardIndex, double mmPerSecond, bool turnAxis) const
{
    return mmPerSecond / mmPerTurn(boardIndex, turnAxis);
}

void MainWindow::logMotionIssueOnce(const QString &key, const QString &message, int intervalMs)
{
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    const qint64 last = m_motionIssueLogTimes.value(key, 0);
    if (last > 0 && (now - last) < intervalMs) {
        return;
    }

    m_motionIssueLogTimes.insert(key, now);
    appendLog(message);
}

QString MainWindow::motionAxisKey(int boardIndex, quint8 nodeId, bool turnAxis) const
{
    return QStringLiteral("%1/%2/%3")
            .arg(boardIndex)
            .arg(nodeId)
            .arg(turnAxis ? QStringLiteral("turn") : QStringLiteral("drive"));
}

bool MainWindow::axisHasActiveErrors(int boardIndex, quint8 nodeId) const
{
    if (boardIndex < 0 || boardIndex >= m_boardRuntimes.size() || !m_boardRuntimes[boardIndex].controller) {
        return true;
    }

    const ODriveMotorController::AxisStatus &status = m_boardRuntimes[boardIndex].controller->status(nodeId);
    return status.axisError != 0
            || status.motorError != 0
            || status.encoderError != 0
            || status.controllerError != 0;
}

bool MainWindow::axisInClosedLoop(int boardIndex, quint8 nodeId) const
{
    if (boardIndex < 0 || boardIndex >= m_boardRuntimes.size() || !m_boardRuntimes[boardIndex].controller) {
        return false;
    }

    const ODriveMotorController::AxisStatus &status = m_boardRuntimes[boardIndex].controller->status(nodeId);
    return status.axisState == ODriveMotorController::ClosedLoopControl && status.axisError == 0;
}

bool MainWindow::stopAxisVelocity(int boardIndex, quint8 nodeId)
{
    if (boardIndex < 0 || boardIndex >= m_boardRuntimes.size() || nodeId > 63) {
        return false;
    }

    ODriveMotorController *controller = m_boardRuntimes[boardIndex].controller;
    if (!controller || !controller->isConnected()) {
        return false;
    }

    const ODriveMotorController::AxisStatus &status = controller->status(nodeId);
    if (status.axisError != 0 || status.motorError != 0 || status.encoderError != 0 || status.controllerError != 0) {
        return false;
    }

    if (status.currentControlMode != ODriveMotorController::VelocityControl
            || status.currentInputMode != ODriveMotorController::Passthrough) {
        controller->setControllerMode(nodeId,
                                      ODriveMotorController::VelocityControl,
                                      ODriveMotorController::Passthrough);
    }

    return controller->setVelocity(nodeId, 0.0f, 0.0f);
}

int MainWindow::connectedInterfaceCount() const
{
    return m_interfaceCountCombo ? qMax(1, m_interfaceCountCombo->currentData().toInt()) : 1;
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