#include "Client.hpp"
#include "Logger.hpp"
#include "utils/generators/GeneratorUtils.hpp"
#include "utils/HexUtils.hpp"
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
        Logger::get()->info("[Client] Disconnected from server");
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
            start_heartbeat();
        }
    });
}

void Client::send_ping() {
    uint32_t ping_id = GeneratorUtils::random_uint32();

    if (get_session_mode() == SessionMode::AUTH_SESSION) {
        AuthPacket ping(AuthOpcodes::CMSG_PING);
        ping.write_uint32_le(ping_id);
        send_packet(ping);
    } else {
        WorkPacket ping(WorkOpcodes::CMSG_PING);
        ping.write_uint32_le(ping_id);
        send_packet(ping);
    }

    Logger::get()->trace("[Client] Sent CMSG_PING");
}

void Client::send_message(const std::string &msg) {
    if (get_session_mode() == SessionMode::AUTH_SESSION)
        return;
    WorkPacket packet(WorkOpcodes::CMSG_MESSAGE);
    packet.write_string_nt_le(msg);
    send_packet(packet);
}

void Client::handle_logon_challenge(const std::string &username, const std::string &password) {
    srp_->set_credentials(username, password);

    AuthPacket packet(AuthOpcodes::CMSG_AUTH_LOGON_CHALLENGE);
    packet.write_string_nt_le(username);
    send_packet(packet);
    Logger::get()->debug("[AuthPacket] Sent CMSG_AUTH_LOGON_CHALLENGE for user: {}", username);
}

void Client::send_packet(const AuthPacket &packet) {
    send_packet_impl(packet.build_packet());
}

void Client::send_packet(const WorkPacket &packet) {
    send_packet_impl(packet.build_packet());
}

void Client::send_packet_impl(const std::vector<uint8_t> &full_packet) {
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
    auto log = Logger::get();

    if (get_session_mode() == SessionMode::AUTH_SESSION) {
        auto header = std::make_shared<std::vector<uint8_t>>(3);

        boost::asio::async_read(socket, boost::asio::buffer(*header),
                                [this, header](boost::system::error_code ec, std::size_t) {
                                    auto log = Logger::get();
                                    if (ec) {
                                        if (ec == boost::asio::error::operation_aborted ||
                                            ec == boost::asio::error::eof) {
                                            log->info("[Client] Disconnected while reading auth header");
                                        } else {
                                            log->error("[Client] Auth header read failed: {}", ec.message());
                                        }
                                        connected = false;
                                        schedule_reconnect();
                                        return;
                                    }

                                    uint16_t length = (static_cast<uint16_t>((*header)[1]) << 8) | (*header)[2];
                                    if (length > 2048) {
                                        log->error("[Client] Auth payload too large: {}", length);
                                        connected = false;
                                        schedule_reconnect();
                                        return;
                                    }

                                    if (length == 0) {
                                        process_empty<AuthPacket>(header);
                                    } else {
                                        read_payload<AuthPacket>(header, length);
                                    }
                                });

    } else {
        auto header = std::make_shared<std::vector<uint8_t>>(4);

        boost::asio::async_read(socket, boost::asio::buffer(*header),
                                [this, header](boost::system::error_code ec, std::size_t) {
                                    auto log = Logger::get();
                                    if (ec) {
                                        log->error("[Client] Work header read failed: {}", ec.message());
                                        connected = false;
                                        schedule_reconnect();
                                        return;
                                    }

                                    uint16_t length = (static_cast<uint16_t>((*header)[2]) << 8) | (*header)[3];
                                    if (length > 2048) {
                                        log->error("[Client] Work payload too large: {}", length);
                                        connected = false;
                                        schedule_reconnect();
                                        return;
                                    }

                                    if (length == 0) {
                                        process_empty<WorkPacket>(header);
                                    } else {
                                        read_payload<WorkPacket>(header, length);
                                    }
                                });
    }
}

template<typename PacketT>
void Client::read_payload(std::shared_ptr<std::vector<uint8_t>> header, uint16_t length) {
    auto payload = std::make_shared<std::vector<uint8_t>>(length);

    boost::asio::async_read(socket, boost::asio::buffer(*payload),
                            [this, header, payload](boost::system::error_code ec, std::size_t) {
                                auto log = Logger::get();
                                if (ec) {
                                    if (ec == boost::asio::error::operation_aborted || ec == boost::asio::error::eof) {
                                        log->info("[Client] Disconnected while reading payload");
                                    } else {
                                        log->error("[Client] Payload read failed: {}", ec.message());
                                    }
                                    connected = false;
                                    schedule_reconnect();
                                    return;
                                }

                                std::vector<uint8_t> raw_packet;
                                raw_packet.reserve(header->size() + payload->size());
                                raw_packet.insert(raw_packet.end(), header->begin(), header->end());
                                raw_packet.insert(raw_packet.end(), payload->begin(), payload->end());

                                try {
                                    PacketT packet;
                                    packet.deserialize(raw_packet);
                                    handle_packet(packet);
                                } catch (const std::exception &ex) {
                                    log->error("[Client] Failed to parse packet: {}", ex.what());
                                }

                                start_receive_loop();
                            });
}

