#include "Client.hpp"
#include "Logger.hpp"
#include "generators/GeneratorUtils.hpp"
#include <utility>

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
    heartbeat_timer.expires_after(30s);
    heartbeat_timer.async_wait([this](const boost::system::error_code &ec) {
        if (!ec && connected) {
            send_ping();
            start_heartbeat(); // повторяем
        }
    });
}

void Client::send_ping() {
    AuthPacket ping(AuthCmd::AUTH_CMSG_PING);
    uint32_t ping_id = GeneratorUtils::random_uint32();
    ping.write_uint32_le(ping_id);
    send_packet(ping);
    Logger::get()->debug("[Client] Sent AUTH_CMSG_PING");
}

void Client::handle_logon_challenge() {
    AuthPacket packet(AuthCmd::AUTH_LOGON_CHALLENGE);

    //// Пример чтения в порядке протокола
    //        uint8_t cmd = p.read_uint8_be();             // 0x00
    //        uint8_t error = p.read_uint8_be();           // 0x01
    //        uint16_t size = p.read_uint16_be();          // 0x02
    //
    //        std::string gamename = p.read_string_raw(4); // 0x04
    //
    //        uint8_t version1 = p.read_uint8_be();        // 0x08
    //        uint8_t version2 = p.read_uint8_be();        // 0x09
    //        uint8_t version3 = p.read_uint8_be();        // 0x0A
    //
    //        uint16_t build = p.read_uint16_be();         // 0x0B
    //
    //        std::string platform = p.read_string_raw(4); // 0x0D
    //        std::string os = p.read_string_raw(4);       // 0x11
    //        std::string country = p.read_string_raw(4);  // 0x15
    //
    //        uint32_t timezone = p.read_uint32_le();             // 0x19 (LE)
    //        uint32_t ip = p.read_uint32_le();                   // 0x1D (LE)
    //
    //        uint8_t name_len = p.read_uint8_be();               // 0x21
    //        std::string username = p.read_string_raw(name_len); // 0x22
    packet.write_string_nt_le("Test_user");
    send_packet(packet);
    Logger::get()->debug("[Client] Sent AUTH_CMSG_LOGON_CHALLENGE");
}

void Client::send_packet(const AuthPacket &packet) {
    std::vector<uint8_t> full_packet = packet.build_packet();

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
    auto header = std::make_shared<std::vector<uint8_t>>(3); // [Length LE(2)][Opcode(1)]

    boost::asio::async_read(
            socket, boost::asio::buffer(*header),
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

                // Читаем длину пакета LE: Length = 1 (opcode) + PayloadSize
                uint16_t length = (*header)[0] | ((*header)[1] << 8);
                uint8_t opcode = (*header)[2];

                if (length == 0) {
                    log->error("[Client] Invalid length = 0");
                    connected = false;
                    schedule_reconnect();
                    return;
                }

                uint16_t payload_size = length - 1; // Payload = Length - 1 (opcode)

                if (payload_size > 2048) {
                    log->error("[Client] Payload size too large: {}", payload_size);
                    connected = false;
                    schedule_reconnect();
                    return;
                }

                if (payload_size == 0) {
                    try {
                        AuthCmd auth_opcode = static_cast<AuthCmd>(opcode);
                        AuthPacket p = AuthPacket::deserialize(auth_opcode, {});
                        handle_packet(p);
                    } catch (const std::exception &ex) {
                        log->error("[Client] Failed to parse empty packet: {}", ex.what());
                    }
                    start_receive_loop();
                    return;
                }

                auto payload = std::make_shared<std::vector<uint8_t>>(payload_size);

                boost::asio::async_read(
                        socket, boost::asio::buffer(*payload),
                        [this, payload, opcode](boost::system::error_code ec2, std::size_t) {
                            auto log = Logger::get();

                            if (ec2) {
                                if (ec2 == boost::asio::error::operation_aborted || ec2 == boost::asio::error::eof) {
                                    log->info("[Client] Disconnected while reading payload");
                                } else {
                                    log->error("[Client] Payload read failed: {}", ec2.message());
                                }
                                connected = false;
                                schedule_reconnect();
                                return;
                            }

                            try {
                                AuthCmd auth_opcode = static_cast<AuthCmd>(opcode);
                                AuthPacket p = AuthPacket::deserialize(auth_opcode, *payload);
                                handle_packet(p);
                            } catch (const std::exception &ex) {
                                log->error("[Client] Failed to parse packet: {}", ex.what());
                            }

                            start_receive_loop();
                        });
            });
}


void Client::handle_packet(AuthPacket &p) {
    auto log = Logger::get();

    switch (p.cmd()) {
        case AuthCmd::AUTH_SMSG_PONG:
            log->debug("[Client] Received AUTH_SMSG_PONG");
            break;

        case AuthCmd::AUTH_LOGON_CHALLENGE: {
            std::string salt = p.read_string_nt_le();
            std::string public_b = p.read_string_nt_le();
            log->debug("[Client] Received AUTH_SMSG_LOGON_CHALLENGE: salt={}, public_B={}", salt, public_b);
            break;
        }

        default:
            log->warn("[Client] Unknown opcode: {}", static_cast<uint8_t>(p.cmd()));
            break;
    }
}
