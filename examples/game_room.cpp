// examples/game_room.cpp — 多人对战房间 demo
#include "net/TcpServer.h"
#include "net/TcpConnection.h"
#include "core/EventLoop.h"
#include "core/TimerQueue.h"
#include "logger.h"
#include "core/json_config.h"
#include <vector>
#include <map>
#include <iostream>
#include <algorithm>

int main() {
    json_config::load("chat_config.json");
    auto log = registry::instance().get("app.chat");

    EventLoop loop;
    TimerQueue timer(&loop);
    TcpServer server(&loop, 8080);
    server.setThreadNum(4);

    std::map<std::string, std::vector<TcpConnection*>> rooms;
    std::map<TcpConnection*, std::string> playerRoom;
    std::map<TcpConnection*, int> scores;
    int nextId = 1;
    std::map<TcpConnection*, int> playerId;

    // ====== 新连接 ======
    server.setConnectionCallback([&](TcpConnection* conn) {
        scores[conn] = 0;
        playerId[conn] = nextId++;
        conn->send("\r\n== 多人对战房间 ==\r\n"
                   "  join:<room>  加入房间\r\n"
                   "  attack       攻击 (+1分)\r\n"
                   "  score        查看积分\r\n"
                   "  status       查看在线\r\n\r\n");
        LOG_INFO(log, "新连接 #" + std::to_string(playerId[conn]) +
                 "，在线: " + std::to_string(scores.size()));
    });

    // ====== 断连清理 ======
    server.setCloseCallback([&](TcpConnection* conn) {
        auto rit = playerRoom.find(conn);
        if (rit != playerRoom.end()) {
            std::string room = rit->second;
            auto& vec = rooms[room];
            vec.erase(std::find(vec.begin(), vec.end(), conn));
            if (vec.empty()) rooms.erase(room);
            playerRoom.erase(rit);
        }
        scores.erase(conn);
        playerId.erase(conn);
        LOG_INFO(log, "断开，在线: " + std::to_string(scores.size()));
    });

    // ====== 消息处理 ======
    server.setMessageCallback([&](TcpConnection* conn, const char* data, size_t len) {
        std::string msg(data, len);
        LOG_INFO(log, "收到: " + msg);

        if (msg.rfind("join:", 0) == 0) {
            std::string room = msg.substr(5);
            auto it = playerRoom.find(conn);
            if (it != playerRoom.end()) {
                std::string old = it->second;
                auto& vec = rooms[old];
                vec.erase(std::find(vec.begin(), vec.end(), conn));
                if (vec.empty()) rooms.erase(old);
            }
            rooms[room].push_back(conn);
            playerRoom[conn] = room;
            conn->send("进入房间: " + room + " (" +
                       std::to_string(rooms[room].size()) + "人)\r\n");
            for (auto* c : rooms[room]) {
                if (c != conn) c->send("[系统] 有玩家加入\r\n");
            }

        } else if (msg == "attack") {
            auto it = playerRoom.find(conn);
            if (it == playerRoom.end()) {
                conn->send("还没加入房间！先 join:<room>\r\n");
                return;
            }
            scores[conn]++;
            std::string room = it->second;
            conn->send("攻击！你的积分: " + std::to_string(scores[conn]) + "\r\n");
            for (auto* c : rooms[room]) {
                if (c != conn)
                    c->send("[战斗] #" + std::to_string(playerId[conn]) +
                            " 攻击了！\r\n");
            }

        } else if (msg == "score") {
            auto it = playerRoom.find(conn);
            if (it == playerRoom.end()) {
                conn->send("还没加入房间！先 join:<room>\r\n");
                return;
            }
            std::string room = it->second;
            std::string reply = "=== " + room + " 积分 ===\r\n";
            for (auto* c : rooms[room]) {
                reply += "  player#" + std::to_string(playerId[c]) +
                         ": " + std::to_string(scores[c]) + "分\r\n";
            }
            conn->send(reply);

        } else if (msg == "status") {
            std::string reply = "=== 在线 ===\r\n";
            reply += "总人数: " + std::to_string(scores.size()) + "\r\n";
            for (auto& [r, vec] : rooms) {
                reply += "  " + r + ": " + std::to_string(vec.size()) + "人\r\n";
            }
            conn->send(reply);

        } else {
            conn->send("指令: join:<room> / attack / score / status\r\n");
        }
    });

    // ====== 回合倒计时：每 30 秒广播房间排名 ======
    std::function<void()> roundTick;
    roundTick = [&]() {
        for (auto& [room, players] : rooms) {
            if (players.empty()) continue;
            std::sort(players.begin(), players.end(), [&](auto* a, auto* b) {
                return scores[a] > scores[b];
            });
            std::string msg = "[回合结算] " + room + " 排名:\r\n";
            for (size_t i = 0; i < players.size(); i++) {
                msg += "  #" + std::to_string(i + 1) + "  player#" +
                       std::to_string(playerId[players[i]]) + " — " +
                       std::to_string(scores[players[i]]) + "分\r\n";
            }
            for (auto* c : players) c->send(msg);
        }
        timer.runAfter(30000, roundTick);
    };
    timer.runAfter(30000, roundTick);

    LOG_INFO(log, "对战房间启动，端口 8080...");
    server.start();
    loop.loop();
}
