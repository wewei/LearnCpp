#pragma once
#include <functional>

template <typename T>
using Handler = std::function<void(const T &)>;

template <typename T>
using TaskCallback = Handler<Handler<T>>;

template <typename T>
using Delayer = std::function<Handler<T>(int, Handler<T>)>;

typedef std::function<void(int, std::function<void()>)> DelayedRunner;

template <typename T>
class Task {
public:
    template <typename U>
    using Binder = std::function<Task<U>(const T &)>;

private:
    TaskCallback<T> callback_;
public:
    // Constructors
    Task(const TaskCallback<T> &callback): callback_(callback) { }

    void Run(const Handler<T> &handler) const {
        callback_(handler);
    }

    void operator() (const Handler<T> &handler) const {
        callback_(handler);
    }

    template <typename U>
    Task<U> Then(const Binder<U> &binder) const {
        return Task<U>([callback=this->callback_, binder](const Handler<U> &handler) {
            callback([binder, handler](const T &t) { binder(t).Run(handler); });
        });
    }

    Task<T> Tap(const Handler<T> &handler) const {
        return this->Then([handler](const T &t) {
            handler(t);
            return Task<T>::Resolve(t);
        });
    }

    Task<T> DelayedFor(const DelayedRunner &runner, int ms) const {
        return Task<T>([callback = this->callback_, runner, ms](const Handler<T> &handler) {
            runner(ms, [callback, handler]() {
                callback(handler);
            });
        });
    }

    Task<T> ThenDelayFor(const DelayedRunner &runner, int ms) const {
        return Task<T>([callback = this->callback_, runner, ms](const Handler<T> &handler) {
            callback([runner, ms, handler](const T &t) {
                runner(ms, [handler, t]() { handler(t); });
            });
        });
    }

    Task<T> Or(const Task<T> &task) const {
        return Then<T>([task](const T &t) {
            return t ? Task<T>::Resolve(t) : task;
        });
    }

public:
    static Task<T> Resolve(const T &t) {
        return Task<T>([t](const Handler<T> &handler) {
            handler(t);
        });
    }
};
