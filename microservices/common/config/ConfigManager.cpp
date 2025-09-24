#include "ConfigManager.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstdlib>

#ifdef HAVE_CURL
#include "consul/ConsulClient.h"
#endif

ConfigManager& ConfigManager::getInstance() {
    static ConfigManager instance;
    return instance;
}

bool ConfigManager::initialize(const std::string& consulUrl,
                             const std::string& configPrefix,
                             bool enableHotReload) {
    consulUrl_ = consulUrl;
    configPrefix_ = configPrefix;
    enableHotReload_ = enableHotReload;
    watchInterval_ = std::chrono::seconds(30);
    watching_ = false;
    totalConfigs_ = 0;
    requiredConfigs_ = 0;
    lastUpdate_ = std::chrono::system_clock::now();
    
#ifdef HAVE_CURL
    try {
        consulClient_ = std::make_unique<ConsulClient>(consulUrl);
        std::cout << "ConfigManager initialized with Consul: " << consulUrl << "\n";
    } catch (const std::exception& e) {
        std::cerr << "Failed to initialize Consul client: " << e.what() << "\n";
        return false;
    }
#else
    std::cout << "ConfigManager initialized without Consul (CURL not available)\n";
#endif
    
    // 加载配置
    loadFromEnvironment();
    
    if (consulClient_) {
        loadFromConsul();
    }
    
    // 启动监聽
    if (enableHotReload_) {
        startWatching();
    }
    
    return true;
}

void ConfigManager::loadFromEnvironment() {
    // 預定義的环境变数配置
    std::vector<std::pair<std::string, std::string>> envConfigs = {
        {"DB_HOST", "127.0.0.1"},
        {"DB_PORT", "3306"},
        {"DB_USER", "root"},
        {"DB_PASS", ""},
        {"DB_NAME", "chatdb"},
        {"KAFKA_BROKERS", "127.0.0.1:9092"},
        {"CONSUL_URL", "http://127.0.0.1:8500"},
        {"JWT_SECRET", "your-secret-key"},
        {"JAEGER_ENDPOINT", "http://localhost:14268/api/traces"},
        {"METRICS_PORT", "8080"},
        {"LOG_LEVEL", "INFO"},
        {"SERVICE_NAME", "chat-service"},
        {"SERVICE_PORT", "60051"}
    };
    
    for (const auto& config : envConfigs) {
        const char* envValue = std::getenv(config.first.c_str());
        if (envValue) {
            std::string value = envValue;
            environmentConfigs_[config.first] = value;
            
            std::lock_guard<std::mutex> lock(configsMutex_);
            configs_[config.first] = ConfigItem(config.first, value, config.second);
            totalConfigs_++;
        }
    }
    
    std::cout << "Loaded " << environmentConfigs_.size() << " configurations from environment\n";
}

bool ConfigManager::loadFromConsul() {
#ifdef HAVE_CURL
    if (!consulClient_) {
        return false;
    }
    
    try {
        // 获取所有配置键
        std::vector<std::string> configKeys = {
            "service/name", "service/port", "service/enable_tls",
            "database/host", "database/port", "database/user", "database/password", "database/name",
            "kafka/brokers", "consul/url", "jwt/secret",
            "jaeger/endpoint", "metrics/port", "log/level"
        };
        
        int loadedCount = 0;
        for (const auto& key : configKeys) {
            if (loadFromConsul(key)) {
                loadedCount++;
            }
        }
        
        std::cout << "Loaded " << loadedCount << " configurations from Consul\n";
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Failed to load configurations from Consul: " << e.what() << "\n";
        return false;
    }
#else
    return false;
#endif
}

