#include <functional>
#include <thread>
#include <iostream>
#include <vector>
#include <type_traits>
#include "./tasks.h"

template <typename T>
using Handler = std::function<void(T)>;

template <typename Val, typename Err>
using Task = std::function<void(Handler<const Val &>, Handler<const Err &>)>;

template <typename T>
using Decorator = std::function<T(T)>;

template <typename Val, typename Err>
Task<Val, Err> either(Task<Val, Err> task1, Task<Val, Err> task2) {
    return [=](Handler<const Val &> resolve, Handler<const Err &> reject) {
        task1(resolve, [=](Err _) { task2(resolve, reject); });
    };
}

struct TimeoutRunner {
    virtual void timeout(int ms, std::function<void()> callback) = 0;
};

template <typename Val, typename Err>
Task<Val, Err> withDelay(TimeoutRunner &runner, int ms, Task<Val, Err> task) {
    return [=, &runner](Handler<const Val &> resolve, Handler<const Err &> reject) {
        runner.timeout(ms, [=]() { task(resolve, reject); });
    };
}

template <typename Val, typename Err>
Task<Val, Err> withRetry(TimeoutRunner &runner, int ms, Task<Val, Err> task1, Task<Val, Err> task2) {
    return either(task1, withDelay(runner, ms, task2));
}

template <typename Val, typename Err>
Task<Val, Err> withFinally(std::function<void()> callback, Task<Val, Err> task) {
    return [=](Handler<const Val &> resolve, Handler<const Err &> reject) {
        task(
            [=](Val val) { resolve(val); callback(); },
            [=](Err err) { reject(err); callback(); }
        );
    };
}

// template <typename Ret, typename ...Args>
// Decorator<std::function<Ret(Args...)>> prependBehavior(std::function<void(Args...)> callback) {
//     return [=](std::function<Ret(Args...)> func) {
//         return [=](Args ...args) {
//             callback(&args...);
//             return func(&args...);
//         };
//     };
// }

// template <typename F, typename ...Args>
// Decorator<F> prependBehavior(std::function<void(Args...)> callback) {
//     return [=](std::function<void(Args...)> func) {
//         return [=](Args ...args) {
//             callback(&args...);
//             return func(&args...);
//         };
//     };
// }

// template <typename F, typename ...Args>
// Decorator<F> appendBehavior(std::function<void(Ret, Args...)> callback) {
//     return [=](std::function<Ret(Args...)> func) {
//         return [=](Args ...args) {
//             Ret result = func(&args ...);
//             callback(result, &args ...);
//             return result;
//         };
//     };
// }

// template <typename F, typename... Args>
// Decorator<F> appendBehavior(F callback) {
//     return [=](std::function<void(Args...)> func) {
//         return [=](Args ...args) {
//             func(&args ...);
//             callback(&args ...);
//         };
//     };
// }

// template <typename F, typename... Args>
// F appendBehavior2(F callback, F func) {
//     return [=](Args... args) {
//         func(&args...);
//         callback(&args...);
//     };
// }

// Test utilities
template <typename Val, typename Err>
Task<Val, Err> resolveWith(Val val) {
    return [=](Handler<const Val &> resolve, Handler<const Err &> _) { resolve(val); };
}

template <typename Val, typename Err>
Task<Val, Err> rejectWith(Err err) {
    return [=](Handler<const Val &> _, Handler<const Err &> reject) { reject(err); };
}

template <typename Arg, typename ...Args>
void threadLog(Arg&& arg, Args&&... args) {
    std::cout << "[Thread " << std::this_thread::get_id() << "]: ";
    std::cout << std::forward<Arg>(arg);
    ((std::cout << ", " << std::forward<Args>(args)), ...);
    std::cout << std::endl;
}

class ThreadRunner : public TimeoutRunner {
private:
    std::vector<std::thread> threads;
public:
    ~ThreadRunner() {
        for (std::thread &thread: threads) {
            threadLog("Joining thread", thread.get_id());
            thread.join();
        }
    }
    virtual void timeout(int ms, std::function<void()> callback) override;
};

ThreadRunner globalRunner;

void ThreadRunner::timeout(int ms, std::function<void()> callback) {
    threads.push_back(std::thread([=]() {
        threadLog("Thread started");
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
        callback();
    }));
}

void TasksDemo::run() {
    int i = 10;

    rejectWith<int, const char *>("[Error 1]")(
        [](int _) { threadLog("Never get here!"); },
        [](auto err) { threadLog("Task rejected", err); }
    );

    withDelay(globalRunner, 2000, rejectWith<int, const char *>("[Error 2]"))(
        [](int _) { threadLog("Never get here!"); },
        [](auto err) { threadLog("Task rejected", err); }
    );

    withRetry(globalRunner, 3000, rejectWith<int, const char *>("Error 3"), resolveWith<int, const char *>(42))(
        [=](int val) { threadLog("Task resolved", val, "with local variable i", i); },
        [](auto _) { threadLog("Never get here!"); }
    );

    withFinally(
        []() { threadLog("Finally block executed"); },
        withRetry(globalRunner, 4000, rejectWith<int, const char *>("Error 3"), resolveWith<int, const char *>(100))
    )(
        [](int val) { threadLog("Task resolved", val); },
        [](auto _) { threadLog("Never get here!"); }
    );
}
