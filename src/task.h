#pragma once
#include <functional>

template <typename T>
using Handler = std::function<void(T)>;

template <typename T>
using TaskCallback = Handler<Handler<T>>;

template <typename T>
using Delayer = std::function<Handler<T>(int, Handler<T>)>;

typedef std::function<void(int, std::function<void()>)> DelayedRunner;

template <typename T>
class Task {
public:
    template <typename U>
    using Binder = std::function<Task<U>(T)>;

private:
    TaskCallback<T> callback_;
public:
    // Constructors
    Task(TaskCallback<T> callback): callback_(callback) { }

    void run(Handler<T> handler) const {
        callback_(handler);
    }

    void operator() (Handler<T> handler) const {
        callback_(handler);
    }

    template <typename U>
    Task<U> Then(Binder<U> binder) const {
        return Task<U>([callback_=this->callback_, binder](Handler<U> handler) {
            callback_([binder, handler](T t) { binder(t).run(handler); });
        });
    }

    Task<T> Tap(Handler<T> handler) const {
        return this->Then([handler](T t) {
            handler(t);
            return Task<T>::Resolve(t);
        });
    }

    Task<T> DelayedFor(DelayedRunner runner, int ms) const {
        return Task<T>([callback_ = this->callback_, runner, ms](Handler<T> handler) {
            runner(ms, [callback_, handler]() {
                callback_(handler);
            });
        });
    }

    Task<T> ThenDelayFor(DelayedRunner runner, int ms) const {
        return Task<T>([callback_ = this->callback_, runner, ms](Handler<T> handler) {
            callback_([runner, ms, handler](T t) {
                runner(ms, [handler, t]() { handler(t); });
            });
        });
    }

    Task<T> Or(Task<T> task) const {
        return Then<T>([task](T t) {
            return t ? Task<T>::Resolve(t) : task;
        });
    }

public:
    static Task<T> Resolve(T t) {
        return Task<T>([t](Handler<T> handler) {
            handler(t);
        });
    }
};
