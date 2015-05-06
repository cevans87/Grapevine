#ifndef GRAPEVINE_SRC_GV_CHANNEL_HPP_
#define GRAPEVINE_SRC_GV_CHANNEL_HPP_

#include <vector>
#include <map>
#include <mutex>
#include <queue>

#include <fcntl.h> // For O_NONBLOCK on pipes
#include <unistd.h> // pipes

#include "gv_util.h"

// TODO test "select" for channels
// TODO implement RAII class for checkout/return of what's in channel?
// TODO Allow capacity to change?

namespace grapevine {

// When the channel is full or empty, putters or getters may be blocked. We use
// this class to take care of the neccessary item transfer without having to
// reacquire the global channel lock.
template <typename T, typename... D>
struct WaitingTransfer {
    std::condition_variable cv;
    std::mutex mtx;
    std::unique_ptr<T, D...> *item;
};

// Allows thread-safe passing of unique_pointers. Items may be put into the
// channel without blocking until it reaches capacity, at which time any
// further calls to 'put' will block until some item is removed. 'get' will get
// an item from the channel, blocking until an item is available. If capacity
// is 0, items may still be passed but each 'get' blocks until a matching 'put'
// is called and vice-versa. Items are fifo. Priority for multiple threads
// blocked on 'put' or 'get' is also fifo.
template <typename T, typename... D>
class Channel {
    public:
        using fdsRdWr = std::pair<int, int> const;

        Channel() = delete;

        // IN capacity - Number of items the channel will hold with no
        //      consumer.
        Channel(
            IN unsigned int capacity);

        // Gets the next item from the channel. Blocks until an item is
        // available.
        // OUT *itemOut - Moves the next item in the channel to *itemOut.
        // Returns SUCCESS, or CHANNEL_CLOSED if channel was closed
        //      before item could be retrieved.
        GV_ERROR get(
            OUT std::unique_ptr<T, D...> *itemOut);

        // Puts an item into the channel. Blocks until space in the channel is
        // available or if capacity is 0, a 'get' is called to take the item.
        // IN *itemIn - Moves *itemIn into channel storage or to a thread
        //      calling 'get' if capacity is 0.
        // Returns SUCCESS, or CHANNEL_CLOSED if channel was closed
        //      before item could be transferred.
        GV_ERROR put(
            IN std::unique_ptr<T, D...> *itemIn);

        // Same as get(), but fails if an item is not immediately available.
        // Returns SUCCESS, or CHANNEL_CLOSED if channel was closed
        //      before item could be retrieved, or CHANNEL_EMPTY if no
        //      item was available.
        GV_ERROR get_nowait(
            OUT std::unique_ptr<T, D...> *itemOut);

        // Same as put(), but fails if space is not immediately available.
        // Returns SUCCESS, or CHANNEL_CLOSED if channel was closed
        //      before item could be retrieved, or CHANNEL_FULL if no
        //      space was available and no 'get' was waiting.
        GV_ERROR put_nowait(
            IN std::unique_ptr<T, D...> *itemIn);

        // Provides read end of a pipe that is written to when data becomes
        // available in the channel. Pipe is automatically cleared of the
        // message once data is removed from channel.
        // INOUT *pfdNotify - If *pfdNotify is -1, sets *pfdNotify to read end
        //      of newly created pipe used to signal that data is available in
        //      the pipe. No guarantee is made to the selector that data will
        //      still be there when it's asked for. Selectors should not
        //      actually read from the pipe.
        // Returns SUCCESS, or CHANNEL_CLOSED if channel is already
        //      closed, or NO_FD if a pipe could not be created.
        GV_ERROR get_notify_data_available_fd(
            INOUT int *pfdNotify);

        // Provides read end of a pipe that is written to when space becomes
        // available in the channel. Pipe is automatically cleared of the
        // message once data is placed in channel.
        // INOUT *pfdNotify - If *pfdNotify is -1, sets *pfdNotify to read end
        //      of newly created pipe used to signal that space is available in
        //      the pipe. No guarantee is made to the selector that space will
        //      still be there when it's asked for. Selectors should not
        //      actually read from the pipe.
        // Returns SUCCESS, or CHANNEL_CLOSED if channel is already
        //      closed, or NO_FD if a pipe could not be created.
        GV_ERROR get_notify_space_available_fd(
            INOUT int *pfdNotify);

