#include "./tasks3.h"
#include "./task.h"
#include "./threaded-runner.h"
#include "./thread-log.h"

template <typename T>
Delayer<T> threadedDelayer(ThreadedRunner &runner) {
    return [&runner](int ms, Handler<T> handler) {
        return [=, &runner](T t) {
            runner.delay(ms, [handler, t]() {
                handler(t);
            });
        };
    };
}

void Tasks3Demo::run() {
    ThreadedRunner runner;
    auto delayer = threadedDelayer<Handler<int>>(runner);

    // Task 0
    auto task0 = Task<int>::Resolve(1);

    threadLog("Run task0");
    task0.run(threadLog<int>);

    // Task 1
    auto task1 = Task<int>([](Handler<int> handler) {
        threadLog("Kickoff task1");
        handler(123);
    });

    threadLog("Run task1");
    task1.run(threadLog<int>);

    // Task 2
    auto task2 = task1.DelayedFor(delayer, 1000);

    threadLog("Run task2");
    task2.run(threadLog<int>);

    // Task 3
    auto task3 = task1.ThenDelayFor(delayer, 2000);

    threadLog("Run task3");
    task3.run(threadLog<int>);

    // Join all threads
    runner.join();
}
