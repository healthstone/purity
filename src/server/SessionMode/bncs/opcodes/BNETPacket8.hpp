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

    // [ID][Length BE][Payload]
    std::vector<uint8_t> build_packet_with_length() const {
        const auto &body = serialize();

        ByteBuffer header;

        // Заголовок: [ID][Length BE][Payload]
        header.write_uint8_be(static_cast<uint8_t>(get_id()));
        header.write_uint16_be(static_cast<uint16_t>(body.size() + 3)); // +3 байта на заголовок

        // Эффективное объединение данных
        std::vector<uint8_t> full_packet;
        full_packet.reserve(header.size() + body.size());

        const auto &header_data = header.data();
        full_packet.insert(full_packet.end(), header_data.begin(), header_data.end());
        full_packet.insert(full_packet.end(), body.begin(), body.end());

        return full_packet;
    }

    // [ID][Payload]
    std::vector<uint8_t> build_packet_without_length() const {
        const auto &body = serialize();

        ByteBuffer packet;

        // Заголовок: [ID]
        packet.write_uint8_be(static_cast<uint8_t>(get_id()));

        // Тело пакета
        packet.append(body.data(), body.size());

        return packet.data();
    }

    void debug_dump(const std::string &prefix = "[Packet]") const override {
        auto log = Logger::get();
        uint8_t id = static_cast<uint8_t>(get_id());
        const auto &payload = serialize();

        std::ostringstream oss;
        oss << "[" << prefix << "] Dump: ID=0x"
            << fmt::format("{:02X}", id)
            << " PayloadSize=" << payload.size() << " bytes: ";

        for (size_t i = 0; i < payload.size(); ++i) {
            oss << std::hex << std::setw(2) << std::setfill('0')
                << static_cast<int>(payload[i]) << " ";
        }

        log->debug("{}", oss.str());
    }

private:
    BNETOpcode8 id_;
};
