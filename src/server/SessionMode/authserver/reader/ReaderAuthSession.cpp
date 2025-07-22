#include "ReaderAuthSession.hpp"
#include "src/server/SessionMode/authserver/handlers/HandlersAuth.hpp"
#include "src/server/SessionMode/authserver/opcodes/AuthPacket.hpp"

using namespace ReaderAuthSession;

void ReaderAuthSession::process_read_buffer_as_authserver(std::shared_ptr<ClientSession> session) {
    auto log = Logger::get();
    MessageBuffer& buffer = session->read_buffer();

    // Нужно минимум 4 байта заголовка (cmd(1) + error(1) + size(2))
    if (buffer.get_active_size() < 4)
        return;

    const uint8_t* data = buffer.read_ptr();

    uint8_t cmd = data[0];
    uint8_t error = data[1];
    uint16_t size = static_cast<uint16_t>(data[2]) | (static_cast<uint16_t>(data[3]) << 8);

    // Проверяем, что весь пакет полностью в буфере
    if (buffer.get_active_size() < 4 + size)
        return;

    // Ограничение размера
    if (size > 2048) {
        log->error("Payload too big: {}", size);
        session->close();
        return;
    }

    // Копируем полный пакет (заголовок + payload)
    std::vector<uint8_t> full_packet(data, data + 4 + size);

    // Сдвигаем указатель чтения буфера ровно на размер пакета
    buffer.read_completed(4 + size);

    AuthPacket packet;
    packet.raw().write_bytes(full_packet);
    packet.raw().reset_read();

    try {
        packet.verify_size(size);
        HandlersAuth::dispatch(session, packet);
    } catch (const std::exception& ex) {
        log->error("AuthPacket processing failed: {}", ex.what());
        session->close();
    }

    // Не вызываем process_read_buffer_as_authserver() здесь!
    // Следующий вызов сделает do_read() при следующем событии чтения
}