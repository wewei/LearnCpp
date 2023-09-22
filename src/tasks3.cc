#include "./tasks3.h"
#include "./task.h"
#include "./threaded-runner.h"
#include "./thread-log.h"

void Tasks3Demo::run() {
    ThreadedRunner threadedRunner;
    DelayedRunner runner = [&threadedRunner](int ms, std::function<void()> callback) {
        threadedRunner.delay(ms, callback);
    };

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
    auto task2 = task1.DelayedFor(runner, 1000);

    threadLog("Run task2");
    task2.run(threadLog<int>);

    // Task 3
    auto task3 = task1.ThenDelayFor(runner, 2000);

    threadLog("Run task3");
    task3.run(threadLog<int>);

    // Retries
    auto task4 = Task<int>([](Handler<int> handler) {
        threadLog("Kickoff task4");
        handler(0);
    }).DelayedFor(runner, 3000);

    auto task5 = Task<int>([](Handler<int> handler) {
        threadLog("Kickoff task5");
        handler(1);
    }).DelayedFor(runner, 1000);

    auto task6 = task4.Or(task5);

    task6.run(threadLog<int>);

    // Join all threads
    threadedRunner.join();
}
