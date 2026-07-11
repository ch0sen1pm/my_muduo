#include "EventLoop.h"
#include "Channel.h"
#include "Socket.h"
#include "Acceptor.h"
#include <iostream>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include "TcpConnection.h"

int main() {
   EventLoop loop;

   sockaddr_in addr;
   memset(&addr, 0, sizeof(addr));
   addr.sin_family = AF_INET;
   addr.sin_addr.s_addr = INADDR_ANY;
   addr.sin_port = htons(8080);

   Acceptor acceptor(&loop, addr);
   acceptor.setNewConnectionCallback([&](int connfd, const sockaddr_in& peer) {
      char ip[INET_ADDRSTRLEN];
      inet_ntop(AF_INET, &peer.sin_addr, ip, sizeof(ip));
      std::cout << "新连接！fd=" << connfd << " ip=" << ip
                << " port=" << ntohs(peer.sin_port) << std::endl;
      
      auto* conn = new TcpConnection(&loop, connfd);

      conn->setMessageCallback([conn](const char* data, size_t len) {
         std::string msg(data, len);
         std::cout << "收到：" << msg << std::endl;
         conn->send("服务器收到：" + msg + "\r\n");
      });

      conn->connectEstablished();
   });

   acceptor.listen();

   std::cout << "监听 8080..." << std::endl;
   loop.loop();
}
