#pragma once
#include <vector>
#include <set>
#include <map>
#include <functional>
#include <memory>
#include "Channel.h"

class EventLoop;

class TimerQueue {
public:
    using TimerCallback = std::function<void()>;

    TimerQueue(EventLoop* loop);
    ~TimerQueue();

    void addTimer(const TimerCallback& cb, int64_t whenMs);
    void runAfter(int64_t delayMs, const TimerCallback& cb);
private:
    void handleRead();
    int64_t getNextTimer() const;
    void resetTimerFd(int64_t whenMs);

    EventLoop* loop_;
    const int timerfd_;
    Channel timerChannel_;

    std::multimap<int64_t, TimerCallback> timers_;
};