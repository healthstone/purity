#include "ReaderBNCS.hpp"
#include "src/server/SessionMode/bncs/handlers/HandlersBNCS.hpp"

using namespace ReaderBNCS;

/**
 * Чтение пакетов от клиента.
 * Структура BNCS: [ID][Length_LE][Payload]
 *
 * ЛИБО пакеты [ID][Payload]
 */
void ReaderBNCS::process_read_buffer_as_bncs(std::shared_ptr<ClientSession> session) {
    auto log = Logger::get();
    auto& buffer = session->read_buffer();

    // Чтение с явным указанием Little-Endian
    auto read_le16 = [](const uint8_t* data) {
        return static_cast<uint16_t>(data[0]) | (static_cast<uint16_t>(data[1]) << 8);
    };

    // Минимальный размер - 1 байт (opcode)
    if (buffer.get_active_size() < 1) return;

    const uint8_t* data = buffer.read_ptr();
    const uint8_t id_raw = data[0];

    // Жёсткая проверка opcode (BNCS использует только 0x01-0x7F)
    if (id_raw == 0 || id_raw >= 0x80) {
        log->warn("Invalid BNCS opcode: 0x{:02X}", id_raw);
        session->close();
        return;
    }

    // Специальная обработка SID_AUTH_INFO (0x50)
    if (id_raw == 0x50) {
        constexpr size_t W3_AUTH_INFO_SIZE = 49; // 1 + 48
        if (buffer.get_active_size() < W3_AUTH_INFO_SIZE) return;

        // Проверка сигнатуры War3 (первые 4 байта: 00 00 00 00)
        if (std::memcmp(data + 1, "\0\0\0\0", 4) != 0) {
            log->warn("Invalid W3 auth packet structure");
            session->close();
            return;
        }

        auto body = std::vector<uint8_t>(data + 1, data + W3_AUTH_INFO_SIZE);
        buffer.read_completed(W3_AUTH_INFO_SIZE);
        process_valid_packet(session, static_cast<BNETOpcode8>(id_raw), body);
        return;
    }

    // Стандартные BNCS пакеты (требуют 3 байта минимум)
    if (buffer.get_active_size() < 3) return;

    // Явное чтение длины в Little-Endian
    const uint16_t length = read_le16(data + 1);

    // Жёсткие ограничения длины (3-2048 байт)
    if (length < 3 || length > 2048) {
        log->error("Invalid packet length: {} (header: {:02X} {:02X} {:02X})",
                   length, data[0], data[1], data[2]);
        session->close();
        return;
    }

    if (buffer.get_active_size() < length) return;

    // Дополнительные проверки для специфичных пакетов
    if (id_raw == 0x01 && length != 8) { // SID_STOPADV должен быть 8 байт
        log->warn("Invalid SID_STOPADV packet");
        session->close();
        return;
    }

    // Извлечение тела пакета
    auto body = std::vector<uint8_t>(data + 3, data + length);
    buffer.read_completed(length);
    process_valid_packet(session, static_cast<BNETOpcode8>(id_raw), body);
}

void ReaderBNCS::process_valid_packet(std::shared_ptr<ClientSession> session,
                                      BNETOpcode8 id,
                                      const std::vector<uint8_t>& body) {
    auto log = Logger::get();
    try {
        auto packet = BNETPacket8::deserialize(body, id);
        HandlersBNCS::dispatch(session, packet);
    } catch (const std::exception& ex) {
        log->error("Packet processing failed: {}", ex.what());
        session->close();
    }
}

size_t ReaderBNCS::get_packet_expected_size(BNETOpcode8 id) {
    switch (id) {
        // WarCraft 3
        case BNETOpcode8::SID_AUTH_INFO:    return 48;
        case BNETOpcode8::SID_AUTH_CHECK:   return 32;
        case BNETOpcode8::SID_W3_LEAVECHAT: return 0;
        case BNETOpcode8::SID_W3_GAMELIST:  return 4;

            // Diablo 2
        case BNETOpcode8::SID_AUTH_ACCOUNTLOGON:      return 72;
        case BNETOpcode8::SID_AUTH_ACCOUNTLOGONPROOF: return 20;

            // Прочие (стандартная обработка)
        default: return 0;
    }
}