#include "mainwindow.h"

#include <QApplication>
#include <QByteArray>
#include <QCanBus>
#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFont>
#include <QFile>
#include <QFileInfo>
#include <QMutex>
#include <QMutexLocker>
#include <QTextStream>
#include <QtGlobal>

#include <cstdlib>
#include <exception>
#include <typeinfo>

#ifdef Q_OS_WIN
#include <eh.h>
#endif

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

void logCrashLine(const QString &message)
{
    const QString line = QStringLiteral("[%1] [CRASH] %2")
            .arg(QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss.zzz")),
                 message);

    QTextStream err(stderr);
    err << line << '\n';
    err.flush();
    appendRollingLogLine(line);
}

class SafeApplication : public QApplication
{
public:
    using QApplication::QApplication;

    bool notify(QObject *receiver, QEvent *event) override
    {
        try {
            return QApplication::notify(receiver, event);
        } catch (const std::exception &e) {
            const QString receiverName = receiver
                    ? QString::fromLatin1(receiver->metaObject()->className())
                    : QStringLiteral("<null>");
            const QString eventType = event
                    ? QString::number(static_cast<int>(event->type()))
                    : QStringLiteral("<null>");
            logCrashLine(QStringLiteral("Unhandled std::exception in Qt event loop. receiver=%1 eventType=%2 what=%3")
                         .arg(receiverName, eventType, QString::fromLocal8Bit(e.what())));
        } catch (...) {
            const QString receiverName = receiver
                    ? QString::fromLatin1(receiver->metaObject()->className())
                    : QStringLiteral("<null>");
            const QString eventType = event
                    ? QString::number(static_cast<int>(event->type()))
                    : QStringLiteral("<null>");
            logCrashLine(QStringLiteral("Unhandled unknown exception in Qt event loop. receiver=%1 eventType=%2")
                         .arg(receiverName, eventType));
        }
        return false;
    }
};

void terminateHandler()
{
    QString detail = QStringLiteral("std::terminate called");
    const std::exception_ptr exception = std::current_exception();
    if (exception) {
        try {
            std::rethrow_exception(exception);
        } catch (const std::exception &e) {
            detail += QStringLiteral(", reason=%1").arg(QString::fromLocal8Bit(e.what()));
        } catch (...) {
            detail += QStringLiteral(", reason=<non-std exception>");
        }
    }
    logCrashLine(detail);
    abort();
}

#ifdef Q_OS_WIN
LONG WINAPI unhandledExceptionFilter(EXCEPTION_POINTERS *exceptionInfo)
{
    const DWORD code = exceptionInfo && exceptionInfo->ExceptionRecord
            ? exceptionInfo->ExceptionRecord->ExceptionCode
            : 0;
    const void *address = exceptionInfo && exceptionInfo->ExceptionRecord
            ? exceptionInfo->ExceptionRecord->ExceptionAddress
            : nullptr;

    logCrashLine(QStringLiteral("Unhandled Windows SEH exception. code=0x%1 address=%2")
                 .arg(QString::number(code, 16).toUpper(),
                      QStringLiteral("0x%1").arg(reinterpret_cast<quintptr>(address), 0, 16)));
    return EXCEPTION_EXECUTE_HANDLER;
}

void purecallHandler()
{
    logCrashLine(QStringLiteral("Pure virtual function call detected (_purecall)"));
    abort();
}
#endif

void configureQtLogging()
{
    qputenv("QT_DEBUG_PLUGINS", QByteArrayLiteral("1"));
#ifdef Q_OS_WIN
    qputenv("QT_NO_DIRECTWRITE", QByteArrayLiteral("1"));
#endif
    qInstallMessageHandler(messageHandler);
    std::set_terminate(terminateHandler);
#ifdef Q_OS_WIN
    ::SetUnhandledExceptionFilter(unhandledExceptionFilter);
    _set_purecall_handler(purecallHandler);
#endif
}

void applySafeApplicationFont(QApplication &app)
{
#ifdef Q_OS_WIN
    QFont font(QStringLiteral("Microsoft YaHei UI"), 9);
    if (!font.exactMatch()) {
        font = QFont(QStringLiteral("Microsoft YaHei"), 9);
    }
    if (!font.exactMatch()) {
        font = QFont(QStringLiteral("Segoe UI"), 9);
    }
    app.setFont(font);
#else
    Q_UNUSED(app);
#endif
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

    try {
        SafeApplication a(argc, argv);
        applySafeApplicationFont(a);
        dumpStartupInfo();

        MainWindow w;
        w.show();

        const int exitCode = a.exec();
        qInfo() << "Application exited normally with code" << exitCode;
        return exitCode;
    } catch (const std::exception &e) {
        logCrashLine(QStringLiteral("Unhandled std::exception escaped from main(): %1")
                     .arg(QString::fromLocal8Bit(e.what())));
    } catch (...) {
        logCrashLine(QStringLiteral("Unhandled unknown exception escaped from main()"));
    }

    return EXIT_FAILURE;
}