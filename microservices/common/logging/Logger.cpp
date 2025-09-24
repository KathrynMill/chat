#include "Logger.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <filesystem>
#include <algorithm>

// TextFormatter 實現
std::string TextFormatter::format(const LogEvent& event) {
    std::ostringstream oss;
    
    // 時間戳
    auto time_t = std::chrono::system_clock::to_time_t(event.timestamp);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        event.timestamp.time_since_epoch()) % 1000;
    
    oss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    oss << "." << std::setfill('0') << std::setw(3) << ms.count();
    
    // 線程 ID
    oss << " [" << event.threadId << "]";
    
    // 日誌級別
    std::string levelStr;
    switch (event.level) {
        case LogLevel::TRACE: levelStr = "TRACE"; break;
        case LogLevel::DEBUG: levelStr = "DEBUG"; break;
        case LogLevel::INFO: levelStr = "INFO"; break;
        case LogLevel::WARN: levelStr = "WARN"; break;
        case LogLevel::ERROR: levelStr = "ERROR"; break;
        case LogLevel::FATAL: levelStr = "FATAL"; break;
    }
    oss << " [" << levelStr << "]";
    
    // 源文件位置
    if (!event.sourceFile.empty()) {
        oss << " [" << event.sourceFile << ":" << event.sourceLine << "]";
    }
    
    // 函數名
    if (!event.function.empty()) {
        oss << " [" << event.function << "]";
    }
    
    // 消息
    oss << " " << event.message;
    
    // 字段
    if (!event.fields.empty()) {
        oss << " {";
        bool first = true;
        for (const auto& field : event.fields) {
            if (!first) oss << ", ";
            oss << field.first << "=" << field.second;
            first = false;
        }
        oss << "}";
    }
    
    return oss.str();
}

// JsonFormatter 實現
std::string JsonFormatter::format(const LogEvent& event) {
    std::ostringstream oss;
    
    oss << "{";
    
    // 時間戳
    auto time_t = std::chrono::system_clock::to_time_t(event.timestamp);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        event.timestamp.time_since_epoch()) % 1000;
    
    oss << "\"timestamp\":\"" << std::put_time(std::localtime(&time_t), "%Y-%m-%dT%H:%M:%S");
    oss << "." << std::setfill('0') << std::setw(3) << ms.count() << "Z\"";
    
    // 線程 ID
    oss << ",\"thread_id\":\"" << event.threadId << "\"";
    
    // 日誌級別
    std::string levelStr;
    switch (event.level) {
        case LogLevel::TRACE: levelStr = "TRACE"; break;
        case LogLevel::DEBUG: levelStr = "DEBUG"; break;
        case LogLevel::INFO: levelStr = "INFO"; break;
        case LogLevel::WARN: levelStr = "WARN"; break;
        case LogLevel::ERROR: levelStr = "ERROR"; break;
        case LogLevel::FATAL: levelStr = "FATAL"; break;
    }
    oss << ",\"level\":\"" << levelStr << "\"";
    
    // 源文件位置
    if (!event.sourceFile.empty()) {
        oss << ",\"source_file\":\"" << event.sourceFile << "\"";
        oss << ",\"source_line\":" << event.sourceLine;
    }
    
    // 函數名
    if (!event.function.empty()) {
        oss << ",\"function\":\"" << event.function << "\"";
    }
    
    // 消息
    oss << ",\"message\":\"" << event.message << "\"";
    
    // 字段
    if (!event.fields.empty()) {
        oss << ",\"fields\":{";
        bool first = true;
        for (const auto& field : event.fields) {
            if (!first) oss << ",";
            oss << "\"" << field.first << "\":\"" << field.second << "\"";
            first = false;
        }
        oss << "}";
    }
    
    oss << "}";
    return oss.str();
}

// StructuredFormatter 實現
std::string StructuredFormatter::format(const LogEvent& event) {
    std::ostringstream oss;
    
    // 時間戳
    auto time_t = std::chrono::system_clock::to_time_t(event.timestamp);
    oss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    
    // 日誌級別
    std::string levelStr;
    switch (event.level) {
        case LogLevel::TRACE: levelStr = "TRACE"; break;
        case LogLevel::DEBUG: levelStr = "DEBUG"; break;
        case LogLevel::INFO: levelStr = "INFO"; break;
        case LogLevel::WARN: levelStr = "WARN"; break;
        case LogLevel::ERROR: levelStr = "ERROR"; break;
        case LogLevel::FATAL: levelStr = "FATAL"; break;
    }
    
    oss << " level=" << levelStr;
    oss << " msg=\"" << event.message << "\"";
    
    // 字段
    for (const auto& field : event.fields) {
        oss << " " << field.first << "=" << field.second;
    }
    
    return oss.str();
}

