#pragma once

#include "packet/Packet.hpp"
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

    std::vector<uint8_t> build_packet() const override {
        const auto& body = serialize();

        ByteBuffer header;

        // Запись opcode и длины в Little-Endian напрямую
        header.write_uint16_le(static_cast<uint16_t>(get_opcode()));
        header.write_uint16_le(static_cast<uint16_t>(body.size()));

        // Эффективное объединение данных
        std::vector<uint8_t> full_packet;
        full_packet.reserve(header.size() + body.size());  // Оптимизация: резервируем память

        const auto& header_data = header.data();
        full_packet.insert(full_packet.end(), header_data.begin(), header_data.end());
        full_packet.insert(full_packet.end(), body.begin(), body.end());

        return full_packet;
    }

private:
    BNETOpcode16 opcode_;
};
