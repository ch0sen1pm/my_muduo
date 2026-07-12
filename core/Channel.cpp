#include "Channel.h"
#include "EventLoop.h"

Channel::Channel(EventLoop* loop, int fd)
    : loop_(loop)
    , fd_(fd)
    , events_(0)
    , revents_(0)
{}

Channel::~Channel() {

}


void Channel::enableReading() {
    events_ |= kReadEvent;
    update();
}

void Channel::enableWriting() {
    events_ |= kWriteEvent;
    update();
}

void Channel::disableAll() {
    events_ = kNoneEvent;
    update();
}

void Channel::update() {
    loop_->updateChannel(this);
}

void Channel::handleEvent() {
    if ((revents_ & kReadEvent) && readCallback_) {
        readCallback_();
    }

    if ((revents_ & kWriteEvent) && writeCallback_) {
        writeCallback_();
    }
}