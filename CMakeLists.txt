cmake_minimum_required(VERSION 3.16)
project(my_muduo VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(muduo_demo
    main.cpp
    Channel.cpp
    Poller.cpp
    EventLoop.cpp
    Socket.cpp
    Acceptor.cpp
    TcpConnection.cpp
    Buffer.cpp
    TcpServer.cpp
    EventLoopThread.cpp
    EventLoopThreadPool.cpp
    TimerQueue.cpp
)