template<typename PacketT>
void Client::process_empty(std::shared_ptr<std::vector<uint8_t>> header) {
    auto log = Logger::get();
    std::vector<uint8_t> raw_packet(header->begin(), header->end());
    try {
        PacketT packet;
        packet.deserialize(raw_packet);
        handle_packet(packet);
    } catch (const std::exception &ex) {
        log->error("[Client] Failed to parse empty packet: {}", ex.what());
    }
    start_receive_loop();
}

void Client::handle_packet(AuthPacket &p) {
    auto log = Logger::get();

    switch (p.get_opcode()) {
        case AuthOpcodes::SMSG_PONG:
            log->debug("[AuthPacket] Received SMSG_PONG");
            break;

        case AuthOpcodes::SMSG_AUTH_LOGON_CHALLENGE: {
            try {
                // 1) Считываем B (32 байта)
                std::vector<uint8_t> B = p.read_bytes(32);
                // 2) Считываем g (1 байт)
                uint8_t g = p.read_uint8();
                // 3) Считываем N (32 байта)
                std::vector<uint8_t> N = p.read_bytes(32);
                // 4) Считываем salt (32 байта)
                std::vector<uint8_t> salt = p.read_bytes(32);

                log->debug("[AuthPacket] SMSG_AUTH_LOGON_CHALLENGE: received"
                           " B.size={} g={} N.size={} salt.size={}",
                           B.size(), g, N.size(), salt.size());

                // 5) Инициализируем SRP клиент
                srp_->load_constants(N, g);
                srp_->load_salt(salt);
                srp_->generate_client_ephemeral();

                // 6) Вычисляем A и M1
                auto raw_A = srp_->get_A_bytes();
                auto padded_A = HexUtils::pad_bytes_left(raw_A, 32);
                auto raw_M1 = srp_->compute_M1(B);
                auto padded_M1 = HexUtils::pad_bytes_left(raw_M1, 20);

                // 7) Формируем и отправляем CMSG_AUTH_LOGON_PROOF
                AuthPacket proof(AuthOpcodes::CMSG_AUTH_LOGON_PROOF);
                proof.write_bytes(padded_A);   // 32 байта A
                proof.write_bytes(padded_M1);  // 20 байт SHA1(M1)
                send_packet(proof);
                log->debug("[AuthPacket] SMSG_AUTH_LOGON_CHALLENGE: sent CMSG_AUTH_LOGON_PROOF: A={} M1={}",
                           padded_A.size(), padded_M1.size());
            }
            catch (const std::exception& ex) {
                log->error("[AuthPacket] SMSG_AUTH_LOGON_CHALLENGE failed: {}", ex.what());
                disconnect();
            }
            break;
        }

        case AuthOpcodes::SMSG_AUTH_LOGON_PROOF: {
            std::vector<uint8_t> M2_server = p.read_bytes(20);
            if (!srp_->verify_server_proof(srp_->get_last_M1(), M2_server)) {
                log->error("[AuthPacket] SMSG_AUTH_LOGON_PROOF: SRP Server proof M2 invalid — disconnect");
                disconnect();
                break;
            }

            set_session_mode(SessionMode::WORK_SESSION);
            log->debug("[AuthPacket] SMSG_AUTH_LOGON_PROOF: SRP Server proof M2 verified — authentication successful");
            // Продолжаем работу — сессия аутентифицирована
            break;
        }

        case AuthOpcodes::SMSG_AUTH_RESPONSE: {
            uint8_t authcode = p.read_uint8();
            uint8_t errorcode = p.read_uint8();
            log->debug("[AuthPacket] SMSG_AUTH_RESPONSE: authcode={} errorcode={}", authcode, errorcode);
            disconnect();
            break;
        }

        default:
            log->warn("[AuthPacket] Unknown AuthOpcode: {}", static_cast<uint8_t>(p.get_opcode()));
            break;
    }
}

void Client::handle_packet(WorkPacket &p) {
    auto log = Logger::get();

    switch (p.get_opcode()) {
        case WorkOpcodes::SMSG_PONG:
            log->trace("[WorkPacket] Received SMSG_PONG");
            break;

        case WorkOpcodes::SMSG_MESSAGE:
            log->debug("[WorkPacket] Received SMSG_MESSAGE: {}", p.read_string_nt_le());
            break;

        default:
            log->warn("[WorkPacket] Unknown WorkOpcode: {}", static_cast<uint16_t>(p.get_opcode()));
            break;
    }
}
