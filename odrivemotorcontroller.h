#ifndef ODRIVEMOTORCONTROLLER_H
#define ODRIVEMOTORCONTROLLER_H

#include <QObject>
#include <QCanBusDevice>
#include <QDateTime>
#include <QHash>
#include <QList>
#include <QStringList>
#include <QTimer>

class QSerialPort;
class QProcess;

class ODriveMotorController : public QObject
{
    Q_OBJECT

public:
    enum CommandId {
        Heartbeat = 0x01,
        EstopCommand = 0x02,
        GetError = 0x03,
        SetAxisState = 0x07,
        GetEncoderEstimates = 0x09,
        SetControllerMode = 0x0B,
        SetInputPos = 0x0C,
        SetInputVel = 0x0D,
        SetInputTorque = 0x0E,
        SetLimits = 0x0F,
        GetIq = 0x14,
        GetTemperature = 0x15,
        GetBusVoltageCurrent = 0x17,
        ClearErrors = 0x18,
        GetTorques = 0x1C,
        GetPowers = 0x1D
    };
    Q_ENUM(CommandId)

    enum AxisState {
        Undefined = 0,
        Idle = 1,
        StartupSequence = 2,
        FullCalibrationSequence = 3,
        MotorCalibration = 4,
        EncoderIndexSearch = 6,
        EncoderOffsetCalibration = 7,
        ClosedLoopControl = 8,
        LockinSpin = 9,
        EncoderDirFind = 10,
        Homing = 11,
        EncoderHallPolarityCalibration = 12,
        EncoderHallPhaseCalibration = 13,
        AnticoggingCalibration = 14,
        HarmonicCalibration = 15,
        HarmonicCalibrationCommutation = 16
    };
    Q_ENUM(AxisState)

    enum ControlMode {
        VoltageControl = 0,
        TorqueControl = 1,
        VelocityControl = 2,
        PositionControl = 3
    };
    Q_ENUM(ControlMode)

    enum InputMode {
        Inactive = 0,
        Passthrough = 1,
        VelRamp = 2,
        PosFilter = 3,
        MixChannels = 4,
        TrapTraj = 5,
        TorqueRamp = 6,
        Mirror = 7,
        Tuning = 8
    };
    Q_ENUM(InputMode)

    struct AxisStatus
    {
        quint32 axisError = 0;
        quint32 activeErrors = 0;
        quint32 disarmReason = 0;
        quint8 axisState = Undefined;
        quint8 procedureResult = 0;
        bool trajectoryDone = false;
        float positionTurns = 0.0f;
        float velocityTurnsPerSecond = 0.0f;
        float iqSetpointAmps = 0.0f;
        float iqMeasuredAmps = 0.0f;
        float fetTemperatureCelsius = 0.0f;
        float motorTemperatureCelsius = 0.0f;
        float busVoltageVolts = 0.0f;
        float busCurrentAmps = 0.0f;
        float torqueTargetNm = 0.0f;
        float torqueEstimateNm = 0.0f;
        float electricalPowerWatts = 0.0f;
        float mechanicalPowerWatts = 0.0f;
        QDateTime lastHeartbeat;
        QDateTime lastMessage;
    };

    explicit ODriveMotorController(QObject *parent = nullptr);
    ~ODriveMotorController() override;

    QStringList availablePlugins() const;
    QStringList availableInterfaces(const QString &plugin, QString *errorMessage = nullptr) const;

    bool connectDevice(const QString &plugin,
                       const QString &interfaceName,
                       int bitrate,
                       int serialBaudRate,
                       quint8 nodeId);
    void disconnectDevice();

    bool isConnected() const;
    quint8 nodeId() const;
    void setDefaultNodeId(quint8 nodeId);

    const AxisStatus &status() const;
    const AxisStatus &status(quint8 nodeId) const;
    void setTrackedNodeIds(const QList<quint8> &nodeIds);
    QList<quint8> trackedNodeIds() const;
    QList<quint8> recentResponsiveNodeIds(int maxAgeMs = 2000) const;

    bool requestAxisState(AxisState state);
    bool requestAxisState(quint8 nodeId, AxisState state);
    bool requestIdle();
    bool requestIdle(quint8 nodeId);
    bool requestClosedLoop();
    bool requestClosedLoop(quint8 nodeId);
    bool requestMotorCalibration();
    bool requestMotorCalibration(quint8 nodeId);
    bool requestFullCalibration();
    bool requestFullCalibration(quint8 nodeId);

