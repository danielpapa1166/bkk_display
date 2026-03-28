#include "bkk_logger.hpp"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMutex>
#include <QMutexLocker>
#include <QStandardPaths>
#include <QTextStream>

#include <cstdio>
#include <time.h>

namespace {
QMutex g_mutex;
QFile g_logFile;
Logger::Level g_minLevel = Logger::Level::Info;
qint64 g_maxFileSizeBytes = 1024 * 1024;
int g_maxBackupFiles = 3;
bool g_initialized = false;
QString g_bootId = QStringLiteral("unknown");

QString readBootId()
{
    QFile bootIdFile(QStringLiteral("/proc/sys/kernel/random/boot_id"));
    if (!bootIdFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QStringLiteral("unknown");
    }

    const QString bootId = QString::fromUtf8(bootIdFile.readAll()).trimmed();
    return bootId.isEmpty() ? QStringLiteral("unknown") : bootId;
}

quint64 monotonicMsSinceBoot()
{
    // Linux monotonic clock is kernel-provided and unaffected by wall clock/NTP changes.
#ifdef CLOCK_BOOTTIME
    constexpr clockid_t kClockId = CLOCK_BOOTTIME;
#else
    constexpr clockid_t kClockId = CLOCK_MONOTONIC;
#endif

    timespec ts {};
    if (clock_gettime(kClockId, &ts) != 0) {
        return 0;
    }

    return static_cast<quint64>(ts.tv_sec) * 1000ULL
        + static_cast<quint64>(ts.tv_nsec) / 1000000ULL;
}

QString levelToString(Logger::Level level)
{
    switch (level) {
    case Logger::Level::Debug:
        return "DEBUG";
    case Logger::Level::Info:
        return "INFO";
    case Logger::Level::Warning:
        return "WARN";
    case Logger::Level::Error:
        return "ERROR";
    }
    return "INFO";
}

void rotateLogsIfNeededLocked()
{
    if (!g_logFile.isOpen()) {
        return;
    }

    if (g_logFile.size() < g_maxFileSizeBytes) {
        return;
    }

    const QString basePath = g_logFile.fileName();
    g_logFile.close();

    // Rotate from high to low index: app.log.2 -> app.log.3, ..., app.log -> app.log.1
    for (int i = g_maxBackupFiles - 1; i >= 1; --i) {
        const QString src = QStringLiteral("%1.%2").arg(basePath).arg(i);
        const QString dst = QStringLiteral("%1.%2").arg(basePath).arg(i + 1);
        if (QFile::exists(dst)) {
            QFile::remove(dst);
        }
        if (QFile::exists(src)) {
            QFile::rename(src, dst);
        }
    }

    const QString firstBackup = QStringLiteral("%1.1").arg(basePath);
    if (QFile::exists(firstBackup)) {
        QFile::remove(firstBackup);
    }
    if (QFile::exists(basePath)) {
        QFile::rename(basePath, firstBackup);
    }

    g_logFile.setFileName(basePath);
    g_logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);
}
} // namespace

bool Logger::init(const QString &appName, const QString &logDir, qint64 maxFileSizeBytes, int maxBackupFiles)
{
    QMutexLocker locker(&g_mutex);

    if (g_initialized) {
        return true;
    }

    QString resolvedDir = logDir;
    if (resolvedDir.isEmpty()) {
        resolvedDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    }

    QDir dir(resolvedDir);
    if (!dir.exists() && !dir.mkpath(QStringLiteral("."))) {
        return false;
    }

    g_maxFileSizeBytes = maxFileSizeBytes > 0 ? maxFileSizeBytes : 1024 * 1024;
    g_maxBackupFiles = maxBackupFiles > 0 ? maxBackupFiles : 3;
    g_bootId = readBootId();

    const QString filePath = dir.filePath(appName + QStringLiteral(".log"));
    g_logFile.setFileName(filePath);
    if (!g_logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        return false;
    }

    g_initialized = true;
    return true;
}

void Logger::shutdown()
{
    QMutexLocker locker(&g_mutex);
    if (!g_initialized) {
        return;
    }

    if (g_logFile.isOpen()) {
        g_logFile.flush();
        g_logFile.close();
    }
    g_initialized = false;
}

void Logger::setMinLevel(Level level)
{
    QMutexLocker locker(&g_mutex);
    g_minLevel = level;
}

void Logger::debug(const QString &category, const QString &message)
{
    log(Level::Debug, category, message);
}

void Logger::info(const QString &category, const QString &message)
{
    log(Level::Info, category, message);
}

void Logger::warning(const QString &category, const QString &message)
{
    log(Level::Warning, category, message);
}

void Logger::error(const QString &category, const QString &message)
{
    log(Level::Error, category, message);
}

void Logger::log(Level level, const QString &category, const QString &message)
{
    QMutexLocker locker(&g_mutex);

    if (level < g_minLevel) {
        return;
    }

    const QString timestamp = QDateTime::currentDateTime().toString(Qt::ISODateWithMs);
    const QString monoMs = QString::number(monotonicMsSinceBoot()).rightJustified(12, QLatin1Char(' '));
    const QString line = QStringLiteral("%1 [mono_ms=%2] [boot_id=%3] [%4] [%5] %6")
                             .arg(timestamp, monoMs, g_bootId, levelToString(level), category, message);

    // Always mirror to stderr so logs are visible even if file initialization failed.
    std::fprintf(stderr, "%s\n", line.toLocal8Bit().constData());

    if (!g_initialized || !g_logFile.isOpen()) {
        return;
    }

    rotateLogsIfNeededLocked();
    if (!g_logFile.isOpen()) {
        return;
    }

    QTextStream stream(&g_logFile);
    stream << line << '\n';
    stream.flush();
}
