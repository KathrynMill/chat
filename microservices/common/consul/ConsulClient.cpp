#include "ConsulClient.h"
#include <iostream>
#include <sstream>
#include <thread>
#include <chrono>

#ifdef HAVE_CURL
#include <curl/curl.h>
#endif

#ifdef HAVE_JSON
#include "json.hpp"
using json = nlohmann::json;
#endif

ConsulClient::ConsulClient(const std::string& consulUrl) : consulUrl_(consulUrl) {
#ifdef HAVE_CURL
    curl_global_init(CURL_GLOBAL_DEFAULT);
#endif
}

ConsulClient::~ConsulClient() {
#ifdef HAVE_CURL
    curl_global_cleanup();
#endif
}

bool ConsulClient::registerService(const std::string& serviceName, 
                                  const std::string& serviceId,
                                  const std::string& address, 
                                  int port,
                                  const std::vector<std::string>& tags) {
#ifdef HAVE_JSON
    json service;
    service["ID"] = serviceId;
    service["Name"] = serviceName;
    service["Address"] = address;
    service["Port"] = port;
    service["Tags"] = json::array();
    for (const auto& tag : tags) {
        service["Tags"].push_back(tag);
    }
    
    // 健康检查
    json check;
    check["HTTP"] = "http://" + address + ":" + std::to_string(port) + "/health";
    check["Interval"] = "10s";
    check["Timeout"] = "3s";
    service["Check"] = check;
    
    std::string path = "/v1/agent/service/register";
    std::string response = makeRequest("PUT", path, service.dump());
    
    return !response.empty();
#else
    std::cerr << "ConsulClient: JSON library not available\n";
    return false;
#endif
}

bool ConsulClient::deregisterService(const std::string& serviceId) {
    std::string path = "/v1/agent/service/deregister/" + serviceId;
    std::string response = makeRequest("PUT", path);
    return !response.empty();
}

std::vector<ServiceInstance> ConsulClient::getHealthyServiceInstances(const std::string& serviceName) {
    std::vector<ServiceInstance> instances;
    
#ifdef HAVE_JSON
    std::string path = "/v1/health/service/" + serviceName + "?passing=true";
    std::string response = makeRequest("GET", path);
    
    if (!response.empty()) {
        try {
            auto services = json::parse(response);
            for (const auto& service : services) {
                ServiceInstance instance;
                instance.id = service["Service"]["ID"];
                instance.name = service["Service"]["Name"];
                instance.address = service["Service"]["Address"];
                instance.port = service["Service"]["Port"];
                instance.healthy = true;
                
                for (const auto& tag : service["Service"]["Tags"]) {
                    instance.tags.push_back(tag);
                }
                
                instances.push_back(instance);
            }
        } catch (const std::exception& e) {
            std::cerr << "ConsulClient: JSON parse error: " << e.what() << "\n";
        }
    }
#endif
    
    return instances;
}

bool ConsulClient::checkServiceHealth(const std::string& serviceId) {
    std::string path = "/v1/agent/health/service/id/" + serviceId;
    std::string response = makeRequest("GET", path);
    return !response.empty() && response.find("\"Status\":\"passing\"") != std::string::npos;
}

std::string ConsulClient::makeRequest(const std::string& method, const std::string& path, const std::string& body) {
    std::string url = consulUrl_ + path;
    
    if (method == "GET") {
        return httpGet(url);
    } else if (method == "PUT") {
        return httpPut(url, body);
    } else if (method == "DELETE") {
        return httpDelete(url);
    }
    
    return "";
}

std::string ConsulClient::httpGet(const std::string& url) {
#ifdef HAVE_CURL
    CURL* curl = curl_easy_init();
    if (!curl) return "";
    
    std::string response;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, [](void* contents, size_t size, size_t nmemb, std::string* s) {
        size_t newLength = size * nmemb;
        s->append((char*)contents, newLength);
        return newLength;
    });
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
    
    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    
    return (res == CURLE_OK) ? response : "";
#else
    std::cerr << "ConsulClient: CURL not available, using fallback\n";
    return "";
#endif
}

std::string ConsulClient::httpPut(const std::string& url, const std::string& body) {
#ifdef HAVE_CURL
    CURL* curl = curl_easy_init();
    if (!curl) return "";
    
    std::string response;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, body.length());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, [](void* contents, size_t size, size_t nmemb, std::string* s) {
        size_t newLength = size * nmemb;
        s->append((char*)contents, newLength);
        return newLength;
    });
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
    
    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    
    return (res == CURLE_OK) ? response : "";
#else
    std::cerr << "ConsulClient: CURL not available, using fallback\n";
    return "";
#endif
}

std::string ConsulClient::httpDelete(const std::string& url) {
#ifdef HAVE_CURL
    CURL* curl = curl_easy_init();
    if (!curl) return "";
    
    std::string response;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, [](void* contents, size_t size, size_t nmemb, std::string* s) {
        size_t newLength = size * nmemb;
        s->append((char*)contents, newLength);
        return newLength;
    });
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
    
    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    
    return (res == CURLE_OK) ? response : "";
#else
    std::cerr << "ConsulClient: CURL not available, using fallback\n";
    return "";
#endif
}
