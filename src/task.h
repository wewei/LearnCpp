#pragma once
#include <concepts>
#include <functional>

template <template <typename> class Func, typename T>
class Task {
public:
    template <typename U>
    using Binder = Func<Task<Func, U>(T &&)>;

    using Handler = Func<void(T &&)>;

    using TaskCallback = Func<void(const Handler &)>;

    using DelayedRunner = Func<void(int, Func<void()> &&)>;

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
        using HandlerU = typename Task<Func, U>::Handler;
        return Task<Func, U>([self = *this, binder](const HandlerU &handler) {
            self.Run([binder, handler](T &&t) { binder(std::move(t)).Run(handler); });
        });
    }

    Task<Func, T> Tap(const Handler &handler) const {
        return this->Then([handler](T &&t) {
            handler(t);
            return Task<Func, T>::Resolve(std::move(t));
        });
    }

    Task<Func, T> DelayedFor(const DelayedRunner &runner, int ms) const {
        return Task<Func, T>([self = *this, runner, ms](const Handler &handler) {
            runner(ms, [self=std::move(self), handler]() {
                self.Run(handler);
            });
        });
    }

    Task<Func, T> ThenDelayFor(const DelayedRunner &runner, int ms) const {
        return Task<Func, T>([self = *this, runner, ms](const Handler &handler) {
            self.Run([runner, ms, handler](T &&t) {
                runner(ms, [handler, t = std::move(t)]() mutable { handler(std::move(t)); });
            });
        });
    }

    Task<Func, T> Or(const Task<Func, T> &task) const {
        return Then<T>([task](T &&t) {
            return t ? Task<Func, T>::Resolve(std::move(t)) : task;
        });
    }

public:
    static Task<Func, T> Resolve(T t) {
        return Task<Func, T>(std::move([t = std::move(t)](const Handler &handler) {
            T tt(t);
            handler(std::move(tt));
        }));
    }
};
