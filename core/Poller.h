#pragma once
#include <vector>
#include <map>

class Channel;
class EventLoop;
struct epoll_event;

/**
 * Poller — epoll 的 C++ 封装
 *
 * 内部维护 map<fd, Channel*> 做 fd → Channel 的映射。
 * epoll_wait 只返回 fd（int），通过查表找到对应的 Channel 并填充 revents。
 */
class Poller {
public:
    /**
     * @param loop 所属 EventLoop
     */
    Poller(EventLoop* loop);
    ~Poller();

    /**
     * 阻塞等待事件
     * @param timeoutMs       超时毫秒数
     * @param activeChannels  输出参数，返回活跃的 Channel 列表
     */
    void poll(int timeoutMs, std::vector<Channel*>& activeChannels);

    /** 根据 Channel 的 events 状态自动执行 ADD/MOD/DEL */
    void updateChannel(Channel* channel);
    /** 从 epoll 移除 fd */
    void removeChannel(Channel* channel);

private:
    /** 将 epoll_event 数组转为 Channel* 列表 */
    void fillActiveChannels_(int numEvents, std::vector<Channel*>& activeChannels);
    /** 执行 epoll_ctl */
    void update_(int op, Channel* channel);

    EventLoop* loop_;
    int epollfd_;                           ///< epoll_create1 返回的 epoll 实例 fd
    std::map<int, Channel*> channels_;      ///< fd → Channel 映射（核心数据结构）
    std::vector<struct epoll_event> events_; ///< epoll_wait 的输出数组
};
