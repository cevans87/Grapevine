#include <memory>

#include "gv_channel.hpp"

template <class T>
GV_Channel<T>::GV_Channel(
    IN int capacity
    )
{
    _mCapacity = capacity;
    _mItems = new T[capacity + 1];
}


template <class T>
GV_Channel<T>::~GV_Channel(
    )
{
    gv_safe_delete(&_mItems);
}

template <class T>
GV_ERROR
GV_Channel<T>::get(
    OUT std::unique_ptr<T> *out
    )
{
    return GV_ERROR_SUCCESS;
}

template <class T>
GV_ERROR
GV_Channel<T>::put(
    IN std::unique_ptr<T> *in
    )
{
    return GV_ERROR_SUCCESS;
}

template <class T>
GV_ERROR
GV_Channel<T>::get_nowait(
    OUT std::unique_ptr<T> *out
    )
{
    return GV_ERROR_SUCCESS;
}

template <class T>
GV_ERROR
GV_Channel<T>::put_nowait(
    IN std::unique_ptr<T> *in
    )
{
    return GV_ERROR_SUCCESS;
}

template <class T>
GV_ERROR
GV_Channel<T>::close()
{
    return GV_ERROR_SUCCESS;
}
