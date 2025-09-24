#pragma once
#include <string>
#include <memory>
#include <mutex>
#include <fstream>
#include <sstream>
#include <chrono>
#include <thread>
#include <atomic>
#include <queue>
#include <condition_variable>
#include <functional>
#include <unordered_map>

// 日誌級別
enum class LogLevel {
    TRACE = 0,
    DEBUG = 1,
    INFO = 2,
    WARN = 3,
    ERROR = 4,
    FATAL = 5
};

// 日誌格式
enum class LogFormat {
    TEXT,       // 純文本格式
    JSON,       // JSON 格式
    STRUCTURED  // 結構化格式
};

// 日誌輸出目標
enum class LogOutput {
    CONSOLE,    // 控制台
    FILE,       // 文件
    SYSLOG,     // 系統日誌
    REMOTE      // 遠程日誌服務
};

// 日誌配置
struct LogConfig {
    LogLevel level = LogLevel::INFO;
    LogFormat format = LogFormat::TEXT;
    LogOutput output = LogOutput::CONSOLE;
    std::string logFile = "chat.log";
    std::string logDir = "./logs";
    size_t maxFileSize = 100 * 1024 * 1024;  // 100MB
    int maxFiles = 10;
    bool enableAsync = true;
    bool enableColor = true;
    bool enableTimestamp = true;
    bool enableThreadId = true;
    bool enableSourceLocation = false;
    std::string remoteEndpoint = "";
    int flushInterval = 1000;  // 毫秒
};

// 日誌事件
struct LogEvent {
    LogLevel level;
    std::string message;
    std::string sourceFile;
    int sourceLine;
    std::string function;
    std::thread::id threadId;
    std::chrono::system_clock::time_point timestamp;
    std::unordered_map<std::string, std::string> fields;
    
    LogEvent() : level(LogLevel::INFO), sourceLine(0) {
        timestamp = std::chrono::system_clock::now();
        threadId = std::this_thread::get_id();
    }
};

// 日誌格式化器
class LogFormatter {
public:
    virtual ~LogFormatter() = default;
    virtual std::string format(const LogEvent& event) = 0;
};

// 文本格式化器
class TextFormatter : public LogFormatter {
public:
    std::string format(const LogEvent& event) override;
};

// JSON 格式化器
class JsonFormatter : public LogFormatter {
public:
    std::string format(const LogEvent& event) override;
};

// 結構化格式化器
class StructuredFormatter : public LogFormatter {
public:
    std::string format(const LogEvent& event) override;
};

// 日誌輸出器
class LogAppender {
public:
    virtual ~LogAppender() = default;
    virtual void append(const LogEvent& event) = 0;
    virtual void flush() = 0;
    virtual void close() = 0;
};

// 控制台輸出器
class ConsoleAppender : public LogAppender {
public:
    ConsoleAppender(bool enableColor = true);
    void append(const LogEvent& event) override;
    void flush() override;
    void close() override;

private:
    bool enableColor_;
    std::string getColorCode(LogLevel level);
};

// 文件輸出器
class FileAppender : public LogAppender {
public:
    FileAppender(const std::string& filename, size_t maxFileSize = 100 * 1024 * 1024, int maxFiles = 10);
    void append(const LogEvent& event) override;
    void flush() override;
    void close() override;

private:
    void rotateFile();
    std::string getTimestampString();
    
    std::string filename_;
    size_t maxFileSize_;
    int maxFiles_;
    std::ofstream file_;
    std::mutex fileMutex_;
    size_t currentFileSize_;
};

// 遠程輸出器
class RemoteAppender : public LogAppender {
public:
    RemoteAppender(const std::string& endpoint);
    void append(const LogEvent& event) override;
    void flush() override;
    void close() override;

private:
    std::string endpoint_;
    std::queue<std::string> logQueue_;
    std::mutex queueMutex_;
};

// 日誌器
class Logger {
public:
    static Logger& getInstance();
    
    // 初始化日誌器
    bool initialize(const LogConfig& config = LogConfig{});
    
    // 設置配置
    void setConfig(const LogConfig& config);
    
    // 設置日誌級別
    void setLevel(LogLevel level);
    
    // 添加輸出器
    void addAppender(std::shared_ptr<LogAppender> appender);
    
    // 移除輸出器
    void removeAppender(std::shared_ptr<LogAppender> appender);
    
    // 設置格式化器
    void setFormatter(std::shared_ptr<LogFormatter> formatter);
    
    // 日誌記錄方法
    void log(LogLevel level, const std::string& message, 
             const std::string& file = "", int line = 0, const std::string& function = "");
    
    void log(LogLevel level, const std::string& message, 
             const std::unordered_map<std::string, std::string>& fields,
             const std::string& file = "", int line = 0, const std::string& function = "");
    
