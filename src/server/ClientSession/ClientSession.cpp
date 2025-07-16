#include "ClientSession.hpp"
#include "src/server/handlers/bncs/HandlersBNCS.hpp"
#include "src/server/handlers/w3gs/HandlersW3GS.hpp"
#include "Logger.hpp"
#include <iostream>

using boost::asio::ip::tcp;

ClientSession::ClientSession(tcp::socket socket, std::shared_ptr<Server> server)
        : socket_(std::move(socket)), server_(std::move(server)), read_buffer_(4096) {}

void ClientSession::start() {
    auto ep = socket_.remote_endpoint();
    Logger::get()->debug("[client_session][start] New connection from {}:{}",
                         ep.address().to_string(), ep.port());

    set_session_mode(SessionMode::BNCS);  // Начинаем с BNCS
    do_read();
}

void ClientSession::close() {
    if (closed_.exchange(true)) return;

    auto log = Logger::get();

    boost::system::error_code ec;
    socket_.cancel(ec);
    if (ec && ec != boost::asio::error::operation_aborted && ec != boost::asio::error::eof) {
        log->error("[client_session][close] Failed to cancel socket: {}", ec.message());
    }

    socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);

    socket_.close(ec);
    if (ec && ec != boost::asio::error::operation_aborted && ec != boost::asio::error::eof) {
        log->error("[client_session][close] Failed to close socket: {}", ec.message());
    }

    write_queue_.clear();

    Logger::get()->debug("[client_session][close] closing socket: is_open={} closed_={}", socket_.is_open(), closed_.load());

    if (server_) {
        server_->remove_session(shared_from_this());
    }
}

void ClientSession::do_read() {
    auto self = shared_from_this();

    read_buffer_.ensure_free_space(512);

    socket_.async_read_some(
            boost::asio::buffer(read_buffer_.write_ptr(), read_buffer_.get_remaining_space()),
            [this, self](boost::system::error_code ec, std::size_t bytes_transferred) {
                auto log = Logger::get();

                if (ec) {
                    if (ec == boost::asio::error::operation_aborted ||
                        ec == boost::asio::error::eof ||
                        ec == boost::asio::error::connection_reset) {
                        log->debug("[client_session][do_read] Client disconnected: {}", ec.message());
                    } else {
                        log->error("[client_session] Read error: {}", ec.message());
                    }
                    close();
                    return;
                }

                read_buffer_.write_completed(bytes_transferred);
                process_read_buffer();

                if (!closed_) {
                    do_read();
                }
            }
    );
}

void ClientSession::process_read_buffer() {
    switch (session_mode_) {
        case SessionMode::BNCS:
            process_read_buffer_as_bncs();
            break;
        case SessionMode::W3ROUTE:
            process_read_buffer_as_w3gs();
            break;
        default:
            break;
    }
}

void ClientSession::process_read_buffer_as_bncs() {
    auto log = Logger::get();

    while (true) {
        // Нужно хотя бы 3 байта: [ID][Length_LE]
        if (read_buffer_.get_active_size() < 3)
            return;

        uint8_t *data = read_buffer_.read_ptr();

        // [0]: ID
        uint8_t id = data[0];

        // [1,2]: Length_LE
        uint16_t length = data[1] | (data[2] << 8);

        if (length < 3) {
            log->error("[client_session] [process_read_buffer_as_bncs] Invalid length < 3: {}", length);
            close();
            return;
        }

        size_t body_size = length - 3;

        if (read_buffer_.get_active_size() < length)
            return; // Пакет ещё не полный

        std::vector<uint8_t> body(data + 3, data + 3 + body_size);

        read_buffer_.read_completed(length);
        read_buffer_.normalize();

        try {
            BNETPacket8 packet = BNETPacket8::deserialize(body, static_cast<BNETOpcode8>(id));
            HandlersBNCS::dispatch(shared_from_this(), packet);
        } catch (const std::exception &ex) {
            log->error("[client_session] [process_read_buffer_as_bncs] Packet deserialization failed: {}", ex.what());
        }
    }
}

