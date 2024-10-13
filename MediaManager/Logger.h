#ifndef LOGGER_H
#define LOGGER_H

#include <iostream>
#include <fstream>
#include <mutex>
#include <memory>
#include <string>
#include <chrono>
#include <iomanip>
#include <sstream> // 添加ostringstream
#include <cstdarg> // 添加可变参数支持

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

    void setLogLevel(LogLevel level);
    void setOutputFile(const std::string& filename);

    // 添加格式化的日志函数
    void debug(const char* format, ...);
    void info(const char* format, ...);
    void warning(const char* format, ...);
    void error(const char* format, ...);
    void critical(const char* format, ...);

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
