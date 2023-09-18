#pragma once
#include <concepts>
#include <string>

template<typename T>
concept Demo = requires(T) {
    { T::name } -> std::convertible_to<std::string>;
    { T::run() } -> std::same_as<void>;
};

template<Demo D>
void runDemo() {
    std::cout << "Run demo {" << D::name << "}" << std::endl;
    D::run();
}
