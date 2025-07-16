#pragma once

#include "ByteBuffer.hpp"
#include <vector>
#include <string>
#include <cstdint>
#include <stdexcept>

class Packet {
protected:
    ByteBuffer buffer_;

public:
    Packet() = default;

    virtual ~Packet() = default;

    ByteBuffer& raw() { return buffer_; }

    const std::vector<uint8_t>& serialize() const { return buffer_.data(); }

    // --- Write ---
    void write_uint8(uint8_t value)   { buffer_.write_uint8(value); }
    void write_uint16(uint16_t value) { buffer_.write_uint16(value); }
    void write_uint32(uint32_t value) { buffer_.write_uint32(value); }
    void write_uint64(uint64_t value) { buffer_.write_uint64(value); }

    void write_int8(int8_t value)   { buffer_.write_int8(value); }
    void write_int16(int16_t value) { buffer_.write_int16(value); }
    void write_int32(int32_t value) { buffer_.write_int32(value); }
    void write_int64(int64_t value) { buffer_.write_int64(value); }

    void write_float(float value)   { buffer_.write_float(value); }
    void write_bool(bool value)     { buffer_.write_bool(value); }
    void write_string(const std::string& str) { buffer_.write_string(str); }

    // --- Read ---
    uint8_t read_uint8()   { return buffer_.read_uint8(); }
    uint16_t read_uint16() { return buffer_.read_uint16(); }
    uint32_t read_uint32() { return buffer_.read_uint32(); }
    uint64_t read_uint64() { return buffer_.read_uint64(); }

    int8_t read_int8()   { return buffer_.read_int8(); }
    int16_t read_int16() { return buffer_.read_int16(); }
    int32_t read_int32() { return buffer_.read_int32(); }
    int64_t read_int64() { return buffer_.read_int64(); }

    float read_float()   { return buffer_.read_float(); }
    bool read_bool()     { return buffer_.read_bool(); }
    std::string read_string() { return buffer_.read_string(); }

    void debug_dump(const std::string& prefix = "[Packet]") const {
        const auto& data = buffer_.data();

        std::ostringstream oss;
        oss << prefix << " Dump (" << data.size() << " bytes): ";

        for (size_t i = 0; i < data.size(); ++i) {
            oss << std::hex << std::setw(2) << std::setfill('0')
                << static_cast<int>(data[i]) << " ";
        }

        spdlog::debug("{}", oss.str());
    }
};
