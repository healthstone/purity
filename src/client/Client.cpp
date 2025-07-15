#include "Client.hpp"
#include "Logger.hpp"
#include <iostream>
#include <utility>

using boost::asio::ip::tcp;
using namespace std::chrono_literals;

Client::Client(boost::asio::io_context& io_context, std::string h, int p)
        : io(io_context), socket(io_context), host(std::move(h)), port(p), connected(false),
          reconnect_timer(io_context), heartbeat_timer(io_context) {}

void Client::connect() {
    tcp::resolver resolver(io);
    auto endpoints = resolver.resolve(host, std::to_string(port));

    boost::asio::async_connect(socket, endpoints,
                               [this](boost::system::error_code ec, const tcp::endpoint&) {
                                   auto log = Logger::get();
                                   if (!ec) {
                                       connected = true;
                                       log->info("[Client] Connected to server");

                                       start_receive_loop();
                                       flush_queue();
                                       start_heartbeat();
                                   } else {
                                       log->error("[Client] Connection failed: {}", ec.message());
                                       schedule_reconnect();
                                   }
                               });
}

void Client::disconnect() {
    boost::asio::post(io, [this]() {
        boost::system::error_code ignored_ec;

        heartbeat_timer.cancel(ignored_ec);
        reconnect_timer.cancel(ignored_ec);

        if (socket.is_open()) {
            socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
            socket.close(ignored_ec);
        }

        connected = false;
        Logger::get()->info("[Client] Disconnected from server (disconnect())");
    });
}

void Client::schedule_reconnect() {
    connected = false;
    socket.close();
    reconnect_timer.expires_after(3s);
    reconnect_timer.async_wait([this](const boost::system::error_code&) {
        Logger::get()->info("[Client] Attempting reconnect...");
        connect();
    });
}

void Client::start_heartbeat() {
    heartbeat_timer.expires_after(5s);
    heartbeat_timer.async_wait([this](const boost::system::error_code& ec) {
        if (!ec && connected) {
            send_ping();
            start_heartbeat(); // next
        }
    });
}

void Client::send_ping() {
    Packet ping(Opcode::CMSG_PING);
    ping.write_uint32(static_cast<uint32_t>(std::chrono::system_clock::now().time_since_epoch().count()));
    send_packet(ping);
}

void Client::send_message(const std::string& msg) {
    Packet p(Opcode::CMSG_CHAT_MESSAGE);
    p.write_string(msg);
    send_packet(p);
}

void Client::send_select_acc_by_username(const std::string& username) {
    Packet p(Opcode::CMSG_ACCOUNT_LOOKUP_BY_NAME);
    p.write_string(username);
    send_packet(p);
}

void Client::send_packet(const Packet& packet) {
    const auto& body = packet.serialize();

    ByteBuffer header;
    header.write_uint16(static_cast<uint16_t>(body.size() + 2)); // size = body + opcode
    header.write_uint16(static_cast<uint16_t>(packet.get_opcode()));

    std::vector<uint8_t> full_packet = header.data();
    full_packet.insert(full_packet.end(), body.begin(), body.end());

    if (!connected) {
        Logger::get()->info("[Client] Not connected. Queuing packet.");
        outgoing_queue.push(full_packet);
        return;
    }

    outgoing_queue.push(full_packet);

    if (outgoing_queue.size() == 1) {
        flush_queue();
    }
}

void Client::flush_queue() {
    if (!connected || outgoing_queue.empty())
        return;

    auto& front = outgoing_queue.front();
    boost::asio::async_write(socket, boost::asio::buffer(front),
                             [this](boost::system::error_code ec, std::size_t) {
                                 if (ec) {
                                     Logger::get()->error("[Client] Write failed: {}", ec.message());
                                     connected = false;
                                     schedule_reconnect();
                                     return;
                                 }
                                 outgoing_queue.pop();
                                 flush_queue();
                             });
}

void Client::start_receive_loop() {
    auto header = std::make_shared<std::vector<uint8_t>>(4); // 2 bytes size + 2 bytes opcode!

    boost::asio::async_read(socket, boost::asio::buffer(*header),
                            [this, header](boost::system::error_code ec, std::size_t) {
                                auto log = Logger::get();
                                if (ec) {
                                    if (ec == boost::asio::error::operation_aborted || ec == boost::asio::error::eof) {
                                        log->info("[Client] Disconnected while reading header");
                                    } else {
                                        log->error("[Client] Header read failed: {}", ec.message());
                                    }
                                    connected = false;
                                    schedule_reconnect();
                                    return;
                                }

                                uint16_t total_size = ((*header)[0] << 8) | (*header)[1];
                                uint16_t raw_opcode = ((*header)[2] << 8) | (*header)[3];
                                Opcode opcode = static_cast<Opcode>(raw_opcode);

                                size_t body_size = total_size - 2;
                                if (body_size == 0) {
                                    log->error("[Client] Invalid body size 0");
                                    connected = false;
                                    schedule_reconnect();
                                    return;
                                }

                                auto body = std::make_shared<std::vector<uint8_t>>(body_size);
                                boost::asio::async_read(socket, boost::asio::buffer(*body),
                                                        [this, body, opcode](boost::system::error_code ec2, std::size_t) {
                                                            auto log = Logger::get();
                                                            if (ec2) {
                                                                if (ec2 == boost::asio::error::operation_aborted || ec2 == boost::asio::error::eof) {
                                                                    log->info("[Client] Disconnected while reading body");
                                                                } else {
                                                                    log->error("[Client] Body read failed: {}", ec2.message());
                                                                }
                                                                connected = false;
                                                                schedule_reconnect();
                                                                return;
                                                            }

                                                            try {
                                                                Packet p = Packet::deserialize(*body, opcode);
                                                                handle_packet(p);
                                                            } catch (const std::exception& ex) {
                                                                log->error("[Client] Failed to parse packet: {}", ex.what());
                                                            }

                                                            start_receive_loop();
                                                        });
                            });
}

void Client::handle_packet(Packet& p) {
    auto log = Logger::get();
    switch (p.get_opcode()) {
        case Opcode::SMSG_PONG: {
            uint32_t ts = p.read_uint32();
            log->debug("[Client] Received SMSG_PONG, timestamp={}", ts);
            break;
        }
        case Opcode::SMSG_ACCOUNT_LOOKUP_BY_NAME: {
            std::string name = p.read_string();
            bool found = p.read_bool();
            log->info("[Client] Account lookup result: name=[{}] found=[{}]", name, found);
            break;
        }
        default:
            log->warn("[Client] Unknown opcode: {}", static_cast<uint16_t>(p.get_opcode()));
            break;
    }
}
