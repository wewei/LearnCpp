#pragma once

#define DefineBindOperator(M)                           \
template <typename T, typename U>                       \
M<U> operator >> (M<T> ma, std::function<M<U>(T)> f) {  \
    return Monad<M>::mbind(ma, f);                      \
}                                                       \


template <template <typename> class M, typename T, typename U>
using binder_t = std::function<M<U>(T)>;

template <template <typename> class M>
struct Monad {
    template <typename T>
    static M<T> mreturn(T);

    template <typename T, typename U>
    static M<U> mbind(M<T>, binder_t<M, T, U>);
};

