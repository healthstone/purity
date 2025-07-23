#include "HandlersAuth.hpp"

#include <utility>
#include "packet/PacketUtils.hpp"
#include "src/server/SessionMode/authstage/opcodes/AuthPacket.hpp"
#include "generators/GeneratorUtils.hpp"

using namespace HandlersAuth;

void HandlersAuth::dispatch(std::shared_ptr<ClientSession> session, AuthPacket &p) {
    AuthOpcodes opcode = p.get_opcode();
    auto log = Logger::get();

    switch (opcode) {
        case AuthOpcodes::CMSG_PING:
            handle_ping(std::move(session), p);
            break;

        case AuthOpcodes::CMSG_AUTH_LOGON_CHALLENGE:
            boost::asio::co_spawn(
                    session->socket().get_executor(),
                    handle_logon_challenge(session, p),
                    boost::asio::detached
            );
            break;

        case AuthOpcodes::CMSG_AUTH_LOGON_PROOF:
            handle_logon_proof(std::move(session), p);
            break;

        default:
            Logger::get()->warn("[handler] Unknown opcode: {}", static_cast<uint8_t>(opcode));
            break;
    }
}

void HandlersAuth::handle_ping(std::shared_ptr<ClientSession> session, AuthPacket &p) {
    // Считываем ping ID из клиента (4 байта LE)
    uint32_t ping = p.read_uint32_le();
    Logger::get()->debug("[HandlersAuth] CMSG_PING with ping: {}", ping);

    // Формируем PONG — возвращаем то же самое число
    AuthPacket reply(AuthOpcodes::SMSG_PONG);
    reply.write_uint32_le(ping);

    // Отправляем обратно клиенту
    PacketUtils::send_packet_as<AuthPacket>(std::move(session), reply);
}

boost::asio::awaitable<void>
HandlersAuth::handle_logon_challenge(std::shared_ptr<ClientSession> session, AuthPacket &p) {
    auto log = Logger::get();
    std::string username = p.read_string_nt_le();
    log->info("[handler] CMSG_AUTH_LOGON_CHALLENGE with user {}", username);

    AuthPacket reply(AuthOpcodes::SMSG_AUTH_LOGON_CHALLENGE);
    uint32_t randomUint32 = GeneratorUtils::random_uint32();
    reply.write_uint32_le(randomUint32);
    PacketUtils::send_packet_as<AuthPacket>(std::move(session), reply);
    co_return;
}

void HandlersAuth::handle_logon_proof(std::shared_ptr<ClientSession> session, AuthPacket &p) {
    auto log = Logger::get();
    log->info("[handler] CMSG_AUTH_LOGON_PROOF");
}