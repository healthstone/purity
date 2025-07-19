#include "Client.hpp"
#include "Logger.hpp"
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
    heartbeat_timer.expires_after(5s);
    heartbeat_timer.async_wait([this](const boost::system::error_code &ec) {
        if (!ec && connected) {
            send_ping();
            start_heartbeat(); // repeat
        }
    });
}

void Client::send_ping() {
    BNETPacket8 ping(BNETOpcode8::SID_PING);
    send_packet(ping);
    Logger::get()->debug("[Client] Sent SID_PING");
}

void Client::send_message(const std::string &msg) {
    BNETPacket8 packet(BNETOpcode8::SID_CHATCOMMAND);
    packet.write_string_nt(msg);
    send_packet(packet);
}

void Client::send_auth() {
//    BNETPacket8 packet(BNETOpcode8::SID_AUTH_INFO);
//    packet.write_string(msg);
//    send_packet(packet);
}

void Client::send_packet(const BNETPacket8 &packet) {
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
    auto header = std::make_shared<std::vector<uint8_t>>(3); // [ID][LEN_BE]

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

                uint8_t id = (*header)[0];
                uint16_t payload_size = (static_cast<uint16_t>((*header)[1]) << 8) | (*header)[2];

                if (payload_size > 2048) {
                    log->error("[Client] Invalid payload size: {} (too large)", payload_size);
                    connected = false;
                    schedule_reconnect();
                    return;
                }

                if (payload_size == 0) {
                    //log->warn("[Client] Received zero-length payload, opcode: 0x{:02X}", id);
                    try {
                        // Всё ещё обрабатываем пустой пакет как валидный
                        BNETOpcode8 opcode = static_cast<BNETOpcode8>(id);
                        BNETPacket8 p = BNETPacket8::deserialize(opcode, {});
                        handle_packet(p);
                    } catch (const std::exception& ex) {
                        log->error("[Client] Failed to parse empty packet: {}", ex.what());
                    }

                    start_receive_loop();
                    return;
                }

                auto payload = std::make_shared<std::vector<uint8_t>>(payload_size);

                boost::asio::async_read(
                        socket, boost::asio::buffer(*payload),
                        [this, payload, id](boost::system::error_code ec2, std::size_t) {
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
                                BNETOpcode8 opcode = static_cast<BNETOpcode8>(id);
                                BNETPacket8 p = BNETPacket8::deserialize(opcode, *payload);
                                handle_packet(p);
                            } catch (const std::exception& ex) {
                                log->error("[Client] Failed to parse packet: {}", ex.what());
                            }

                            // Старт следующего чтения
                            start_receive_loop();
                        }
                );
            }
    );
}


void Client::handle_packet(BNETPacket8 &p) {
    auto log = Logger::get();
    switch (p.get_id()) {
        case BNETOpcode8::SID_PING: {
            log->debug("[Client] Received SID_PING");
            break;
        }
        default:
            log->warn("[Client] Unknown opcode: {}", static_cast<uint8_t>(p.get_id()));
            break;
    }
}
