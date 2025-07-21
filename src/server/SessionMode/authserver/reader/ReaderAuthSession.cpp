#include "ReaderAuthSession.hpp"
#include "src/server/SessionMode/authserver/opcodes/AuthPacket.hpp"
#include "src/server/SessionMode/authserver/handlers/HandlersAuth.hpp"

using namespace ReaderAuthSession;

void ReaderAuthSession::process_read_buffer_as_authserver(std::shared_ptr<ClientSession> session) {
    auto log = Logger::get();
    auto& buffer = session->read_buffer();

    // Минимальный размер для проверки: 2 байта длины + 1 байт opcode
    if (buffer.get_active_size() < 3) return;

    const uint8_t* data = buffer.read_ptr();

    // Считаем длину пакета (Length LE - 2 байта)
    uint16_t packet_length = data[0] | (data[1] << 8);

    // Проверяем, что полный пакет в буфере
    if (buffer.get_active_size() < 2 + packet_length) return;

    // Opcode — следующий байт после длины (big-endian, 1 байт)
    uint8_t opcode = data[2];

    // Проверка ограничения размера (например, 2048)
    if (packet_length - 1 > 2048) {
        log->error("Payload too big: {}", packet_length - 1);
        session->close();
        return;
    }

    // Payload начинается после 3-го байта
    size_t payload_size = packet_length - 1;
    auto body = std::vector<uint8_t>(data + 3, data + 3 + payload_size);

    // Помечаем прочитанными все байты пакета (длина + opcode + payload)
    buffer.read_completed(2 + packet_length);

    // Обрабатываем валидный пакет с использованием AuthPacket
    process_valid_packet(session, static_cast<AuthOpcode>(opcode), body);
}

void ReaderAuthSession::process_valid_packet(std::shared_ptr<ClientSession> session,
                                             AuthOpcode opcode,
                                             const std::vector<uint8_t>& body) {
    auto log = Logger::get();
    try {
        // Создаем пакет с заданным opcode
        AuthPacket packet(opcode);

        // Копируем body в буфер пакета
        packet.raw().write_bytes(body);

        // Вызываем обработчик
        HandlersAuth::dispatch(session, packet);

    } catch (const std::exception& ex) {
        log->error("AuthPacket processing failed: {}", ex.what());
        session->close();
    }
}