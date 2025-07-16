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

    void debug_dump(const std::string& prefix = "[Packet]") const override {
        auto log = Logger::get();

        std::ostringstream oss;

        uint16_t id = static_cast<uint16_t>(get_opcode());
        const auto& payload = serialize();
        uint16_t length = static_cast<uint16_t>(payload.size() + 4); // 2 байта opcode + 2 байта length

        oss << prefix << " [BNETPacket16] Dump: ID=0x"
            << fmt::format("{:04X}", id)
            << " Length_LE=" << length
            << " PayloadSize=" << payload.size() << " bytes\n";

        oss << prefix << " Raw: ";

        // Opcode (2 байта, little-endian)
        oss << fmt::format("{:02X} {:02X} ", id & 0xFF, (id >> 8) & 0xFF);

        // Length (2 байта, little-endian)
        oss << fmt::format("{:02X} {:02X} ", length & 0xFF, (length >> 8) & 0xFF);

        // Payload
        for (auto b : payload) {
            oss << fmt::format("{:02X} ", b);
        }

        log->debug("{}", oss.str());
    }

private:
    BNETOpcode16 opcode_;
};
