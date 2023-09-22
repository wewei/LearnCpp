#pragma once
#include <iostream>
#include <chrono>
#include <ctime>
#include <iomanip>

template <typename Arg, typename ...Args>
void threadLog(Arg&& arg, Args&&... args) {
    auto time = std::chrono::system_clock::now();
    time_t ctime = std::chrono::system_clock::to_time_t(time);
    std::cout << "[Thread "
              << std::this_thread::get_id()
              << " ("
              << std::put_time(std::localtime(&ctime), "%c %Z") << ")]: "
              << std::forward<Arg>(arg);

    ((std::cout << ", " << std::forward<Args>(args)), ...);
    std::cout << std::endl;
}
