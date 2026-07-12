#pragma once
#include <vector>
#include <set>
#include <map>
#include <functional>
#include <memory>
#include "Channel.h"

class EventLoop;

/**
 * TimerQueue — 基于 timerfd 的定时器队列
 *
 * timerfd 是一个普通的 fd，超时后自动可读。
 * 把 timerfd 包进 Channel 丢进 epoll，跟 socket fd 统一管理，
 * 不需要额外线程。
 */
class TimerQueue {
public:
    using TimerCallback = std::function<void()>;

    /**
     * @param loop 所属 EventLoop
     */
    TimerQueue(EventLoop* loop);
    ~TimerQueue();

    /**
     * 添加绝对时间定时器
     * @param cb     到期后执行的回调
     * @param whenMs 到期绝对时间（毫秒，CLOCK_MONOTONIC）
     */
    void addTimer(const TimerCallback& cb, int64_t whenMs);

    /**
     * 添加相对延迟定时器（一次性的）
     * @param delayMs 多少毫秒后执行
     * @param cb      到期后执行的回调
     */
    void runAfter(int64_t delayMs, const TimerCallback& cb);

private:
    /** timerfd 可读 → 有定时器到期 → 执行所有到期回调 */
    void handleRead();
    /** @return 最近到期的定时器时间（毫秒），空则返回 0 */
    int64_t getNextTimer() const;
    /** 重设 timerfd 的超时时间 */
    void resetTimerFd(int64_t whenMs);

    EventLoop* loop_;
    const int timerfd_;           ///< timerfd_create 返回的 fd
    Channel timerChannel_;        ///< 包进 Channel 丢进 epoll

    /** 到期时间 → 回调列表（multimap 自动按 key 排序） */
    std::multimap<int64_t, TimerCallback> timers_;
};
