#pragma once

#include <boost/asio.hpp>
#include <queue>
#include <string>
#include <vector>
#include "src/server/SessionMode/bncs/opcodes/BNETPacket8.hpp"

class Client {
public:
    Client(boost::asio::io_context &io_context, std::string host, int port);

    void connect();

    void disconnect();

    void send_message(const std::string &msg);

    void send_auth();

private:

    void schedule_reconnect();

    void start_heartbeat();

    void send_ping();

    void send_packet(const BNETPacket8 &packet);

    void flush_queue();

    void start_receive_loop();

    void handle_packet(BNETPacket8 &packet);

    boost::asio::io_context &io;
    boost::asio::ip::tcp::socket socket;
    std::string host;
    int port;
    bool connected;

    boost::asio::steady_timer reconnect_timer;
    boost::asio::steady_timer heartbeat_timer;

    std::queue<std::vector<uint8_t>> outgoing_queue;
};
