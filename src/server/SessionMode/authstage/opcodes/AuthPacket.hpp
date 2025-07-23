#pragma once

#include "packet/Packet.hpp"
#include "AuthOpcodes.hpp"

class AuthPacket : public Packet {
private:
    AuthOpcodes opcode_ = AuthOpcodes::EMPTY_STAGE;

public:
    AuthPacket() = default;

    explicit AuthPacket(AuthOpcodes opcode)
            : opcode_(opcode) {}

    void set_opcode(AuthOpcodes opcode) { opcode_ = opcode; }

    AuthOpcodes get_opcode() const { return opcode_; }

    std::vector<uint8_t> build_packet() const override {
        ByteBuffer temp;

        temp.write_uint8(static_cast<uint8_t>(opcode_));
        temp.write_uint16_be(static_cast<uint16_t>(buffer_.size()));
        temp.write_bytes(buffer_.data());

        return temp.data();
    }

    // [Opcode(uint8)][Length(uint16)][Payload]
    void deserialize(const std::vector<uint8_t> &raw_data) override {
        ByteBuffer temp(raw_data);

        opcode_ = static_cast<AuthOpcodes>(temp.read_uint8());
        uint16_t length = temp.read_uint16_be();

        if (length != raw_data.size() - 3) {
            throw std::runtime_error("AuthPacket::deserialize: payload length mismatch");
        }

        buffer_.clear();
        auto payload = temp.read_bytes(length);
        buffer_.write_bytes(payload);
    }
};
