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
    // Пропускаем сигнатуру (4 байта)
    p.skip(4);

    // Чтение входящих данных
    uint32_t protocol = p.read_uint32_be();
    uint32_t archtag_raw = p.read_uint32_le();
    std::string archtag;
    archtag += static_cast<char>((archtag_raw >> 24) & 0xFF);
    archtag += static_cast<char>((archtag_raw >> 16) & 0xFF);
    archtag += static_cast<char>((archtag_raw >> 8) & 0xFF);
    archtag += static_cast<char>(archtag_raw & 0xFF);

    uint32_t clienttag_raw = p.read_uint32_le();
    std::string clienttag;
    clienttag += static_cast<char>((clienttag_raw >> 24) & 0xFF);
    clienttag += static_cast<char>((clienttag_raw >> 16) & 0xFF);
    clienttag += static_cast<char>((clienttag_raw >> 8) & 0xFF);
    clienttag += static_cast<char>(clienttag_raw & 0xFF);

    uint32_t versionid = p.read_uint32_le();
    uint32_t gamelang = p.read_uint32_be();
    uint32_t localip = p.read_uint32_be();
    int16_t timezone_bias = p.read_int16_le();
    p.skip(2);  // Выравнивание
    uint32_t lcid = p.read_uint32_be();
    uint32_t langid = p.read_uint32_be();
    std::string langstr = p.read_string_raw(4);    // 4 байта
    std::string countrystr = p.read_string_raw(4); // 4 байта

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

//    // Формируем ответ
//    BNETPacket8 reply(BNETOpcode8::SID_AUTH_INFO);
//
//    // Основная часть (40 байт)
//    reply.write_uint32_be(0);               // Protocol
//    reply.write_uint32_le('IX86');          // Platform
//    reply.write_uint32_le('W3XP');          // Product
//    reply.write_uint32_le(versionid);       // Version (тот же что пришел)
//    reply.write_uint32_be('ruRU');          // Language
//    reply.write_uint32_be(localip);         // Local IP
//    reply.write_int16_le(timezone_bias);    // Timezone bias
//    reply.write_uint16_be(0);                // Padding
//    reply.write_uint32_be(lcid);             // Locale ID
//    reply.write_uint32_be(langid);           // Language ID
//    reply.write_uint32_be(0x20535552);       // Country abbreviation "RUS "
//    reply.write_uint32_be(0x20737552);       // Country "Rus "
//
//    // War3 специфичные поля (23 байта)
//    uint32_t server_token = static_cast<uint32_t>(std::rand());
//    session->getBNCSAccount()->setServerToken(server_token);
//
//    reply.write_uint32_be(server_token);    // Server token
//    reply.write_uint32_be(0);                // UDP value
//    reply.write_uint64_le(0);                // MPQ filetime (8 байт)
//
//    const uint8_t mpq_filename[16] = {'v','e','r','-','I','X','8','6','-','1','.','m','p','q',0,0};
//    reply.write_bytes(mpq_filename, 16);
//
//    const uint8_t value_string[16] = "A=0 B=0 C=0 D=0";
//    reply.write_bytes(value_string, 16);
//
//    // Отправляем ответ
//    PacketUtils::send_packet_as<BNETPacket8>(std::move(session), reply);
}

void HandlersBNCS::handle_auth_check(std::shared_ptr<ClientSession> session, BNETPacket8 &p) {
    Logger::get()->debug("[handler] SID_AUTH_CHECK");
}