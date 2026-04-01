/****************************************************************************
** Meta object code from reading C++ file 'odrivemotorcontroller.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.12.4)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../odrivemotorcontroller.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'odrivemotorcontroller.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.12.4. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_ODriveMotorController_t {
    QByteArrayData data[69];
    char stringdata0[985];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_ODriveMotorController_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_ODriveMotorController_t qt_meta_stringdata_ODriveMotorController = {
    {
QT_MOC_LITERAL(0, 0, 21), // "ODriveMotorController"
QT_MOC_LITERAL(1, 22, 17), // "connectionChanged"
QT_MOC_LITERAL(2, 40, 0), // ""
QT_MOC_LITERAL(3, 41, 9), // "connected"
QT_MOC_LITERAL(4, 51, 13), // "statusUpdated"
QT_MOC_LITERAL(5, 65, 17), // "nodeStatusUpdated"
QT_MOC_LITERAL(6, 83, 6), // "nodeId"
QT_MOC_LITERAL(7, 90, 10), // "logMessage"
QT_MOC_LITERAL(8, 101, 7), // "message"
QT_MOC_LITERAL(9, 109, 9), // "rxMessage"
QT_MOC_LITERAL(10, 119, 12), // "errorMessage"
QT_MOC_LITERAL(11, 132, 10), // "readFrames"
QT_MOC_LITERAL(12, 143, 15), // "readSlcanFrames"
QT_MOC_LITERAL(13, 159, 16), // "readCandleOutput"
QT_MOC_LITERAL(14, 176, 17), // "handleDeviceError"
QT_MOC_LITERAL(15, 194, 26), // "QCanBusDevice::CanBusError"
QT_MOC_LITERAL(16, 221, 5), // "error"
QT_MOC_LITERAL(17, 227, 23), // "handleDeviceStateChange"
QT_MOC_LITERAL(18, 251, 32), // "QCanBusDevice::CanBusDeviceState"
QT_MOC_LITERAL(19, 284, 5), // "state"
QT_MOC_LITERAL(20, 290, 9), // "CommandId"
QT_MOC_LITERAL(21, 300, 9), // "Heartbeat"
QT_MOC_LITERAL(22, 310, 12), // "EstopCommand"
QT_MOC_LITERAL(23, 323, 8), // "GetError"
QT_MOC_LITERAL(24, 332, 12), // "SetAxisState"
QT_MOC_LITERAL(25, 345, 19), // "GetEncoderEstimates"
QT_MOC_LITERAL(26, 365, 17), // "SetControllerMode"
QT_MOC_LITERAL(27, 383, 11), // "SetInputPos"
QT_MOC_LITERAL(28, 395, 11), // "SetInputVel"
QT_MOC_LITERAL(29, 407, 14), // "SetInputTorque"
QT_MOC_LITERAL(30, 422, 9), // "SetLimits"
QT_MOC_LITERAL(31, 432, 5), // "GetIq"
QT_MOC_LITERAL(32, 438, 14), // "GetTemperature"
QT_MOC_LITERAL(33, 453, 20), // "GetBusVoltageCurrent"
QT_MOC_LITERAL(34, 474, 11), // "ClearErrors"
QT_MOC_LITERAL(35, 486, 10), // "GetTorques"
QT_MOC_LITERAL(36, 497, 9), // "GetPowers"
QT_MOC_LITERAL(37, 507, 9), // "AxisState"
QT_MOC_LITERAL(38, 517, 9), // "Undefined"
QT_MOC_LITERAL(39, 527, 4), // "Idle"
QT_MOC_LITERAL(40, 532, 15), // "StartupSequence"
QT_MOC_LITERAL(41, 548, 23), // "FullCalibrationSequence"
QT_MOC_LITERAL(42, 572, 16), // "MotorCalibration"
QT_MOC_LITERAL(43, 589, 18), // "EncoderIndexSearch"
QT_MOC_LITERAL(44, 608, 24), // "EncoderOffsetCalibration"
QT_MOC_LITERAL(45, 633, 17), // "ClosedLoopControl"
QT_MOC_LITERAL(46, 651, 10), // "LockinSpin"
QT_MOC_LITERAL(47, 662, 14), // "EncoderDirFind"
QT_MOC_LITERAL(48, 677, 6), // "Homing"
QT_MOC_LITERAL(49, 684, 30), // "EncoderHallPolarityCalibration"
QT_MOC_LITERAL(50, 715, 27), // "EncoderHallPhaseCalibration"
QT_MOC_LITERAL(51, 743, 22), // "AnticoggingCalibration"
QT_MOC_LITERAL(52, 766, 19), // "HarmonicCalibration"
QT_MOC_LITERAL(53, 786, 30), // "HarmonicCalibrationCommutation"
QT_MOC_LITERAL(54, 817, 11), // "ControlMode"
QT_MOC_LITERAL(55, 829, 14), // "VoltageControl"
QT_MOC_LITERAL(56, 844, 13), // "TorqueControl"
QT_MOC_LITERAL(57, 858, 15), // "VelocityControl"
QT_MOC_LITERAL(58, 874, 15), // "PositionControl"
QT_MOC_LITERAL(59, 890, 9), // "InputMode"
QT_MOC_LITERAL(60, 900, 8), // "Inactive"
QT_MOC_LITERAL(61, 909, 11), // "Passthrough"
QT_MOC_LITERAL(62, 921, 7), // "VelRamp"
QT_MOC_LITERAL(63, 929, 9), // "PosFilter"
QT_MOC_LITERAL(64, 939, 11), // "MixChannels"
QT_MOC_LITERAL(65, 951, 8), // "TrapTraj"
QT_MOC_LITERAL(66, 960, 10), // "TorqueRamp"
QT_MOC_LITERAL(67, 971, 6), // "Mirror"
QT_MOC_LITERAL(68, 978, 6) // "Tuning"

    },
    "ODriveMotorController\0connectionChanged\0"
    "\0connected\0statusUpdated\0nodeStatusUpdated\0"
    "nodeId\0logMessage\0message\0rxMessage\0"
    "errorMessage\0readFrames\0readSlcanFrames\0"
    "readCandleOutput\0handleDeviceError\0"
    "QCanBusDevice::CanBusError\0error\0"
    "handleDeviceStateChange\0"
    "QCanBusDevice::CanBusDeviceState\0state\0"
    "CommandId\0Heartbeat\0EstopCommand\0"
    "GetError\0SetAxisState\0GetEncoderEstimates\0"
    "SetControllerMode\0SetInputPos\0SetInputVel\0"
    "SetInputTorque\0SetLimits\0GetIq\0"
    "GetTemperature\0GetBusVoltageCurrent\0"
    "ClearErrors\0GetTorques\0GetPowers\0"
    "AxisState\0Undefined\0Idle\0StartupSequence\0"
    "FullCalibrationSequence\0MotorCalibration\0"
    "EncoderIndexSearch\0EncoderOffsetCalibration\0"
    "ClosedLoopControl\0LockinSpin\0"
    "EncoderDirFind\0Homing\0"
    "EncoderHallPolarityCalibration\0"
    "EncoderHallPhaseCalibration\0"
    "AnticoggingCalibration\0HarmonicCalibration\0"
    "HarmonicCalibrationCommutation\0"
    "ControlMode\0VoltageControl\0TorqueControl\0"
    "VelocityControl\0PositionControl\0"
    "InputMode\0Inactive\0Passthrough\0VelRamp\0"
    "PosFilter\0MixChannels\0TrapTraj\0"
    "TorqueRamp\0Mirror\0Tuning"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_ODriveMotorController[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      11,   14, // methods
       0,    0, // properties
       4,   94, // enums/sets
       0,    0, // constructors
       0,       // flags
       6,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   69,    2, 0x06 /* Public */,
       4,    0,   72,    2, 0x06 /* Public */,
       5,    1,   73,    2, 0x06 /* Public */,
       7,    1,   76,    2, 0x06 /* Public */,
       9,    1,   79,    2, 0x06 /* Public */,
      10,    1,   82,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
      11,    0,   85,    2, 0x08 /* Private */,
      12,    0,   86,    2, 0x08 /* Private */,
      13,    0,   87,    2, 0x08 /* Private */,
      14,    1,   88,    2, 0x08 /* Private */,
      17,    1,   91,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void, QMetaType::Bool,    3,
    QMetaType::Void,
    QMetaType::Void, QMetaType::UChar,    6,
    QMetaType::Void, QMetaType::QString,    8,
    QMetaType::Void, QMetaType::QString,    8,
    QMetaType::Void, QMetaType::QString,    8,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 15,   16,
    QMetaType::Void, 0x80000000 | 18,   19,

 // enums: name, alias, flags, count, data
      20,   20, 0x0,   16,  114,
      37,   37, 0x0,   16,  146,
      54,   54, 0x0,    4,  178,
      59,   59, 0x0,    9,  186,

 // enum data: key, value
      21, uint(ODriveMotorController::Heartbeat),
      22, uint(ODriveMotorController::EstopCommand),
      23, uint(ODriveMotorController::GetError),
      24, uint(ODriveMotorController::SetAxisState),
      25, uint(ODriveMotorController::GetEncoderEstimates),
      26, uint(ODriveMotorController::SetControllerMode),
      27, uint(ODriveMotorController::SetInputPos),
      28, uint(ODriveMotorController::SetInputVel),
      29, uint(ODriveMotorController::SetInputTorque),
      30, uint(ODriveMotorController::SetLimits),
      31, uint(ODriveMotorController::GetIq),
      32, uint(ODriveMotorController::GetTemperature),
      33, uint(ODriveMotorController::GetBusVoltageCurrent),
      34, uint(ODriveMotorController::ClearErrors),
      35, uint(ODriveMotorController::GetTorques),
      36, uint(ODriveMotorController::GetPowers),
      38, uint(ODriveMotorController::Undefined),
      39, uint(ODriveMotorController::Idle),
      40, uint(ODriveMotorController::StartupSequence),
      41, uint(ODriveMotorController::FullCalibrationSequence),
      42, uint(ODriveMotorController::MotorCalibration),
      43, uint(ODriveMotorController::EncoderIndexSearch),
      44, uint(ODriveMotorController::EncoderOffsetCalibration),
      45, uint(ODriveMotorController::ClosedLoopControl),
      46, uint(ODriveMotorController::LockinSpin),
      47, uint(ODriveMotorController::EncoderDirFind),
      48, uint(ODriveMotorController::Homing),
      49, uint(ODriveMotorController::EncoderHallPolarityCalibration),
      50, uint(ODriveMotorController::EncoderHallPhaseCalibration),
      51, uint(ODriveMotorController::AnticoggingCalibration),
      52, uint(ODriveMotorController::HarmonicCalibration),
      53, uint(ODriveMotorController::HarmonicCalibrationCommutation),
      55, uint(ODriveMotorController::VoltageControl),
      56, uint(ODriveMotorController::TorqueControl),
      57, uint(ODriveMotorController::VelocityControl),
      58, uint(ODriveMotorController::PositionControl),
      60, uint(ODriveMotorController::Inactive),
      61, uint(ODriveMotorController::Passthrough),
      62, uint(ODriveMotorController::VelRamp),
      63, uint(ODriveMotorController::PosFilter),
      64, uint(ODriveMotorController::MixChannels),
      65, uint(ODriveMotorController::TrapTraj),
      66, uint(ODriveMotorController::TorqueRamp),
      67, uint(ODriveMotorController::Mirror),
      68, uint(ODriveMotorController::Tuning),

       0        // eod
};

