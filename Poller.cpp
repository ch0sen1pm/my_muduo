#include "Poller.h"
#include "Channel.h"
#include <sys/epoll.h>
#include <unistd.h>

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

void Poller::poll(int timeoutMs, std::vector<Channel*> activeChannels) {
    int numEvents = ::epoll_wait(epollfd_,
                                events_.data(),
                                events_.size(),
                                timeoutMs);

    if (numEvents > 0) {
        fillActiveChannels_(numEvents, activeChannels);

        if (static_cast<size_t>(numEvents) == events_.size()) {
            events_.resize(events_.size() * 2);
        }
    }
}

void Poller::fillActiveChannels_(int numEvents, std::vector<Channel*>& activeChannels) {
    for (int i = 0; i < numEvents; i ++) {
        int fd = events_[i].data.fd;
        auto it = channels_.find(fd);
        if (it != channels_,end()) {
            Channel* channel = it->second;
            channel->set_revents(events_[i].events);
            activeChannels.push_back(channel);
        }
    }
}