// ConsoleAppender 實現
ConsoleAppender::ConsoleAppender(bool enableColor) : enableColor_(enableColor) {}

void ConsoleAppender::append(const LogEvent& event) {
    std::string formatted = TextFormatter().format(event);
    
    if (enableColor_) {
        std::string colorCode = getColorCode(event.level);
        std::cout << colorCode << formatted << "\033[0m" << std::endl;
    } else {
        std::cout << formatted << std::endl;
    }
}

void ConsoleAppender::flush() {
    std::cout.flush();
}

void ConsoleAppender::close() {
    flush();
}

std::string ConsoleAppender::getColorCode(LogLevel level) {
    switch (level) {
        case LogLevel::TRACE: return "\033[37m";  // 白色
        case LogLevel::DEBUG: return "\033[36m";  // 青色
        case LogLevel::INFO: return "\033[32m";   // 綠色
        case LogLevel::WARN: return "\033[33m";   // 黃色
        case LogLevel::ERROR: return "\033[31m";  // 紅色
        case LogLevel::FATAL: return "\033[35m";  // 紫色
        default: return "\033[0m";
    }
}

// FileAppender 實現
FileAppender::FileAppender(const std::string& filename, size_t maxFileSize, int maxFiles)
    : filename_(filename), maxFileSize_(maxFileSize), maxFiles_(maxFiles), currentFileSize_(0) {
    
    // 確保目錄存在
    std::filesystem::path filePath(filename_);
    std::filesystem::create_directories(filePath.parent_path());
    
    file_.open(filename_, std::ios::app);
}

void FileAppender::append(const LogEvent& event) {
    std::lock_guard<std::mutex> lock(fileMutex_);
    
    if (!file_.is_open()) {
        return;
    }
    
    std::string formatted = TextFormatter().format(event);
    file_ << formatted << std::endl;
    currentFileSize_ += formatted.length() + 1;
    
    if (currentFileSize_ >= maxFileSize_) {
        rotateFile();
    }
}

void FileAppender::flush() {
    std::lock_guard<std::mutex> lock(fileMutex_);
    if (file_.is_open()) {
        file_.flush();
    }
}

void FileAppender::close() {
    std::lock_guard<std::mutex> lock(fileMutex_);
    if (file_.is_open()) {
        file_.close();
    }
}

void FileAppender::rotateFile() {
    if (file_.is_open()) {
        file_.close();
    }
    
    // 重命名現有文件
    for (int i = maxFiles_ - 1; i > 0; --i) {
        std::string oldFile = filename_ + "." + std::to_string(i);
        std::string newFile = filename_ + "." + std::to_string(i + 1);
        
        if (std::filesystem::exists(oldFile)) {
            std::filesystem::rename(oldFile, newFile);
        }
    }
    
    // 重命名當前文件
    std::string backupFile = filename_ + ".1";
    if (std::filesystem::exists(filename_)) {
        std::filesystem::rename(filename_, backupFile);
    }
    
    // 打開新文件
    file_.open(filename_, std::ios::app);
    currentFileSize_ = 0;
}

std::string FileAppender::getTimestampString() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S");
    return oss.str();
}

// RemoteAppender 實現
RemoteAppender::RemoteAppender(const std::string& endpoint) : endpoint_(endpoint) {}

void RemoteAppender::append(const LogEvent& event) {
    std::string formatted = JsonFormatter().format(event);
    
    std::lock_guard<std::mutex> lock(queueMutex_);
    logQueue_.push(formatted);
}

void RemoteAppender::flush() {
    std::lock_guard<std::mutex> lock(queueMutex_);
    
    // 這裡應該發送到遠程端點
    // 簡化實現：清空隊列
    while (!logQueue_.empty()) {
        logQueue_.pop();
    }
}

void RemoteAppender::close() {
    flush();
}

// Logger 實現
Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

