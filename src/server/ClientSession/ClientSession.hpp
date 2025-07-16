#pragma once

#include <boost/asio.hpp>
#include <memory>
#include <vector>
#include <deque>
#include <atomic>

#include "src/server/Server.hpp"
#include "MessageBuffer.hpp"
#include "packet/Packet.hpp"

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

    // PvPGN handshake данные
    void setArchTag(const uint32_t &val) { archtag_ = val; }

    void setClientTag(const uint32_t &val) { clienttag_ = val; }

    void setVersionId(const uint32_t &val) { versionid_ = val; }

    void setServerToken(const uint32_t &val) { servertoken_ = val; }

    void setClientToken_(const uint32_t &val) { clienttoken_ = val; }

    // Режим: BNCS или W3ROUTE
    void set_session_mode(SessionMode mode) { session_mode_ = mode; }

    SessionMode get_session_mode() const { return session_mode_; }

    MessageBuffer& read_buffer() {
        return read_buffer_;
    }

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

    // PvPGN handshake info
    uint32_t archtag_ = 0;
    uint32_t clienttag_ = 0;
    uint32_t versionid_ = 0;
    uint32_t servertoken_ = 0;
    uint32_t clienttoken_ = 0;
};
