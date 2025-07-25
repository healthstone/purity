#pragma once

#include <boost/asio.hpp>
#include <queue>
#include <string>
#include <vector>

#include "src/server/ClientSession/ClientSession.hpp"
#include "src/server/SessionMode/authstage/opcodes/AuthPacket.hpp"
#include "src/server/SessionMode/workstage/opcodes/WorkPacket.hpp"

class Client {
public:
    Client(boost::asio::io_context &io_context, std::string host, int port);

    void connect();
    void disconnect();

    void handle_logon_challenge(const std::string &username, const std::string &password);

    void set_session_mode(SessionMode mode) { session_mode_ = mode; }
    SessionMode get_session_mode() const { return session_mode_; }

private:
    void schedule_reconnect();
    void start_heartbeat();
    void send_ping();

    void send_packet(const AuthPacket &packet);
    void send_packet(const WorkPacket &packet);
    void send_packet_impl(const std::vector<uint8_t> &packet_data);

    void flush_queue();

    void start_receive_loop();

    template<typename PacketT>
    void read_payload(std::shared_ptr<std::vector<uint8_t>> header, uint16_t length);

    template<typename PacketT>
    void process_empty(std::shared_ptr<std::vector<uint8_t>> header);

    void handle_packet(AuthPacket &packet);
    void handle_packet(WorkPacket &packet);

    boost::asio::io_context &io;
    boost::asio::ip::tcp::socket socket;
    std::string host;
    int port;
    bool connected;

    boost::asio::steady_timer reconnect_timer;
    boost::asio::steady_timer heartbeat_timer;

    std::queue<std::vector<uint8_t>> outgoing_queue;

    SessionMode session_mode_ = SessionMode::AUTH_SESSION;
    std::shared_ptr<SRP6> srp_ = std::make_shared<SRP6>();
};
