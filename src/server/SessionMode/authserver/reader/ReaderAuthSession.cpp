#include "ReaderAuthSession.hpp"
#include "src/server/SessionMode/authserver/opcodes/AuthPacket.hpp"
#include "src/server/SessionMode/authserver/handlers/HandlersAuth.hpp"

using namespace ReaderAuthSession;

void ReaderAuthSession::process_read_buffer_as_authserver(std::shared_ptr<ClientSession> session) {
    auto log = Logger::get();
    MessageBuffer& buffer = session->read_buffer();

    // Проверяем, что в буфере минимум 4 байта заголовка (cmd(1) + error(1) + size(2))
    if (buffer.get_active_size() < 4)
        return;

    const uint8_t* data = buffer.read_ptr();

    // Читаем заголовок (cmd, error, size LE)
    uint8_t cmd = data[0];
    uint8_t error = data[1];
    uint16_t size = static_cast<uint16_t>(data[2]) | (static_cast<uint16_t>(data[3]) << 8);

    // Проверяем, что весь пакет целиком есть в буфере (payload длиной size байт)
    if (buffer.get_active_size() < 4 + size)
        return;

    // Проверка ограничения размера payload
    if (size > 2048) {
        log->error("Payload too big: {}", size);
        session->close();
        return;
    }

    // Копируем payload (без заголовка)
    std::vector<uint8_t> payload(data + 4, data + 4 + size);

    // Отмечаем, что прочитано 4 + size байт
    buffer.read_completed(4 + size);

    AuthPacket packet;
    packet.raw().write_bytes(payload);   // пишем только payload без заголовка

    try {
        packet.set_header(static_cast<AuthOpcode>(cmd), error, size);
        packet.raw().reset_read();
        HandlersAuth::dispatch(session, packet);
    } catch (const std::exception& ex) {
        log->error("AuthPacket processing failed: {}", ex.what());
        session->close();
    }
}