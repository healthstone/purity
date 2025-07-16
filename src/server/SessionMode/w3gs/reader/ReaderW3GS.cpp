#include "src/server/SessionMode/w3gs/handlers/HandlersW3GS.hpp"
#include "ReaderW3GS.hpp"

using namespace ReaderW3GS;

/**
 * Чтение пакетов от клиента.
 * Структура W3GS: [Opcode_LE][Length_LE][Payload]
 */
void ReaderW3GS::process_read_buffer_as_w3gs(std::shared_ptr<ClientSession> session) {
    auto log = Logger::get();

    if (session->read_buffer().get_active_size() < 4) return;

    uint8_t *data = session->read_buffer().read_ptr();

    uint16_t raw_opcode = data[0] | (data[1] << 8);
    uint16_t length = data[2] | (data[3] << 8);

    if (length == 0) {
        log->error("[client_session][W3GS] Invalid length 0");
        session->close();
        return;
    }

    if (session->read_buffer().get_active_size() < 4 + length) return;

    std::vector<uint8_t> body(data + 4, data + 4 + length);

    session->read_buffer().read_completed(4 + length);
    session->read_buffer().normalize();

    try {
        auto packet = BNETPacket16::deserialize(body, static_cast<BNETOpcode16>(raw_opcode));
        HandlersW3GS::dispatch(session, packet);
    } catch (const std::exception &ex) {
        log->error("[client_session][W3GS] Deserialization failed: {}", ex.what());
    }
}