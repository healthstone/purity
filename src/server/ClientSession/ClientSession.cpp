#include "ClientSession.hpp"
#include "src/server/handlers/Handlers.hpp"
#include "Logger.hpp"
#include <iostream>

using boost::asio::ip::tcp;

ClientSession::ClientSession(tcp::socket socket, std::shared_ptr<Server> server)
        : socket_(std::move(socket)), server_(std::move(server)), read_buffer_(4096) {}

void ClientSession::start() {
    do_read();
}

void ClientSession::close() {
    if (closed_.exchange(true)) {
        return; // Уже закрыто
    }

    boost::system::error_code ec;

    if (socket_.is_open()) {
        socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
        socket_.close(ec);
    }

    write_queue_.clear();

    if (server_) {
        server_->remove_session(shared_from_this());
    }

    Logger::get()->info("[client_session] Closed");
}

void ClientSession::do_read() {
    auto self = shared_from_this();

    read_buffer_.ensure_free_space(512);

    socket_.async_read_some(
            boost::asio::buffer(read_buffer_.write_ptr(), read_buffer_.get_remaining_space()),
            [this, self](boost::system::error_code ec, std::size_t bytes_transferred) {
                auto log = Logger::get();

                if (ec) {
                    if (ec == boost::asio::error::operation_aborted || ec == boost::asio::error::eof) {
                        log->info("[client_session] Client disconnected");
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
    auto log = Logger::get();

    while (true) {
        // Нужно хотя бы 4 байта: 2 size + 2 opcode
        if (read_buffer_.get_active_size() < 4)
            return;

        uint8_t* data = read_buffer_.read_ptr();
        uint16_t total_size = (data[0] << 8) | data[1];
        uint16_t raw_opcode = (data[2] << 8) | data[3];

        Opcode opcode = static_cast<Opcode>(raw_opcode);

        size_t body_size = total_size - 2;

        if (read_buffer_.get_active_size() < 4 + body_size)
            return; // Пакет ещё не полный

        std::vector<uint8_t> body(data + 4, data + 4 + body_size);

        read_buffer_.read_completed(4 + body_size);
        read_buffer_.normalize();

        try {
            Packet packet = Packet::deserialize(body, opcode);
            Handlers::dispatch(shared_from_this(), packet);
        } catch (const std::exception& ex) {
            log->error("[client_session] Packet deserialization failed: {}", ex.what());
        }
    }
}

// ✅ Безопасная версия send_packet для вызова из любых потоков и корутин
void ClientSession::send_packet(const Packet& packet) {
    auto self = shared_from_this();
    boost::asio::post(
            socket_.get_executor(),
            [self, packet]() {
                self->do_send_packet(packet);
            }
    );
}

/** структура пакета (в big-endian): [2 size][2 opcode][payload] — приватный метод **/
void ClientSession::do_send_packet(const Packet& packet) {
    const auto& body = packet.serialize();

    ByteBuffer header;
    header.write_uint16(static_cast<uint16_t>(body.size() + 2)); // size = body + opcode
    header.write_uint16(static_cast<uint16_t>(packet.get_opcode())); // cast из enum class!

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
