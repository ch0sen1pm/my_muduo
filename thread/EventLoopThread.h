// EventLoopThread.h — 一个线程跑一个 EventLoop
//
// 把 EventLoop 和 std::thread 打包。
// startLoop() 启动线程 → 线程里跑 EventLoop::loop() → 返回 loop 指针给调用者。

#pragma once
#include <thread>
#include <mutex>
#include <condition_variable>

class EventLoop;

class EventLoopThread {
public:
    EventLoopThread();
    /** 析构时自动 join() 等待线程结束 */
    ~EventLoopThread();

    /**
     * 启动线程并在其中创建 EventLoop
     * @return 线程中的 EventLoop 指针（调用者用于分配连接）
     */
    EventLoop* startLoop();

private:
    /** 线程入口：创建 EventLoop → 通知主线程 → loop() */
    void threadFunc();

    EventLoop* loop_;                    ///< 子线程中的 EventLoop
    std::thread thread_;                 ///< C++ 线程对象
    std::mutex mutex_;                   ///< 保护 loop_ 的互斥锁
    std::condition_variable cond_;       ///< 主线程等待 loop_ 创建完成
};
