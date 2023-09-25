#include <optional>

#include "./tasks3.h"
#include "./task.h"
#include "./threaded-runner.h"
#include "./thread-log.h"

struct Result {
    std::optional<int> value_;
    Result(): value_(std::nullopt) { }
    Result(int value): value_(value) { }
    Result(const Result &r) {
        threadLog("Copying", r);
        value_ = r.value_;
    }
    Result(Result &&r) {
        threadLog("Moving", r);
        value_ = r.value_;
    }

    explicit operator bool() const {
        return (bool)value_;
    }
};

std::ostream &operator << (std::ostream &ostm, const Result &rlt) {
    if (rlt.value_.has_value()) {
        ostm << "Result(" << rlt.value_.value() << ")";
    } else {
        ostm << "Result(nullopt)";
    }
    return ostm;
}

void f(const Result &r) {
    [r]() {
        std::cout << r << std::endl;
    }();
}

template <typename T>
using Tk = Task<std::function, T>;

template <typename T>
using Hdl = typename Tk<T>::Handler;

void Tasks3Demo::run() {
    ThreadedRunner threadedRunner;
    auto runner = [&threadedRunner](int ms, std::function<void()> callback) {
        threadedRunner.delay(ms, callback);
    };

    f(Result(357));

    Result r(7);
    // Task 0
    auto task0 = Tk<Result>::Resolve(std::move(r));

    threadLog("Run task0");
    task0.Run(threadLog<const Result &>);

    // Task 1
    Tk<Result> task1([](const Hdl<Result> &handler) {
        threadLog("Kickoff task1");
        handler(123);
    });

    threadLog("Run task1");
    task1.Run(threadLog<const Result &>);

    // Task 2
    auto task2 = task1.DelayedFor(runner, 1000);

    threadLog("Run task2");
    task2.Run(threadLog<const Result &>);

    // Task 3
    auto task3 = task1.ThenDelayFor(runner, 2000);

    threadLog("Run task3");
    task3.Run(threadLog<const Result &>);

    // Retries
    auto task4 = Tk<Result>([](const Hdl<Result> &handler) {
        threadLog("Kickoff task4");
        handler(Result());
    }).DelayedFor(runner, 3000);

    auto task5 = Tk<Result>([](const Hdl<Result> &handler) {
        threadLog("Kickoff task5");
        handler(1);
    }).DelayedFor(runner, 1000);

    auto task6 = task4.Or(task5);

    task6.Run(threadLog<const Result &>);

    // Join all threads
    threadedRunner.join();
}