bool Logger::initialize(const LogConfig& config) {
    std::lock_guard<std::mutex> lock(configMutex_);
    
    config_ = config;
    
    // 創建格式化器
    switch (config_.format) {
        case LogFormat::TEXT:
            formatter_ = std::make_shared<TextFormatter>();
            break;
        case LogFormat::JSON:
            formatter_ = std::make_shared<JsonFormatter>();
            break;
        case LogFormat::STRUCTURED:
            formatter_ = std::make_shared<StructuredFormatter>();
            break;
    }
    
    // 創建輸出器
    switch (config_.output) {
        case LogOutput::CONSOLE:
            addAppender(std::make_shared<ConsoleAppender>(config_.enableColor));
            break;
        case LogOutput::FILE:
            addAppender(std::make_shared<FileAppender>(config_.logFile, config_.maxFileSize, config_.maxFiles));
            break;
        case LogOutput::REMOTE:
            if (!config_.remoteEndpoint.empty()) {
                addAppender(std::make_shared<RemoteAppender>(config_.remoteEndpoint));
            }
            break;
    }
    
    // 啟動異步日誌
    if (config_.enableAsync) {
        startAsyncLogging();
    }
    
    initialized_ = true;
    std::cout << "Logger initialized with level: " << static_cast<int>(config_.level) << std::endl;
    return true;
}

void Logger::setConfig(const LogConfig& config) {
    std::lock_guard<std::mutex> lock(configMutex_);
    config_ = config;
}

void Logger::setLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(configMutex_);
    config_.level = level;
}

void Logger::addAppender(std::shared_ptr<LogAppender> appender) {
    std::lock_guard<std::mutex> lock(appendersMutex_);
    appenders_.push_back(appender);
}

void Logger::removeAppender(std::shared_ptr<LogAppender> appender) {
    std::lock_guard<std::mutex> lock(appendersMutex_);
    appenders_.erase(
        std::remove(appenders_.begin(), appenders_.end(), appender),
        appenders_.end()
    );
}

void Logger::setFormatter(std::shared_ptr<LogFormatter> formatter) {
    formatter_ = formatter;
}

void Logger::log(LogLevel level, const std::string& message, 
                const std::string& file, int line, const std::string& function) {
    if (level < config_.level) {
        return;
    }
    
    LogEvent event = createLogEvent(level, message, file, line, function);
    
    if (config_.enableAsync) {
        std::lock_guard<std::mutex> lock(queueMutex_);
        logQueue_.push(event);
        queueCondition_.notify_one();
    } else {
        processLog(event);
    }
}

void Logger::log(LogLevel level, const std::string& message, 
                const std::unordered_map<std::string, std::string>& fields,
                const std::string& file, int line, const std::string& function) {
    if (level < config_.level) {
        return;
    }
    
    LogEvent event = createLogEvent(level, message, fields, file, line, function);
    
    if (config_.enableAsync) {
        std::lock_guard<std::mutex> lock(queueMutex_);
        logQueue_.push(event);
        queueCondition_.notify_one();
    } else {
        processLog(event);
    }
}

void Logger::trace(const std::string& message, const std::string& file, int line, const std::string& function) {
    log(LogLevel::TRACE, message, file, line, function);
}

void Logger::debug(const std::string& message, const std::string& file, int line, const std::string& function) {
    log(LogLevel::DEBUG, message, file, line, function);
}

void Logger::info(const std::string& message, const std::string& file, int line, const std::string& function) {
    log(LogLevel::INFO, message, file, line, function);
}

void Logger::warn(const std::string& message, const std::string& file, int line, const std::string& function) {
    log(LogLevel::WARN, message, file, line, function);
}

void Logger::error(const std::string& message, const std::string& file, int line, const std::string& function) {
    log(LogLevel::ERROR, message, file, line, function);
}

void Logger::fatal(const std::string& message, const std::string& file, int line, const std::string& function) {
    log(LogLevel::FATAL, message, file, line, function);
}

void Logger::trace(const std::string& message, const std::unordered_map<std::string, std::string>& fields, 
                  const std::string& file, int line, const std::string& function) {
    log(LogLevel::TRACE, message, fields, file, line, function);
}

void Logger::debug(const std::string& message, const std::unordered_map<std::string, std::string>& fields, 
                  const std::string& file, int line, const std::string& function) {
    log(LogLevel::DEBUG, message, fields, file, line, function);
}

void Logger::info(const std::string& message, const std::unordered_map<std::string, std::string>& fields, 
                 const std::string& file, int line, const std::string& function) {
    log(LogLevel::INFO, message, fields, file, line, function);
}

