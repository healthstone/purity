#include "HandlersAuth.hpp"

#include <utility>
#include "packet/PacketUtils.hpp"
#include "utf8utils/UTF8Utils.hpp"
#include "src/server/SessionMode/authserver/opcodes/AuthResult.hpp"
#include "utils/NetUtils.hpp"
#include "utils/HexUtils.hpp"
#include "src/server/SessionMode/authserver/opcodes/AuthPacket.hpp"

std::array<uint8_t, 16> VersionChallenge = { { 0xBA, 0xA3, 0x1E, 0x99, 0xA0, 0x0B, 0x21, 0x57, 0xFC, 0x37, 0x3F, 0xB3, 0x69, 0xCD, 0xD2, 0xF1 } };

using namespace HandlersAuth;

void HandlersAuth::dispatch(std::shared_ptr<ClientSession> session, AuthPacket &p) {
    AuthCmd opcode = p.cmd();
    auto log = Logger::get();

    switch (opcode) {
        case AuthCmd::AUTH_CMSG_PING:
            handle_ping(std::move(session), p);
            break;

        case AuthCmd::AUTH_LOGON_CHALLENGE:
            boost::asio::co_spawn(
                    session->socket().get_executor(),
                    handle_logon_challenge(session, p),
                    boost::asio::detached
            );
            break;

//        case AuthCmd::AUTH_CMSG_LOGON_PROOF:
//            handle_logon_proof(std::move(session), p);
//            break;
//
//        case AuthCmd::AUTH_CMSG_RECONNECT_CHALLENGE:
//            handle_reconnect_challenge(std::move(session), p);
//            break;
//
//        case AuthCmd::AUTH_CMSG_RECONNECT_PROOF:
//            handle_reconnect_proof(std::move(session), p);
//            break;
//
//        case AuthCmd::AUTH_CMSG_REALM_LIST:
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
    AuthPacket reply(AuthCmd::AUTH_SMSG_PONG);
    reply.write_uint32_le(ping);

    // Отправляем обратно клиенту
    PacketUtils::send_packet_as<AuthPacket>(std::move(session), reply);
}

boost::asio::awaitable<void>
HandlersAuth::handle_logon_challenge(std::shared_ptr<ClientSession> session, AuthPacket &p) {
    auto log = Logger::get();
    // оборачиваем в try/catch, в случае проблем с чтением (криво заполненные поля или несоответствие length) - шлем AUTH_UNKNOWN_ACCOUNT
    try {
        // Первые 4 байта уже прочитаны
        uint8_t cmd = p.read_uint8();
        uint8_t error = p.read_uint8();
        uint16_t size = p.read_uint16_le();

        // Считывать начинаем из payload
        std::string gamename = p.read_string_raw_le(4);     // 0x04 (LE?)

        uint8_t version1 = p.read_uint8();                  // 0x08
        uint8_t version2 = p.read_uint8();                  // 0x09
        uint8_t version3 = p.read_uint8();                  // 0x0A

        uint16_t build = p.read_uint16_le();                // 0x0B  (LE)

        std::string platform = p.read_string_raw_le(4); // 0x0D (LE)
        std::string os = p.read_string_raw_le(4);       // 0x11 (LE)
        std::string country = p.read_string_raw_le(4);  // 0x15 (LE)

        uint32_t timezone = p.read_uint32_le();             // 0x19 (LE)
        uint32_t ip = p.read_uint32_le();                   // 0x1D
        std::string ip_str_le = NetUtils::uint32_to_ip_le(ip); // "127.0.0.1"

        uint8_t name_len = p.read_uint8();               // 0x21
        std::string username = p.read_string_raw_be(name_len); // 0x22

        log->info(
                "[HandlersAuth] AUTH_CMSG_LOGON_CHALLENGE:\n"
                " cmd=0x{:02X}\n"
                " error=0x{:02X}\n"
                " size={}\n"
                " gamename={}\n"
                " version={}.{}.{}\n"
                " build={}\n"
                " platform={}\n"
                " os={}\n"
                " country={}\n"
                " timezone={}\n"
                " ip={}\n"
                " name_len={}\n"
                " username={}",
                cmd, error, size,
                gamename,
                version1, version2, version3,
                build,
                platform,
                os,
                country,
                std::to_string(timezone),
                ip_str_le,
                name_len,
                username
        );

        if (!UTF8Utils::is_valid_utf8(username)) {
            log->error("[HandlersAuth] AUTH_CMSG_LOGON_CHALLENGE - Invalid UTF-8 username received: {}", username);
            // Обработка ошибки, например отказ
            AuthPacket reply(AuthCmd::AUTH_LOGON_CHALLENGE);
            reply.write_uint8(0);
            reply.write_uint8(WOW_FAIL_UNKNOWN_ACCOUNT);
            PacketUtils::send_packet_as<AuthPacket>(std::move(session), reply);
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
                log->error("[HandlersAuth] AUTH_CMSG_LOGON_CHALLENGE - No salt or verifier found for user '{}'",
                           username);

                AuthPacket reply(AuthCmd::AUTH_LOGON_CHALLENGE);
                reply.write_uint8(0);
                reply.write_uint8(WOW_FAIL_INCORRECT_PASSWORD);
                PacketUtils::send_packet_as<AuthPacket>(std::move(session), reply);
                co_return;
            }

            // Загружаем salt и verifier в SRP
            auto srp = auth->srp();
            srp->load_verifier(salt, verifier);

            // Генерируем серверный публичный ключ B на основе текущего verifier
            srp->generate_server_ephemeral();

            // salt, verifier (B), N — передаются как бинарные массивы, не как строки
            auto salt_bin = HexUtils::hex_to_bytes(salt);        // salt - бинарный массив
            auto B_bin = srp->get_B_bytes();                     // публичный ключ B, бинарный массив
            auto N_bin = srp->get_N_bytes();                     // простое число N, бинарный массив
            uint8_t g = srp->get_generator();                    // обычно 7

            AuthPacket reply(AuthCmd::AUTH_LOGON_CHALLENGE);
            reply.write_uint8(0);                       // error byte
            reply.write_uint8(WOW_SUCCESS);             // success = 0

            reply.write_bytes(B_bin);                   // 32 байта B
            reply.write_uint8(1);                       // param marker
            reply.write_uint8(g);                       // 1 байт g
            reply.write_uint8(32);                      // length(N)
            reply.write_bytes(N_bin);                   // 32 байта N
            reply.write_bytes(salt_bin);                // 32 байта salt
            reply.write_bytes(VersionChallenge.data(), VersionChallenge.size()); // 16 байт
            reply.write_uint8(0);           // 1 байт

            PacketUtils::send_packet_as<AuthPacket>(std::move(session), reply);
        } else {
            log->error("[HandlersAuth] User '{}' not founded", username);
            // УЗ не найдена: отправить отказ
            AuthPacket reply(AuthCmd::AUTH_LOGON_CHALLENGE);
            reply.write_uint8(0);
            reply.write_uint8(WOW_FAIL_NO_GAME_ACCOUNT);
            PacketUtils::send_packet_as<AuthPacket>(std::move(session), reply);
        }
        co_return;
    } catch (const std::exception& ex) {
        log->error("[HandlersAuth] AUTH_CMSG_LOGON_CHALLENGE - couldn't read the package fields {}", ex.what());
        // Формируем ответ
        AuthPacket reply(AuthCmd::AUTH_LOGON_CHALLENGE);
        reply.write_uint8(0);
        reply.write_uint8(WOW_FAIL_DISCONNECTED);
        PacketUtils::send_packet_as<AuthPacket>(std::move(session), reply);
        co_return;
    }
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
//    AuthPacket reply(AuthCmd::AUTH_SMSG_LOGON_PROOF);
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
//    AuthPacket reply(AuthCmd::AUTH_SMSG_RECONNECT_CHALLENGE);
//    reply.write_string(session->srp->get_salt());
//
//    PacketUtils::send_packet_as<AuthPacket>(std::move(session), reply);
//}
//
//void HandlersAuth::handle_reconnect_proof(std::shared_ptr<ClientSession> session, AuthPacket &p) {
//    auto log = Logger::get();
//    log->info("[handler] AUTH_CMSG_RECONNECT_PROOF");
//
//    AuthPacket reply(AuthCmd::AUTH_SMSG_RECONNECT_PROOF);
//    reply.write_uint8(0x00);  // success
//
//    PacketUtils::send_packet_as<AuthPacket>(std::move(session), reply);
//}
//
//void HandlersAuth::handle_realm_list(std::shared_ptr<ClientSession> session, AuthPacket &p) {
//    auto log = Logger::get();
//    log->info("[handler] AUTH_CMSG_REALM_LIST");
//
//    AuthPacket reply(AuthCmd::AUTH_SMSG_REALM_LIST);
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