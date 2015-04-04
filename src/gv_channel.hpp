#ifndef GRAPEVINE_SRC_GV_CHANNEL_HPP_
#define GRAPEVINE_SRC_GV_CHANNEL_HPP_

#include <vector>
#include <mutex>

#include "gv_util.hpp"
#include "gv_type.hpp"

// TODO implement "select" for channels?
// TODO implement RAII class for checkout/return of what's in channel?

namespace grapevine {

// Allows thread-safe passing of unique_pointers. Items may be put into the
// channel without blocking until it reaches capacity, at which time any
// further calls to 'put' will block until some item is removed. 'get' will get
// an item from the channel, blocking until an item is available. If capacity
// is 0, items may still be passed but each 'get' blocks until a matching 'put'
// is called and vice-versa. Items are fifo. No guarantees are made about
// ordering for multiple threads blocked on 'put' or 'get' once space or items
// become available.
template <class T>
class Channel {
    public:
        // IN capacity - Number of items the channel will hold with no
        //      consumer.
        Channel(
            IN unsigned int capacity);

        // Gets the next item from the channel. Blocks until an item is
        // available.
        // OUT *itemOut - Moves the next item in the channel to *itemOut.
        // Returns SUCCESS, or ERROR_CHANNEL_CLOSED if channel was closed
        //      before item could be retrieved.
        GV_ERROR get(
            OUT std::unique_ptr<T> *itemOut);

        // Puts an item into the channel. Blocks until space in the channel is
        // available or if capacity is 0, a 'get' is called to take the item.
        // IN *itemIn - Moves *itemIn into channel storage or to a thread
        //      calling 'get' if capacity is 0.
        // Returns SUCCESS, or ERROR_CHANNEL_CLOSED if channel was closed
        //      before item could be transferred.
        GV_ERROR put(
            IN std::unique_ptr<T> *itemIn);

        // Same as get(), but fails if an item is not immediately available.
        // Returns SUCCESS, or ERROR_CHANNEL_CLOSED if channel was closed
        //      before item could be retrieved, or ERROR_CHANNEL_EMPTY if no
        //      item was available.
        GV_ERROR get_nowait(
            OUT std::unique_ptr<T> *itemOut);

        // Same as put(), but fails if space is not immediately available.
        // Returns SUCCESS, or ERROR_CHANNEL_CLOSED if channel was closed
        //      before item could be retrieved, or ERROR_CHANNEL_FULL if no
        //      space was available and no 'get' was waiting.
        GV_ERROR put_nowait(
            IN std::unique_ptr<T> *itemIn);

        // Close the channel. No more items may be 'put', but any remaining
        // items may still be retrieved via 'get'. Calls to 'put' on a closed
        // channel and calls to 'get' on an empty, closed channel return
        // ERROR_CHANNEL_CLOSED.
        // Returns SUCCESS.
        GV_ERROR close();

    private:
        std::mutex _mtx;
        // For notifying getters that items are available.
        std::condition_variable _getter_cv;
        // For notifying putters that space is available.
        std::condition_variable _putter_cv;
        // For notifying highest priority putter that they may leave.
        std::condition_variable _overputter_cv;

        // Storage and required information for a circular fifo for items.
        // FIXME just use a std queue...
        std::vector<std::unique_ptr<T>> _items;
        unsigned int _uCapacity;
        unsigned int _uItemsBegin;
        unsigned int _uItemsCount;

        unsigned int _uItemsTransferred;

        // Number of getters waiting for an item. Used to detect that an item
        // may be 'put' without blocking when no space is available in channel.
        unsigned int _uWaitingGetters;

        // Since the existence of a putter means one item will exist in _items
        // as an "overput", we don't need a _uWaitingPutters to tell us we can
        // do a non-blocking 'get'.

        // Whether to accept items.
        bool _bClosed;

