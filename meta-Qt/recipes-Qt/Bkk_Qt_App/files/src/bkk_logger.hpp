#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <QString>

class Logger {
public:
    enum class Level {
        Debug = 0,
        Info,
        Warning,
        Error,
    };

    // Initialize file logging. If logDir is empty, a writable app data location is used.
    static bool init(const QString &appName,
                     const QString &logDir = QString(),
                     qint64 maxFileSizeBytes = 1024 * 1024,
                     int maxBackupFiles = 3);

    static void shutdown();
    static void setMinLevel(Level level);

    static void debug(const QString &category, const QString &message);
    static void info(const QString &category, const QString &message);
    static void warning(const QString &category, const QString &message);
    static void error(const QString &category, const QString &message);

private:
    static void log(Level level, const QString &category, const QString &message);
};

#endif // LOGGER_HPP
