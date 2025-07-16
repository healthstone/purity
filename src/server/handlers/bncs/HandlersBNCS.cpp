#include "HandlersBNCS.hpp"
#include "Logger.hpp"
#include "packet/opcodes8.hpp"

#include <algorithm>

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

        default:
            Logger::get()->warn("[handler] Unknown opcode: {}", static_cast<uint8_t>(opcode));
            break;
    }
}

void HandlersBNCS::handle_sid_null(std::shared_ptr<ClientSession> session, BNETPacket8 &p) {
    Logger::get()->debug("[handler] SID_NULL");
    BNETPacket8 reply(BNETOpcode8::SID_NULL);
    session->send_packet(reply);
}

void HandlersBNCS::handle_sid_init(std::shared_ptr<ClientSession> session, BNETPacket8 &p) {
    Logger::get()->debug("[handler] SID_INIT");
    BNETPacket8 reply(BNETOpcode8::SID_INIT);
    session->send_packet(reply);
}

void HandlersBNCS::handle_ping(std::shared_ptr<ClientSession> session, BNETPacket8 &p) {
    Logger::get()->debug("[handler] CMSG_PING");
    uint32_t pingval = p.read_uint32();

    BNETPacket8 reply(BNETOpcode8::SID_PING);
    reply.write_uint32(pingval);

    session->send_packet(reply);
}

void HandlersBNCS::handle_auth_info(std::shared_ptr<ClientSession> session, BNETPacket8 &p) {
    Logger::get()->debug("[handler] SID_AUTH_INFO");

    uint32_t protocol = p.read_uint32(); //Версия BNCS протокола (часто 0)

    /** ✅ Что такое archtag ?
archtag (IX86 в Warcraft III) — это идентификатор платформы, на которой запущен клиент:
Значение	Расшифровка
IX86 (0x49583836)	Intel x86 архитектура (Windows PC)
PMAC	PowerPC Macintosh
X64	64-bit вариант (в классике нет)

🔑 PvPGN обычно проверяет archtag, чтобы убедиться, что клиент корректный (PC, Mac).
Практически: на PvPGN для Warcraft III он почти всегда IX86.

👉 Влияет ли на логику?
Прямо нет — это чисто информационное поле.
Может влиять на выбор EXE hash или MPQ filename — например, на Mac клиент другой патч-файл.
Сервер может использовать это, чтобы подставить другой checksum formula.**/
    uint32_t archtag = p.read_uint32(); //Платформа (например IX86)

    /** Что такое clienttag ?
clienttag — это продукт.
Это ключевой тег, который определяет, с какой игрой работает клиент.

Значение	Расшифровка
WAR3 (0x57415233)	Warcraft III: Reign of Chaos
W3XP (0x57335850)	Warcraft III: The Frozen Throne
D2DV	Diablo II Vanilla
D2XP	Diablo II Expansion
STAR	StarCraft
SEXP	StarCraft: Brood War
W2BN	Warcraft II BNE
JSTR	Japanese StarCraft **/
    uint32_t clienttag = p.read_uint32(); // Продукт (например WAR3 или W3XP)
    uint32_t versionid = p.read_uint32(); // Версия exe-файла
    uint32_t gamelang = p.read_uint32(); //	Язык игры
    uint32_t localip = p.read_uint32(); // Локальный IP клиента
    uint32_t timezone_bias = p.read_uint32(); // Смещение временной зоны
    uint32_t lcid = p.read_uint32(); // Windows LCID
    uint32_t langid = p.read_uint32(); // Windows LangID

    std::string langstr = p.read_string(); // Язык строкой ("enUS")
    std::string countrystr = p.read_string(); // Название страны ("United States")

    session->setArchTag(archtag);
    session->setClientTag(clienttag);
    session->setVersionId(versionid);

    // Генерируем токен для сессии
    uint32_t server_token_ = static_cast<uint32_t>(std::rand());
    session->setServerToken(server_token_);

    BNETPacket8 reply(BNETOpcode8::SID_AUTH_CHECK);
    reply.write_uint32(server_token_);
    reply.write_uint32(0); // UDP value
    reply.write_string("War3Patch.mpq");
    reply.write_uint32(0); // value1
    reply.write_uint32(0); // value2
    reply.write_uint32(0); // value3
    reply.write_uint32(0); // exe_info
    reply.write_string("IX86Ver1.mpq"); // формула — для вида
    session->send_packet(reply);
}