    // 便捷方法
    void trace(const std::string& message, const std::string& file = "", int line = 0, const std::string& function = "");
    void debug(const std::string& message, const std::string& file = "", int line = 0, const std::string& function = "");
    void info(const std::string& message, const std::string& file = "", int line = 0, const std::string& function = "");
    void warn(const std::string& message, const std::string& file = "", int line = 0, const std::string& function = "");
    void error(const std::string& message, const std::string& file = "", int line = 0, const std::string& function = "");
    void fatal(const std::string& message, const std::string& file = "", int line = 0, const std::string& function = "");
    
    // 帶字段的日誌
    void trace(const std::string& message, const std::unordered_map<std::string, std::string>& fields, 
               const std::string& file = "", int line = 0, const std::string& function = "");
    void debug(const std::string& message, const std::unordered_map<std::string, std::string>& fields, 
               const std::string& file = "", int line = 0, const std::string& function = "");
    void info(const std::string& message, const std::unordered_map<std::string, std::string>& fields, 
              const std::string& file = "", int line = 0, const std::string& function = "");
    void warn(const std::string& message, const std::unordered_map<std::string, std::string>& fields, 
              const std::string& file = "", int line = 0, const std::string& function = "");
    void error(const std::string& message, const std::unordered_map<std::string, std::string>& fields, 
               const std::string& file = "", int line = 0, const std::string& function = "");
    void fatal(const std::string& message, const std::unordered_map<std::string, std::string>& fields, 
               const std::string& file = "", int line = 0, const std::string& function = "");
    
    // 異步日誌處理
    void startAsyncLogging();
    void stopAsyncLogging();
    
    // 刷新緩衝區
    void flush();
    
    // 關閉日誌器
    void shutdown();
    
    // 獲取日誌統計
    struct LogStats {
        std::atomic<long> totalLogs;
        std::atomic<long> traceLogs;
        std::atomic<long> debugLogs;
        std::atomic<long> infoLogs;
        std::atomic<long> warnLogs;
        std::atomic<long> errorLogs;
        std::atomic<long> fatalLogs;
        std::atomic<long> droppedLogs;
    };
    LogStats getStats();

private:
    Logger() = default;
    ~Logger() = default;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    
    // 異步日誌處理線程
    void asyncLoggingThread();
    
    // 同步日誌處理
    void processLog(const LogEvent& event);
    
    // 創建日誌事件
    LogEvent createLogEvent(LogLevel level, const std::string& message, 
                           const std::string& file, int line, const std::string& function);
    
    // 創建帶字段的日誌事件
    LogEvent createLogEvent(LogLevel level, const std::string& message, 
                           const std::unordered_map<std::string, std::string>& fields,
                           const std::string& file, int line, const std::string& function);
    
    // 獲取級別字符串
    std::string getLevelString(LogLevel level);
    
    // 獲取級別顏色
    std::string getLevelColor(LogLevel level);
    
    // 配置
    LogConfig config_;
    std::mutex configMutex_;
    
    // 輸出器和格式化器
    std::vector<std::shared_ptr<LogAppender>> appenders_;
    std::shared_ptr<LogFormatter> formatter_;
    std::mutex appendersMutex_;
    
    // 異步日誌
    std::atomic<bool> asyncEnabled_;
    std::queue<LogEvent> logQueue_;
    std::mutex queueMutex_;
    std::condition_variable queueCondition_;
    std::thread asyncThread_;
    
    // 統計
    LogStats stats_;
    std::atomic<bool> initialized_;
};

// 便捷宏定義
#define LOG_TRACE(msg) Logger::getInstance().trace(msg, __FILE__, __LINE__, __FUNCTION__)
#define LOG_DEBUG(msg) Logger::getInstance().debug(msg, __FILE__, __LINE__, __FUNCTION__)
#define LOG_INFO(msg) Logger::getInstance().info(msg, __FILE__, __LINE__, __FUNCTION__)
#define LOG_WARN(msg) Logger::getInstance().warn(msg, __FILE__, __LINE__, __FUNCTION__)
#define LOG_ERROR(msg) Logger::getInstance().error(msg, __FILE__, __LINE__, __FUNCTION__)
#define LOG_FATAL(msg) Logger::getInstance().fatal(msg, __FILE__, __LINE__, __FUNCTION__)

#define LOG_TRACE_FIELDS(msg, fields) Logger::getInstance().trace(msg, fields, __FILE__, __LINE__, __FUNCTION__)
#define LOG_DEBUG_FIELDS(msg, fields) Logger::getInstance().debug(msg, fields, __FILE__, __LINE__, __FUNCTION__)
#define LOG_INFO_FIELDS(msg, fields) Logger::getInstance().info(msg, fields, __FILE__, __LINE__, __FUNCTION__)
#define LOG_WARN_FIELDS(msg, fields) Logger::getInstance().warn(msg, fields, __FILE__, __LINE__, __FUNCTION__)
#define LOG_ERROR_FIELDS(msg, fields) Logger::getInstance().error(msg, fields, __FILE__, __LINE__, __FUNCTION__)
#define LOG_FATAL_FIELDS(msg, fields) Logger::getInstance().fatal(msg, fields, __FILE__, __LINE__, __FUNCTION__)
