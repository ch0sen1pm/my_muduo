#include "EventLoop.h"
#include "Poller.h"
#include "Channel.h"

EventLoop::EventLoop()
    : poller_(std::make_unique<Poller>(this))
    , quit_(false)
{}

EventLoop::~EventLoop() = default;

void EventLoop::loop() {
    while (!quit_) {
        activeChannels_.clear();
        poller_->poll(1000, activeChannels_);
        for (auto* channel : activeChannels_) {
            channel->handleEvent();
        }
    }
}

void EventLoop::updateChannel(Channel* channel) {
    poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel* channel) {
    poller_->removeChannel(channel);
}
