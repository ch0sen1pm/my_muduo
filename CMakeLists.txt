cmake_minimum_required(VERSION 3.16)
project(my_muduo VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_SOURCE_DIR})

add_executable(muduo_demo
    main.cpp
    core/Channel.cpp
    core/Poller.cpp
    core/EventLoop.cpp
    core/TimerQueue.cpp
    net/Socket.cpp
    net/Acceptor.cpp
    net/TcpConnection.cpp
    net/Buffer.cpp
    net/TcpServer.cpp
    thread/EventLoopThread.cpp
    thread/EventLoopThreadPool.cpp
)

add_executable(chat_server
    examples/chat_server.cpp
    core/Channel.cpp
    core/Poller.cpp
    core/EventLoop.cpp
    core/TimerQueue.cpp
    net/Socket.cpp
    net/Acceptor.cpp
    net/TcpConnection.cpp
    net/Buffer.cpp
    net/TcpServer.cpp
    thread/EventLoopThread.cpp
    thread/EventLoopThreadPool.cpp
)

target_include_directories(chat_server PRIVATE /mnt/d/my_logger)


add_executable(game_room
    examples/game_room.cpp
    core/Channel.cpp
    core/Poller.cpp
    core/EventLoop.cpp
    core/TimerQueue.cpp
    net/Socket.cpp
    net/Acceptor.cpp
    net/TcpConnection.cpp
    net/Buffer.cpp
    net/TcpServer.cpp
    thread/EventLoopThread.cpp
    thread/EventLoopThreadPool.cpp
)

target_include_directories(game_room PRIVATE /mnt/d/my_logger)