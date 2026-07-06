#pragma once
#include <vector>
#include <map>

Class Channel;
Class EventLoop;
struct epoll_event;

class Poller {
public:
    Poller(EventLoop* loop);
    ~Poller();

    void poll(int timeoutMs, std::vector<Channel*>& activeChannels);

    void updateChannel(Channel* Channel);
    void removeChannel(Channel* Channel);
private:
    void fillActiveChannels_(int numEvents, std::vector<Channel*>& activeChannels);

    void update_(int op, Channel* channel);

    EventLoop* loop_;
    int epollfd_;
    std::map<int, Channel*> channels_;
    std::vector<struct epoll_event> events_;
};