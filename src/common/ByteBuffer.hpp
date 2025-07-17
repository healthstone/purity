#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <endian.h>
#include <algorithm>

class ByteBuffer {
public:
    ByteBuffer() = default;
    ByteBuffer(const std::vector<uint8_t>& data) : buffer_(data), read_pos_(0) {}

    // ==================== WRITE METHODS ====================

    // ---------- Big-Endian ----------
    void write_uint8_be(uint8_t value) { buffer_.push_back(value); }
    void write_int8_be(int8_t value) { buffer_.push_back(static_cast<uint8_t>(value)); }

    void write_uint16_be(uint16_t value) {
        uint16_t be = htobe16(value);
        append(reinterpret_cast<uint8_t*>(&be), sizeof(be));
    }
    void write_int16_be(int16_t value) {
        write_uint16_be(static_cast<uint16_t>(value));
    }

    void write_uint32_be(uint32_t value) {
        uint32_t be = htobe32(value);
        append(reinterpret_cast<uint8_t*>(&be), sizeof(be));
    }
    void write_int32_be(int32_t value) {
        write_uint32_be(static_cast<uint32_t>(value));
    }

    void write_uint64_be(uint64_t value) {
        uint64_t be = htobe64(value);
        append(reinterpret_cast<uint8_t*>(&be), sizeof(be));
    }
    void write_int64_be(int64_t value) {
        write_uint64_be(static_cast<uint64_t>(value));
    }

    void write_float_be(float value) {
        static_assert(sizeof(float) == sizeof(uint32_t), "Float size mismatch");
        uint32_t int_val;
        std::memcpy(&int_val, &value, sizeof(int_val));
        write_uint32_be(int_val);
    }

    void write_double_be(double value) {
        static_assert(sizeof(double) == sizeof(uint64_t), "Double size mismatch");
        uint64_t int_val;
        std::memcpy(&int_val, &value, sizeof(int_val));
        write_uint64_be(int_val);
    }

    // ---------- Little-Endian ----------
    void write_uint8_le(uint8_t value) { buffer_.push_back(value); }
    void write_int8_le(int8_t value) { buffer_.push_back(static_cast<uint8_t>(value)); }

    void write_uint16_le(uint16_t value) {
        uint16_t le = htole16(value);
        append(reinterpret_cast<uint8_t*>(&le), sizeof(le));
    }
    void write_int16_le(int16_t value) {
        write_uint16_le(static_cast<uint16_t>(value));
    }

    void write_uint32_le(uint32_t value) {
        uint32_t le = htole32(value);
        append(reinterpret_cast<uint8_t*>(&le), sizeof(le));
    }
    void write_int32_le(int32_t value) {
        write_uint32_le(static_cast<uint32_t>(value));
    }

    void write_uint64_le(uint64_t value) {
        uint64_t le = htole64(value);
        append(reinterpret_cast<uint8_t*>(&le), sizeof(le));
    }
    void write_int64_le(int64_t value) {
        write_uint64_le(static_cast<uint64_t>(value));
    }

    void write_float_le(float value) {
        static_assert(sizeof(float) == sizeof(uint32_t), "Float size mismatch");
        uint32_t int_val;
        std::memcpy(&int_val, &value, sizeof(int_val));
        write_uint32_le(int_val);
    }

    void write_double_le(double value) {
        static_assert(sizeof(double) == sizeof(uint64_t), "Double size mismatch");
        uint64_t int_val;
        std::memcpy(&int_val, &value, sizeof(int_val));
        write_uint64_le(int_val);
    }

    // ---------- Boolean ----------
    void write_bool(bool value) {
        buffer_.push_back(value ? 1 : 0);
    }

    // ---------- Strings ----------
    void write_string_nt(const std::string& str) {
        append(reinterpret_cast<const uint8_t*>(str.c_str()), str.size());
        write_uint8_be(0x00); // Null-terminated
    }

    void write_string_raw(const std::string& str) {
        append(reinterpret_cast<const uint8_t*>(str.c_str()), str.size());
    }

    // ==================== READ METHODS ====================

    // ---------- Big-Endian ----------
    uint8_t read_uint8_be() {
        check_read(sizeof(uint8_t));
        return buffer_[read_pos_++];
    }
    int8_t read_int8_be() {
        return static_cast<int8_t>(read_uint8_be());
    }