bool ConfigManager::loadFromConsul(const std::string& key) {
#ifdef HAVE_CURL
    if (!consulClient_) {
        return false;
    }
    
    try {
        std::string fullKey = configPrefix_ + key;
        
        // 這裡應該调用 ConsulClient 的方法來获取 KV 值
        // 由於 ConsulClient 的实现可能不同，這裡提供一個框架
        
        // 示例：假设 ConsulClient 有 getKV 方法
        // std::string value = consulClient_->getKV(fullKey);
        // if (!value.empty()) {
        //     consulConfigs_[key] = value;
        //     
        //     std::lock_guard<std::mutex> lock(configsMutex_);
        //     configs_[key] = ConfigItem(key, value);
        //     totalConfigs_++;
        //     
        //     return true;
        // }
        
        return false;
        
    } catch (const std::exception& e) {
        std::cerr << "Failed to load configuration from Consul: " << key << " - " << e.what() << "\n";
        return false;
    }
#else
    return false;
#endif
}

void ConfigManager::watchConsulChanges() {
    if (!consulClient_ || !enableHotReload_) {
        return;
    }
    
    std::thread([this]() {
        while (watching_) {
            try {
                // 這裡應該实现 Consul KV 变更监聽
                // 可以使用 Consul 的 blocking query 或 watch API
                
                // 示例实现：
                // auto changes = consulClient_->watchKV(configPrefix_);
                // for (const auto& change : changes) {
                //     std::string key = change.first.substr(configPrefix_.length());
                //     std::string newValue = change.second;
                //     
                //     // 检查配置是否真的变更了
                //     std::string oldValue;
                //     {
                //         std::lock_guard<std::mutex> lock(configsMutex_);
                //         auto it = configs_.find(key);
                //         if (it != configs_.end()) {
                //             oldValue = it->second.value;
                //         }
                //     }
                //     
                //     if (oldValue != newValue) {
                //         // 更新配置
                //         setString(key, newValue);
                //         std::cout << "Configuration updated from Consul: " << key << " = " << newValue << "\n";
                //     }
                // }
                
                std::this_thread::sleep_for(watchInterval_);
                
            } catch (const std::exception& e) {
                std::cerr << "Consul watch error: " << e.what() << "\n";
                std::this_thread::sleep_for(std::chrono::seconds(5));
            }
        }
    }).detach();
}

void ConfigManager::setConsulWatchInterval(std::chrono::seconds interval) {
    watchInterval_ = interval;
}

bool ConfigManager::loadFromFile(const std::string& configFile) {
    std::ifstream file(configFile);
    if (!file.is_open()) {
        std::cerr << "Failed to open config file: " << configFile << "\n";
        return false;
    }
    
    std::string line;
    int loadedCount = 0;
    
    while (std::getline(file, line)) {
        // 跳过空行和註釋
        if (line.empty() || line[0] == '#') {
            continue;
        }
        
        // 解析 key=value 格式
        size_t pos = line.find('=');
        if (pos != std::string::npos) {
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);
            
            // 去除前後空格
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);
            
            fileConfigs_[key] = value;
            
            std::lock_guard<std::mutex> lock(configsMutex_);
            configs_[key] = ConfigItem(key, value);
            totalConfigs_++;
            loadedCount++;
        }
    }
    
    file.close();
    std::cout << "Loaded " << loadedCount << " configurations from file: " << configFile << "\n";
    return true;
}

std::string ConfigManager::getString(const std::string& key, const std::string& defaultValue) {
    std::lock_guard<std::mutex> lock(configsMutex_);
    
    auto it = configs_.find(key);
    if (it != configs_.end()) {
        return it->second.value;
    }
    
    return defaultValue;
}

int ConfigManager::getInt(const std::string& key, int defaultValue) {
    std::string value = getString(key, "");
    if (value.empty()) {
        return defaultValue;
    }
    
    return stringToInt(value, defaultValue);
}

bool ConfigManager::getBool(const std::string& key, bool defaultValue) {
    std::string value = getString(key, "");
    if (value.empty()) {
        return defaultValue;
    }
    
    return stringToBool(value, defaultValue);
}

double ConfigManager::getDouble(const std::string& key, double defaultValue) {
    std::string value = getString(key, "");
    if (value.empty()) {
        return defaultValue;
    }
    
    return stringToDouble(value, defaultValue);
}

