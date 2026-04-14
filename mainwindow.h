#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QList>
#include <QString>
#include <QVector>
#include <QStringList>

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
    bool commandAxisVelocity(int boardIndex,
                             quint8 nodeId,
                             double speedMmPerSecond,
                             bool turnAxis);
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

    Ui::MainWindow *ui;
    ODriveMotorController *m_controller;
    QDialog *m_advancedDialog;

    QComboBox *m_pluginCombo;
    QComboBox *m_interfaceCountCombo;
    QSpinBox *m_bitrateSpin;
    QSpinBox *m_serialBaudSpin;
    QPushButton *m_refreshInterfacesButton;
    QPushButton *m_advancedConnectButton;
    QPushButton *m_connectButton;
    QLabel *m_linkStatusLabel;
    QLineEdit *m_ipEdit;
    QStringList m_availableInterfaces;
    QVector<BoardConfigRow> m_boardConfigRows;
    QVector<BoardRuntime> m_boardRuntimes;

    QPushButton *m_openAdvancedButton;
    QComboBox *m_debugBoardCombo;
    QComboBox *m_debugNodeCombo;
    QPushButton *m_rescanNodesButton;
    QLabel *m_activeNodesLabel;
    QDoubleSpinBox *m_debugScaleSpin;
    QLabel *m_frontLeftPositionLabel;
    QLabel *m_frontRightPositionLabel;
    QLabel *m_rearLeftPositionLabel;
    QLabel *m_rearRightPositionLabel;
    QPushButton *m_setZeroButton;
    QDoubleSpinBox *m_homeSpeedSpin;
    QDoubleSpinBox *m_jogSpeedSpin;
    QPushButton *m_forwardButton;
    QPushButton *m_backwardButton;
    QPushButton *m_leftButton;
    QPushButton *m_rightButton;
    QDoubleSpinBox *m_scanAxisLengthSpin;
    QDoubleSpinBox *m_scanSpeedSpin;
    QDoubleSpinBox *m_stepAxisLengthSpin;
    QDoubleSpinBox *m_stepLengthSpin;
    QComboBox *m_scanModeCombo;
    QPushButton *m_homeButton;
    QPushButton *m_resetButton;
    QPushButton *m_enableButton;
    QPushButton *m_scanStartButton;
    QPushButton *m_stopButton;
    QLabel *m_programStatusLabel;

    QComboBox *m_controlModeCombo;
    QComboBox *m_inputModeCombo;
    QPushButton *m_applyModeButton;

    QPushButton *m_idleButton;
    QPushButton *m_motorCalibrationButton;
    QPushButton *m_fullCalibrationButton;
    QPushButton *m_closedLoopButton;
    QPushButton *m_estopButton;
    QPushButton *m_clearErrorsButton;
    QPushButton *m_refreshStatusButton;

    QDoubleSpinBox *m_positionSpin;
    QDoubleSpinBox *m_positionVelFfSpin;
    QDoubleSpinBox *m_positionTorqueFfSpin;
    QPushButton *m_sendPositionButton;

    QDoubleSpinBox *m_velocitySpin;
    QDoubleSpinBox *m_velocityTorqueFfSpin;
    QPushButton *m_sendVelocityButton;

    QDoubleSpinBox *m_torqueSpin;
    QPushButton *m_sendTorqueButton;

    QDoubleSpinBox *m_velocityLimitSpin;
    QDoubleSpinBox *m_currentLimitSpin;
    QPushButton *m_applyLimitsButton;

    QLabel *m_axisErrorValue;
    QLabel *m_axisErrorDetailValue;
    QLabel *m_activeErrorsValue;
    QLabel *m_activeErrorsDetailValue;
    QLabel *m_disarmReasonValue;
    QLabel *m_disarmReasonDetailValue;
    QLabel *m_axisStateValue;
    QLabel *m_procedureResultValue;
    QLabel *m_trajectoryDoneValue;
    QLabel *m_positionValue;
    QLabel *m_velocityValue;
    QLabel *m_busVoltageValue;
    QLabel *m_busCurrentValue;
    QLabel *m_iqValue;
    QLabel *m_temperatureValue;
    QLabel *m_torqueValue;
    QLabel *m_powerValue;
    QLabel *m_lastUpdateValue;

    QPlainTextEdit *m_logEdit;
    QPlainTextEdit *m_rxEdit;

    QLineEdit *m_txIdEdit;
    QSpinBox *m_txDlcSpin;
    QCheckBox *m_txExtendedCheck;
    QCheckBox *m_txRemoteCheck;
    QSpinBox *m_txPeriodSpin;
    QPushButton *m_txSendButton;
    QPushButton *m_txToggleButton;
    QLabel *m_txStatusLabel;
    QCheckBox *m_txErrorCheck;
    QVector<QLineEdit *> m_txByteEdits;
    QTimer *m_txTimer;
    QTimer *m_txResponseTimer;
    QTimer *m_logFlushTimer;
    QTimer *m_rxFlushTimer;
    QTimer *m_statusUpdateTimer;
    QTimer *m_settingsSaveTimer;
    QStringList m_pendingLogLines;
    QStringList m_pendingRxLines;
    QString m_lastLogMessage;
    QString m_lastRxMessage;
    quint32 m_lastTxFrameId;
    quint8 m_lastTxNodeId;
    int m_lastTxBoardIndex;
    bool m_waitingForTxResponse;
    ScanState m_scanState;
};

#endif // MAINWINDOW_H
