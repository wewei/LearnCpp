#include <concepts>
#include <variant>
#include <optional>
#include <functional>

#include <iostream>
#include <thread>
#include "./monad.h"
#include "./tasks2.h"
#include "./thread-log.h"

template <typename T>
using Task = std::function<void(std::function<void(T)>)>;

template <>
struct Monad<Task> {
    template <typename T>
    static Task<T> mreturn(T t) {
        return [t](auto callback) { callback(t); };
    }

    template <typename T, typename U>
    static Task<U> mbind(Task<T> task, binder_t<Task, T, U> f) {
        return [task, f](auto callback) {
            task([f, callback](T va) { f(va)(callback); });
        };
    }
};

DefineBindOperator(Task)

template <typename T>
concept rejectable = requires (T t) {
    { !t } -> std::convertible_to<bool>;
};

template <typename ...Args>
bool operator ! (std::variant<Args ...> var) {
    return var.index() == 0;
}

template <rejectable T>
Task<T> operator | (Task<T> t1, Task<T> t2) {
    return [=](auto callback) {
        t1([=](auto v1) {
            if (!v1) {
                t2(callback);
            }
            else {
                callback(v1);
            }
        });
    };
}

typedef std::function<void(int, std::function<void()>)> timeout_t;

template <typename T>
binder_t<Task, T, T> delay(timeout_t timeout, int ms) {
    return [timeout, ms](T t) {
        return [timeout, ms, t](auto callback) {
            timeout(ms, [=]() { callback(t); });
        };
    };
}

template <typename T>
Task<T> delayed(timeout_t timeout, int ms, Task<T> task) {
    return [timeout, ms, task](auto callback) {
        timeout(ms, [task, callback]() {
            task(callback);
        });
    };
}

template <rejectable T>
Task<T> race(Task<T> task1, Task<T> task2) {
    int state1 = 0;
    int state2 = 0;
    return [=](auto callback) mutable {
        task1([=, &state1, &state2](T t) mutable {
            if (state1 != 0 || state2 > 0) {
                return;
            }
            if (state2 < 0 || !!t) {
                state1 = !t ? -1 : 1;
                callback(t);
            }
        });
        task2([=, &state1, &state2](T t) mutable {
            if (state2 != 0 || state1 > 0) {
                return;
            }
            if (state1 < 0 || !!t) {
                state2 = !t ? -1 : 1;
                callback(t);
            }
        });
    };
}

template <rejectable T, rejectable U>
Task<std::pair<T, U>> both(Task<T> task1, Task<U> task2) {

}

template <typename T>
binder_t<Task, T, T> tap(std::function<void(T)> callback) {
    return [=](T t) {
        callback(t);
        return Monad<Task>::mreturn(t);
    };
}

class TRunner {
private:
    std::vector<std::thread> threads;
public:
    void timeout(int ms, std::function<void()> callback);
    void join();
};

void TRunner::timeout(int ms, std::function<void()> callback) {
    threads.push_back(std::thread([=]() {
        threadLog("Thread started");
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
        callback();
    }));
}

void TRunner::join() {
    while (threads.size() > 0) {
        std::vector<std::thread> threadsT = std::move(threads);
        for (std::thread &thread : threadsT) {
            threadLog("Joining thread", thread.get_id());
            thread.join();
        }
    }
}


void Tasks2Demo::run() {
    Task<int> t1 = Monad<Task>::mreturn(42);
    Task<int> t2 = t1 >> (binder_t<Task, int, int>)[](int x) {
        return Monad<Task>::mreturn(x + 1);
    };

    t2(threadLog<int>);

    (Monad<Task>::mreturn(0) | Monad<Task>::mreturn(5))(threadLog<int>);
    (Monad<Task>::mreturn(3) | Monad<Task>::mreturn(5))(threadLog<int>);

    TRunner runner;
    timeout_t timeout = [&runner](int ms, std::function<void()> callback) {
        runner.timeout(ms, callback);
    };

    // Sequenced
    auto t3 = Monad<Task>::mreturn(10)
        >> delay<int>(timeout, 1000)
        >> tap<int>(threadLog<int>)
        >> delay<int>(timeout, 1000);

    t3(threadLog<int>);

    // Retries
    auto t4 = (Monad<Task>::mreturn(0) >> tap<int>(threadLog<int>))
            | (delayed(timeout, 1000, Monad<Task>::mreturn(0)) >> tap<int>(threadLog<int>))
            | (delayed(timeout, 2000, Monad<Task>::mreturn(0)) >> tap<int>(threadLog<int>))
            | (delayed(timeout, 4000, Monad<Task>::mreturn(0)) >> tap<int>(threadLog<int>))
            | (delayed(timeout, 8000, Monad<Task>::mreturn(6)) >> tap<int>(threadLog<int>));

    t4(threadLog<int>);

    // Races
    auto t5 = delayed(timeout, 1000, Monad<Task>::mreturn(7)) >> tap<int>(threadLog<int>);
    auto t6 = delayed(timeout, 500, Monad<Task>::mreturn(0)) >> tap<int>(threadLog<int>);
    auto t7 = race(t5, t6);

    t7([](int x) {
        threadLog("Result of t7", x);
    });

    runner.join();
}