    bool setControllerMode(ControlMode controlMode, InputMode inputMode);
    bool setControllerMode(quint8 nodeId, ControlMode controlMode, InputMode inputMode);
    bool setPosition(float positionTurns,
                     float velocityFeedforwardTurnsPerSecond = 0.0f,
                     float torqueFeedforwardNm = 0.0f);
    bool setPosition(quint8 nodeId,
                     float positionTurns,
                     float velocityFeedforwardTurnsPerSecond = 0.0f,
                     float torqueFeedforwardNm = 0.0f);
    bool setVelocity(float velocityTurnsPerSecond, float torqueFeedforwardNm = 0.0f);
    bool setVelocity(quint8 nodeId,
                     float velocityTurnsPerSecond,
                     float torqueFeedforwardNm = 0.0f);
    bool setTorque(float torqueNm);
    bool setTorque(quint8 nodeId, float torqueNm);
    bool setLimits(float velocityLimitTurnsPerSecond, float currentLimitAmps);
    bool setLimits(quint8 nodeId, float velocityLimitTurnsPerSecond, float currentLimitAmps);

    bool estop();
    bool estop(quint8 nodeId);
    bool clearErrors(bool identify = false);
    bool clearErrors(quint8 nodeId, bool identify = false);
    void requestAllTelemetry(bool quiet = true);
    void requestAllTelemetry(quint8 nodeId, bool quiet = true);
    void requestTrackedTelemetry(bool quiet = true);
    void probeNode(quint8 nodeId);
    void probeAllNodes();
    bool sendRawCanFrame(quint32 frameId,
                         const QByteArray &payload,
                         bool extendedFrame,
                         bool remoteFrame,
                         bool quiet = false);

    static QString axisStateName(quint8 axisState);
    static QString procedureResultName(quint8 procedureResult);

signals:
    void connectionChanged(bool connected);
    void statusUpdated();
    void nodeStatusUpdated(quint8 nodeId);
    void logMessage(const QString &message);
    void rxMessage(const QString &message);
    void errorMessage(const QString &message);

private slots:
    void readFrames();
    void readSlcanFrames();
    void readCandleOutput();
    void handleDeviceError(QCanBusDevice::CanBusError error);
    void handleDeviceStateChange(QCanBusDevice::CanBusDeviceState state);

private:
    bool connectSlcanDevice(const QString &interfaceName, int bitrate, int serialBaudRate);
    bool connectCandleDevice(const QString &interfaceName, int bitrate);
    bool isSlcanPlugin() const;
    bool isCandlePlugin() const;
    bool isCanBusConnected() const;
    bool isSlcanConnected() const;
    bool isCandleConnected() const;
    bool sendFrame(quint8 nodeId, CommandId commandId, const QByteArray &payload, bool quiet = false);
    bool requestRemoteFrame(quint8 nodeId, CommandId commandId, bool quiet = true);
    bool slcanWriteFrame(quint32 frameId, const QByteArray &payload, bool remoteFrame, bool quiet, const QString &label, bool extendedFrame = false);
    bool slcanWriteRaw(const QByteArray &command, bool quiet = false);
    bool slcanWriteCommand(const QByteArray &command,
                           QByteArray *response = nullptr,
                           int timeoutMs = 300,
                           bool requireAck = true);
    bool candleWriteFrame(quint32 frameId, const QByteArray &payload, bool remoteFrame, bool quiet, const QString &label, bool extendedFrame = false);
    bool candleWriteCommand(const QByteArray &jsonLine, bool quiet = false);
    static QByteArray slcanBitrateCommand(int bitrate);
    static QList<int> slcanProbeBaudRates(int preferredBaudRate);
    static QByteArray encodeSlcanFrame(quint32 frameId, const QByteArray &payload, bool remoteFrame, bool extendedFrame);
    bool processSlcanLine(const QByteArray &line);
    bool processCandleLine(const QByteArray &line);
    bool processFrame(quint8 nodeId, quint32 frameId, const QByteArray &payload);
    quint32 frameIdFor(quint8 nodeId, CommandId commandId) const;
    void clearDevice();
    void clearSerialPort();
    void clearCandleProcess();
    void setConnectedState(bool connected);
    static QString commandName(CommandId commandId);
    static int expectedReplyLength(CommandId commandId);

    static void appendUInt32(QByteArray &payload, quint32 value);
    static void appendInt16(QByteArray &payload, qint16 value);
    static void appendFloat(QByteArray &payload, float value);
    static quint32 readUInt32(const QByteArray &payload, int offset);
    static float readFloat(const QByteArray &payload, int offset);

    QCanBusDevice *m_device;
    QSerialPort *m_serialPort;
    QProcess *m_candleProcess;
    QTimer m_pollTimer;
    QTimer m_slcanReadTimer;
    quint8 m_nodeId;
    QList<quint8> m_trackedNodeIds;
    bool m_connected;
    QString m_activePlugin;
    QByteArray m_slcanReadBuffer;
    QByteArray m_candleReadBuffer;
    bool m_slcanProtocolMismatchWarned = false;
    AxisStatus m_emptyStatus;
    QHash<quint8, AxisStatus> m_statusByNode;
};

#endif // ODRIVEMOTORCONTROLLER_H
