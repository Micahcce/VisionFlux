#ifndef LOGGER_H
#define LOGGER_H

#include <iostream>
#include <fstream>
#include <mutex>
#include <memory>
#include <string>
#include <chrono>
#include <iomanip>

enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    CRITICAL
};

class Logger {
public:
    Logger();
    ~Logger();

    void setLogLevel(LogLevel level);                 // 设置输出等级，默认DEBUG
    void setOutputFile(const std::string& filename);  // 设置输出文件，默认无

    void debug(const std::string& message);
    void info(const std::string& message);
    void warning(const std::string& message);
    void error(const std::string& message);
    void critical(const std::string& message);

private:

    std::string logLevelToString(LogLevel level);
    std::string getCurrentTime(); // 获取当前时间
    void log(LogLevel level, const std::string& message);
    std::string colorize(const std::string& message, LogLevel level, bool isTime = false); // 添加颜色输出

    LogLevel logLevel_; // 当前日志等级
    std::ofstream outputFile_; // 输出文件流
    std::mutex mutex_; // 线程安全
    bool isOutputFileSet_; // 是否设置输出文件
};

extern Logger logger; // 声明全局变量

#endif // LOGGER_H
