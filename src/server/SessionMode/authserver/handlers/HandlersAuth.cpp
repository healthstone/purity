#include "HandlersAuth.hpp"

#include <utility>
#include "packet/PacketUtils.hpp"
#include "utf8utils/UTF8Utils.hpp"

using namespace HandlersAuth;

void HandlersAuth::dispatch(std::shared_ptr<ClientSession> session, AuthPacket &p) {
    AuthOpcode opcode = p.get_id();
    auto log = Logger::get();

    switch (opcode) {
        case AuthOpcode::AUTH_CMSG_PING:
            handle_ping(std::move(session), p);
            break;

        case AuthOpcode::AUTH_CMSG_LOGON_CHALLENGE:
            boost::asio::co_spawn(
                    session->socket().get_executor(),
                    handle_logon_challenge(session, p),
                    boost::asio::detached
            );
            break;

//        case AuthOpcode::AUTH_CMSG_LOGON_PROOF:
//            handle_logon_proof(std::move(session), p);
//            break;
//
//        case AuthOpcode::AUTH_CMSG_RECONNECT_CHALLENGE:
//            handle_reconnect_challenge(std::move(session), p);
//            break;
//
//        case AuthOpcode::AUTH_CMSG_RECONNECT_PROOF:
//            handle_reconnect_proof(std::move(session), p);
//            break;
//
//        case AuthOpcode::AUTH_CMSG_REALM_LIST:
//            handle_realm_list(std::move(session), p);
//            break;

        default:
            Logger::get()->warn("[handler] Unknown opcode: {}", static_cast<uint8_t>(opcode));
            break;
    }
}

void HandlersAuth::handle_ping(std::shared_ptr<ClientSession> session, AuthPacket &p) {
    // Считываем ping ID из клиента (4 байта LE)
    uint32_t ping = p.read_uint32_le();
    Logger::get()->debug("[HandlersAuth] AUTH_CMSG_PING with ping: {}", ping);

    // Формируем PONG — возвращаем то же самое число
    AuthPacket reply(AuthOpcode::AUTH_SMSG_PONG);
    reply.write_uint32_le(ping);

    // Отправляем обратно клиенту
    PacketUtils::send_packet_as<AuthPacket>(std::move(session), reply);
}

boost::asio::awaitable<void> HandlersAuth::handle_logon_challenge(std::shared_ptr<ClientSession> session, AuthPacket &p) {
    auto log = Logger::get();

    std::string username = p.read_string_nt();
    log->info("[HandlersAuth] AUTH_CMSG_LOGON_CHALLENGE username: {}", username);

    if (!UTF8Utils::is_valid_utf8(username)) {
        log->error("[HandlersAuth] AUTH_CMSG_LOGON_CHALLENGE Invalid UTF-8 username received: {}", username);
        // Обработка ошибки, например отказ
        co_return;
    }

    username = UTF8Utils::to_lowercase(username);

    // Лезем в бд и смотрим запись:
    PreparedStatement stmt("SELECT_ACCOUNT_BY_USERNAME");
    stmt.set_param(0, username);

    auto user = session->server()->db()->execute_sync<AccountsRow>(stmt);
    if (user) {
        log->info("[HandlersAuth] User '{}' found.", username);

        auto auth = session->getAuthSession();
        // Загружаем salt и verifier из AuthSession (предполагается, что они уже есть)
        std::string salt = user->salt.value();            // salt в hex формате
        std::string verifier = user->verifier.value();    // verifier в hex формате

        if (salt.empty() || verifier.empty()) {
            log->error("No salt or verifier found for user '{}'", username);
            // Обработка ошибки: возможно, закрыть сессию или отправить отказ
            co_return;
        }

        // Загружаем salt и verifier в SRP
        auto srp = auth->srp();
        srp->load_verifier(salt, verifier);

        // Генерируем серверный публичный ключ B на основе текущего verifier
        srp->generate_server_ephemeral();

        // Формируем ответ
        AuthPacket reply(AuthOpcode::AUTH_SMSG_LOGON_CHALLENGE);
        reply.write_string_nt(salt);                             // salt
        reply.write_string_nt(srp->bn_to_hex_str(srp->get_B())); // server public B
        PacketUtils::send_packet_as<AuthPacket>(std::move(session), reply);
    } else {
        log->info("[HandlersAuth] User '{}' not founded", username);
    }
    co_return;
}

//void HandlersAuth::handle_logon_proof(std::shared_ptr<ClientSession> session, AuthPacket &p) {
//    auto log = Logger::get();
//
//    std::string A_hex = p.read_string();
//    std::string client_M1 = p.read_string();
//
//    log->info("[handler] AUTH_CMSG_LOGON_PROOF A={}, M1={}", A_hex, client_M1);
//
//    auto srp = session->srp;
//    if (!srp) {
//        log->error("[handler] No SRP context in session");
//        return;
//    }
//
//    srp->process_client_public(A_hex);
//
//    bool verified = srp->verify_proof(client_M1);
//    log->info("[handler] SRP proof verified: {}", verified);
//
//    AuthPacket reply(AuthOpcode::AUTH_SMSG_LOGON_PROOF);
//    reply.write_uint8(verified ? 0x00 : 0x0C);  // 0x00 = success, 0x0C = fail
//
//    PacketUtils::send_packet_as<AuthPacket>(std::move(session), reply);
//}
//
//void HandlersAuth::handle_reconnect_challenge(std::shared_ptr<ClientSession> session, AuthPacket &p) {
//    auto log = Logger::get();
//    log->info("[handler] AUTH_CMSG_RECONNECT_CHALLENGE");
//
//    auto srp = std::make_shared<SRP>();
//    srp->generate_fake_challenge();
//    session->srp = srp;
//
//    AuthPacket reply(AuthOpcode::AUTH_SMSG_RECONNECT_CHALLENGE);
//    reply.write_string(session->srp->get_salt());
//
//    PacketUtils::send_packet_as<AuthPacket>(std::move(session), reply);
//}
//
//void HandlersAuth::handle_reconnect_proof(std::shared_ptr<ClientSession> session, AuthPacket &p) {
//    auto log = Logger::get();
//    log->info("[handler] AUTH_CMSG_RECONNECT_PROOF");
//
//    AuthPacket reply(AuthOpcode::AUTH_SMSG_RECONNECT_PROOF);
//    reply.write_uint8(0x00);  // success
//
//    PacketUtils::send_packet_as<AuthPacket>(std::move(session), reply);
//}
//
//void HandlersAuth::handle_realm_list(std::shared_ptr<ClientSession> session, AuthPacket &p) {
//    auto log = Logger::get();
//    log->info("[handler] AUTH_CMSG_REALM_LIST");
//
//    AuthPacket reply(AuthOpcode::AUTH_SMSG_REALM_LIST);
//
//    // Примитивный пример одного реалма
//    reply.write_uint32_le(1); // кол-во реалмов
//
//    reply.write_string("TestRealm");
//    reply.write_string("127.0.0.1");
//    reply.write_uint32_le(8085); // порт
//    reply.write_uint8(0x01);     // флаги: онлайн
//
//    PacketUtils::send_packet_as<AuthPacket>(std::move(session), reply);
//}