#pragma once

#include "Packet.hpp"
#include "opcodes16.hpp"
#include <arpa/inet.h>
#include <endian.h>

class BNETPacket16 : public Packet {
public:
    explicit BNETPacket16(BNETOpcode16 opcode)
            : opcode_(opcode) {}

    BNETOpcode16 get_opcode() const { return opcode_; }

    static BNETPacket16 deserialize(const std::vector<uint8_t>& payload, BNETOpcode16 opcode) {
        BNETPacket16 p(opcode);
        p.buffer_ = ByteBuffer(payload);
        return p;
    }

private:
    BNETOpcode16 opcode_;
};
