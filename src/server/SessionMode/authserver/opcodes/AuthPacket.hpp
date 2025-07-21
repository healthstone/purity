#pragma once

#include "packet/Packet.hpp"
#include "AuthOpcodes.hpp"
#include <vector>
#include <stdexcept>
#include <string>
#include <array>

class AuthPacket : public Packet {
public:
    AuthPacket() = default;

    explicit AuthPacket(AuthOpcode cmd)
            : cmd_(cmd)
    {}

    ~AuthPacket() override = default;

    AuthOpcode cmd() const { return cmd_; }
    uint8_t error() const { return error_; }
    uint16_t size() const { return size_; }

    void set_cmd(AuthOpcode cmd) { cmd_ = cmd; }
    void set_error(uint8_t error) { error_ = error; }

    // Заголовок считываем отдельно
    void set_header(AuthOpcode cmd, const uint8_t& error, const uint16_t& size) {
        cmd_ = cmd;
        error_ = error;
        size_ = size;

        if (size_ != buffer_.size()) {
            Logger::get()->error("[AuthPacket] payload bytes: {}, expected {}", buffer_.size(), size_);
            throw std::runtime_error("[AuthPacket] Mismatch: size != payload bytes");
        }
    }

    // Статический метод десериализации пакета с конкретным опкодом
    static AuthPacket deserialize(AuthOpcode id, const std::vector<uint8_t>& payload) {
        AuthPacket pkt;
        pkt.cmd_ = static_cast<AuthOpcode>(id);

        // Заполняем buffer_ данными из payload
        pkt.buffer_.write_bytes(payload);

        pkt.size_ = static_cast<uint16_t>(payload.size());

        // Если нужно, можно тут добавить специфичный разбор содержимого по id
        // Например, для AUTH_CMSG_LOGON_CHALLENGE можно читать поля из pkt.buffer_
        // Но в этой реализации просто заполняем буфер — конкретный разбор можно делать из вне

        return pkt;
    }

    // Метод построения финального пакета с заголовком
    std::vector<uint8_t> build_packet() const override {
        std::vector<uint8_t> result;

        result.push_back(static_cast<uint8_t>(cmd_));
        result.push_back(error_);

        uint16_t payload_size = static_cast<uint16_t>(buffer_.size());
        // Записываем размер в LE: младший байт, затем старший
        result.push_back(static_cast<uint8_t>(payload_size & 0xFF));        // младший байт
        result.push_back(static_cast<uint8_t>((payload_size >> 8) & 0xFF)); // старший байт

        const auto& payload = buffer_.data();
        result.insert(result.end(), payload.begin(), payload.end());

        return result;
    }

private:
    AuthOpcode cmd_;
    uint8_t error_ = 0;
    uint16_t size_ = 0;
};
