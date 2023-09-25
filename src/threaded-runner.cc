#include "./threaded-runner.h"
#include "./thread-log.h"

void ThreadedRunner::delay(int ms, std::function<void()> &&callback) {
    threads.push_back(std::thread([ms, callback = std::move(callback)]() {
        // threadLog("Thread started");
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
        callback();
    }));
}

void ThreadedRunner::join() {
    while (threads.size() > 0) {
        std::vector<std::thread> threadsT = std::move(threads);
        for (std::thread &thread : threadsT) {
            threadLog("Joining thread", thread.get_id());
            thread.join();
        }
    }
}