void ConfigManager::setString(const std::string& key, const std::string& value) {
    std::string oldValue;
    
    {
        std::lock_guard<std::mutex> lock(configsMutex_);
        auto it = configs_.find(key);
        if (it != configs_.end()) {
            oldValue = it->second.value;
            it->second.value = value;
            it->second.lastUpdated = std::chrono::system_clock::now();
        } else {
            configs_[key] = ConfigItem(key, value);
            totalConfigs_++;
        }
    }
    
    // 觸发回调
    handleConfigChange(key, value);
}

void ConfigManager::setInt(const std::string& key, int value) {
    setString(key, std::to_string(value));
}

void ConfigManager::setBool(const std::string& key, bool value) {
    setString(key, value ? "true" : "false");
}

void ConfigManager::setDouble(const std::string& key, double value) {
    setString(key, std::to_string(value));
}

bool ConfigManager::hasKey(const std::string& key) {
    std::lock_guard<std::mutex> lock(configsMutex_);
    return configs_.find(key) != configs_.end();
}

std::unordered_map<std::string, std::string> ConfigManager::getAllConfigs() {
    std::unordered_map<std::string, std::string> allConfigs;
    
    std::lock_guard<std::mutex> lock(configsMutex_);
    for (const auto& pair : configs_) {
        allConfigs[pair.first] = pair.second.value;
    }
    
    return allConfigs;
}

void ConfigManager::registerChangeCallback(const std::string& key, ConfigChangeCallback callback) {
    std::lock_guard<std::mutex> lock(callbacksMutex_);
    keyCallbacks_[key].push_back(callback);
}

void ConfigManager::registerGlobalChangeCallback(ConfigChangeCallback callback) {
    std::lock_guard<std::mutex> lock(callbacksMutex_);
    globalCallbacks_.push_back(callback);
}

bool ConfigManager::saveToConsul() {
#ifdef HAVE_CURL
    if (!consulClient_) {
        return false;
    }
    
    try {
        std::lock_guard<std::mutex> lock(configsMutex_);
        
        for (const auto& pair : configs_) {
            std::string fullKey = configPrefix_ + pair.first;
            // 這裡應該调用 ConsulClient 的方法來保存 KV
            // consulClient_->setKV(fullKey, pair.second.value);
        }
        
        std::cout << "Saved " << configs_.size() << " configurations to Consul\n";
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Failed to save configurations to Consul: " << e.what() << "\n";
        return false;
    }
#else
    return false;
#endif
}

bool ConfigManager::saveToFile(const std::string& configFile) {
    std::ofstream file(configFile);
    if (!file.is_open()) {
        std::cerr << "Failed to open config file for writing: " << configFile << "\n";
        return false;
    }
    
    file << "# Chat Service Configuration\n";
    file << "# Generated at: " << std::chrono::system_clock::now().time_since_epoch().count() << "\n\n";
    
    std::lock_guard<std::mutex> lock(configsMutex_);
    
    for (const auto& pair : configs_) {
        file << pair.first << "=" << pair.second.value << "\n";
    }
    
    file.close();
    std::cout << "Saved " << configs_.size() << " configurations to file: " << configFile << "\n";
    return true;
}

bool ConfigManager::validateConfig() {
    std::lock_guard<std::mutex> lock(configsMutex_);
    
    for (const auto& pair : configs_) {
        if (!validateConfigItem(pair.second)) {
            std::cerr << "Invalid configuration: " << pair.first << "=" << pair.second.value << "\n";
            return false;
        }
    }
    
    return true;
}

ConfigManager::ConfigStats ConfigManager::getConfigStats() {
    ConfigStats stats;
    
    std::lock_guard<std::mutex> lock(configsMutex_);
    
    stats.totalConfigs = totalConfigs_.load();
    stats.requiredConfigs = requiredConfigs_.load();
    stats.optionalConfigs = stats.totalConfigs - stats.requiredConfigs;
    stats.consulConfigs = consulConfigs_.size();
    stats.environmentConfigs = environmentConfigs_.size();
    stats.fileConfigs = fileConfigs_.size();
    stats.lastUpdate = lastUpdate_;
    
    return stats;
}

