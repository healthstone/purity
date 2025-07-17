#pragma once

#include "packet/Packet.hpp"
#include "opcodes8.hpp"
#include <endian.h>

class BNETPacket8 : public Packet {
public:
    explicit BNETPacket8(BNETOpcode8 id)
            : id_(id) {}

    BNETOpcode8 get_id() const { return id_; }

    static BNETPacket8 deserialize(const std::vector<uint8_t> &payload, BNETOpcode8 id) {
        BNETPacket8 p(id);
        p.buffer_ = ByteBuffer(payload);
        return p;
    }

    std::vector<uint8_t> build_packet() const override {
        switch (get_id()) {
            // BNCS
            case BNETOpcode8::SID_AUTH_INFO:
            case BNETOpcode8::SID_AUTH_CHECK:
            case BNETOpcode8::SID_W3_LEAVECHAT:
            case BNETOpcode8::SID_W3_GAMELIST:
            // Diablo 2
            case BNETOpcode8::SID_AUTH_ACCOUNTLOGON:
            case BNETOpcode8::SID_AUTH_ACCOUNTLOGONPROOF: {
                return build_packet_without_length();
            }
            default: {
                return build_packet_with_length();
            }
        }
    }

    std::vector<uint8_t> build_packet_with_length() const {
        const auto& body = serialize();

        ByteBuffer header;

        uint8_t id = static_cast<uint8_t>(get_id());
        uint16_t length_be = htobe16(static_cast<uint16_t>(body.size() + 3)); // BE вместо LE

        header.write_uint8(id);
        header.write_uint16(length_be); // Записываем в BE порядке

        std::vector<uint8_t> full_packet = header.data();
        full_packet.insert(full_packet.end(), body.begin(), body.end());

        return full_packet;
    }

    std::vector<uint8_t> build_packet_without_length() const {
        const auto &body = serialize(); // Сериализованные данные (Payload)

        ByteBuffer packet;

        // Записываем ID пакета (1 байт)
        uint8_t id = static_cast<uint8_t>(get_id());
        packet.write_uint8(id);

        // Добавляем тело пакета (Payload) используя метод append
        packet.append(body.data(), body.size());

        return packet.data();
    }

    void debug_dump(const std::string &prefix = "[Packet]") const override {
        auto log = Logger::get();

        std::ostringstream oss;

        uint8_t id = static_cast<uint8_t>(get_id());
        const auto &payload = serialize();
        uint16_t length = static_cast<uint16_t>(payload.size() + 3);

        oss << prefix << " [BNETPacket8] Dump: ID=0x"
            << fmt::format("{:02X}", id)
            << " Length_LE=" << length
            << " PayloadSize=" << payload.size() << " bytes\n";

        oss << prefix << " Raw: ";

        // ID
        oss << fmt::format("{:02X} ", id);

        // Length LE
        oss << fmt::format("{:02X} {:02X} ", length & 0xFF, (length >> 8) & 0xFF);

        // Payload
        for (auto b: payload) {
            oss << fmt::format("{:02X} ", b);
        }

        log->debug("{}", oss.str());
    }

private:
    BNETOpcode8 id_;
};