    uint16_t read_uint16_be() {
        check_read(sizeof(uint16_t));
        uint16_t be;
        std::memcpy(&be, &buffer_[read_pos_], sizeof(be));
        read_pos_ += sizeof(be);
        return be16toh(be);
    }
    int16_t read_int16_be() {
        return static_cast<int16_t>(read_uint16_be());
    }

    uint32_t read_uint32_be() {
        check_read(sizeof(uint32_t));
        uint32_t be;
        std::memcpy(&be, &buffer_[read_pos_], sizeof(be));
        read_pos_ += sizeof(be);
        return be32toh(be);
    }
    int32_t read_int32_be() {
        return static_cast<int32_t>(read_uint32_be());
    }

    uint64_t read_uint64_be() {
        check_read(sizeof(uint64_t));
        uint64_t be;
        std::memcpy(&be, &buffer_[read_pos_], sizeof(be));
        read_pos_ += sizeof(be);
        return be64toh(be);
    }
    int64_t read_int64_be() {
        return static_cast<int64_t>(read_uint64_be());
    }

    float read_float_be() {
        uint32_t int_val = read_uint32_be();
        float value;
        std::memcpy(&value, &int_val, sizeof(value));
        return value;
    }

    double read_double_be() {
        uint64_t int_val = read_uint64_be();
        double value;
        std::memcpy(&value, &int_val, sizeof(value));
        return value;
    }

    // ---------- Little-Endian ----------
    uint8_t read_uint8_le() {
        check_read(sizeof(uint8_t));
        return buffer_[read_pos_++];
    }
    int8_t read_int8_le() {
        return static_cast<int8_t>(read_uint8_le());
    }

    uint16_t read_uint16_le() {
        check_read(sizeof(uint16_t));
        uint16_t le;
        std::memcpy(&le, &buffer_[read_pos_], sizeof(le));
        read_pos_ += sizeof(le);
        return le16toh(le);
    }
    int16_t read_int16_le() {
        return static_cast<int16_t>(read_uint16_le());
    }

    uint32_t read_uint32_le() {
        check_read(sizeof(uint32_t));
        uint32_t le;
        std::memcpy(&le, &buffer_[read_pos_], sizeof(le));
        read_pos_ += sizeof(le);
        return le32toh(le);
    }
    int32_t read_int32_le() {
        return static_cast<int32_t>(read_uint32_le());
    }

    uint64_t read_uint64_le() {
        check_read(sizeof(uint64_t));
        uint64_t le;
        std::memcpy(&le, &buffer_[read_pos_], sizeof(le));
        read_pos_ += sizeof(le);
        return le64toh(le);
    }
    int64_t read_int64_le() {
        return static_cast<int64_t>(read_uint64_le());
    }

    float read_float_le() {
        uint32_t int_val = read_uint32_le();
        float value;
        std::memcpy(&value, &int_val, sizeof(value));
        return value;
    }

    double read_double_le() {
        uint64_t int_val = read_uint64_le();
        double value;
        std::memcpy(&value, &int_val, sizeof(value));
        return value;
    }

    // ---------- Boolean ----------
    bool read_bool() {
        uint8_t val = read_uint8_be();
        return val != 0;
    }

    // ---------- Strings ----------
    std::string read_string_nt() {
        std::string str;
        while (read_pos_ < buffer_.size()) {
            char c = static_cast<char>(buffer_[read_pos_++]);
            if (c == '\0') break;
            str += c;
        }
        return str;
    }

    std::string read_string_raw(size_t length) {
        check_read(length);
        std::string str(reinterpret_cast<const char*>(&buffer_[read_pos_]), length);
        read_pos_ += length;
        return str;
    }

    // ==================== UTILITY METHODS ====================
    const std::vector<uint8_t>& data() const { return buffer_; }
    size_t size() const { return buffer_.size(); }
    size_t read_pos() const { return read_pos_; }
    void skip(size_t bytes) { check_read(bytes); read_pos_ += bytes; }

    void append(const uint8_t* data, size_t size) {
        buffer_.insert(buffer_.end(), data, data + size);
    }

private:
    void check_read(size_t size) const {
        if (read_pos_ + size > buffer_.size()) {
            throw std::out_of_range("ByteBuffer: not enough data to read");
        }
    }

    std::vector<uint8_t> buffer_;
    size_t read_pos_ = 0;
};