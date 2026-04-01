#include "mainwindow.h"

#include <QApplication>
#include <QByteArray>
#include <QCanBus>
#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMutex>
#include <QMutexLocker>
#include <QTextStream>
#include <QtGlobal>

#include <cstdlib>

#ifdef Q_OS_WIN
#include <windows.h>
#include <cstdio>
#endif

namespace {

QString runtimeLogPath()
{
    const QString baseDir = QCoreApplication::instance()
            ? QCoreApplication::applicationDirPath()
            : QDir::currentPath();
    return QDir(baseDir).filePath(QStringLiteral("odriver_runtime.log"));
}

void appendRollingLogLine(const QString &line)
{
    static QMutex mutex;
    constexpr int maxLogLines = 600;

    QMutexLocker locker(&mutex);

    QFile file(runtimeLogPath());
    QStringList lines;
    if (file.exists() && file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        while (!in.atEnd()) {
            lines << in.readLine();
        }
        file.close();
    }

    lines << line;
    while (lines.size() > maxLogLines) {
        lines.removeFirst();
    }

    const QFileInfo info(file);
    QDir().mkpath(info.absolutePath());
    if (file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        QTextStream out(&file);
        for (const QString &entry : lines) {
            out << entry << '\n';
        }
        out.flush();
        file.close();
    }
}

void initializeDebugConsole()
{
#ifdef Q_OS_WIN
    if (::GetConsoleWindow() == nullptr) {
        ::AllocConsole();
    }

    FILE *stream = nullptr;
    freopen_s(&stream, "CONOUT$", "w", stdout);
    freopen_s(&stream, "CONOUT$", "w", stderr);
    freopen_s(&stream, "CONIN$", "r", stdin);
#endif
}

void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &message)
{
    Q_UNUSED(context);

    QString level;
    switch (type) {
    case QtDebugMsg:
        level = QStringLiteral("DEBUG");
        break;
    case QtInfoMsg:
        level = QStringLiteral("INFO ");
        break;
    case QtWarningMsg:
        level = QStringLiteral("WARN ");
        break;
    case QtCriticalMsg:
        level = QStringLiteral("ERROR");
        break;
    case QtFatalMsg:
        level = QStringLiteral("FATAL");
        break;
    }

    const QString line = QStringLiteral("[%1] [%2] %3")
            .arg(QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss.zzz")),
                 level,
                 message);

    QTextStream err(stderr);
    err << line << '\n';
    err.flush();
    appendRollingLogLine(line);

    if (type == QtFatalMsg) {
        abort();
    }
}

void configureQtLogging()
{
    qputenv("QT_DEBUG_PLUGINS", QByteArrayLiteral("1"));
    qInstallMessageHandler(messageHandler);
}

void dumpStartupInfo()
{
    qInfo().noquote() << "Application dir:" << QCoreApplication::applicationDirPath();
    qInfo().noquote() << "Qt CAN plugins:" << QCanBus::instance()->plugins().join(QStringLiteral(", "));
}

} // namespace

int main(int argc, char *argv[])
{
    initializeDebugConsole();
    configureQtLogging();

    QApplication a(argc, argv);
    dumpStartupInfo();

    MainWindow w;
    w.show();

    return a.exec();
}
