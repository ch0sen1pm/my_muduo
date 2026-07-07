#pragma once
#include <netinet/in.h>

class Socket {
public:
    explicit Socket(int sockfd);
    ~Socket();

    int fd() const { return sockfd_; }

    void bindAddress(const sockaddr_in& addr);
    void listen();
    void setReuseAddr(bool on);

    static Socket createTcp();

    Socket(Socket&&) noexcept;
    Socket& operator=(Socket&&) noexcept;
    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;

private:
    int sockfd_;
};