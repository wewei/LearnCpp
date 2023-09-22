#pragma once
#include <functional>

template <typename T>
using Handler = std::function<void(T)>;

template <typename T>
using TaskCallback = Handler<Handler<T>>;

template <typename T>
using Delayer = std::function<Handler<T>(int, Handler<T>)>;

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

    Task<T> DelayedFor(Delayer<Handler<T>> delayer, int ms) const {
        return Task<T>(delayer(ms, callback_));
    }

    Task<T> ThenDelayFor(Delayer<Handler<T>> delayer, int ms) const {
        return Then<T>([delayer, ms](T t) {
            return Task<T>::Resolve(t).DelayedFor(delayer, ms);
        });
    }

public:
    static Task<T> Resolve(T t) {
        return Task<T>([t](Handler<T> handler) {
            handler(t);
        });
    }
};
