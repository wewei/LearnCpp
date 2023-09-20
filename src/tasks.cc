#include <functional>
#include <thread>
#include <iostream>
#include <vector>
#include <type_traits>
#include <variant>
#include "./tasks.h"

template <typename T>
using Handler = std::function<void(T)>;

template <typename Err, typename Val>
using Task = std::function<void(Handler<Err>, Handler<Val>)>;

template <typename T>
using Decorator = std::function<T(T)>;

template <typename Err, typename Val>
Task<Err, Val> either(Task<Err, Val> task1, Task<Err, Val> task2) {
    return [=](Handler<const Err &> reject, Handler<Val> resolve) {
        task1([=](Err _) { task2(reject, resolve); }, resolve);
    };
}

struct TimeoutRunner {
    virtual void timeout(int ms, std::function<void()> callback) = 0;
};

template <typename Err, typename Val>
Task<Err, Val> withDelay(TimeoutRunner &runner, int ms, Task<Err, Val> task) {
    return [=, &runner](Handler<Err> reject, Handler<Val> resolve) {
        runner.timeout(ms, [=]() { task(reject, resolve); });
    };
}

template <typename Err, typename Val>
Task<Err, Val> withRetry(TimeoutRunner &runner, int ms, Task<Err, Val> task1, Task<Err, Val> task2) {
    return either(task1, withDelay(runner, ms, task2));
}

template <typename Err, typename Val>
Task<Err, Val> withFinally(std::function<void()> callback, Task<Err, Val> task) {
    return [=](Handler<Err> reject, Handler<Val> resolve) {
        task(
            [=](Err err) { reject(err); callback(); },
            [=](Val val) { resolve(val); callback(); }
        );
    };
}

template <typename Err, typename Val>
Task<Err, Val> resolveWith(Val val) {
    return [=](Handler<Err> _, Handler<Val> resolve) { resolve(val); };
}

template <typename Err, typename Val>
Task<Err, Val> rejectWith(Err err) {
    return [=](Handler<Err> reject, Handler<Val> _) {
        reject(err);
    };
}


template <typename Err, typename Val>
Task<Err, Val> pure(Val val) {
    return resolveWith<Err, Val>(val);
}

template <typename Err, typename ValF, typename ValT>
Task<Err, ValT> operator >> (Task<Err, ValF> taskF, std::function<Task<Err, ValT>(ValF)> f) {
    return [=](Handler<Err> reject, Handler<ValT> resolve) {
        taskF(reject, [=](ValF valF) {
            f(valF)(reject, resolve);
        });
    };
}

template <typename Err, typename Val>
Task<Err, Val> operator | (Task<Err, Val> task1, Task<Err, Val> task2) {
    return either(task1, task2);
}

template <typename Err, typename Val>
std::function<Task<Err, Val>(Val)> delay(TimeoutRunner &runner, int ms) {
    return [=, &runner](Val value) {
        return [=, &runner](auto _, auto resolve) {
            runner.timeout(ms, [=]() { resolve(value); });
        };
    };
}

template <typename Err, typename Val>
std::function<Task<Err, Val>(Val)> tap(std::function<void(Val)> callback) {
    return [=](Val val) {
        callback(val);
        return pure<Err, Val>(val);
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
    virtual void timeout(int ms, std::function<void()> callback) override;
    void join();
};

void ThreadRunner::timeout(int ms, std::function<void()> callback) {
    threads.push_back(std::thread([=]() {
        threadLog("Thread started");
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
        callback();
    }));
}

void ThreadRunner::join() {
    while (threads.size() > 0) {
        std::vector<std::thread> threadsT = std::move(threads);
        for (std::thread &thread : threadsT) {
            threadLog("Joining thread", thread.get_id());
            thread.join();
        }
    }
}

void TasksDemo::run() {
    int i = 10;

    ThreadRunner runner;

    // resolveWith<const char *, int>(13);

    // rejectWith<const char *, int>("[Error 1]")(
    //     [](auto err) { threadLog("Task rejected", err); },
    //     [](int _) { threadLog("Never get here!"); }
    // );

    // withDelay(globalRunner, 2000, rejectWith<const char *, int>("[Error 2]"))(
    //     [](auto err) { threadLog("Task rejected", err); },
    //     [](int _) { threadLog("Never get here!"); }
    // );

    // withRetry(globalRunner, 3000, rejectWith<const char *, int>("Error 3"), resolveWith<const char *, int>(42))(
    //     [](auto _) { threadLog("Never get here!"); },
    //     [=](int val) { threadLog("Task resolved", val, "with local variable i", i); }
    // );

    // withFinally<const char *, int>(
    //     []() { threadLog("Finally block executed"); },
    //     withRetry(globalRunner, 4000, rejectWith<const char *, int>("Error 3"), resolveWith<const char *, int>(100))
    // )(
    //     [](auto _) { threadLog("Never get here!"); },
    //     [](int val) { threadLog("Task resolved", val); }
    // );

    auto task1 =
        ( withDelay<const char *, int>(runner, 1000, rejectWith<const char *, int>("Failed 1st time"))
        | withDelay<const char *, int>(runner, 1000, rejectWith<const char *, int>("Failed 2nd time"))
        | withDelay<const char *, int>(runner, 1000, rejectWith<const char *, int>("Failed 3rd time"))
        | withDelay<const char *, int>(runner, 1000, resolveWith<const char *, int>(123))
        )
            >> delay<const char *, int>(runner, 1000)
            >> tap<const char *, int>([](int val) { threadLog("Hello 1"); })
            >> delay<const char *, int>(runner, 1000)
            >> tap<const char *, int>([](int val) { threadLog("Hello 2"); });

    task1(
        [](auto _) { threadLog("Never get here!"); },
        [](int val) { threadLog("Task resolved", val); }
    );

    runner.join();
    threadLog("Exit demo");
}