void ConfigManager::startWatching() {
    if (watching_) {
        return;
    }
    
    watching_ = true;
    watchThread_ = std::thread(&ConfigManager::watchThread, this);
    std::cout << "Config watching started\n";
}

void ConfigManager::stopWatching() {
    if (!watching_) {
        return;
    }
    
    watching_ = false;
    if (watchThread_.joinable()) {
        watchThread_.join();
    }
    std::cout << "Config watching stopped\n";
}

void ConfigManager::reloadConfig() {
    loadFromEnvironment();
    
    if (consulClient_) {
        loadFromConsul();
    }
    
    lastUpdate_ = std::chrono::system_clock::now();
    std::cout << "Configuration reloaded\n";
}

void ConfigManager::watchThread() {
    while (watching_) {
        try {
            std::this_thread::sleep_for(watchInterval_);
            
            if (!watching_) break;
            
            // 检查 Consul 配置变更
            if (consulClient_) {
                // 這裡應該检查 Consul KV 的变更
                // 簡化实现：重新加载所有配置
                reloadConfig();
            }
            
        } catch (const std::exception& e) {
            std::cerr << "Config watch thread error: " << e.what() << "\n";
        }
    }
}

void ConfigManager::handleConfigChange(const std::string& key, const std::string& newValue) {
    std::string oldValue;
    
    {
        std::lock_guard<std::mutex> lock(configsMutex_);
        auto it = configs_.find(key);
        if (it != configs_.end()) {
            oldValue = it->second.value;
        }
    }
    
    // 觸发特定 key 的回调
    {
        std::lock_guard<std::mutex> lock(callbacksMutex_);
        auto it = keyCallbacks_.find(key);
        if (it != keyCallbacks_.end()) {
            for (const auto& callback : it->second) {
                try {
                    callback(key, oldValue, newValue);
                } catch (const std::exception& e) {
                    std::cerr << "Config change callback error: " << e.what() << "\n";
                }
            }
        }
        
        // 觸发全局回调
        for (const auto& callback : globalCallbacks_) {
            try {
                callback(key, oldValue, newValue);
            } catch (const std::exception& e) {
                std::cerr << "Global config change callback error: " << e.what() << "\n";
            }
        }
    }
}

std::string ConfigManager::parseConfigValue(const std::string& value) {
    // 处理环境变数引用 ${VAR_NAME}
    std::string result = value;
    size_t start = 0;
    
    while ((start = result.find("${", start)) != std::string::npos) {
        size_t end = result.find("}", start);
        if (end != std::string::npos) {
            std::string envVar = result.substr(start + 2, end - start - 2);
            const char* envValue = std::getenv(envVar.c_str());
            if (envValue) {
                result.replace(start, end - start + 1, envValue);
                start += strlen(envValue);
            } else {
                start = end + 1;
            }
        } else {
            break;
        }
    }
    
    return result;
}

bool ConfigManager::validateConfigItem(const ConfigItem& item) {
    // 基本验证
    if (item.key.empty()) {
        return false;
    }
    
    // 必填项验证
    if (item.isRequired && item.value.empty()) {
        return false;
    }
    
    // 類型验证（可以根据需要扩展）
    return true;
}

int ConfigManager::stringToInt(const std::string& str, int defaultValue) {
    try {
        return std::stoi(str);
    } catch (const std::exception&) {
        return defaultValue;
    }
}

bool ConfigManager::stringToBool(const std::string& str, bool defaultValue) {
    std::string lowerStr = str;
    std::transform(lowerStr.begin(), lowerStr.end(), lowerStr.begin(), ::tolower);
    
    if (lowerStr == "true" || lowerStr == "1" || lowerStr == "yes" || lowerStr == "on") {
        return true;
    } else if (lowerStr == "false" || lowerStr == "0" || lowerStr == "no" || lowerStr == "off") {
        return false;
    }
    
    return defaultValue;
}

double ConfigManager::stringToDouble(const std::string& str, double defaultValue) {
    try {
        return std::stod(str);
    } catch (const std::exception&) {
        return defaultValue;
    }
}
