#pragma once
#include <iostream>
#include <chrono>

template <typename Arg, typename ...Args>
void threadLog(Arg&& arg, Args&&... args) {
    std::cout << "[Thread " << std::this_thread::get_id() << " (" << std::chrono::system_clock::now() << ")]: ";
    std::cout << std::forward<Arg>(arg);
    ((std::cout << ", " << std::forward<Args>(args)), ...);
    std::cout << std::endl;
}
