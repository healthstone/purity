#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <endian.h>  // Linux; на Windows: Winsock2.h или ручной swap

class ByteBuffer {
public:
    ByteBuffer() = default;

    // Для чтения из существующего буфера
    ByteBuffer(const std::vector<uint8_t>& data) : buffer_(data), read_pos_(0) {}

    // --- WRITE ---

    void write_uint8(uint8_t value) { buffer_.push_back(value); }
    void write_uint16(uint16_t value) {
        uint16_t le = htole16(value);
        append(reinterpret_cast<uint8_t*>(&le), sizeof(le));
    }
    void write_uint32(uint32_t value) {
        uint32_t le = htole32(value);
        append(reinterpret_cast<uint8_t*>(&le), sizeof(le));
    }
    void write_uint64(uint64_t value) {
        uint64_t le = htole64(value);
        append(reinterpret_cast<uint8_t*>(&le), sizeof(le));
    }

    void write_int8(int8_t value) { buffer_.push_back(static_cast<uint8_t>(value)); }
    void write_int16(int16_t value) {
        int16_t le = htole16(value);
        append(reinterpret_cast<uint8_t*>(&le), sizeof(le));
    }
    void write_int32(int32_t value) {
        int32_t le = htole32(value);
        append(reinterpret_cast<uint8_t*>(&le), sizeof(le));
    }
    void write_int64(int64_t value) {
        int64_t le = htole64(value);
        append(reinterpret_cast<uint8_t*>(&le), sizeof(le));
    }

    void write_float(float value) {
        static_assert(sizeof(float) == sizeof(uint32_t), "Unexpected float size");
        uint32_t raw;
        std::memcpy(&raw, &value, sizeof(raw));
        write_uint32(raw);
    }

    void write_double(double value) {
        static_assert(sizeof(double) == sizeof(uint64_t), "Unexpected double size");
        uint64_t raw;
        std::memcpy(&raw, &value, sizeof(raw));
        write_uint64(raw);
    }

    void write_bool(bool value) { buffer_.push_back(value ? 1 : 0); }

    void write_string(const std::string& str) {
        append(reinterpret_cast<const uint8_t*>(str.c_str()), str.size());
        write_uint8(0x00); // Null-terminated
    }

    void append(const uint8_t* data, size_t size) {
        buffer_.insert(buffer_.end(), data, data + size);
    }

    // --- READ ---

    uint8_t read_uint8() {
        check_read(sizeof(uint8_t));
        return buffer_[read_pos_++];
    }

    uint16_t read_uint16() {
        check_read(sizeof(uint16_t));
        uint16_t le;
        std::memcpy(&le, &buffer_[read_pos_], sizeof(le));
        read_pos_ += sizeof(le);
        return le16toh(le);
    }

    uint32_t read_uint32() {
        check_read(sizeof(uint32_t));
        uint32_t le;
        std::memcpy(&le, &buffer_[read_pos_], sizeof(le));
        read_pos_ += sizeof(le);
        return le32toh(le);
    }

    uint64_t read_uint64() {
        check_read(sizeof(uint64_t));
        uint64_t le;
        std::memcpy(&le, &buffer_[read_pos_], sizeof(le));
        read_pos_ += sizeof(le);
        return le64toh(le);
    }

    int8_t read_int8() {
        return static_cast<int8_t>(read_uint8());
    }

    int16_t read_int16() {
        return static_cast<int16_t>(read_uint16());
    }

    int32_t read_int32() {
        return static_cast<int32_t>(read_uint32());
    }

    int64_t read_int64() {
        return static_cast<int64_t>(read_uint64());
    }

    float read_float() {
        uint32_t raw = read_uint32();
        float value;
        std::memcpy(&value, &raw, sizeof(value));
        return value;
    }

    double read_double() {
        uint64_t raw = read_uint64();
        double value;
        std::memcpy(&value, &raw, sizeof(value));
        return value;
    }

    bool read_bool() {
        return read_uint8() != 0;
    }

    std::string read_string() {
        std::string str;
        while (read_pos_ < buffer_.size()) {
            char c = static_cast<char>(buffer_[read_pos_++]);
            if (c == '\0') break;
            str += c;
        }
        return str;
    }

    // --- ACCESS ---

    const std::vector<uint8_t>& data() const { return buffer_; }
    size_t size() const { return buffer_.size(); }
    size_t read_pos() const { return read_pos_; }

private:
    void check_read(size_t size) const {
        if (read_pos_ + size > buffer_.size()) {
            throw std::out_of_range("ByteBuffer: not enough data to read");
        }
    }

    std::vector<uint8_t> buffer_;
    size_t read_pos_ = 0;
};
