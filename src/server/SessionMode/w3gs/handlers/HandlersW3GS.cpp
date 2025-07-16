#include "HandlersW3GS.hpp"
#include "Logger.hpp"

#include "packet/PacketUtils.hpp"

using namespace HandlersW3GS;

void HandlersW3GS::dispatch(std::shared_ptr<ClientSession> session, BNETPacket16 &p) {
    BNETOpcode16 opcode = p.get_opcode();

    switch (opcode) {
        default:
            Logger::get()->warn("[handler] Unknown opcode: {}", static_cast<uint16_t>(opcode));
            break;
    }
}