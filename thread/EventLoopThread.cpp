#include "../core/EventLoop.h"
#include "EventLoopThread.h"

EventLoopThread::EventLoopThread()
    : loop_(nullptr)
{}

EventLoopThread::~EventLoopThread() {
    if (thread_.joinable()) {
        thread_.join();
    }
}

EventLoop* EventLoopThread::startLoop() {
    thread_ = std::thread([this]() { threadFunc(); });

    {
        std::unique_lock<std::mutex> lock(mutex_);
        // 等子线程里 EventLoop 创建好 → loop_ 不为空才返回
        // 不加 wait 可能拿到 nullptr → 调用者崩溃
        cond_.wait(lock, [this]() { return loop_ != nullptr; });
    }

    return loop_;
}

void EventLoopThread::threadFunc() {
    EventLoop loop;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        loop_ = &loop;
    }
    cond_.notify_one();
    loop.loop();
}