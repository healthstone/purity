#pragma once

#include <boost/asio.hpp>
#include <memory>
#include <vector>
#include <deque>

#include "Packet.hpp"

#include "src/server/Server.hpp"
#include "MessageBuffer.hpp"

class Server; // forward declaration

class ClientSession : public std::enable_shared_from_this<ClientSession> {
public:
    ClientSession(boost::asio::ip::tcp::socket socket, std::shared_ptr<Server> server);

    void start();

    void close();

    bool isOpened() { return closed_; }

    void send_packet(const Packet &packet);

    boost::asio::ip::tcp::socket& socket() {
        return socket_;
    }
    std::shared_ptr<Server> server() const { return server_; }

    // Battle.net методы
    void setArchTag(const uint32_t& val) { archtag_ = val; }
    void setClientTag(const uint32_t& val) { clienttag_ = val; }
    void setVersionId(const uint32_t& val) { versionid_ = val; }
    void setServerToken(const uint32_t& val) { servertoken_ = val; }

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

    // pvpgn data
    uint32_t archtag_;
    uint32_t clienttag_;
    uint32_t versionid_;

    uint32_t servertoken_;
};
