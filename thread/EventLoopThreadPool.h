// EventLoopThreadPool.h — 线程池
//
// 管理 N 个 EventLoopThread，新连接通过轮转（round-robin）分配到不同线程。

#pragma once
#include <vector>
#include <memory>

class EventLoop;
class EventLoopThread;

class EventLoopThreadPool {
public:
    /**
     * @param baseLoop   主线程 EventLoop（numThreads=0 时使用）
     * @param numThreads 工作线程数，0 = 单线程模式
     */
    EventLoopThreadPool(EventLoop* baseLoop, int numThreads);
    ~EventLoopThreadPool();

    /** 启动所有工作线程 */
    void start();

    /**
     * 轮转获取下一个 EventLoop
     * @return 如果没有工作线程则返回 baseLoop_
     */
    EventLoop* getNextLoop();

private:
    EventLoop* baseLoop_;
    int numThreads_;
    int next_;                                       ///< 轮转索引

    std::vector<std::unique_ptr<EventLoopThread>> threads_;  ///< 保持线程存活
    std::vector<EventLoop*> loops_;                           ///< 每个线程的 loop 指针
};
