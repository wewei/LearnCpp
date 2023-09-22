#pragma once

#include <vector>
#include <thread>
#include <functional>

class ThreadedRunner {
private:
    std::vector<std::thread> threads;
public:
    void delay(int ms, std::function<void()> callback);
    void join();
};
