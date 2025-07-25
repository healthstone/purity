#include "HandlersWork.hpp"

#include <utility>
#include "utils/PacketUtils.hpp"
#include "src/server/SessionMode/workstage/opcodes/WorkPacket.hpp"

using namespace HandlersWork;

void HandlersWork::dispatch(std::shared_ptr<ClientSession> session, WorkPacket &p) {
    WorkOpcodes opcode = p.get_opcode();
    switch (opcode) {
        case WorkOpcodes::CMSG_PING:
            handle_ping(std::move(session), p);
            break;

        case WorkOpcodes::CMSG_MESSAGE:
            handle_message(std::move(session), p);
            break;

        default:
            Logger::get()->warn("[HandlersWork] Unknown opcode: {}", static_cast<uint16_t>(opcode));
            break;
    }
}

void HandlersWork::handle_ping(std::shared_ptr<ClientSession> session, WorkPacket &p) {
    // Считываем ping ID из клиента (4 байта LE)
    uint32_t ping = p.read_uint32_le();

    // Формируем PONG — возвращаем то же самое число
    WorkPacket reply(WorkOpcodes::SMSG_PONG);
    reply.write_uint32_le(ping);

    Logger::get()->trace("[HandlersWork] CMSG_PING");
    // Отправляем обратно клиенту
    PacketUtils::send_packet_as<WorkPacket>(std::move(session), reply);
}

void HandlersWork::handle_message(std::shared_ptr<ClientSession> session, WorkPacket &p) {
    std::string msg = p.read_string_nt_le();
    Logger::get()->info("[HandlersWork] CMSG_MESSAGE: {}", msg);
}