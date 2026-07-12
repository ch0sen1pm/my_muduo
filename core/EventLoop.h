// EventLoop.h — 事件循环引擎
//
// 核心逻辑：while(true) { poll → dispatch }
// 每个 EventLoop 独占一个 Poller（one loop per poller）

#pragma once
#include <vector>
#include <memory>

class Channel;
class Poller;

/**
 * EventLoop — Reactor 模式的事件循环
 *
 * 持有 Poller，loop() 中不断 poll → dispatch，是 muduo 的发动机。
 * 一个线程最多一个 EventLoop（one loop per thread）。
 */
class EventLoop {
public:
    EventLoop();
    ~EventLoop();

    /** 进入事件循环（阻塞，直到 quit） */
    void loop();

    /** 委托 Poller 更新 Channel */
    void updateChannel(Channel* channel);
    /** 委托 Poller 移除 Channel */
    void removeChannel(Channel* channel);

private:
    std::unique_ptr<Poller> poller_;             ///< epoll 封装
    std::vector<Channel*> activeChannels_;       ///< 本轮活跃的 Channel
    bool quit_;                                  ///< 退出标志
};
