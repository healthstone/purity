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
        log_raw_payload(payload, "[BNETPacket8::deserialize] RAW FULL DUMP:");

        BNETPacket8 p(id);
        p.buffer_ = ByteBuffer(payload);
        return p;
    }

    std::vector<uint8_t> build_packet() const override {
        const auto &body = serialize();

        ByteBuffer header;

        // [ID][Length BE][Payload]
        header.write_uint8_be(static_cast<uint8_t>(get_id()));
        header.write_uint16_be(static_cast<uint16_t>(body.size())); // Только body

        std::vector<uint8_t> full_packet;
        full_packet.reserve(3 + body.size());

        const auto &header_data = header.data();
        full_packet.insert(full_packet.end(), header_data.begin(), header_data.end());
        full_packet.insert(full_packet.end(), body.begin(), body.end());

        log_raw_payload(full_packet, "[BNETPacket8::build_packet] RAW FULL DUMP:");
        return full_packet;
    }

private:
    BNETOpcode8 id_;
};

