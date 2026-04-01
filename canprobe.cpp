#include <QCanBus>
#include <QCanBusDevice>
#include <QCanBusDeviceInfo>
#include <QCanBusFrame>
#include <QCoreApplication>
#include <QDateTime>
#include <QSet>
#include <QTextStream>
#include <QTimer>

namespace {

QString commandName(quint32 commandId)
{
    switch (commandId) {
    case 0x01: return QStringLiteral("Heartbeat");
    case 0x03: return QStringLiteral("GetError");
    case 0x07: return QStringLiteral("SetAxisState");
    case 0x09: return QStringLiteral("GetEncoderEstimates");
    case 0x0B: return QStringLiteral("SetControllerMode");
    case 0x0C: return QStringLiteral("SetInputPos");
    case 0x0D: return QStringLiteral("SetInputVel");
    case 0x0E: return QStringLiteral("SetInputTorque");
    case 0x0F: return QStringLiteral("SetLimits");
    case 0x14: return QStringLiteral("GetIq");
    case 0x15: return QStringLiteral("GetTemperature");
    case 0x17: return QStringLiteral("GetBusVoltageCurrent");
    case 0x18: return QStringLiteral("ClearErrors");
    case 0x1C: return QStringLiteral("GetTorques");
    case 0x1D: return QStringLiteral("GetPowers");
    default:   return QStringLiteral("Cmd%1").arg(commandId);
    }
}

QString frameTypeName(QCanBusFrame::FrameType type)
{
    switch (type) {
    case QCanBusFrame::DataFrame:
        return QStringLiteral("data");
    case QCanBusFrame::RemoteRequestFrame:
        return QStringLiteral("rtr");
    case QCanBusFrame::ErrorFrame:
        return QStringLiteral("error");
    case QCanBusFrame::InvalidFrame:
        return QStringLiteral("invalid");
    default:
        return QStringLiteral("unknown");
    }
}

} // namespace

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QTextStream out(stdout);
    QTextStream err(stderr);

    QString plugin = QStringLiteral("systeccan");
    QString interfaceName = QStringLiteral("can0");
    int bitrate = 250000;
    int durationMs = 5000;

    const QStringList args = app.arguments();
    if (args.size() > 1) {
        plugin = args.at(1);
    }
    if (args.size() > 2) {
        interfaceName = args.at(2);
    }
    if (args.size() > 3) {
        bitrate = args.at(3).toInt();
    }
    if (args.size() > 4) {
        durationMs = args.at(4).toInt();
    }

    out << "Qt CAN probe\n";
    out << "plugin=" << plugin << ", interface=" << interfaceName
        << ", bitrate=" << bitrate << ", duration_ms=" << durationMs << "\n";

    const QStringList plugins = QCanBus::instance()->plugins();
    out << "available plugins: " << plugins.join(QStringLiteral(", ")) << "\n";
    for (const QString &pluginName : plugins) {
        QString errorText;
        const QList<QCanBusDeviceInfo> infos = QCanBus::instance()->availableDevices(pluginName, &errorText);
        QStringList names;
        for (const QCanBusDeviceInfo &info : infos) {
            names << info.name();
        }
        out << "  " << pluginName << ": ";
        out << (names.isEmpty() ? QStringLiteral("<none>") : names.join(QStringLiteral(", ")));
        if (!errorText.trimmed().isEmpty()) {
            out << " [error=" << errorText << "]";
        }
        out << "\n";
    }
    out.flush();

    QString errorText;
    QCanBusDevice *device = QCanBus::instance()->createDevice(plugin, interfaceName, &errorText);
    if (!device) {
        err << "createDevice failed: " << errorText << "\n";
        return 2;
    }

    device->setConfigurationParameter(QCanBusDevice::BitRateKey, bitrate);
    device->setConfigurationParameter(QCanBusDevice::ReceiveOwnKey, false);

    QObject::connect(device, &QCanBusDevice::stateChanged, [&](QCanBusDevice::CanBusDeviceState state) {
        out << "stateChanged: " << state << "\n";
        out.flush();
    });
    QObject::connect(device, &QCanBusDevice::errorOccurred, [&](QCanBusDevice::CanBusError canError) {
        err << "errorOccurred: " << canError << " " << device->errorString() << "\n";
        err.flush();
    });

    QSet<int> heartbeatNodes;
    QSet<int> allNodes;
    int frameCount = 0;

    QObject::connect(device, &QCanBusDevice::framesReceived, [&]() {
        while (device->framesAvailable() > 0) {
            const QCanBusFrame frame = device->readFrame();
            ++frameCount;

            const quint32 frameId = frame.frameId();
            const quint32 commandId = frameId & 0x1F;
            const quint32 nodeId = (frameId >> 5) & 0x3F;
            allNodes.insert(static_cast<int>(nodeId));
            if (commandId == 0x01 && frame.frameType() == QCanBusFrame::DataFrame) {
                heartbeatNodes.insert(static_cast<int>(nodeId));
            }

            out << QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss.zzz"))
                << " id=0x" << QString::number(frameId, 16).rightJustified(3, QLatin1Char('0'))
                << " node=" << nodeId
                << " cmd=" << commandName(commandId)
                << " type=" << frameTypeName(frame.frameType())
                << " dlc=" << frame.payload().size()
                << " payload=" << frame.payload().toHex(' ')
                << "\n";
        }
        out.flush();
    });

    if (!device->connectDevice()) {
        err << "connectDevice failed: " << device->errorString() << "\n";
        delete device;
        return 3;
    }

    QTimer::singleShot(durationMs, &app, [&]() {
        QStringList heartbeatNodeStrings;
        for (const int nodeId : heartbeatNodes) {
            heartbeatNodeStrings << QString::number(nodeId);
        }
        QStringList allNodeStrings;
        for (const int nodeId : allNodes) {
            allNodeStrings << QString::number(nodeId);
        }

        out << "summary: frame_count=" << frameCount
            << ", heartbeat_nodes=" << (heartbeatNodeStrings.isEmpty() ? QStringLiteral("<none>") : heartbeatNodeStrings.join(QStringLiteral(",")))
            << ", all_nodes=" << (allNodeStrings.isEmpty() ? QStringLiteral("<none>") : allNodeStrings.join(QStringLiteral(",")))
            << "\n";
        out.flush();
        app.quit();
    });

    const int rc = app.exec();
    device->disconnectDevice();
    delete device;
    return rc;
}
