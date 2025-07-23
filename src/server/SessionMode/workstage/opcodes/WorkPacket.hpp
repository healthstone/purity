#pragma once

#include "packet/Packet.hpp"
#include "WorkOpcodes.hpp"

class WorkPacket : public Packet {
private:
    WorkOpcodes opcode_ = WorkOpcodes::EMPTY_STAGE;

public:
    WorkPacket() = default;

    explicit WorkPacket(WorkOpcodes opcode)
            : opcode_(opcode) {}

    void set_opcode(WorkOpcodes opcode) { opcode_ = opcode; }

    WorkOpcodes get_opcode() const { return opcode_; }

    std::vector<uint8_t> build_packet() const override {
        ByteBuffer temp;

        temp.write_uint16_be(static_cast<uint16_t>(opcode_));
        temp.write_uint16_be(static_cast<uint16_t>(buffer_.size()));
        temp.write_bytes(buffer_.data());

        return temp.data();
    }

    // [Opcode(uint16)][Length(uint16)][Payload]
    void deserialize(const std::vector<uint8_t> &raw_data) override {
        ByteBuffer temp(raw_data);

        opcode_ = static_cast<WorkOpcodes>(temp.read_uint16_be());
        uint16_t length = temp.read_uint16_be();

        if (length != raw_data.size() - 4) {
            throw std::runtime_error("WorkPacket::deserialize: payload length mismatch");
        }

        buffer_.clear();
        auto payload = temp.read_bytes(length);
        buffer_.write_bytes(payload);
    }
};
