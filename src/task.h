#pragma once
#include <concepts>
#include <functional>

template <template <typename> class Func, typename T>
class Task {
public:
    template <typename U>
    using Binder = Func<Task<Func, U>(const T &)>;

    using Handler = Func<void(const T &)>;

    using TaskCallback = Func<void(const Handler &)>;

    using DelayedRunner = Func<void(int, Func<void()>)>;

private:
    TaskCallback callback_;
public:
    // Constructors
    Task(TaskCallback &&callback): callback_(std::move(callback)) { }

    void Run(const Handler &handler) const {
        callback_(handler);
    }

    void operator() (const Handler &handler) const {
        callback_(handler);
    }

    template <typename U>
    Task<Func, U> Then(const Binder<U> &binder) const {
        return Task<Func, U>([callback=this->callback_, binder](const Handler &handler) {
            callback([binder, handler](const T &t) { binder(t).Run(handler); });
        });
    }

    Task<Func, T> Tap(const Handler &handler) const {
        return this->Then([handler](const T &t) {
            handler(t);
            return Task<Func, T>::Resolve(t);
        });
    }

    Task<Func, T> DelayedFor(const DelayedRunner &runner, int ms) const {
        return Task<Func, T>([callback = this->callback_, runner, ms](const Handler &handler) {
            runner(ms, [callback, handler]() {
                callback(handler);
            });
        });
    }

    Task<Func, T> ThenDelayFor(const DelayedRunner &runner, int ms) const {
        return Task<Func, T>([callback = this->callback_, runner, ms](const Handler &handler) {
            callback([runner, ms, handler](const T &t) {
                runner(ms, [handler, t]() { handler(t); });
            });
        });
    }

    Task<Func, T> Or(const Task<Func, T> &task) const {
        return Then<T>([task](const T &t) {
            return t ? Task<Func, T>::Resolve(t) : task;
        });
    }

public:
    static Task<Func, T> Resolve(const T &t) {
        return Task<Func, T>(std::move([t = std::move(t)](const Handler &handler) {
            handler(t);
        }));
    }
};
