#pragma once

#include "packet/Packet.hpp"
#include "AuthOpcodes.hpp"
#include <endian.h>
#include <vector>
#include <cstdint>

class AuthPacket : public Packet {
public:
    explicit AuthPacket(AuthOpcode id)
            : id_(id) {}

    AuthOpcode get_id() const { return id_; }

    /// TrinityCore AUTH format: [Length LE][Opcode][Payload]
    std::vector<uint8_t> build_packet() const override {
        const auto &body = serialize();

        ByteBuffer header;

        uint16_t total_length = static_cast<uint16_t>(1 + body.size()); // 1 byte opcode + payload
        header.write_uint16_le(total_length);                  // Length LE (2 byte)
        header.write_uint8_be(static_cast<uint8_t>(get_id())); // Opcode    (1 byte)

        std::vector<uint8_t> full_packet;
        full_packet.reserve(2 + 1 + body.size());

        const auto &header_data = header.data();
        full_packet.insert(full_packet.end(), header_data.begin(), header_data.end());
        full_packet.insert(full_packet.end(), body.begin(), body.end());

        //log_raw_payload(static_cast<uint16_t>(get_id()), body, "[AuthPacket::build_packet]");
        return full_packet;
    }

    static AuthPacket deserialize(AuthOpcode id, const std::vector<uint8_t> &payload) {
        //log_raw_payload(static_cast<uint16_t>(id), payload, "[AuthPacket::deserialize]");

        AuthPacket p(id);
        p.buffer_ = ByteBuffer(payload);
        return p;
    }


private:
    AuthOpcode id_;
};
