#include "Logger.h"

Logger logger; // 定义全局变量

Logger::Logger() : logLevel_(LogLevel::DEBUG), isOutputFileSet_(false) {} // 默认等级为DEBUG，输出文件未设置

Logger::~Logger() {
    if (outputFile_.is_open()) {
        outputFile_.close();
    }
}

void Logger::setLogLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(mutex_);
    logLevel_ = level;
}

void Logger::setOutputFile(const std::string& filename) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (outputFile_.is_open()) {
        outputFile_.close();
    }
    outputFile_.open(filename, std::ios::out | std::ios::app);
    isOutputFileSet_ = true; // 设置输出文件标志
}

std::string Logger::logLevelToString(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG:   return "DEBUG   ";   // DEBUG补齐3个空格
        case LogLevel::INFO:    return "INFO    ";   // INFO补齐2个空格
        case LogLevel::WARNING:  return "WARNING ";   // WARNING补齐1个空格
        case LogLevel::ERROR:    return "ERROR   ";   // ERROR补齐3个空格
        case LogLevel::CRITICAL: return "CRITICAL";    // CRITICAL不补齐
        default:                return "UNKNOWN  ";   // 默认情况
    }
}

std::string Logger::getCurrentTime() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);

    std::tm local_tm;
    localtime_s(&local_tm, &now_time); // 用于安全地获取本地时间

    char buffer[20]; // yyyy-mm-dd hh:mm:ss
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &local_tm);
    return std::string(buffer);
}

std::string Logger::colorize(const std::string& message, LogLevel level, bool isTime) {
    if (isTime) {
        return "\033[34m" + message + "\033[0m"; // 时间用蓝色
    }

    switch (level) {
        case LogLevel::DEBUG:    return "\033[36m" + message + "\033[0m"; // DEBUG用青色
        case LogLevel::INFO:     return "\033[32m" + message + "\033[0m"; // INFO用绿色
        case LogLevel::WARNING:  return "\033[33m" + message + "\033[0m"; // WARNING用黄色
        case LogLevel::ERROR:    return "\033[31m" + message + "\033[0m"; // ERROR用红色
        case LogLevel::CRITICAL: return "\033[41m" + message + "\033[0m"; // CRITICAL用红色背景
        default:                 return message; // 默认无颜色
    }
}

void Logger::log(LogLevel level, const std::string& message) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (level >= logLevel_) {
        std::string levelStr = colorize(logLevelToString(level), level); // 日志等级使用颜色
        std::string currentTime = colorize(getCurrentTime(), level, true); // 时间使用颜色
        std::string messageWithColor = colorize(message, level); // 日志消息使用与等级相同的颜色

        // 构建不带颜色的完整日志消息用于文件
        std::string fullMessageWithoutColor = "[" + logLevelToString(level) + "][" + getCurrentTime() + "] - " + message;

        // 输出到控制台
        std::cout << "[" << levelStr << "][" << currentTime << "] - " << messageWithColor << std::endl; // 输出到控制台

        // 如果设置了输出文件，才输出到文件
        if (isOutputFileSet_ && outputFile_.is_open()) {
            outputFile_ << fullMessageWithoutColor << std::endl; // 输出到文件
        }
    }
}

void Logger::debug(const std::string& message) {
    log(LogLevel::DEBUG, message);
}

void Logger::info(const std::string& message) {
    log(LogLevel::INFO, message);
}

void Logger::warning(const std::string& message) {
    log(LogLevel::WARNING, message);
}

void Logger::error(const std::string& message) {
    log(LogLevel::ERROR, message);
}

void Logger::critical(const std::string& message) {
    log(LogLevel::CRITICAL, message);
}
