#include "ReaderBNCS.hpp"
#include "src/server/SessionMode/bncs/handlers/HandlersBNCS.hpp"

using namespace ReaderBNCS;

void ReaderBNCS::process_read_buffer_as_bncs(std::shared_ptr<ClientSession> session) {
    auto log = Logger::get();
    auto& buffer = session->read_buffer();

    // Минимальный размер для проверки - 1 байт (opcode)
    if (buffer.get_active_size() < 1) return;

    const uint8_t* data = buffer.read_ptr();
    const uint8_t opcode = data[0];

    // Специальная обработка Warcraft III клиента
    if (buffer.get_active_size() >= 3 &&
        data[0] == 0x01 &&
        data[1] == 0xFF &&
        data[2] == 0x50) {

        // Полный размер начального пакета W3 (1 + 48 = 49 байт)
        constexpr size_t W3_INIT_PACKET_SIZE = 49;

        if (buffer.get_active_size() < W3_INIT_PACKET_SIZE) return;

        // Обрабатываем как SID_AUTH_INFO (0x50)
        auto body = std::vector<uint8_t>(data + 1, data + W3_INIT_PACKET_SIZE);
        buffer.read_completed(W3_INIT_PACKET_SIZE);
        process_valid_packet(session, BNETOpcode8::SID_AUTH_INFO, body);
        return;
    }

    // Стандартная обработка BNCS пакетов
    if (opcode == 0 || opcode >= 0x80) {
        log->warn("Invalid BNCS opcode: 0x{:02X}", opcode);
        session->close();
        return;
    }

    // Обработка пакетов фиксированного размера
    const auto expected_size = get_packet_expected_size(static_cast<BNETOpcode8>(opcode));
    if (expected_size > 0) {
        const size_t total_size = 1 + expected_size;
        if (buffer.get_active_size() < total_size) return;

        auto body = std::vector<uint8_t>(data + 1, data + total_size);
        buffer.read_completed(total_size);
        process_valid_packet(session, static_cast<BNETOpcode8>(opcode), body);
        return;
    }

    // Обработка пакетов с переменной длиной (стандартные BNCS)
    // [ID][LEN_BE][PAYLOAD]
    if (buffer.get_active_size() < 3) return;

    const uint8_t id = data[0];
    const uint16_t payload_size = (data[1] << 8) | data[2];

    if (payload_size > 2048) {
        log->error("Payload too big: {}", payload_size);
        session->close();
        return;
    }

    if (buffer.get_active_size() < 3 + payload_size) return;

    auto body = std::vector<uint8_t>(data + 3, data + 3 + payload_size);
    buffer.read_completed(3 + payload_size);

    process_valid_packet(session, static_cast<BNETOpcode8>(id), body);
}

void ReaderBNCS::process_valid_packet(std::shared_ptr<ClientSession> session,
                                      BNETOpcode8 opcode,
                                      const std::vector<uint8_t>& body) {
    auto log = Logger::get();
    try {
        auto packet = BNETPacket8::deserialize(opcode, body);
        HandlersBNCS::dispatch(session, packet);
    } catch (const std::exception& ex) {
        log->error("Packet processing failed: {}", ex.what());
        session->close();
    }
}

size_t ReaderBNCS::get_packet_expected_size(BNETOpcode8 opcode) {
    static const std::unordered_map<BNETOpcode8, size_t> fixed_size_packets = {
            // WarCraft 3
            {BNETOpcode8::SID_AUTH_INFO,    48},  // 0x50
            {BNETOpcode8::SID_AUTH_CHECK,   32},  // 0x51
            {BNETOpcode8::SID_W3_LEAVECHAT, 0},   // 0x0A
            {BNETOpcode8::SID_W3_GAMELIST,  4},   // 0x09

            // Diablo 2
            {BNETOpcode8::SID_AUTH_ACCOUNTLOGON,      72},  // 0x53
            {BNETOpcode8::SID_AUTH_ACCOUNTLOGONPROOF, 20}   // 0x54
    };

    auto it = fixed_size_packets.find(opcode);
    return it != fixed_size_packets.end() ? it->second : 0;
}