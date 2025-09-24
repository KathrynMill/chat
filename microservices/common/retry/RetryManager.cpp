#include "RetryManager.h"
#include <iostream>
#include <future>
#include <thread>
#include <cmath>

RetryManager& RetryManager::getInstance() {
    static RetryManager instance;
    return instance;
}

bool RetryManager::initialize() {
    std::cout << "RetryManager initialized\n";
    return true;
}
