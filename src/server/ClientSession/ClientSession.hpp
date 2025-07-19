#pragma once

#include <boost/asio.hpp>
#include <memory>
#include <vector>
#include <deque>
#include <atomic>

#include "src/server/Server.hpp"
#include "MessageBuffer.hpp"
#include "packet/Packet.hpp"
#include "src/server/SessionMode/bncs/entity/account/BNCSAccount.hpp"

class Server; // forward declaration

enum class SessionMode {
    BNCS,
    W3ROUTE
};

class ClientSession : public std::enable_shared_from_this<ClientSession> {
public:
    ClientSession(boost::asio::ip::tcp::socket socket, std::shared_ptr<Server> server);

    void start();

    void close();

    bool isOpened() const { return !closed_; }

    void send_packet(std::shared_ptr<const Packet> packet);

    boost::asio::ip::tcp::socket &socket() { return socket_; }

    std::shared_ptr<Server> server() const { return server_; }

    // Режим: BNCS или W3ROUTE
    void set_session_mode(SessionMode mode) { session_mode_ = mode; }

    SessionMode get_session_mode() const { return session_mode_; }

    MessageBuffer& read_buffer() {
        return read_buffer_;
    }

    BNCSAccount* getBNCSAccount() { return bncsAccount_.get(); }

private:
    void do_read();

    void process_read_buffer();

    void do_write();

    void do_send_packet(const Packet &packet);

    boost::asio::ip::tcp::socket socket_;
    std::shared_ptr<Server> server_;

    MessageBuffer read_buffer_;

    std::deque<std::vector<uint8_t>> write_queue_;
    bool writing_ = false;
    std::atomic<bool> closed_{false};

    SessionMode session_mode_ = SessionMode::BNCS;

    // PvPGN
    std::shared_ptr<BNCSAccount> bncsAccount_;
};
