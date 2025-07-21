#include "HandlersAuth.hpp"

#include <utility>
#include "packet/PacketUtils.hpp"

using namespace HandlersAuth;

void HandlersAuth::dispatch(std::shared_ptr<ClientSession> session, AuthPacket &p) {
    AuthOpcode opcode = p.get_id();
    auto log = Logger::get();

    switch (opcode) {
        case AuthOpcode::AUTH_CMSG_PING:
            handle_ping(std::move(session), p);
            break;

        case AuthOpcode::AUTH_CMSG_LOGON_CHALLENGE:
            log->info("[AUTH_CMSG_LOGON_CHALLENGE]");
            break;

        case AuthOpcode::AUTH_CMSG_LOGON_PROOF:
            log->info("[AUTH_CMSG_LOGON_PROOF]");
            break;

        case AuthOpcode::AUTH_CMSG_RECONNECT_CHALLENGE:
            log->info("[AUTH_CMSG_RECONNECT_CHALLENGE]");
            break;

        case AuthOpcode::AUTH_CMSG_RECONNECT_PROOF:
            log->info("[AUTH_CMSG_RECONNECT_PROOF]");
            break;

        case AuthOpcode::AUTH_CMSG_REALM_LIST:
            log->info("[AUTH_CMSG_REALM_LIST]");
            break;

        default:
            Logger::get()->warn("[handler] Unknown opcode: {}", static_cast<uint8_t>(opcode));
            break;
    }
}

void HandlersAuth::handle_ping(std::shared_ptr<ClientSession> session, AuthPacket &p) {
    Logger::get()->debug("[handler] AUTH_CMSG_PING");
    AuthPacket reply(AuthOpcode::AUTH_SMSG_PONG);
    PacketUtils::send_packet_as<AuthPacket>(std::move(session), reply);
}