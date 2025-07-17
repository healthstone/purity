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
    // Пропустить сигнатуру (4 байта)
    p.skip(4);

    uint32_t protocol = p.read_uint32_be();

    uint32_t archtag_raw = p.read_uint32_le();  // LE !
    std::string archtag;
    archtag += static_cast<char>((archtag_raw >> 24) & 0xFF);
    archtag += static_cast<char>((archtag_raw >> 16) & 0xFF);
    archtag += static_cast<char>((archtag_raw >> 8) & 0xFF);
    archtag += static_cast<char>(archtag_raw & 0xFF);

    uint32_t clienttag_raw = p.read_uint32_le();  // LE !
    std::string clienttag;
    clienttag += static_cast<char>((clienttag_raw >> 24) & 0xFF);
    clienttag += static_cast<char>((clienttag_raw >> 16) & 0xFF);
    clienttag += static_cast<char>((clienttag_raw >> 8) & 0xFF);
    clienttag += static_cast<char>(clienttag_raw & 0xFF);

    uint32_t versionid = p.read_uint32_le();  // LE !
    uint32_t gamelang = p.read_uint32_be();
    uint32_t localip = p.read_uint32_be();
    int16_t timezone_bias = p.read_int16_le(); // LE ! <-- Timezone: только 2 байта!

    p.skip(2);  // <-- выравнивание! (padding)
    uint32_t lcid = p.read_uint32_be();
    uint32_t langid = p.read_uint32_be();
    std::string langstr = p.read_string_raw(4);
    std::string countrystr = p.read_string_raw(4);

    //p.debug_dump("SID_AUTH_INFO");

    Logger::get()->debug(
            "[handler] SID_AUTH_INFO:\n"
            "  protocol=0x{:08X}\n"
            "  archtag={}\n"
            "  clienttag={}\n"
            "  versionid=0x{:08X} ({})\n"
            "  gamelang=0x{:08X}\n"
            "  localip={}.{}.{}.{}\n"
            "  timezone_bias={} minutes\n"
            "  lcid=0x{:08X}\n"
            "  langid=0x{:08X}\n"
            "  langstr={}\n"
            "  countrystr={}",
            protocol,
            archtag,
            clienttag,
            versionid, versionid,
            gamelang,
            (localip >> 24) & 0xFF, (localip >> 16) & 0xFF,
            (localip >> 8) & 0xFF, localip & 0xFF,
            timezone_bias,
            lcid,
            langid,
            langstr,
            countrystr
    );
}

void HandlersBNCS::handle_auth_check(std::shared_ptr<ClientSession> session, BNETPacket8 &p) {
    Logger::get()->debug("[handler] SID_AUTH_CHECK");
}