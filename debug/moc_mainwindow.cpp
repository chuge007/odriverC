/****************************************************************************
** Meta object code from reading C++ file 'mainwindow.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.12.4)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../mainwindow.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'mainwindow.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.12.4. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_MainWindow_t {
    QByteArrayData data[33];
    char stringdata0[510];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_MainWindow_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_MainWindow_t qt_meta_stringdata_MainWindow = {
    {
QT_MOC_LITERAL(0, 0, 10), // "MainWindow"
QT_MOC_LITERAL(1, 11, 20), // "refreshCanInterfaces"
QT_MOC_LITERAL(2, 32, 0), // ""
QT_MOC_LITERAL(3, 33, 16), // "toggleConnection"
QT_MOC_LITERAL(4, 50, 21), // "updateConnectionState"
QT_MOC_LITERAL(5, 72, 9), // "connected"
QT_MOC_LITERAL(6, 82, 17), // "updateStatusPanel"
QT_MOC_LITERAL(7, 100, 9), // "appendLog"
QT_MOC_LITERAL(8, 110, 7), // "message"
QT_MOC_LITERAL(9, 118, 15), // "appendRxMessage"
QT_MOC_LITERAL(10, 134, 16), // "sendCustomTxOnce"
QT_MOC_LITERAL(11, 151, 14), // "toggleCustomTx"
QT_MOC_LITERAL(12, 166, 11), // "requestIdle"
QT_MOC_LITERAL(13, 178, 23), // "requestMotorCalibration"
QT_MOC_LITERAL(14, 202, 22), // "requestFullCalibration"
QT_MOC_LITERAL(15, 225, 17), // "requestClosedLoop"
QT_MOC_LITERAL(16, 243, 19), // "applyControllerMode"
QT_MOC_LITERAL(17, 263, 19), // "sendPositionCommand"
QT_MOC_LITERAL(18, 283, 19), // "sendVelocityCommand"
QT_MOC_LITERAL(19, 303, 17), // "sendTorqueCommand"
QT_MOC_LITERAL(20, 321, 11), // "applyLimits"
QT_MOC_LITERAL(21, 333, 16), // "refreshTelemetry"
QT_MOC_LITERAL(22, 350, 17), // "openAdvancedPanel"
QT_MOC_LITERAL(23, 368, 16), // "syncAxisReadouts"
QT_MOC_LITERAL(24, 385, 12), // "setZeroPoint"
QT_MOC_LITERAL(25, 398, 8), // "sendHome"
QT_MOC_LITERAL(26, 407, 16), // "resetMotionState"
QT_MOC_LITERAL(27, 424, 10), // "enableAxes"
QT_MOC_LITERAL(28, 435, 16), // "startScanProgram"
QT_MOC_LITERAL(29, 452, 11), // "stopProgram"
QT_MOC_LITERAL(30, 464, 15), // "stepAdvanceOnce"
QT_MOC_LITERAL(31, 480, 22), // "handleNodeStatusUpdate"
QT_MOC_LITERAL(32, 503, 6) // "nodeId"

    },
    "MainWindow\0refreshCanInterfaces\0\0"
    "toggleConnection\0updateConnectionState\0"
    "connected\0updateStatusPanel\0appendLog\0"
    "message\0appendRxMessage\0sendCustomTxOnce\0"
    "toggleCustomTx\0requestIdle\0"
    "requestMotorCalibration\0requestFullCalibration\0"
    "requestClosedLoop\0applyControllerMode\0"
    "sendPositionCommand\0sendVelocityCommand\0"
    "sendTorqueCommand\0applyLimits\0"
    "refreshTelemetry\0openAdvancedPanel\0"
    "syncAxisReadouts\0setZeroPoint\0sendHome\0"
    "resetMotionState\0enableAxes\0"
    "startScanProgram\0stopProgram\0"
    "stepAdvanceOnce\0handleNodeStatusUpdate\0"
    "nodeId"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_MainWindow[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      28,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    0,  154,    2, 0x08 /* Private */,
       3,    0,  155,    2, 0x08 /* Private */,
       4,    1,  156,    2, 0x08 /* Private */,
       6,    0,  159,    2, 0x08 /* Private */,
       7,    1,  160,    2, 0x08 /* Private */,
       9,    1,  163,    2, 0x08 /* Private */,
      10,    0,  166,    2, 0x08 /* Private */,
      11,    0,  167,    2, 0x08 /* Private */,
      12,    0,  168,    2, 0x08 /* Private */,
      13,    0,  169,    2, 0x08 /* Private */,
      14,    0,  170,    2, 0x08 /* Private */,
      15,    0,  171,    2, 0x08 /* Private */,
      16,    0,  172,    2, 0x08 /* Private */,
      17,    0,  173,    2, 0x08 /* Private */,
      18,    0,  174,    2, 0x08 /* Private */,
      19,    0,  175,    2, 0x08 /* Private */,
      20,    0,  176,    2, 0x08 /* Private */,
      21,    0,  177,    2, 0x08 /* Private */,
      22,    0,  178,    2, 0x08 /* Private */,
      23,    0,  179,    2, 0x08 /* Private */,
      24,    0,  180,    2, 0x08 /* Private */,
      25,    0,  181,    2, 0x08 /* Private */,
      26,    0,  182,    2, 0x08 /* Private */,
      27,    0,  183,    2, 0x08 /* Private */,
      28,    0,  184,    2, 0x08 /* Private */,
      29,    0,  185,    2, 0x08 /* Private */,
      30,    0,  186,    2, 0x08 /* Private */,
      31,    1,  187,    2, 0x08 /* Private */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Bool,    5,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,    8,
    QMetaType::Void, QMetaType::QString,    8,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::UChar,   32,

       0        // eod
};

void MainWindow::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<MainWindow *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->refreshCanInterfaces(); break;
        case 1: _t->toggleConnection(); break;
        case 2: _t->updateConnectionState((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 3: _t->updateStatusPanel(); break;
        case 4: _t->appendLog((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 5: _t->appendRxMessage((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 6: _t->sendCustomTxOnce(); break;
        case 7: _t->toggleCustomTx(); break;
        case 8: _t->requestIdle(); break;
        case 9: _t->requestMotorCalibration(); break;
        case 10: _t->requestFullCalibration(); break;
        case 11: _t->requestClosedLoop(); break;
        case 12: _t->applyControllerMode(); break;
        case 13: _t->sendPositionCommand(); break;
        case 14: _t->sendVelocityCommand(); break;
        case 15: _t->sendTorqueCommand(); break;
        case 16: _t->applyLimits(); break;
        case 17: _t->refreshTelemetry(); break;
        case 18: _t->openAdvancedPanel(); break;
        case 19: _t->syncAxisReadouts(); break;
        case 20: _t->setZeroPoint(); break;
        case 21: _t->sendHome(); break;
        case 22: _t->resetMotionState(); break;
        case 23: _t->enableAxes(); break;
        case 24: _t->startScanProgram(); break;
        case 25: _t->stopProgram(); break;
        case 26: _t->stepAdvanceOnce(); break;
        case 27: _t->handleNodeStatusUpdate((*reinterpret_cast< quint8(*)>(_a[1]))); break;
        default: ;
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject MainWindow::staticMetaObject = { {
    &QMainWindow::staticMetaObject,
    qt_meta_stringdata_MainWindow.data,
    qt_meta_data_MainWindow,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *MainWindow::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *MainWindow::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_MainWindow.stringdata0))
        return static_cast<void*>(this);
    return QMainWindow::qt_metacast(_clname);
}

int MainWindow::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 28)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 28;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 28)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 28;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
