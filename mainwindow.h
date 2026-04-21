#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QList>
#include <QString>
#include <QVector>
#include <QStringList>
#include <QMap>

class QComboBox;
class QDialog;
class QDoubleSpinBox;
class QGridLayout;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QSpinBox;
class QCheckBox;
class QTimer;
class QVBoxLayout;
class QWidget;
class ODriveMotorController;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void refreshCanInterfaces();
    void toggleConnection();
    void updateConnectionState(bool connected);
    void updateStatusPanel();
    void appendLog(const QString &message);
    void appendRxMessage(const QString &message);
    void sendCustomTxOnce();
    void toggleCustomTx();
    void requestIdle();
    void requestMotorCalibration();
    void requestFullCalibration();
    void requestClosedLoop();
    void applyControllerMode();
    void sendPositionCommand();
    void sendVelocityCommand();
    void sendTorqueCommand();
    void applyLimits();
    void readPidParams();
    void applyPidParams();
    void runDiagnoseFix();
    void initializeMachine();
    void refreshTelemetry();
    void openAdvancedPanel();
    void syncAxisReadouts();
    void setZeroPoint();
    void sendHome();
    void resetMotionState();
    void enableAxes();
    void startScanProgram();
    void stopProgram();
    void stepAdvanceOnce();
    void handleNodeStatusUpdate(quint8 nodeId);
    void flushQueuedMessages();
    void flushQueuedRxMessages();
    void scheduleStatusPanelUpdate();
    void scheduleSaveSettings();
    void handleCustomTxRxMessage(const QString &message);
    void handleCustomTxResponseTimeout();
    void scanForActiveNodes();
    void updateDetectedNodesList();
    void switchToDetectedNode(quint8 nodeId);

