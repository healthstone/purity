#pragma once

#include "packet/Packet.hpp"
#include "opcodes8.hpp"
#include <endian.h>

class BNETPacket8 : public Packet {
public:
    explicit BNETPacket8(BNETOpcode8 id)
            : id_(id) {}

    BNETOpcode8 get_id() const { return id_; }

    static BNETPacket8 deserialize(const std::vector<uint8_t>& payload, BNETOpcode8 id) {
        BNETPacket8 p(id);
        p.buffer_ = ByteBuffer(payload);
        return p;
    }

private:
    BNETOpcode8 id_;
};
