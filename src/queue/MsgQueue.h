#pragma once
//
//  MsgQueue.h

#include <queue>

#include "threading.h"


template <typename T>
class MsgQueue
{
public:
    
    T pop()
    {
        std::unique_lock<std::mutex> mlock(mutex_);
        while (queue_.empty())
        {
            cond_.wait(mlock);
        }
        auto item = queue_.front();
        queue_.pop();
        return item;
    }

    void pop(T& item)
    {
        std::unique_lock<std::mutex> mlock(mutex_);
        while (queue_.empty())
        {
            cond_.wait(mlock);
        }
        item = queue_.front();
        queue_.pop();
    }
    
    void push(const T& item)
    {
        std::unique_lock<std::mutex> mlock(mutex_);
        queue_.push(item);
        mlock.unlock();
        cond_.notify_one();
    }
    
#ifndef OT_USE_TR1
    void push(T&& item)
    {
        INSTANTIATE_MLOCK(mutex_);
        queue_.push(std::move(item));
        mlock.unlock();
        cond_.notify_one();
    }
#endif
    
    int size()
    {
        std::unique_lock<std::mutex> mlock(mutex_);
        int size = queue_.size();
        mlock.unlock();
        return size;
    }
    
    void clear()
    {
        std::unique_lock<std::mutex> mlock(mutex_);
        while(!queue_.empty())
            queue_.pop();
        mlock.unlock();
    }
    
private:
    std::queue<T> queue_;
    std::mutex mutex_;
    std::condition_variable cond_;
};