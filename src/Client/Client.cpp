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
    AuthPacket ping(AuthOpcodes::CMSG_PING);
    uint32_t ping_id = GeneratorUtils::random_uint32();
    ping.write_uint32_le(ping_id);
    send_packet(ping);
    Logger::get()->debug("[Client] Sent CMSG_PING");
}

void Client::handle_logon_challenge() {
    AuthPacket packet(AuthOpcodes::CMSG_AUTH_LOGON_CHALLENGE);
    packet.write_string_nt_le("Test_user");
    send_packet(packet);
    Logger::get()->debug("[Client] Sent CMSG_AUTH_LOGON_CHALLENGE");
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
    // Читаем заголовок: [Opcode(2 BE)] [Length(2 BE)]
    auto header = std::make_shared<std::vector<uint8_t>>(4);

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

                // Распаковка заголовка
                uint16_t opcode = (static_cast<uint16_t>((*header)[0]) << 8) | (*header)[1];
                uint16_t length = (static_cast<uint16_t>((*header)[2]) << 8) | (*header)[3];

                if (length > 2048) {
                    log->error("[Client] Payload too large: {}", length);
                    connected = false;
                    schedule_reconnect();
                    return;
                }

                if (length == 0) {
                    // Пустой payload
                    std::vector<uint8_t> raw_packet;
                    raw_packet.reserve(4);
                    raw_packet.insert(raw_packet.end(), header->begin(), header->end());

                    try {
                        AuthPacket packet;
                        packet.deserialize(raw_packet);
                        handle_packet(packet);
                    } catch (const std::exception &ex) {
                        log->error("[Client] Failed to parse empty AuthPacket: {}", ex.what());
                    }

                    start_receive_loop();
                    return;
                }

                // Иначе читаем payload
                auto payload = std::make_shared<std::vector<uint8_t>>(length);

                boost::asio::async_read(
                        socket, boost::asio::buffer(*payload),
                        [this, payload, header](boost::system::error_code ec2, std::size_t) {
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

                            // Формируем полный пакет: [Opcode(2)][Length(2)][Payload]
                            std::vector<uint8_t> raw_packet;
                            raw_packet.reserve(4 + payload->size());
                            raw_packet.insert(raw_packet.end(), header->begin(), header->end());
                            raw_packet.insert(raw_packet.end(), payload->begin(), payload->end());

                            try {
                                AuthPacket packet;
                                packet.deserialize(raw_packet);
                                handle_packet(packet);
                            } catch (const std::exception &ex) {
                                log->error("[Client] Failed to parse AuthPacket: {}", ex.what());
                            }

                            start_receive_loop();
                        });
            });
}

void Client::handle_packet(AuthPacket &p) {
    auto log = Logger::get();

    switch (p.get_opcode()) {
        case AuthOpcodes::SMSG_PONG:
            log->debug("[Client] Received SMSG_PONG");
            break;

        case AuthOpcodes::SMSG_AUTH_LOGON_CHALLENGE: {
            uint32_t someuint32 = p.read_uint32_le();
            log->debug("[Client] Received SMSG_AUTH_LOGON_CHALLENGE with uint32_t={}", someuint32);
            break;
        }

        default:
            log->warn("[Client] Unknown opcode: {}", static_cast<uint16_t>(p.get_opcode()));
            break;
    }
}
