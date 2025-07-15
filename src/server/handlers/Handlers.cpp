#include "Handlers.hpp"
#include "Logger.hpp"
#include "opcodes.hpp"

#include <algorithm>

using namespace Handlers;

void Handlers::dispatch(std::shared_ptr<ClientSession> session, Packet &p) {
    Opcode opcode = p.get_opcode();

    switch (opcode) {
        case Opcode::CMSG_PING:
            handle_ping(session, p);
            break;

        case Opcode::CMSG_CHAT_MESSAGE:
            Handlers::handle_chat_message(session, p);
            break;

        case Opcode::CMSG_ACCOUNT_LOOKUP_BY_NAME:
            boost::asio::co_spawn(
                    session->server()->thread_pool(),
                    handle_auth_select(session, p),
                    boost::asio::detached
            );
            break;
        default:
            Logger::get()->warn("[Handlers] Unknown opcode: {}", static_cast<uint16_t>(opcode));
            break;
    }
}

void Handlers::handle_ping(std::shared_ptr<ClientSession> session, Packet &p) {
    uint32_t ping_value = p.read_uint32();
    Logger::get()->debug("[Handlers] CMSG_PING: {}", ping_value);

    Packet pong(Opcode::SMSG_PONG);
    pong.write_uint32(ping_value);
    session->send_packet(pong);
}

void Handlers::handle_chat_message(std::shared_ptr<ClientSession> session, Packet &p) {
    std::string msg = p.read_string();
    Logger::get()->info("[Handlers] CMSG_CHAT_MESSAGE: {}", msg);
}

boost::asio::awaitable<void> Handlers::handle_auth_select(std::shared_ptr<ClientSession> session, Packet &p) {
    auto log = Logger::get();
    std::string username = p.read_string();
    std::transform(username.begin(), username.end(), username.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    log->info("[Handlers] CMSG_ACCOUNT_LOOKUP_BY_NAME with username [{}]", username);

    PreparedStatement stmt("SELECT_ACCOUNT_BY_USERNAME");
    stmt.set_param(0, username);

    uint64_t answer_id = 0;
    std::string answer_name = "";

    // Проверим правильно ли ты вызываешь sync метод в worker потоке
    try {
        auto user = session->server()->db()->execute_sync<AccountsRow>(stmt);
        if (user) {
            answer_id = user->id;
            log->debug("[handle_auth_select] User '{}' found. ID: {}", user->name.value(), user->id);
        } else {
            log->debug("[handle_auth_select] User '{}' not found.", username);
        }
    } catch (const std::exception& ex) {
        Logger::get()->error("DB Exception: {}", ex.what());
    }

    Packet reply(Opcode::SMSG_ACCOUNT_LOOKUP_BY_NAME);
    reply.write_uint64(answer_id);
    reply.write_string(answer_name);
    session->send_packet(reply);
    co_return;
}