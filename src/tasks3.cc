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

void Tasks3Demo::run() {
    ThreadedRunner threadedRunner;
    DelayedRunner runner = [&threadedRunner](int ms, std::function<void()> callback) {
        threadedRunner.delay(ms, callback);
    };

    f(Result(357));

    // Task 0
    auto task0 = Task<Result>::Resolve(Result(1));

    threadLog("Run task0");
    task0.Run(threadLog<const Result &>);

    // Task 1
    auto task1 = Task<Result>([](Handler<Result> handler) {
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
    auto task4 = Task<Result>([](Handler<Result> handler) {
        threadLog("Kickoff task4");
        handler(Result());
    }).DelayedFor(runner, 3000);

    auto task5 = Task<Result>([](Handler<Result> handler) {
        threadLog("Kickoff task5");
        handler(1);
    }).DelayedFor(runner, 1000);

    auto task6 = task4.Or(task5);

    task6.Run(threadLog<const Result &>);

    // Join all threads
    threadedRunner.join();
}
