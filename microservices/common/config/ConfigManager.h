#pragma once
#include <string>
#include <unordered_map>
#include <mutex>
#include <functional>
#include <vector>
#include <memory>
#include <thread>
#include <atomic>
#include <chrono>

#ifdef HAVE_CURL
#include "consul/ConsulClient.h"
#endif

// 配置变更回调函数類型
using ConfigChangeCallback = std::function<void(const std::string& key, const std::string& oldValue, const std::string& newValue)>;

// 配置项
struct ConfigItem {
    std::string key;
    std::string value;
    std::string defaultValue;
    bool isRequired;
    std::string description;
    std::chrono::system_clock::time_point lastUpdated;
    
    ConfigItem() : isRequired(false) {}
    ConfigItem(const std::string& k, const std::string& v, const std::string& def = "", bool required = false)
        : key(k), value(v), defaultValue(def), isRequired(required) {
        lastUpdated = std::chrono::system_clock::now();
    }
};

// 配置管理器
class ConfigManager {
public:
    static ConfigManager& getInstance();
    
    // 初始化配置管理器
    bool initialize(const std::string& consulUrl = "http://127.0.0.1:8500",
                   const std::string& configPrefix = "chat/",
                   bool enableHotReload = true);
    
    // 從环境变数加载配置
    void loadFromEnvironment();
    
    // 從 Consul KV 加载配置
    bool loadFromConsul();
    
    // 從 Consul KV 加载特定配置
    bool loadFromConsul(const std::string& key);
    
    // 监聽 Consul KV 变更
    void watchConsulChanges();
    
    // 设置 Consul 监聽間隔
    void setConsulWatchInterval(std::chrono::seconds interval);
    
    // 從文件加载配置
    bool loadFromFile(const std::string& configFile);
    
    // 获取配置值
    std::string getString(const std::string& key, const std::string& defaultValue = "");
    int getInt(const std::string& key, int defaultValue = 0);
    bool getBool(const std::string& key, bool defaultValue = false);
    double getDouble(const std::string& key, double defaultValue = 0.0);
    
    // 设置配置值
    void setString(const std::string& key, const std::string& value);
    void setInt(const std::string& key, int value);
    void setBool(const std::string& key, bool value);
    void setDouble(const std::string& key, double value);
    
    // 检查配置是否存在
    bool hasKey(const std::string& key);
    
    // 获取所有配置
    std::unordered_map<std::string, std::string> getAllConfigs();
    
    // 註冊配置变更回调
    void registerChangeCallback(const std::string& key, ConfigChangeCallback callback);
    void registerGlobalChangeCallback(ConfigChangeCallback callback);
    
    // 保存配置到 Consul KV
    bool saveToConsul();
    
    // 保存配置到文件
    bool saveToFile(const std::string& configFile);
    
    // 验证配置
    bool validateConfig();
    
    // 获取配置统計
    struct ConfigStats {
        int totalConfigs;
        int requiredConfigs;
        int optionalConfigs;
        int consulConfigs;
        int environmentConfigs;
        int fileConfigs;
        std::chrono::system_clock::time_point lastUpdate;
    };
    ConfigStats getConfigStats();
    
    // 启动配置监聽
    void startWatching();
    
    // 停止配置监聽
    void stopWatching();
    
    // 重新加载配置
    void reloadConfig();

private:
    ConfigManager() = default;
    ~ConfigManager() = default;
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;
    
    // 配置监聽線程
    void watchThread();
    
    // 处理配置变更
    void handleConfigChange(const std::string& key, const std::string& newValue);
    
    // 解析配置值
    std::string parseConfigValue(const std::string& value);
    
    // 验证配置项
    bool validateConfigItem(const ConfigItem& item);
    
    // 從字符串转换為其他類型
    int stringToInt(const std::string& str, int defaultValue = 0);
    bool stringToBool(const std::string& str, bool defaultValue = false);
    double stringToDouble(const std::string& str, double defaultValue = 0.0);
    
#ifdef HAVE_CURL
    std::unique_ptr<ConsulClient> consulClient_;
#endif
    
    // 配置存儲
    std::unordered_map<std::string, ConfigItem> configs_;
    std::mutex configsMutex_;
    
    // 回调函数
    std::unordered_map<std::string, std::vector<ConfigChangeCallback>> keyCallbacks_;
    std::vector<ConfigChangeCallback> globalCallbacks_;
    std::mutex callbacksMutex_;
    
    // 配置源
    std::unordered_map<std::string, std::string> consulConfigs_;
    std::unordered_map<std::string, std::string> environmentConfigs_;
    std::unordered_map<std::string, std::string> fileConfigs_;
    
    // 配置
    std::string consulUrl_;
    std::string configPrefix_;
    bool enableHotReload_;
    
    // 监聽線程
    std::atomic<bool> watching_;
    std::thread watchThread_;
    std::chrono::seconds watchInterval_;
    
    // 统計
    std::atomic<int> totalConfigs_;
    std::atomic<int> requiredConfigs_;
    std::chrono::system_clock::time_point lastUpdate_;
};
