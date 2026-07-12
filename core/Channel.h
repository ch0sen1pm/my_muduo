#pragma once
#include <functional>

class EventLoop;

/**
 * Channel — 一个 fd 的事件描述
 *
 * 负责记录 fd 关心什么事件（events_）以及事件发生时的回调（readCallback_/writeCallback_）。
 * Channel 不拥有 fd——fd 的生命周期由 Socket 管理。
 */
class Channel {
public:
    using EventCallback = std::function<void()>;

    /**
     * @param loop 所属 EventLoop
     * @param fd   要观察的文件描述符
     */
    Channel(EventLoop* loop, int fd);
    ~Channel();

    /** 设置 fd 可读时的回调 */
    void setReadCallback(EventCallback cb) { readCallback_ = std::move(cb); }
    /** 设置 fd 可写时的回调 */
    void setWriteCallback(EventCallback cb) { writeCallback_ = std::move(cb); }

    /** 关注可读事件，注册到 epoll */
    void enableReading();
    /** 关注可写事件，注册到 epoll */
    void enableWriting();
    /** 取消所有事件关注 */
    void disableAll();

    /** @return 观察的 fd */
    int fd() const { return fd_; }
    /** @return 当前关注的事件（kReadEvent/kWriteEvent 的位组合） */
    int events() const { return events_; }

    /** Poller 填充内核返回的事件 */
    void set_revents(int revt) { revents_ = revt; }

    /** 事件分发：根据 revents_ 调 readCallback_ 或 writeCallback_ */
    void handleEvent();

private:
    /** 通过 EventLoop 更新 Poller 中的注册 */
    void update();

    EventLoop* loop_;
    int fd_;
    int events_;      ///< 关心的事件（kReadEvent | kWriteEvent）
    int revents_;     ///< 内核实际返回的事件（Poller 填充）

    static const int kNoneEvent = 0;
    static const int kReadEvent = 1;
    static const int kWriteEvent = 2;

    EventCallback readCallback_;
    EventCallback writeCallback_;
};
