#include <concepts>
#include <variant>
#include <optional>
#include <functional>

#include <iostream>
#include <thread>
#include "./tasks2.h"

#define DefineBindOperator(m)                           \
template <typename a, typename b>                       \
m<b> operator >> (m<a> ma, std::function<m<b>(a)> f) {  \
    return Monad<m>::mbind(ma, f);                      \
}                                                       \


template <template <typename> class m, typename a, typename b>
using binder_t = std::function<m<b>(a)>;

template <template <typename> class m>
struct Monad {
    template <typename a>
    static m<a> mreturn(a);

    template <typename a, typename b>
    static m<b> mbind(m<a>, binder_t<m, a, b>);
};

template <typename T>
using Task = std::function<void(std::function<void(T)>)>;

template <>
struct Monad<Task> {
    template <typename T>
    static Task<T> mreturn(T t) {
        return [t](auto callback) { callback(t); };
    }

    template <typename a, typename b>
    static Task<b> mbind(Task<a> task, binder_t<Task, a, b> f) {
        return [task, f](auto callback) {
            task([f, callback](a va) { f(va)(callback); });
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

template <rejectable a>
Task<a> operator | (Task<a> t1, Task<a> t2) {
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
    return [=](T t) {
        return [=](auto callback) {
            timeout(ms, [=]() { callback(t); });
        };
    };
}

template <typename T>
binder_t<Task, T, T> tap(std::function<void(T)> callback) {
    return [=](T t) {
        callback(t);
        return Monad<Task>::mreturn(t);
    };
}

template <typename Arg, typename ...Args>
void threadLog(Arg&& arg, Args&&... args) {
    std::cout << "[Thread " << std::this_thread::get_id() << "]: ";
    std::cout << std::forward<Arg>(arg);
    ((std::cout << ", " << std::forward<Args>(args)), ...);
    std::cout << std::endl;
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
    auto t3 = Monad<Task>::mreturn(10)
        >> delay<int>(timeout, 1000)
        >> tap<int>(threadLog<int>)
        >> delay<int>(timeout, 1000);

    t3(threadLog<int>);

    runner.join();
}