void ClientSession::process_read_buffer_as_w3gs() {
    auto log = Logger::get();

    while (true) {
        // Нужно хотя бы 4 байта: [Opcode_LE][Length_LE]
        if (read_buffer_.get_active_size() < 4)
            return;

        uint8_t *data = read_buffer_.read_ptr();

        // [0,1]: Opcode_LE
        uint16_t raw_opcode = data[0] | (data[1] << 8);

        // [2,3]: Length_LE
        uint16_t length = data[2] | (data[3] << 8);

        if (length == 0) {
            log->error("[client_session] [process_read_buffer_as_w3gs] Invalid length 0");
            close();
            return;
        }

        size_t body_size = length;

        if (read_buffer_.get_active_size() < 4 + body_size)
            return; // Пакет ещё не полный

        std::vector<uint8_t> body(data + 4, data + 4 + body_size);

        read_buffer_.read_completed(4 + body_size);
        read_buffer_.normalize();

        try {
            BNETPacket16 packet = BNETPacket16::deserialize(body, static_cast<BNETOpcode16>(raw_opcode));
            HandlersW3GS::dispatch(shared_from_this(), packet);
        } catch (const std::exception &ex) {
            log->error("[client_session] [process_read_buffer_as_w3gs] Packet deserialization failed: {}", ex.what());
        }
    }
}

// --- Отправка BNCS ---
void ClientSession::send_packet(const BNETPacket8 &packet) {
    auto self = shared_from_this();
    boost::asio::post(
            socket_.get_executor(),
            [self, packet]() {
                self->do_send_packet(packet);
            });
}

/**
 * Отправка пакета клиенту.
 * Структура BNCS: [ID][Length_LE][Payload]
 */
void ClientSession::do_send_packet(const BNETPacket8 &packet) {
    const auto &body = packet.serialize();
    ByteBuffer header;

    uint8_t id = static_cast<uint8_t>(packet.get_id());
    uint16_t length_le = htole16(static_cast<uint16_t>(body.size() + 3));

    header.write_uint8(id);
    header.write_uint16(length_le);

    std::vector<uint8_t> full_packet = header.data();
    full_packet.insert(full_packet.end(), body.begin(), body.end());

    write_queue_.push_back(std::move(full_packet));

    if (!writing_) {
        do_write();
    }
}

// --- Отправка W3ROUTE ---
void ClientSession::send_packet(const BNETPacket16 &packet) {
    auto self = shared_from_this();
    boost::asio::post(
            socket_.get_executor(),
            [self, packet]() {
                self->do_send_packet(packet);
            });
}

/**
 * Отправка пакета клиенту.
 * Структура W3GS: [Opcode_LE][Length_LE][Payload]
 */
void ClientSession::do_send_packet(const BNETPacket16 &packet) {
    const auto &body = packet.serialize();

    // --- Сборка заголовка ---
    ByteBuffer header;

    uint16_t opcode_le = htole16(static_cast<uint16_t>(packet.get_opcode()));
    uint16_t length_le = htole16(static_cast<uint16_t>(body.size()));

    header.write_uint16(opcode_le);
    header.write_uint16(length_le);

    std::vector<uint8_t> full_packet = header.data();
    full_packet.insert(full_packet.end(), body.begin(), body.end());

    write_queue_.push_back(std::move(full_packet));

    if (!writing_) {
        do_write();
    }
}

void ClientSession::do_write() {
    if (write_queue_.empty()) {
        writing_ = false;
        return;
    }

    writing_ = true;
    auto self = shared_from_this();

    boost::asio::async_write(
            socket_,
            boost::asio::buffer(write_queue_.front()),
            [this, self](boost::system::error_code ec, std::size_t) {
                auto log = Logger::get();

                if (ec) {
                    if (ec != boost::asio::error::operation_aborted && ec != boost::asio::error::eof) {
                        log->error("[client_session] Write failed: {}", ec.message());
                    }
                    close();
                    return;
                }

                write_queue_.pop_front();
                do_write();
            }
    );
}