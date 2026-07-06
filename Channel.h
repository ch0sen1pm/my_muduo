#pragma once
#include <functional>

class EventLoop;

class Channel {
public:
    using EventCallback = std::function<void()>;

    Channel(EventLoop* loop, int fd);
    ~Channel();

    void setReadCallback(EventCallback cb) { readCallback_ = std::move(cb); }
    void setWriteCallback(EventCallback cb) { wirteCallback_ = std::move(cb); }

    void enableReading();
    void enableWriting();
    void disableAll();

    int fd() const { return fd_; }
    int events() const { return events_; }

    void set_revents(int revt) { revents_ = revt; }

    void handleEvent();

private:
    void update();

    EventLoop* loop_;
    int fd_;
    int events_;
    int revents_;

    static const int kNoneEvent = 0;
    static const int kReadEvent = 1;
    static const int kWriteEvent = 2;

    EventCallback readCallback_;
    EventCallback wirteCallback_;
};