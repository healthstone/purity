#pragma once

#include "packet/Packet.hpp"
#include "AuthCmd.hpp"
#include <vector>
#include <string>
#include <stdexcept>

class AuthPacket : public Packet {
public:
    AuthPacket() = default;

    explicit AuthPacket(AuthCmd cmd)
            : cmd_(cmd)
    {}

    ~AuthPacket() override = default;

    AuthCmd cmd() const { return cmd_; }

    // Проверяем, что размер payload совпадает с ожидаемым
    void verify_size(uint16_t expected_size) const {
        // В buffer_ хранится только payload (без opcode и длины)
        if (buffer_.size() != expected_size + 4) {
            throw std::runtime_error("[AuthPacket] Payload size mismatch");
        }
    }

    // Создаем пакет из opcode и payload (payload — без заголовка)
    static AuthPacket deserialize(AuthCmd cmd, const std::vector<uint8_t>& payload) {
        AuthPacket pkt;
        pkt.cmd_ = cmd;
        pkt.buffer_.write_bytes(payload);
        return pkt;
    }

    // Собираем пакет: [cmd] + [payload]
    std::vector<uint8_t> build_packet() const override {
        std::vector<uint8_t> result;
        const auto& payload = buffer_.data();

        // Opcode
        result.push_back(static_cast<uint8_t>(cmd_));

        // Payload
        result.insert(result.end(), payload.begin(), payload.end());

        log_raw_payload(std::to_string(static_cast<uint8_t>(cmd_)), result, "AuthPacket");

        return result;
    }

private:
    AuthCmd cmd_;
};
