#include "Poller.h"
#include "Channel.h"
#include <sys/epoll.h>
#include <unistd.h>

// epoll_create1: 创建 epoll 实例，返回一个 fd
// EPOLL_CLOEXEC: exec 时自动关闭，防 fd 泄漏
Poller::Poller(EventLoop* loop)
    : loop_(loop)
    , epollfd_(epoll_create1(EPOLL_CLOEXEC)) {
    if (epollfd_ < 0) {
        abort();
    }
    events_.resize(16);
}

Poller::~Poller() {
    ::close(epollfd_);
}

void Poller::poll(int timeoutMs, std::vector<Channel*>& activeChannels) {
    int numEvents = ::epoll_wait(epollfd_,
                                events_.data(),
                                events_.size(),
                                timeoutMs);

    if (numEvents > 0) {
        fillActiveChannels_(numEvents, activeChannels);

        // events_ 满了 → 扩容翻倍，下次能装更多
        if (static_cast<size_t>(numEvents) == events_.size()) {
            events_.resize(events_.size() * 2);
        }
    }
}

void Poller::fillActiveChannels_(int numEvents, std::vector<Channel*>& activeChannels) {
    for (int i = 0; i < numEvents; i ++) {
        int fd = events_[i].data.fd;
        auto it = channels_.find(fd);
        if (it != channels_.end()) {
            Channel* channel = it->second;
            channel->set_revents(events_[i].events);
            activeChannels.push_back(channel);
        }
    }
}

void Poller::updateChannel(Channel* channel) {
    int fd = channel->fd();

    // ADD/MOD/DEL 三种操作：第一次加、events=0 删除、否则修改
    if (channels_.find(fd) == channels_.end()) {
        channels_[fd] = channel;
        update_(EPOLL_CTL_ADD, channel);
    } else if (channel->events() == 0) {
        update_(EPOLL_CTL_DEL, channel);
    } else {
        update_(EPOLL_CTL_MOD, channel);
    }
}

void Poller::removeChannel(Channel* channel) {
    int fd = channel->fd();
    channels_.erase(fd);
    update_(EPOLL_CTL_DEL, channel);
}

void Poller::update_(int op, Channel* channel) {
    struct epoll_event ev;
    ev.events = 0;
    ev.data.fd = channel->fd();

    if (channel->events() & 1) {
        ev.events |= EPOLLIN;
    }

    if (channel->events() & 2) {
        ev.events |= EPOLLOUT;
    }

    ::epoll_ctl(epollfd_, op, channel->fd(), &ev);
}