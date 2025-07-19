#include <algorithm>
#include <utility>

#include "HandlersBNCS.hpp"
#include "Logger.hpp"
#include "packet/PacketUtils.hpp"

using namespace HandlersBNCS;

void HandlersBNCS::dispatch(std::shared_ptr<ClientSession> session, BNETPacket8 &p) {
    BNETOpcode8 opcode = p.get_id();

    switch (opcode) {
        case BNETOpcode8::SID_NULL:
            handle_sid_null(session, p);
            break;

        case BNETOpcode8::SID_INIT:
            handle_sid_init(session, p);
            break;

        case BNETOpcode8::SID_PING:
            handle_ping(session, p);
            break;

        case BNETOpcode8::SID_AUTH_INFO:
            handle_auth_info(session, p);
            break;

        case BNETOpcode8::SID_AUTH_CHECK:
            handle_auth_check(session, p);
            break;

        case BNETOpcode8::SID_STOPADV:
            handle_sid_stopadv(session, p);
            break;

        default:
            Logger::get()->warn("[handler] Unknown opcode: {}", static_cast<uint8_t>(opcode));
            break;
    }
}

void HandlersBNCS::handle_sid_null(std::shared_ptr<ClientSession> session, BNETPacket8 &p) {
    Logger::get()->debug("[handler] SID_NULL");
    BNETPacket8 reply(BNETOpcode8::SID_NULL);
    PacketUtils::send_packet_as<BNETPacket8>(std::move(session), reply);
}

void HandlersBNCS::handle_sid_init(std::shared_ptr<ClientSession> session, BNETPacket8 &p) {
    Logger::get()->debug("[handler] SID_INIT");
    BNETPacket8 reply(BNETOpcode8::SID_INIT);
    PacketUtils::send_packet_as<BNETPacket8>(std::move(session), reply);
}

void HandlersBNCS::handle_sid_stopadv(std::shared_ptr<ClientSession> session, BNETPacket8 &p) {
    uint32_t game_id = p.read_uint32_be(); // ID игры, которую нужно удалить
    Logger::get()->debug("[handler] SID_STOPADV id: {}", game_id);
}

void HandlersBNCS::handle_ping(std::shared_ptr<ClientSession> session, BNETPacket8 &p) {
    Logger::get()->debug("[handler] CMSG_PING");
    BNETPacket8 reply(BNETOpcode8::SID_PING);
    PacketUtils::send_packet_as<BNETPacket8>(std::move(session), reply);
}

void HandlersBNCS::handle_auth_info(std::shared_ptr<ClientSession> session, BNETPacket8 &p) {
    // === Разбор SID_AUTH_INFO ===
    p.skip(4); // Signature

    uint32_t protocol = p.read_uint32_be();
    uint32_t archtag_raw = p.read_uint32_le();
    uint32_t clienttag_raw = p.read_uint32_le();
    uint32_t versionid = p.read_uint32_le();
    uint32_t gamelang = p.read_uint32_be();
    uint32_t localip = p.read_uint32_be();
    int16_t timezone_bias = p.read_int16_le();
    p.skip(2); // Alignment
    uint32_t lcid = p.read_uint32_be();
    uint32_t langid = p.read_uint32_be();
    std::string langstr = p.read_string_raw(4);
    std::string countrystr = p.read_string_raw(4);

    Logger::get()->debug(
            "[handler] SID_AUTH_INFO:\n"
            "  protocol=0x{:08X}\n"
            "  archtag=0x{:08X}\n"
            "  clienttag=0x{:08X}\n"
            "  versionid=0x{:08X}\n"
            "  gamelang=0x{:08X}\n"
            "  localip=0x{:08X}\n"
            "  timezone_bias={} min\n"
            "  lcid=0x{:08X}\n"
            "  langid=0x{:08X}\n"
            "  langstr={}\n"
            "  countrystr={}",
            protocol, archtag_raw, clienttag_raw, versionid,
            gamelang, localip, timezone_bias, lcid, langid,
            langstr, countrystr
    );

    // === Формируем корректный SID_AUTH_INFO ===
    BNETPacket8 reply(BNETOpcode8::SID_AUTH_INFO);

    // === Основная часть (40 байт) ===
    reply.write_uint32_be(0);                    // Protocol
    reply.write_uint32_le(archtag_raw);          // Platform
    reply.write_uint32_le(clienttag_raw);        // Product
    reply.write_uint32_le(versionid);            // Version (тот же)
    reply.write_uint32_be(gamelang);             // Language
    reply.write_uint32_be(localip);              // Local IP
    reply.write_int16_le(timezone_bias);         // Timezone bias
    reply.write_uint16_be(0);                    // Padding
    reply.write_uint32_be(lcid);                 // Locale ID
    reply.write_uint32_be(langid);               // Language ID
    reply.write_uint32_be(0x20535552);           // "RUS "
    reply.write_uint32_be(0x20737552);           // "Rus "
    Logger::get()->debug("основная часть = {}", reply.size());

    // === War3 специфичные поля ===
    uint32_t server_token = static_cast<uint32_t>(std::rand());
    session->getBNCSAccount()->setServerToken(server_token);

    reply.write_uint32_be(server_token);         // Server Token
    reply.write_uint32_be(0);                    // UDP Value
    reply.write_uint64_le(0);                    // MPQ Filetime

    reply.write_string_nt("ver-IX86-1.mpq");        // MPQ Filename (ASCIIZ)
    reply.write_string_nt("A=0 B=0 C=0 D=0");       // Value String (ASCIIZ)

    Logger::get()->debug("Final packet size: {} bytes", reply.size());

    PacketUtils::send_packet_as<BNETPacket8>(std::move(session), reply);
}


void HandlersBNCS::handle_auth_check(std::shared_ptr<ClientSession> session, BNETPacket8 &p) {
    Logger::get()->debug("[handler] SID_AUTH_CHECK");
}