#include "ReaderAuthSession.hpp"
#include "src/server/SessionMode/authstage/handlers/HandlersAuth.hpp"
#include "src/server/SessionMode/authstage/opcodes/AuthPacket.hpp"

using namespace ReaderAuthSession;

void ReaderAuthSession::process_read_buffer_as_authserver(std::shared_ptr<ClientSession> session) {
    auto log = Logger::get();
    MessageBuffer &buffer = session->read_buffer();

    // Нужно минимум 4 байта заголовка (opcode(2) + length(2))
    if (buffer.get_active_size() < 4)
        return;

    const uint8_t *data = buffer.read_ptr();

    // Читаем opcode (Big Endian)
    uint16_t opcode = static_cast<uint16_t>(data[0]) << 8 | static_cast<uint16_t>(data[1]);
    // Читаем length (Big Endian)
    uint16_t size = static_cast<uint16_t>(data[2]) << 8 | static_cast<uint16_t>(data[3]);

    // Проверяем, что весь пакет полностью в буфере
    if (buffer.get_active_size() < 4 + size)
        return;

    // Ограничение размера payload
    if (size > 2048) {
        log->error("AuthPacket payload too big: {}", size);
        session->close();
        return;
    }

    // Копируем весь пакет [opcode][length][payload]
    std::vector<uint8_t> full_packet(data, data + 4 + size);

    // Сдвигаем read_ptr
    buffer.read_completed(4 + size);

    AuthPacket packet;

    try {
        // Парсим [opcode][length][payload]
        packet.deserialize(full_packet);

        // Лог (по желанию)
        Packet::log_raw_payload(fmt::format("{:04X}", static_cast<uint16_t>(packet.get_opcode())), full_packet);

        // Обработка
        HandlersAuth::dispatch(session, packet);

    } catch (const std::exception &ex) {
        log->error("[ReaderAuthSession] AuthPacket processing failed: {}", ex.what());
        session->close();
    }

    // Следующий вызов обработает do_read
}
