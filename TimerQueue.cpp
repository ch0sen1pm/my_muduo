#include "TimerQueue.h"
#include "EventLoop.h"
#include <sys/timerfd.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <iostream>

namespace {

int createTimerfd() {
    int fd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);

    if (fd < 0) {
        abort();
    }
    return fd;
}

int64_t nowMs() {
    struct timespec ts;
    ::clock_gettime(CLOCK_MONOTONIC, &ts);

    return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

void setTimerfd(int fd, int64_t whenMs) {
    int64_t delayMs = whenMs - nowMs();
    if (delayMs < 0) {
        delayMs = 0;
    }

    struct itimerspec newVal;
    memset(&newVal, 0, sizeof(newVal));

    newVal.it_value.tv_sec = delayMs / 1000;
    newVal.it_value.tv_nsec = (delayMs % 1000) * 1000000;

    ::timerfd_settime(fd, 0, &newVal, nullptr);
}
}

TimerQueue::TimerQueue(EventLoop* loop)
    : loop_(loop)
    , timerfd_(createTimerfd())
    , timerChannel_(loop, timerfd_)
{
    timerChannel_.setReadCallback([this]() { handleRead(); });
}

TimerQueue::~TimerQueue() {
    ::close(timerfd_);
}

void TimerQueue::addTimer(const TimerCallback& cb, int64_t whenMs) {
    timers_.emplace(whenMs, cb);

    if (timerChannel_.events() == 0) {
        resetTimerFd(whenMs);
        timerChannel_.enableReading();
    } else if (whenMs < getNextTimer()) {
        resetTimerFd(whenMs);
    }
}

void TimerQueue::handleRead() {
    uint64_t expired;
    ::read(timerfd_, &expired, sizeof(expired));

    int64_t now = nowMs();

    auto it = timers_.begin();
    while (it != timers_.end() && it->first <= now) {
        it->second();
        it = timers_.erase(it);
    }

    if (!timers_.empty()) {
        resetTimerFd(getNextTimer());
    } else {
        timerChannel_.disableAll();
    }
}

int64_t TimerQueue::getNextTimer() const {
    return timers_.empty() ? 0 : timers_.begin()->first;
}

void TimerQueue::resetTimerFd(int64_t whenMs) {
    setTimerfd(timerfd_, whenMs);
}

void TimerQueue::runAfter(int64_t delayMs, const TimerCallback& cb) {
    addTimer(cb, nowMs() + delayMs);
}