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

        ~GV_Channel();

        GV_ERROR get(
            OUT std::unique_ptr<T> *out);

        GV_ERROR put(
            IN std::unique_ptr<T> *in);

        GV_ERROR get_nowait(
            OUT std::unique_ptr<T> *out);

        GV_ERROR put_nowait(
            IN std::unique_ptr<T> *in);

        GV_ERROR close();

    private:
        std::mutex _mtx;
        std::vector<T> _mItems;
        int _mCapacity;
        int _mItemsBegin;
        int _mCount;
};

#endif // GRAPEVINE_SRC_GV_CHANNEL_HPP_

