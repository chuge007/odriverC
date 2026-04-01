#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class QComboBox;
class QDialog;
class QDoubleSpinBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QSpinBox;
class QTextEdit;
class QCheckBox;
class QVBoxLayout;
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

private:
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
    void setMotionControlsEnabled(bool enabled);
    void updateTrackedNodes();
    void syncConnectionEditorsFromAdvanced();
    void updateConnectionEditorHints();
    void updateLinkStatusDisplay();
    bool hasAnyTrackedNodeResponse() const;
    bool hasFreshTrackedHeartbeat() const;
    void updateProgramStatus(const QString &message);
    void advanceScanStateMachine();
    void startScanSweep(double targetMm);
    bool ensureAxisClosedLoop(quint8 nodeId);
    bool commandAxisVelocity(quint8 nodeId, double speedMmPerSecond, bool turnAxis);
    bool commandAxisPosition(quint8 nodeId,
                             double targetMm,
                             double speedMmPerSecond,
                             bool turnAxis);
    void stopAllMotion(bool requestIdleState);
    bool axisReached(quint8 nodeId, double targetMm, bool turnAxis, double toleranceMm = 0.8) const;
    double axisPositionMm(quint8 nodeId, bool turnAxis) const;
    double axisAbsoluteTurns(quint8 nodeId, double targetMm, bool turnAxis) const;
    double turnsToMm(double turns, bool turnAxis) const;
    double mmToTurns(double mm, bool turnAxis) const;
    double mmPerTurn(bool turnAxis) const;
    double mmPerSecondToTurnsPerSecond(double mmPerSecond, bool turnAxis) const;
    quint8 turnNodeId() const;
    quint8 driveNodeId() const;
    double currentLimitAmps() const;
    static QLabel *createValueLabel();
    static QLabel *createReadoutLabel();

    Ui::MainWindow *ui;
    ODriveMotorController *m_controller;
    QDialog *m_advancedDialog;

    QComboBox *m_pluginCombo;
    QComboBox *m_interfaceCombo;
    QSpinBox *m_bitrateSpin;
    QSpinBox *m_serialBaudSpin;
    QSpinBox *m_nodeIdSpin;
    QPushButton *m_refreshInterfacesButton;
    QPushButton *m_connectButton;
    QLabel *m_linkStatusLabel;
    QLineEdit *m_ipEdit;

    QPushButton *m_openAdvancedButton;
    QLabel *m_turnPositionLabel;
    QLabel *m_drivePositionLabel;
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

    QSpinBox *m_turnNodeIdSpin;
    QSpinBox *m_driveNodeIdSpin;
    QDoubleSpinBox *m_turnScaleSpin;
    QDoubleSpinBox *m_driveScaleSpin;

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
    QLabel *m_activeErrorsValue;
    QLabel *m_disarmReasonValue;
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

    QTextEdit *m_logEdit;
    QTextEdit *m_rxEdit;

    QLineEdit *m_txIdEdit;
    QSpinBox *m_txDlcSpin;
    QLineEdit *m_txDataEdit;
    QCheckBox *m_txExtendedCheck;
    QCheckBox *m_txRemoteCheck;
    QSpinBox *m_txPeriodSpin;
    QPushButton *m_txSendButton;
    QPushButton *m_txToggleButton;
    QTimer *m_txTimer;

    double m_turnZeroTurns;
    double m_driveZeroTurns;
    bool m_zeroPointInitialized;
    ScanState m_scanState;
};

#endif // MAINWINDOW_H