        // Closes pipe created by a previous call to
        // get_notify_data_available_fd.
        // INOUT *pfdNotify - Pointer to write end of pipe to be closed. Read
        //      end will also be located and closed by channel.
        // Returns SUCCESS, or INVALID_ARG if *pfdNotify isn't the write-end of
        // a pipe that this channel is managing.
        GV_ERROR close_notify_data_available_fd(
            INOUT int *pfdNotify);

        // Closes pipe created by a previous call to
        // get_notify_space_available_fd.
        // INOUT *pfdNotify - Pointer to write end of pipe to be closed. Read
        //      end will also be located and closed by channel.
        // Returns SUCCESS, or INVALID_ARG if *pfdNotify isn't the write-end of
        // a pipe that this channel is managing.
        GV_ERROR close_notify_space_available_fd(
            INOUT int *pfdNotify);

        // Closes the channel. No more items may be 'put', but any remaining
        // items may still be retrieved via 'get'. Calls to 'put' on a closed
        // channel and calls to 'get' on an empty, closed channel return
        // CHANNEL_CLOSED.
        // Returns SUCCESS.
        GV_ERROR close();

    private:
        std::mutex _mtx;

        // map writeFd to readFd.
        std::map<int, int> _mapfdNotifyDataAvailable;
        std::map<int, int> _mapfdNotifySpaceAvailable;

        // Queued/blocked items from getters or putters. Either putters xor
        // getters may be blocked at any given moment.
        std::queue<WaitingTransfer<T, D...>> _qGetters;
        std::queue<WaitingTransfer<T, D...>> _qPutters;

        std::queue<std::unique_ptr<T, D...>> _qItems;

        unsigned int _uCapacity;

        // Whether to accept items.
        bool _bClosed;

        char __padding__[3];