        char __padding__[3];
};

template <class T>
Channel<T>::Channel(
    IN unsigned int capacity
) {
    _uCapacity = capacity;
    // At capacity 0, we still want the ability to transfer one item, so min
    // cap is 1. Putting an item exceeding the cap should block the
    // 'overputter' thread until an item is taken, though.
    _items.resize(capacity + 1);
    _uItemsBegin = 0;
    _uItemsCount = 0;

    _uItemsTransferred = 0;

    _uWaitingGetters = 0;

    _bClosed = false;
}

template <class T>
GV_ERROR
Channel<T>::get(
    OUT std::unique_ptr<T> *itemOut
) {
    GV_ERROR error = GV_ERROR_SUCCESS;
    std::unique_lock<std::mutex> lk(_mtx);

    ++_uWaitingGetters;

    // Block until item is available.
    while (0 >= _uItemsCount) {
        // Still no item to get. Maybe woken by a call to 'close'.
        if (_bClosed) {
            error = GV_ERROR_CHANNEL_CLOSED;
            BAIL_ON_GV_ERROR(error);
        }
        _getter_cv.wait(lk);
    }

    // Retrieve item.
    *itemOut = move(_items.at(_uItemsBegin));
    _uItemsBegin = (_uItemsBegin + 1) % (_uCapacity + 1);
    --_uItemsCount;

    // Let putters know we made space.
    _overputter_cv.notify_one();
    _putter_cv.notify_one();

    ++_uItemsTransferred;
out:
    --_uWaitingGetters;
    return error;

error:
    goto out;
}

template <class T>
GV_ERROR
Channel<T>::put(
    IN std::unique_ptr<T> *itemIn
    )
{
    GV_ERROR error = GV_ERROR_SUCCESS;
    unsigned int uxPut;
    unsigned int uTransferredBegin;
    std::unique_lock<std::mutex> lk(_mtx);

    // Block until there is space or channel closes.
    while (_uItemsCount > _uCapacity && !_bClosed) {
        _putter_cv.wait(lk);
    }

    if (_bClosed) {
        error = GV_ERROR_CHANNEL_CLOSED;
        BAIL_ON_GV_ERROR(error);
    }

    // Put the item in channel storage.
    uxPut = (_uItemsBegin + _uItemsCount) % (_uCapacity + 1);
    _items.at(uxPut) = move(*itemIn);
    ++_uItemsCount;

    // Let getters know we made an item availalble.
    _getter_cv.notify_one();

    // If we sleep because we're an "overputter", another "overputter" might
    // sneak in and make it look like we are still over capacity just before we
    // wake up again. If _ItemsTransferred changes, we can still tell that we
    // are supposed to return.
    uTransferredBegin = _uItemsTransferred;
    while ((_uItemsCount > _uCapacity) &&
            (uTransferredBegin == _uItemsTransferred) &&
            (0 == _uWaitingGetters)) {
        if (_bClosed) {
            *itemIn = move(_items.at(uxPut));
            error = GV_ERROR_CHANNEL_CLOSED;
            BAIL_ON_GV_ERROR(error);
        }
        // Even though we placed an item, channel would be over capacity if we
        // left now. We're not really down with item ownership. Wait until one
        // item is taken from channel.
        _overputter_cv.wait(lk);
    }

out:
    return error;

error:
    goto out;
}

template <class T>
GV_ERROR
Channel<T>::get_nowait(
    OUT std::unique_ptr<T> *itemOut
    )
{
    GV_ERROR error = GV_ERROR_SUCCESS;
    std::unique_lock<std::mutex> lk(_mtx);

    // Make sure we can do a non-blocking 'get'.
    if (_bClosed && _uItemsCount <= 0) {
        error = GV_ERROR_CHANNEL_CLOSED;
    } else if (_uItemsCount <= 0) {
        error = GV_ERROR_CHANNEL_EMPTY;
    }
    BAIL_ON_GV_ERROR(error);

    // Retrieve item.
    *itemOut = move(_items.at(_uItemsBegin));
    _uItemsBegin = (_uItemsBegin + 1) % (_uCapacity + 1);
    --_uItemsCount;

    // Let putters know we made space.
    _putter_cv.notify_one();
    _overputter_cv.notify_one();

    ++_uItemsTransferred;
out:
    return error;

error:
    goto out;
}

template <class T>
GV_ERROR
Channel<T>::put_nowait(
    IN std::unique_ptr<T> *itemIn
    )
{
    GV_ERROR error = GV_ERROR_SUCCESS;
    unsigned int uxPut;
    std::unique_lock<std::mutex> lk(_mtx);

    // Make sure we can do a non-blocking 'put'.
    if (_bClosed) {
        error = GV_ERROR_CHANNEL_CLOSED;
    } else if ((_uItemsCount == _uCapacity && 0 == _uWaitingGetters) ||
            (_uItemsCount > _uCapacity)) {
        error = GV_ERROR_CHANNEL_FULL;
    }
    BAIL_ON_GV_ERROR(error);

    // Put the item in channel storage.
    uxPut = (_uItemsBegin + _uItemsCount) % (_uCapacity + 1);
    _items.at(uxPut) = move(*itemIn);
    ++_uItemsCount;

    // Let getters know we made an item availalble.
    _getter_cv.notify_one();

out:
    return error;

error:
    goto out;
}

template <class T>
GV_ERROR
Channel<T>::close()
{
    std::unique_lock<std::mutex> lk(_mtx);

    _bClosed = true;

    // Wake all blocked threads up so they can bail. The channel is closed.
    _getter_cv.notify_all();
    _putter_cv.notify_all();
    _overputter_cv.notify_all();

    return GV_ERROR_SUCCESS;
}

template <typename T>
using UP_Channel = std::unique_ptr<Channel<T>>;

} // namespace grapevine

#endif // GRAPEVINE_SRC_GV_CHANNEL_HPP_