private:
    struct BoardConfigRow
    {
        QLabel *label = nullptr;
        QComboBox *interfaceCombo = nullptr;
        QPushButton *scanNodesButton = nullptr;
        QComboBox *turnNodeCombo = nullptr;
        QComboBox *driveNodeCombo = nullptr;
        QDoubleSpinBox *turnScaleSpin = nullptr;
        QDoubleSpinBox *driveScaleSpin = nullptr;
        QLabel *statusLabel = nullptr;
        QList<quint8> detectedNodes;
        QString preferredInterfaceName;
        int preferredTurnNodeId = 16;
        int preferredDriveNodeId = 1;
    };

    struct BoardRuntime
    {
        ODriveMotorController *controller = nullptr;
        QList<quint8> detectedNodes;
        double turnZeroTurns = 0.0;
        double driveZeroTurns = 0.0;
        bool zeroPointInitialized = false;
        bool connected = false;
        bool ownsController = false;
    };

    enum ScanModeOption {
        BowMode = 0,
        OneWayMode = 1
    };

    enum ScanPhase {
        ScanIdle,
        ScanHoming,
        ScanSweeping,
        ScanStepping,
        ScanReturning
    };

    struct ScanState
    {
        bool active = false;
        ScanPhase phase = ScanIdle;
        bool forward = true;
        double scanTargetMm = 0.0;
        double stepTargetMm = 0.0;
        double currentStepMm = 0.0;
        double maxStepMm = 0.0;
    };

    void buildUi();
    void buildMainInterface(QVBoxLayout *mainLayout);
    void buildAdvancedDialog();
    void buildAdvancedContent(QVBoxLayout *mainLayout, QWidget *parent);
    void populatePlugins();
    void populateControlModes();
    void populateInputModes();
    void buildBoardConfigRows(QGridLayout *layout, QWidget *parent);
    void loadSettings();
    void saveSettings() const;
    void connectSettingsPersistence();
    void setMotionControlsEnabled(bool enabled);
    void refreshBoardInterfaceOptions();
    void updateBoardRowVisibility();
    void updateBoardNodeCombos(int boardIndex);
    void updateAllBoardNodeCombos();
    void updateBoardStatusLabel(int boardIndex, const QString &statusText = QString());
    void updateDebugBoardCombo();
    void updateDebugNodeCombo();
    void updateDebugScaleSpin();
    void refreshCurrentBoardNodes();
    void refreshBoardNodes(int boardIndex, bool temporaryConnectionAllowed = true);
    void connectBoardControllerSignals(int boardIndex, ODriveMotorController *controller);
    void disconnectAllBoards();
    void updateConnectionStateFromBoards();
    void syncConnectionEditorsFromAdvanced();
    void updateConnectionEditorHints();
    void updateLinkStatusDisplay();
    bool hasAnyConnectedBoard() const;
    QList<int> configuredBoardIndices() const;
    QList<int> connectedBoardIndices() const;
    QString boardInterfaceName(int boardIndex) const;
    QString boardDisplayName(int boardIndex) const;
    QString boardLogPrefix(int boardIndex) const;
    int currentDebugBoardIndex() const;
    quint8 currentDebugNodeId() const;
    ODriveMotorController *currentDebugController() const;
    int boardIndexForController(const ODriveMotorController *controller) const;
    quint8 comboNodeValue(const QComboBox *combo, quint8 fallback = 0) const;
    quint8 boardTurnNodeId(int boardIndex) const;
    quint8 boardDriveNodeId(int boardIndex) const;
    double boardScale(int boardIndex, bool turnAxis) const;
    bool hasAnyTrackedNodeResponse() const;
    bool hasFreshTrackedHeartbeat() const;
    void updateProgramStatus(const QString &message);
    void advanceScanStateMachine();
    void startScanSweep(double targetMm);
    bool ensureAxisClosedLoop(int boardIndex, quint8 nodeId);
    bool prepareAxisVelocityControl(int boardIndex,
                                    quint8 nodeId,
                                    double speedMmPerSecond,
                                    bool turnAxis);
    bool sendPreparedAxisVelocity(int boardIndex,
                                  quint8 nodeId,
                                  double speedMmPerSecond,
                                  bool turnAxis);
    bool commandWheelVelocity(int boardIndex, bool leftWheel, double speedMmPerSecond);
    bool commandAxisVelocity(int boardIndex,
                             quint8 nodeId,
                             double speedMmPerSecond,
                             bool turnAxis);
    void logMotionIssueOnce(const QString &key,
                            const QString &message,
                            int intervalMs = 1500);
    QString motionAxisKey(int boardIndex, quint8 nodeId, bool turnAxis) const;
    bool axisHasActiveErrors(int boardIndex, quint8 nodeId) const;
    bool axisInClosedLoop(int boardIndex, quint8 nodeId) const;
    bool stopAxisVelocity(int boardIndex, quint8 nodeId);
    bool commandAxisPosition(int boardIndex,
                             quint8 nodeId,
                             double targetMm,
                             double speedMmPerSecond,
                             bool turnAxis);
    void stopAllMotion(bool requestIdleState);
    bool axisReached(int boardIndex,
                     quint8 nodeId,
                     double targetMm,
                     bool turnAxis,
                     double toleranceMm = 0.8) const;
    double axisPositionMm(int boardIndex, quint8 nodeId, bool turnAxis) const;
    double axisAbsoluteTurns(int boardIndex, quint8 nodeId, double targetMm, bool turnAxis) const;
    double turnsToMm(double turns, bool turnAxis) const;
    double mmToTurns(double mm, bool turnAxis) const;
    double mmPerTurn(int boardIndex, bool turnAxis) const;
    double mmPerSecondToTurnsPerSecond(int boardIndex, double mmPerSecond, bool turnAxis) const;
    int connectedInterfaceCount() const;
    double currentLimitAmps() const;
    static QLabel *createValueLabel();
    static QLabel *createReadoutLabel();

    Ui::MainWindow *ui = nullptr;
    ODriveMotorController *m_controller = nullptr;
    QDialog *m_advancedDialog = nullptr;

    QComboBox *m_pluginCombo = nullptr;
    QComboBox *m_interfaceCountCombo = nullptr;
    QSpinBox *m_bitrateSpin = nullptr;
    QSpinBox *m_serialBaudSpin = nullptr;
    QPushButton *m_refreshInterfacesButton = nullptr;
    QPushButton *m_advancedConnectButton = nullptr;
    QPushButton *m_connectButton = nullptr;
    QLabel *m_linkStatusLabel = nullptr;
    QLineEdit *m_ipEdit = nullptr;
    QStringList m_availableInterfaces;
    QVector<BoardConfigRow> m_boardConfigRows;
    QVector<BoardRuntime> m_boardRuntimes;

    QPushButton *m_openAdvancedButton = nullptr;
    QComboBox *m_debugBoardCombo = nullptr;
    QComboBox *m_debugNodeCombo = nullptr;
    QPushButton *m_rescanNodesButton = nullptr;
    QLabel *m_activeNodesLabel = nullptr;
    QDoubleSpinBox *m_debugScaleSpin = nullptr;
    QLabel *m_frontLeftPositionLabel = nullptr;
    QLabel *m_frontRightPositionLabel = nullptr;
    QLabel *m_rearLeftPositionLabel = nullptr;
    QLabel *m_rearRightPositionLabel = nullptr;
    QPushButton *m_setZeroButton = nullptr;
    QDoubleSpinBox *m_homeSpeedSpin = nullptr;
    QDoubleSpinBox *m_jogSpeedSpin = nullptr;
    QPushButton *m_forwardButton = nullptr;
    QPushButton *m_backwardButton = nullptr;
    QPushButton *m_leftButton = nullptr;
    QPushButton *m_rightButton = nullptr;
    QDoubleSpinBox *m_scanAxisLengthSpin = nullptr;
    QDoubleSpinBox *m_scanSpeedSpin = nullptr;
    QDoubleSpinBox *m_stepAxisLengthSpin = nullptr;
    QDoubleSpinBox *m_stepLengthSpin = nullptr;
    QComboBox *m_scanModeCombo = nullptr;
    QPushButton *m_homeButton = nullptr;
    QPushButton *m_resetButton = nullptr;
    QPushButton *m_enableButton = nullptr;
    QPushButton *m_scanStartButton = nullptr;
    QPushButton *m_stopButton = nullptr;
    QLabel *m_programStatusLabel = nullptr;

    QComboBox *m_controlModeCombo = nullptr;
    QComboBox *m_inputModeCombo = nullptr;
    QPushButton *m_applyModeButton = nullptr;

    QPushButton *m_idleButton = nullptr;
    QPushButton *m_motorCalibrationButton = nullptr;
    QPushButton *m_fullCalibrationButton = nullptr;
    QPushButton *m_closedLoopButton = nullptr;
    QPushButton *m_estopButton = nullptr;
    QPushButton *m_clearErrorsButton = nullptr;
    QPushButton *m_refreshStatusButton = nullptr;
    QPushButton *m_diagnoseFixButton = nullptr;
    QPushButton *m_initializeMachineButton = nullptr;

    QDoubleSpinBox *m_positionSpin = nullptr;
    QDoubleSpinBox *m_positionVelFfSpin = nullptr;
    QDoubleSpinBox *m_positionTorqueFfSpin = nullptr;
    QPushButton *m_sendPositionButton = nullptr;

    QDoubleSpinBox *m_velocitySpin = nullptr;
    QDoubleSpinBox *m_velocityTorqueFfSpin = nullptr;
    QPushButton *m_sendVelocityButton = nullptr;

    QDoubleSpinBox *m_torqueSpin = nullptr;
    QPushButton *m_sendTorqueButton = nullptr;

    QDoubleSpinBox *m_velocityLimitSpin = nullptr;
    QDoubleSpinBox *m_currentLimitSpin = nullptr;
    QPushButton *m_applyLimitsButton = nullptr;

    QDoubleSpinBox *m_posGainSpin = nullptr;
    QDoubleSpinBox *m_velGainSpin = nullptr;
    QDoubleSpinBox *m_velIntegratorGainSpin = nullptr;
    QDoubleSpinBox *m_velLimitToleranceSpin = nullptr;
    QDoubleSpinBox *m_inputFilterBandwidthSpin = nullptr;
    QDoubleSpinBox *m_inertiaSpin = nullptr;
    QDoubleSpinBox *m_trajVelLimitSpin = nullptr;
    QDoubleSpinBox *m_trajAccelLimitSpin = nullptr;
    QDoubleSpinBox *m_trajDecelLimitSpin = nullptr;
    QDoubleSpinBox *m_torqueRampRateSpin = nullptr;
    QDoubleSpinBox *m_torqueLimitSpin = nullptr;
    QCheckBox *m_enableVelLimitCheck = nullptr;
    QCheckBox *m_enableTorqueModeVelLimitCheck = nullptr;
    QPushButton *m_readPidButton = nullptr;
    QPushButton *m_applyPidButton = nullptr;

    QLabel *m_axisErrorValue = nullptr;
    QLabel *m_axisErrorDetailValue = nullptr;
    QLabel *m_activeErrorsValue = nullptr;
    QLabel *m_activeErrorsDetailValue = nullptr;
    QLabel *m_disarmReasonValue = nullptr;
    QLabel *m_disarmReasonDetailValue = nullptr;
    QLabel *m_axisStateValue = nullptr;
    QLabel *m_procedureResultValue = nullptr;
    QLabel *m_trajectoryDoneValue = nullptr;
    QLabel *m_positionValue = nullptr;
    QLabel *m_velocityValue = nullptr;
    QLabel *m_busVoltageValue = nullptr;
    QLabel *m_busCurrentValue = nullptr;
    QLabel *m_iqValue = nullptr;
    QLabel *m_temperatureValue = nullptr;
    QLabel *m_torqueValue = nullptr;
    QLabel *m_powerValue = nullptr;
    QLabel *m_lastUpdateValue = nullptr;

    QPlainTextEdit *m_logEdit = nullptr;
    QPlainTextEdit *m_rxEdit = nullptr;

    QLineEdit *m_txIdEdit = nullptr;
    QSpinBox *m_txDlcSpin = nullptr;
    QCheckBox *m_txExtendedCheck = nullptr;
    QCheckBox *m_txRemoteCheck = nullptr;
    QSpinBox *m_txPeriodSpin = nullptr;
    QPushButton *m_txSendButton = nullptr;
    QPushButton *m_txToggleButton = nullptr;
    QLabel *m_txStatusLabel = nullptr;
    QCheckBox *m_txErrorCheck = nullptr;
    QVector<QLineEdit *> m_txByteEdits;
    QTimer *m_txTimer = nullptr;
    QTimer *m_txResponseTimer = nullptr;
    QTimer *m_logFlushTimer = nullptr;
    QTimer *m_rxFlushTimer = nullptr;
    QTimer *m_statusUpdateTimer = nullptr;
    QTimer *m_settingsSaveTimer = nullptr;
    QStringList m_pendingLogLines;
    QStringList m_pendingRxLines;
    QString m_lastLogMessage;
    QString m_lastRxMessage;
    quint32 m_lastTxFrameId = 0;
    quint8 m_lastTxNodeId = 0;
    int m_lastTxBoardIndex = -1;
    bool m_waitingForTxResponse = false;
    bool m_forwardPressed = false;
    bool m_backwardPressed = false;
    bool m_leftPressed = false;
    bool m_rightPressed = false;
    int m_motionCommandGeneration = 0;
    ScanState m_scanState;
    bool m_updatingDebugNodeCombo = false;
    bool m_switchingDebugNode = false;
    QMap<QString, qint64> m_motionIssueLogTimes;
};

#endif // MAINWINDOW_H