void ODriveMotorController::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<ODriveMotorController *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->connectionChanged((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 1: _t->statusUpdated(); break;
        case 2: _t->nodeStatusUpdated((*reinterpret_cast< quint8(*)>(_a[1]))); break;
        case 3: _t->logMessage((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 4: _t->rxMessage((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 5: _t->errorMessage((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 6: _t->readFrames(); break;
        case 7: _t->readSlcanFrames(); break;
        case 8: _t->readCandleOutput(); break;
        case 9: _t->handleDeviceError((*reinterpret_cast< QCanBusDevice::CanBusError(*)>(_a[1]))); break;
        case 10: _t->handleDeviceStateChange((*reinterpret_cast< QCanBusDevice::CanBusDeviceState(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (ODriveMotorController::*)(bool );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ODriveMotorController::connectionChanged)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (ODriveMotorController::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ODriveMotorController::statusUpdated)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (ODriveMotorController::*)(quint8 );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ODriveMotorController::nodeStatusUpdated)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (ODriveMotorController::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ODriveMotorController::logMessage)) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (ODriveMotorController::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ODriveMotorController::rxMessage)) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (ODriveMotorController::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ODriveMotorController::errorMessage)) {
                *result = 5;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject ODriveMotorController::staticMetaObject = { {
    &QObject::staticMetaObject,
    qt_meta_stringdata_ODriveMotorController.data,
    qt_meta_data_ODriveMotorController,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *ODriveMotorController::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ODriveMotorController::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ODriveMotorController.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int ODriveMotorController::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 11)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 11;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 11)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 11;
    }
    return _id;
}

// SIGNAL 0
void ODriveMotorController::connectionChanged(bool _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void ODriveMotorController::statusUpdated()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void ODriveMotorController::nodeStatusUpdated(quint8 _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void ODriveMotorController::logMessage(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void ODriveMotorController::rxMessage(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void ODriveMotorController::errorMessage(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
