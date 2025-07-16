#include "ReaderBNCS.hpp"
#include "src/server/SessionMode/bncs/handlers/HandlersBNCS.hpp"

using namespace ReaderBNCS;

/**
 * Чтение пакетов от клиента.
 * Структура BNCS: [ID][Length_LE][Payload]
 */
void ReaderBNCS::process_read_buffer_as_bncs(std::shared_ptr<ClientSession> session) {
    auto log = Logger::get();
    if (session->read_buffer().get_active_size() < 3) return;

    uint8_t *data = session->read_buffer().read_ptr();

    uint8_t id = data[0];
    uint16_t length = data[1] | (data[2] << 8);

    if (length < 3) {
        log->error("[client_session][BNCS] Invalid length < 3: {}", length);
        session->close();
        return;
    }

    if (session->read_buffer().get_active_size() < length) return;

    std::vector<uint8_t> body(data + 3, data + length);

    session->read_buffer().read_completed(length);
    session->read_buffer().normalize();

    try {
        auto packet = BNETPacket8::deserialize(body, static_cast<BNETOpcode8>(id));
        HandlersBNCS::dispatch(session, packet);
    } catch (const std::exception &ex) {
        log->error("[ReaderBNCS] Deserialization failed: {}", ex.what());
    }
}