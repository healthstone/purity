#include "Client.hpp"
#include "Logger.hpp"
#include <iostream>
#include <utility>
#include <endian.h>  // htole16

using boost::asio::ip::tcp;
using namespace std::chrono_literals;

Client::Client(boost::asio::io_context &io_context, std::string h, int p)
        : io(io_context), socket(io_context), host(std::move(h)), port(p), connected(false),
          reconnect_timer(io_context), heartbeat_timer(io_context) {}

void Client::connect() {
    tcp::resolver resolver(io);
    auto endpoints = resolver.resolve(host, std::to_string(port));

    boost::asio::async_connect(socket, endpoints,
                               [this](boost::system::error_code ec, const tcp::endpoint &) {
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
    reconnect_timer.async_wait([this](const boost::system::error_code &) {
        Logger::get()->info("[Client] Attempting reconnect...");
        connect();
    });
}

void Client::start_heartbeat() {
    heartbeat_timer.expires_after(5s);
    heartbeat_timer.async_wait([this](const boost::system::error_code &ec) {
        if (!ec && connected) {
            send_ping();
            start_heartbeat();
        }
    });
}

void Client::send_ping() {
    Packet ping(Opcode::SID_PING);
    ping.write_uint32(static_cast<uint32_t>(std::chrono::system_clock::now().time_since_epoch().count()));
    send_packet(ping);
}

void Client::send_message(const std::string &msg) {
    Packet p(Opcode::SID_CHATCOMMAND);
    p.write_string(msg);
    send_packet(p);
}

void Client::send_select_acc_by_username(const std::string &username) {
    Packet p(Opcode::SID_AUTH_ACCOUNTLOGON);
    p.write_string(username);
    send_packet(p);
}

void Client::send_packet(const Packet &packet) {
    const auto &body = packet.serialize();

    // PvPGN: [Opcode_LE][Length_LE][Payload]
    uint16_t opcode_le = htole16(static_cast<uint16_t>(packet.get_opcode()));
    uint16_t length_le = htole16(static_cast<uint16_t>(body.size() + 4)); // header (4) + payload

    std::vector<uint8_t> full_packet;

    // [0,1]: Opcode (LE)
    full_packet.insert(full_packet.end(),
                       reinterpret_cast<const uint8_t *>(&opcode_le),
                       reinterpret_cast<const uint8_t *>(&opcode_le) + sizeof(opcode_le));

    // [2,3]: Length (LE)
    full_packet.insert(full_packet.end(),
                       reinterpret_cast<const uint8_t *>(&length_le),
                       reinterpret_cast<const uint8_t *>(&length_le) + sizeof(length_le));

    // Payload
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

    auto &front = outgoing_queue.front();
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
    auto header = std::make_shared<std::vector<uint8_t>>(4); // [Opcode_LE][Length_LE]

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

                                // PvPGN: [0,1]: Opcode_LE, [2,3]: Length_LE
                                uint16_t raw_opcode = (*header)[0] | ((*header)[1] << 8);
                                uint16_t total_size = (*header)[2] | ((*header)[3] << 8);

                                Opcode opcode = static_cast<Opcode>(raw_opcode);

                                size_t body_size = total_size - 4; // Payload = total - header
                                if (body_size == 0) {
                                    log->warn("[Client] Received empty payload (opcode {})", static_cast<uint16_t>(opcode));
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
                                                            } catch (const std::exception &ex) {
                                                                log->error("[Client] Failed to parse packet: {}", ex.what());
                                                            }

                                                            start_receive_loop();
                                                        });
                            });
}

void Client::handle_packet(Packet &p) {
    auto log = Logger::get();
    switch (p.get_opcode()) {
        case Opcode::SID_PING: {
            uint32_t ts = p.read_uint32();
            log->debug("[Client] Received SID_PING, timestamp={}", ts);
            break;
        }
        case Opcode::SID_CHATEVENT: {
            std::string msg = p.read_string();
            log->info("[Client] Received chat event: {}", msg);
            break;
        }
        default:
            log->warn("[Client] Unknown opcode: {}", static_cast<uint16_t>(p.get_opcode()));
            break;
    }
}