void Logger::warn(const std::string& message, const std::unordered_map<std::string, std::string>& fields, 
                 const std::string& file, int line, const std::string& function) {
    log(LogLevel::WARN, message, fields, file, line, function);
}

void Logger::error(const std::string& message, const std::unordered_map<std::string, std::string>& fields, 
                  const std::string& file, int line, const std::string& function) {
    log(LogLevel::ERROR, message, fields, file, line, function);
}

void Logger::fatal(const std::string& message, const std::unordered_map<std::string, std::string>& fields, 
                  const std::string& file, int line, const std::string& function) {
    log(LogLevel::FATAL, message, fields, file, line, function);
}

void Logger::startAsyncLogging() {
    if (asyncEnabled_) {
        return;
    }
    
    asyncEnabled_ = true;
    asyncThread_ = std::thread(&Logger::asyncLoggingThread, this);
}

void Logger::stopAsyncLogging() {
    if (!asyncEnabled_) {
        return;
    }
    
    asyncEnabled_ = false;
    queueCondition_.notify_all();
    
    if (asyncThread_.joinable()) {
        asyncThread_.join();
    }
}

void Logger::flush() {
    std::lock_guard<std::mutex> lock(appendersMutex_);
    for (auto& appender : appenders_) {
        appender->flush();
    }
}

void Logger::shutdown() {
    stopAsyncLogging();
    flush();
    
    std::lock_guard<std::mutex> lock(appendersMutex_);
    for (auto& appender : appenders_) {
        appender->close();
    }
    appenders_.clear();
    
    initialized_ = false;
}

Logger::LogStats Logger::getStats() {
    return stats_;
}

void Logger::asyncLoggingThread() {
    while (asyncEnabled_) {
        std::unique_lock<std::mutex> lock(queueMutex_);
        
        queueCondition_.wait(lock, [this] { return !logQueue_.empty() || !asyncEnabled_; });
        
        while (!logQueue_.empty()) {
            LogEvent event = logQueue_.front();
            logQueue_.pop();
            lock.unlock();
            
            processLog(event);
            
            lock.lock();
        }
    }
}

void Logger::processLog(const LogEvent& event) {
    // 更新統計
    stats_.totalLogs++;
    switch (event.level) {
        case LogLevel::TRACE: stats_.traceLogs++; break;
        case LogLevel::DEBUG: stats_.debugLogs++; break;
        case LogLevel::INFO: stats_.infoLogs++; break;
        case LogLevel::WARN: stats_.warnLogs++; break;
        case LogLevel::ERROR: stats_.errorLogs++; break;
        case LogLevel::FATAL: stats_.fatalLogs++; break;
    }
    
    // 發送到所有輸出器
    std::lock_guard<std::mutex> lock(appendersMutex_);
    for (auto& appender : appenders_) {
        try {
            appender->append(event);
        } catch (const std::exception& e) {
            // 日誌輸出失敗，記錄到統計中
            stats_.droppedLogs++;
        }
    }
}

LogEvent Logger::createLogEvent(LogLevel level, const std::string& message, 
                               const std::string& file, int line, const std::string& function) {
    LogEvent event;
    event.level = level;
    event.message = message;
    event.sourceFile = file;
    event.sourceLine = line;
    event.function = function;
    return event;
}

LogEvent Logger::createLogEvent(LogLevel level, const std::string& message, 
                               const std::unordered_map<std::string, std::string>& fields,
                               const std::string& file, int line, const std::string& function) {
    LogEvent event;
    event.level = level;
    event.message = message;
    event.sourceFile = file;
    event.sourceLine = line;
    event.function = function;
    event.fields = fields;
    return event;
}

std::string Logger::getLevelString(LogLevel level) {
    switch (level) {
        case LogLevel::TRACE: return "TRACE";
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO: return "INFO";
        case LogLevel::WARN: return "WARN";
        case LogLevel::ERROR: return "ERROR";
        case LogLevel::FATAL: return "FATAL";
        default: return "UNKNOWN";
    }
}

std::string Logger::getLevelColor(LogLevel level) {
    switch (level) {
        case LogLevel::TRACE: return "\033[37m";
        case LogLevel::DEBUG: return "\033[36m";
        case LogLevel::INFO: return "\033[32m";
        case LogLevel::WARN: return "\033[33m";
        case LogLevel::ERROR: return "\033[31m";
        case LogLevel::FATAL: return "\033[35m";
        default: return "\033[0m";
    }
}
