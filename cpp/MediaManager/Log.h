#ifndef LOG_H
#define LOG_H

#include <iostream>
#include <fstream>
#include <mutex>
#include <string>
#include <chrono>
#include <cstdarg> // 添加可变参数支持

enum class LogGrade {
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    CRITICAL
};

// 将全局日志级别声明为 extern
extern LogGrade globalLogGrade;

// 其他函数声明
std::string logGradeToString(LogGrade grade);
std::string getCurrentTime();
std::string colorize(const std::string& message, LogGrade grade, bool isTime);
void log(LogGrade grade, const std::string& message, const char* file, int line);
std::string formatMessage(const char* format, ...);

// 定义宏
#define LOG_DEBUG(msg, ...) log(LogGrade::DEBUG, formatMessage(msg, __VA_ARGS__), __FILE__, __LINE__)
#define LOG_INFO(msg, ...) log(LogGrade::INFO, formatMessage(msg, __VA_ARGS__), __FILE__, __LINE__)
#define LOG_WARNING(msg, ...) log(LogGrade::WARNING, formatMessage(msg, __VA_ARGS__), __FILE__, __LINE__)
#define LOG_ERROR(msg, ...) log(LogGrade::ERROR, formatMessage(msg, __VA_ARGS__), __FILE__, __LINE__)
#define LOG_CRITICAL(msg, ...) log(LogGrade::CRITICAL, formatMessage(msg, __VA_ARGS__), __FILE__, __LINE__)

// 在一个源文件中定义全局变量
inline LogGrade globalLogGrade = LogGrade::DEBUG; // 默认日志级别为 DEBUG

inline std::string logGradeToString(LogGrade grade) {
    switch (grade) {
        case LogGrade::DEBUG:   return "DEBUG   ";
        case LogGrade::INFO:    return "INFO    ";
        case LogGrade::WARNING:  return "WARNING ";
        case LogGrade::ERROR:    return "ERROR   ";
        case LogGrade::CRITICAL: return "CRITICAL";
        default:                return "UNKNOWN  ";
    }
}

inline std::string getCurrentTime() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    std::tm local_tm;
    localtime_s(&local_tm, &now_time);
    char buffer[20]; // yyyy-mm-dd hh:mm:ss
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &local_tm);
    return std::string(buffer);
}

inline std::string colorize(const std::string& message, LogGrade grade, bool isTime) {
    // 输出颜色设置（根据需要调整）
    if (isTime) {
        return "\033[34m" + message + "\033[0m"; // 时间用蓝色
    }

    switch (grade) {
        case LogGrade::DEBUG:    return "\033[36m" + message + "\033[0m"; // DEBUG用青色
        case LogGrade::INFO:     return "\033[32m" + message + "\033[0m"; // INFO用绿色
        case LogGrade::WARNING:  return "\033[33m" + message + "\033[0m"; // WARNING用黄色
        case LogGrade::ERROR:    return "\033[31m" + message + "\033[0m"; // ERROR用红色
        case LogGrade::CRITICAL: return "\033[41m" + message + "\033[0m"; // CRITICAL用红色背景
        default:                 return message; // 默认无颜色
    }
}

inline void log(LogGrade grade, const std::string& message, const char* file, int line) {
    if (grade >= globalLogGrade) {
        static std::mutex logMutex;
        std::lock_guard<std::mutex> lock(logMutex);
        std::string gradeStr = colorize(logGradeToString(grade), grade, false); // 传递 false
        std::string currentTime = colorize(getCurrentTime(), grade, true);

        std::cout << "[" << gradeStr << "][" << currentTime << "] [" << file << ":" << line << "] - " << message << std::endl;
    }
}

inline std::string formatMessage(const char* format, ...) {
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    return std::string(buffer);
}

#endif // LOG_H
