#ifndef GRAPEVINE_SRC_GV_CHANNEL_HPP_
#define GRAPEVINE_SRC_GV_CHANNEL_HPP_

#include <vector>
#include <mutex>

#include "gv_util.hpp"
#include "gv_type.hpp"

// TODO implement "select" for channels?

template <class T>
class GV_Channel
{
    public:
        GV_Channel(
            IN int capacity);

        GV_ERROR get(
            OUT std::unique_ptr<T> *itemOut);

        GV_ERROR put(
            IN std::unique_ptr<T> *itemIn);

        GV_ERROR get_nowait(
            OUT std::unique_ptr<T> *itemOut);

        GV_ERROR put_nowait(
            IN std::unique_ptr<T> *itemIn);

        GV_ERROR close();

    private:
        std::mutex _mtx;
        std::condition_variable _cv;
        std::vector<std::unique_ptr<T>> _mItems;
        int _mCapacity;
        int _mItemsBegin;
        int _mItemsCount;
        bool _bClosed;
};

template <class T>
GV_Channel<T>::GV_Channel(
    IN int capacity
    )
{
    _mCapacity = capacity;
    // At capacity 0, we still want the ability to transfer one item, so min
    // cap is 1. Putting an item exceeding the cap should block, though.
    _mItems.resize(capacity + 1);
    _mItemsBegin = 0;
    _mItemsCount = 0;
    _bClosed = false;
}

template <class T>
GV_ERROR
GV_Channel<T>::get(
    OUT std::unique_ptr<T> *itemOut
    )
{
    GV_ERROR error = GV_ERROR_SUCCESS;
    std::unique_lock<std::mutex> lk(_mtx);

    if (_bClosed) {
        error = GV_ERROR_ALREADY_DISABLED;
        BAIL_ON_GV_ERROR(error);
    }

    while (0 >= _mItemsCount) {
        _cv.wait(lk);
    }

    *itemOut = move(_mItems.at(_mItemsBegin));
    _mItemsBegin = (_mItemsBegin + 1) % (_mCapacity + 1);
    --_mItemsCount;
    _cv.notify_one();

out:
    return GV_ERROR_SUCCESS;

error:
    goto out;
}

template <class T>
GV_ERROR
GV_Channel<T>::put(
    IN std::unique_ptr<T> *itemIn
    )
{
    GV_ERROR error = GV_ERROR_SUCCESS;
    int idxPut;
    std::unique_lock<std::mutex> lk(_mtx);

    if (_bClosed) {
        error = GV_ERROR_ALREADY_DISABLED;
        BAIL_ON_GV_ERROR(error);
    }

    while (_mItemsCount > _mCapacity) {
        _cv.wait(lk);
    }

    idxPut = (_mItemsBegin + _mItemsCount) % (_mCapacity + 1);
    _mItems.at(idxPut) = move(*itemIn);
    ++_mItemsCount;
    _cv.notify_one();
    while (_mItemsCount > _mCapacity) {
        _cv.wait(lk);
    }

out:
    return GV_ERROR_SUCCESS;

error:
    goto out;
}

template <class T>
GV_ERROR
GV_Channel<T>::get_nowait(
    OUT std::unique_ptr<T> *itemOut
    )
{
    GV_ERROR error = GV_ERROR_SUCCESS;
    std::unique_lock<std::mutex> lk(_mtx, std::defer_lock);

    // FIXME the lock isn't what determines unavailability. It's _mItemsCount.
    if (false == lk.try_lock()) {
        error = GV_ERROR_LOCK_UNAVAILABLE;
        BAIL_ON_GV_ERROR(error);
    }
    // TODO get the item.

out:
    return error;

error:
    goto out;
}

template <class T>
GV_ERROR
GV_Channel<T>::put_nowait(
    IN std::unique_ptr<T> *itemIn
    )
{
    GV_ERROR error = GV_ERROR_SUCCESS;
    std::unique_lock<std::mutex> lk(_mtx, std::defer_lock);

    if (false == lk.try_lock()) {
        error = GV_ERROR_LOCK_UNAVAILABLE;
        BAIL_ON_GV_ERROR(error);
    }
    // TODO put the item.

out:
    return error;

error:
    goto out;
}

// Remove all items from channel and disable get and put.
template <class T>
GV_ERROR
GV_Channel<T>::close()
{
    return GV_ERROR_SUCCESS;
}

#endif // GRAPEVINE_SRC_GV_CHANNEL_HPP_