        // TODO comments
        GV_ERROR inc_notify_space_available();
        GV_ERROR inc_notify_data_available();
};

template <typename T, typename... D>
Channel<T, D...>::Channel(
    IN unsigned int capacity
) {

    _uCapacity = capacity;
    _bClosed = false;
}

template <typename T, typename... D>
GV_ERROR
Channel<T, D...>::get(
    OUT std::unique_ptr<T, D...> *itemOut
) {
    GV_ERROR error = GV_ERROR::SUCCESS;
    std::unique_lock<std::mutex> lkChannel(_mtx);

    if (nullptr == itemOut) {
        error = GV_ERROR::INVALID_ARG;
        BAIL_ON_GV_ERROR(error);
    } else if (0 < _qItems.size()) {
        // Just take the first item.
        *itemOut = move(_qItems.front());
        _qItems.pop();
        if (0 < _qPutters.size()) {
            // There are putters waiting. Help them out.
            std::lock_guard<std::mutex> lg(_qPutters.front().mtx);
            _qItems.push(move(*_qPutters.front().item));
            _qPutters.front().cv.notify_one();
            _qPutters.pop();
        }
    } else if (0 < _qPutters.size()) {
        // There are putters waiting.
        std::lock_guard<std::mutex> lg(_qPutters.front().mtx);
        *itemOut = move(*_qPutters.front().item);
        _qPutters.front().cv.notify_one();
        _qPutters.pop();
    } else if (_bClosed) {
        // Channel already closed
        error = GV_ERROR::CHANNEL_CLOSED;
        BAIL_ON_GV_ERROR_EXPECTED(error);
    } else {
        // No getters waiting, no space. We have to block until a getter moves
        // our item into the channel or takes it off our hands.
        _qGetters.emplace();
        WaitingTransfer<T, D...> &transfer = _qGetters.back();
        transfer.item = itemOut;
        // Switching from using the channel lock to the new waiting lock
        // increases the total number of locks we have to take in this case,
        // but we won't ever have to touch the channel lock again. Better for
        // parallelization, worse for individual performance.
        std::unique_lock<std::mutex> lkTransfer(transfer.mtx);
        lkChannel.unlock();

        // Getters are stuck until a putter shows up. The putter will transfer
        // an item to *itemOut for us.
        *itemOut = nullptr;
        while (nullptr == *itemOut && !_bClosed) {
            transfer.cv.wait(lkTransfer);
        }
        if (nullptr == *itemOut && _bClosed) {
            error = GV_ERROR::CHANNEL_CLOSED;
            BAIL_ON_GV_ERROR_EXPECTED(error);
        }
    }

    inc_notify_space_available();

out:
    return error;

error:
    goto out;
}

template <typename T, typename... D>
GV_ERROR
Channel<T, D...>::inc_notify_space_available(
) {
    ssize_t bytesRead;
    char msg; // Don't care what's in here.

    for (std::pair<int, int> const &fds: _mapfdNotifyDataAvailable) {
        // Pull the msg out of the pipe.
        bytesRead = read(fds.first, &msg, sizeof(msg));
        if (0 == bytesRead) {
            // FIXME turn this into a severe error and bail.
            GV_DEBUG_PRINT_SEV(GV_DEBUG::SEVERE,
                "Bytes missing from SpaceAvailable notify pipe");
        }
    }
    for (fdsRdWr &fds: _mapfdNotifySpaceAvailable) {
        // Let selectors know we made data available.
        // FIXME count the bytes written. We need to be sure we wrote 1.
        write(fds.second, &msg, sizeof(msg));
    }
    return GV_ERROR::SUCCESS;
}

template <typename T, typename... D>
GV_ERROR
Channel<T, D...>::inc_notify_data_available(
) {
    ssize_t bytesRead;
    char msg; // Don't care what's in here.

    for (std::pair<int, int> const &fds: _mapfdNotifySpaceAvailable) {
        // Pull the msg out of the pipe.
        bytesRead = read(fds.first, &msg, sizeof(msg));
        if (0 == bytesRead) {
            // FIXME turn this into a severe error and bail.
            GV_DEBUG_PRINT_SEV(GV_DEBUG::SEVERE,
                "Bytes missing from SpaceAvailable notify pipe");
        }
    }
    for (fdsRdWr &fds: _mapfdNotifyDataAvailable) {
        // Let selectors know we made data available.
        // FIXME count the bytes written. We need to be sure we wrote 1.
        write(fds.second, &msg, sizeof(msg));
    }
    return GV_ERROR::SUCCESS;
}

template <typename T, typename... D>
GV_ERROR
Channel<T, D...>::put(
    IN std::unique_ptr<T, D...> *itemIn
) {
    GV_ERROR error = GV_ERROR::SUCCESS;
    std::unique_lock<std::mutex> lkChannel(_mtx);

    if (nullptr == itemIn) {
        error = GV_ERROR::INVALID_ARG;
        BAIL_ON_GV_ERROR(error);
    } else if (_bClosed) {
        // Channel already closed
        error = GV_ERROR::CHANNEL_CLOSED;
        BAIL_ON_GV_ERROR_EXPECTED(error);
    } else if (0 < _qGetters.size()) {
        // There are getters waiting. Give it directly to a getter.
        std::lock_guard<std::mutex> lg(_qGetters.front().mtx);
        *_qGetters.front().item = move(*itemIn);
        _qGetters.front().cv.notify_one();
        _qGetters.pop();
    } else if (_qItems.size() < _uCapacity) {
        // No getters waiting, but space available in channel.
        _qItems.push(move(*itemIn));
    } else {
        // No getters waiting, no space. We have to block until a getter moves
        // our item into the channel or takes it off our hands.
        _qPutters.emplace();
        WaitingTransfer<T, D...> &transfer = _qPutters.back();
        transfer.item = itemIn;
        // Switching from using the channel lock to the new transfer lock
        // increases the total number of locks we have to take in this case,
        // but we won't ever have to touch the channel lock again. Better for
        // parallelization, worse for individual performance.
        std::unique_lock<std::mutex> lkTransfer(transfer.mtx);
        lkChannel.unlock();

        // Putters are stuck until a getter shows up. The getter will transfer
        // the item from *itemIn for us.
        while (nullptr != *itemIn && !_bClosed) {
            transfer.cv.wait(lkTransfer);
        }
        if (nullptr != *itemIn && _bClosed) {
            error = GV_ERROR::CHANNEL_CLOSED;
            BAIL_ON_GV_ERROR(error);
        }
    }

    inc_notify_data_available();

out:
    return error;

error:
    goto out;
}

template <typename T, typename... D>
GV_ERROR
Channel<T, D...>::get_nowait(
    OUT std::unique_ptr<T, D...> *itemOut
) {
    GV_ERROR error = GV_ERROR::SUCCESS;
    std::unique_lock<std::mutex> lkChannel(_mtx);

    if (nullptr == itemOut) {
        error = GV_ERROR::INVALID_ARG;
        BAIL_ON_GV_ERROR(error);
    } else if (0 < _qItems.size()) {
        // Just take the first item.
        *itemOut = move(_qItems.front());
        _qItems.pop();
        if (0 < _qPutters.size()) {
            // There are putters waiting. Help them out.
            std::lock_guard<std::mutex> lg(_qPutters.front().mtx);
            _qItems.push(move(*_qPutters.front().item));
            _qPutters.front().cv.notify_one();
            _qPutters.pop();
        }
    } else if (0 < _qPutters.size()) {
        // There are putters waiting.
        std::lock_guard<std::mutex> lg(_qPutters.front().mtx);
        *itemOut = move(*_qPutters.front().item);
        _qPutters.front().cv.notify_one();
        _qPutters.pop();
    } else if (_bClosed) {
        // Channel already closed
        error = GV_ERROR::CHANNEL_CLOSED;
        BAIL_ON_GV_ERROR_EXPECTED(error);
    } else {
        error = GV_ERROR::CHANNEL_EMPTY;
        BAIL_ON_GV_ERROR_EXPECTED(error);
    }

    inc_notify_space_available();

out:
    return error;

error:
    goto out;
}

template <typename T, typename... D>
GV_ERROR
Channel<T, D...>::put_nowait(
    IN std::unique_ptr<T, D...> *itemIn
) {
    GV_ERROR error = GV_ERROR::SUCCESS;
    std::unique_lock<std::mutex> lkChannel(_mtx);

    if (nullptr == itemIn) {
        error = GV_ERROR::INVALID_ARG;
        BAIL_ON_GV_ERROR(error);
    } else if (_bClosed) {
        // Channel already closed
        error = GV_ERROR::CHANNEL_CLOSED;
        BAIL_ON_GV_ERROR_EXPECTED(error);
    } else if (0 < _qGetters.size()) {
        // There are getters waiting. Give it directly to a getter.
        std::lock_guard<std::mutex> lg(_qGetters.front().mtx);
        *_qGetters.front().item = move(*itemIn);
        _qGetters.front().cv.notify_one();
        _qGetters.pop();
    } else if (_qItems.size() < _uCapacity) {
        // No getters waiting, but space available in channel.
        _qItems.push(move(*itemIn));
    } else {
        error = GV_ERROR::CHANNEL_FULL;
        BAIL_ON_GV_ERROR_EXPECTED(error);
    }

    inc_notify_data_available();

out:
    return error;

error:
    goto out;
}

template <typename T, typename... D>
GV_ERROR
Channel<T, D...>::get_notify_data_available_fd(
    INOUT int *pfdNotify
) {
    GV_ERROR error = GV_ERROR::SUCCESS;
    char msg; // Don't care what's in here.
    int pipeFd[2];
    unsigned int i;

    std::lock_guard<std::mutex> lg(_mtx);

    if (_bClosed) {
        error = GV_ERROR::CHANNEL_CLOSED;
        BAIL_ON_GV_ERROR_WARNING(error);
    } else if (-1 != *pfdNotify) {
        // *pfdNotify already valid, unless caller forgot to set up correctly.
        *pfdNotify = *pfdNotify;
    // Since we're making the read-end of the pipe available to the user of
    // this channel, we don't want to assume they didn't read the value before
    // we do, and we don't want to block.
    } else if (0 == pipe2(pipeFd, O_NONBLOCK)) {
        *pfdNotify = pipeFd[0];
        _mapfdNotifyDataAvailable.insert(
                std::pair<int, int>(pipeFd[0], pipeFd[1]));

        for (i = 0; i < _qItems.size(); ++i) {
            write(pipeFd[1], &msg, sizeof(msg));
        }
    } else {
        error = GV_ERROR::NO_FD;
        BAIL_ON_GV_ERROR(error);
    }

out:
    return error;

error:
    goto out;
}

template <typename T, typename... D>
GV_ERROR
Channel<T, D...>::close_notify_data_available_fd(
    INOUT int *pfdNotify
) {
    GV_ERROR error = GV_ERROR::SUCCESS;
    std::map<int, int>::iterator loc;
    std::lock_guard<std::mutex> lg(_mtx);

    loc = _mapfdNotifyDataAvailable.find(*pfdNotify);

    if (_mapfdNotifyDataAvailable.end() == loc) {
        error = GV_ERROR::INVALID_ARG;
        BAIL_ON_GV_ERROR(error);
    }

    ::close(loc->first);
    ::close(loc->second);
    _mapfdNotifyDataAvailable.erase(loc);
    *pfdNotify = -1;

out:
    return error;

error:
    goto out;
}

template <typename T, typename... D>
GV_ERROR
Channel<T, D...>::get_notify_space_available_fd(
    INOUT int *pfdNotify
) {
    GV_ERROR error = GV_ERROR::SUCCESS;
    char msg;
    int pipeFd[2];
    unsigned int i;

    std::lock_guard<std::mutex> lg(_mtx);

    if (_bClosed) {
        error = GV_ERROR::CHANNEL_CLOSED;
        BAIL_ON_GV_ERROR_WARNING(error);
    } else if (-1 != *pfdNotify) {
        // *pfdNotify already valid, unless caller forgot to set up correctly.
        *pfdNotify = *pfdNotify;
    } else if (0 == pipe2(pipeFd, O_NONBLOCK)) {
        *pfdNotify = pipeFd[0];
        _mapfdNotifyDataAvailable.insert(
                std::pair<int, int>(pipeFd[0], pipeFd[1]));

        for (i = 0; i < _uCapacity - _qItems.size(); ++i) {
            write(pipeFd[1], &msg, sizeof(msg));
        }
    } else {
        error = GV_ERROR::NO_FD;
        BAIL_ON_GV_ERROR(error);
    }

out:
    return error;

error:
    goto out;
}

template <typename T, typename... D>
GV_ERROR
Channel<T, D...>::close_notify_space_available_fd(
    INOUT int *pfdNotify
) {
    GV_ERROR error = GV_ERROR::SUCCESS;
    std::map<int, int>::iterator loc;
    std::lock_guard<std::mutex> lg(_mtx);

    loc = _mapfdNotifySpaceAvailable.find(*pfdNotify);

    if (_mapfdNotifySpaceAvailable.end() == loc) {
        error = GV_ERROR::INVALID_ARG;
        BAIL_ON_GV_ERROR(error);
    }

    ::close(loc->first);
    ::close(loc->second);
    _mapfdNotifySpaceAvailable.erase(loc);
    *pfdNotify = -1;

out:
    return error;

error:
    goto out;
}

template <typename T, typename... D>
GV_ERROR
Channel<T, D...>::close(
) {
    GV_ERROR error = GV_ERROR::SUCCESS;
    char msg; // Don't care what's in here.
    std::lock_guard<std::mutex> lg(_mtx);

    if (_bClosed) {
        // Doing the work a second time would be bad.
        goto out;
    }

    _bClosed = true;

    // Anyone selecting on this channel needs to wake up and find out it's
    // closed.
    for (fdsRdWr &fds: _mapfdNotifySpaceAvailable) {
        write(fds.second, &msg, sizeof(msg));
    }
    for (fdsRdWr &fds: _mapfdNotifyDataAvailable) {
        write(fds.second, &msg, sizeof(msg));
    }

    // Wake all blocked threads up so they can bail.
    while (0 < _qGetters.size()) {
        std::lock_guard<std::mutex> lgGetter(_qGetters.front().mtx);
        _qGetters.front().cv.notify_one();
        _qGetters.pop();
    }
    while (0 < _qPutters.size()) {
        std::lock_guard<std::mutex> lgPutter(_qPutters.front().mtx);
        _qPutters.front().cv.notify_one();
        _qPutters.pop();
    }

out:
    return error;
}

template <typename T, typename... D>
using UPChannel = std::unique_ptr<Channel<T, D...>>;

} // namespace grapevine

#endif // GRAPEVINE_SRC_GV_CHANNEL_HPP_

