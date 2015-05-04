#ifndef GRAPEVINE_SRC_GV_CPP14_COMPATIBILITY_H_
#define GRAPEVINE_SRC_GV_CPP14_COMPATIBILITY_H_

#if __cplusplus == 201103L

#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>

// Allow us to use make_unique in c++11. Code taken from
// https://isocpp.org/files/papers/N3656.txt

namespace std {

template<class T> struct _Unique_if {
    typedef unique_ptr<T> _Single_object;
};

template<class T> struct _Unique_if<T[]> {
    typedef unique_ptr<T[]> _Unknown_bound;
};

template<class T, size_t N> struct _Unique_if<T[N]> {
    typedef void _Known_bound;
};

template<class T, class... Args>
typename _Unique_if<T>::_Single_object
make_unique(Args&&... args) {
    return unique_ptr<T>(new T(std::forward<Args>(args)...));
}

template<class T>
typename _Unique_if<T>::_Unknown_bound
make_unique(size_t n) {
    typedef typename remove_extent<T>::type U;
    return unique_ptr<T>(new U[n]());
}

template<class T, class... Args>
typename _Unique_if<T>::_Known_bound
make_unique(Args&&...) = delete;

} // namespace std

#endif // __cplusplus == 201103L

#endif // GRAPEVINE_SRC_GV_CPP14_COMPATIBILITY_H_
