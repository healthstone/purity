#pragma once

#include <boost/asio.hpp>
#include <memory>
#include <vector>
#include <deque>
#include <atomic>

#include "src/server/Server.hpp"
#include "packet/MessageBuffer.hpp"
#include "packet/Packet.hpp"
#include "src/server/SessionMode/authstage/entity/AccountInfo.hpp"

class Server; // forward declaration

enum class SessionMode {
    AUTH_SESSION,
    WORK_SESSION
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

    // Режим: AUTH или WORK
    void set_session_mode(SessionMode mode) { session_mode_ = mode; }

    SessionMode get_session_mode() const { return session_mode_; }

    MessageBuffer &read_buffer() {
        return read_buffer_;
    }

    AccountInfo *getAccountInfo() { return accountInfo_.get(); }

private:
    void do_read();

    void process_read_buffer();

    void do_write();

    void do_send_packet(const Packet &packet);

    boost::asio::ip::tcp::socket socket_;
    std::shared_ptr<Server> server_;
    std::shared_ptr<AccountInfo> accountInfo_ = std::make_shared<AccountInfo>();

    MessageBuffer read_buffer_;

    std::deque<std::vector<uint8_t>> write_queue_;
    bool writing_ = false;
    std::atomic<bool> closed_{false};

    SessionMode session_mode_ = SessionMode::AUTH_SESSION;
};
