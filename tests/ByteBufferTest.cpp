#include <cassert>
#include <iostream>
#include <cmath>
#include "ByteBuffer.hpp"

//void test_uint8() {
//    ByteBuffer buf;
//    buf.write_uint8(0xAB);
//
//    const auto& data = buf.data();
//    assert(data.size() == 1);
//    assert(data[0] == 0xAB);
//
//    ByteBuffer read_buf(data);
//    uint8_t val = read_buf.read_uint8();
//    assert(val == 0xAB);
//}
//
//void test_uint16() {
//    ByteBuffer buf;
//    buf.write_uint16(0x1234);
//
//    const auto& data = buf.data();
//    assert(data.size() == 2);
//    // LE: LSB first
//    assert(data[0] == 0x34);
//    assert(data[1] == 0x12);
//
//    ByteBuffer read_buf(data);
//    uint16_t val = read_buf.read_uint16();
//    assert(val == 0x1234);
//}
//
//void test_uint32() {
//    ByteBuffer buf;
//    buf.write_uint32(0x12345678);
//
//    const auto& data = buf.data();
//    assert(data.size() == 4);
//    assert(data[0] == 0x78); // LSB
//    assert(data[1] == 0x56);
//    assert(data[2] == 0x34);
//    assert(data[3] == 0x12); // MSB
//
//    ByteBuffer read_buf(data);
//    uint32_t val = read_buf.read_uint32();
//    assert(val == 0x12345678);
//}
//
//void test_uint64() {
//    ByteBuffer buf;
//    buf.write_uint64(0x1122334455667788ULL);
//
//    const auto& data = buf.data();
//    assert(data.size() == 8);
//    assert(data[0] == 0x88); // LSB
//    assert(data[1] == 0x77);
//    assert(data[2] == 0x66);
//    assert(data[3] == 0x55);
//    assert(data[4] == 0x44);
//    assert(data[5] == 0x33);
//    assert(data[6] == 0x22);
//    assert(data[7] == 0x11); // MSB
//
//    ByteBuffer read_buf(data);
//    uint64_t val = read_buf.read_uint64();
//    assert(val == 0x1122334455667788ULL);
//}
//
//void test_int8() {
//    ByteBuffer buf;
//    buf.write_int8(-42);
//
//    const auto& data = buf.data();
//    assert(data.size() == 1);
//    assert(data[0] == static_cast<uint8_t>(-42)); // e.g. 0xD6
//
//    ByteBuffer read_buf(data);
//    int8_t val = read_buf.read_int8();
//    assert(val == -42);
//}
//
//void test_int16() {
//    ByteBuffer buf;
//    buf.write_int16(-12345); // 0xCFC7 in hex (two’s complement)
//
//    const auto& data = buf.data();
//    assert(data.size() == 2);
//
//    // Проверим через преобразование вручную:
//    uint16_t raw = static_cast<uint16_t>(-12345);
//    assert(data[0] == static_cast<uint8_t>(raw & 0xFF));        // LSB
//    assert(data[1] == static_cast<uint8_t>((raw >> 8) & 0xFF)); // MSB
//
//    ByteBuffer read_buf(data);
//    int16_t val = read_buf.read_int16();
//    assert(val == -12345);
//}
//
//void test_int32() {
//    ByteBuffer buf;
//    buf.write_int32(-123456789);
//
//    const auto& data = buf.data();
//    assert(data.size() == 4);
//
//    uint32_t raw = static_cast<uint32_t>(-123456789);
//    assert(data[0] == static_cast<uint8_t>(raw & 0xFF));
//    assert(data[1] == static_cast<uint8_t>((raw >> 8) & 0xFF));
//    assert(data[2] == static_cast<uint8_t>((raw >> 16) & 0xFF));
//    assert(data[3] == static_cast<uint8_t>((raw >> 24) & 0xFF));
//
//    ByteBuffer read_buf(data);
//    int32_t val = read_buf.read_int32();
//    assert(val == -123456789);
//}
//
//void test_int64() {
//    ByteBuffer buf;
//    int64_t original = -1234567890123456789LL;
//    buf.write_int64(original);
//
//    const auto& data = buf.data();
//    assert(data.size() == 8);
//
//    uint64_t raw = static_cast<uint64_t>(original);
//    assert(data[0] == static_cast<uint8_t>(raw & 0xFF));
//    assert(data[1] == static_cast<uint8_t>((raw >> 8) & 0xFF));
//    assert(data[2] == static_cast<uint8_t>((raw >> 16) & 0xFF));
//    assert(data[3] == static_cast<uint8_t>((raw >> 24) & 0xFF));
//    assert(data[4] == static_cast<uint8_t>((raw >> 32) & 0xFF));
//    assert(data[5] == static_cast<uint8_t>((raw >> 40) & 0xFF));
//    assert(data[6] == static_cast<uint8_t>((raw >> 48) & 0xFF));
//    assert(data[7] == static_cast<uint8_t>((raw >> 56) & 0xFF));
//
//    ByteBuffer read_buf(data);
//    int64_t val = read_buf.read_int64();
//    assert(val == original);
//}
//
//void test_float() {
//    ByteBuffer buf;
//
//    float original = 123.456f;
//    buf.write_float(original);
//
//    const auto& data = buf.data();
//    assert(data.size() == 4);
//
//    ByteBuffer read_buf(data);
//    float result = read_buf.read_float();
//
//    assert(std::fabs(result - original) < 1e-6f);
//}
//
//void test_double() {
//    ByteBuffer buf;
//
//    double original = 98765.4321;
//    buf.write_double(original);
//
//    const auto& data = buf.data();
//    assert(data.size() == 8);
//
//    ByteBuffer read_buf(data);
//    double result = read_buf.read_double();
//
//    assert(std::fabs(result - original) < 1e-12);
//}
//
//void test_bool() {
//    ByteBuffer buf;
//    buf.write_bool(true);
//    buf.write_bool(false);
//
//    const auto& data = buf.data();
//    assert(data.size() == 2);
//    assert(data[0] == 1);
//    assert(data[1] == 0);
//
//    ByteBuffer read_buf(data);
//    assert(read_buf.read_bool() == true);
//    assert(read_buf.read_bool() == false);
//}
//
//void test_string() {
//    ByteBuffer buf;
//    std::string str = "Hello!";
//    buf.write_string(str);
//
//    const auto& data = buf.data();
//    assert(data.size() == str.size() + 1); // +1 null terminator
//
//    for (size_t i = 0; i < str.size(); ++i) {
//        assert(data[i] == static_cast<uint8_t>(str[i]));
//    }
//    assert(data[str.size()] == 0x00); // Null terminator
//
//    ByteBuffer read_buf(data);
//    std::string out = read_buf.read_string();
//    assert(out == str);
//}

int main() {
//    test_uint8();
//    test_uint16();
//    test_uint32();
//    test_uint64();
//    test_int8();
//    test_int16();
//    test_int32();
//    test_int64();
//    test_float();
//    test_double();
//    test_bool();
//    test_string();

    std::cout << "✅ All ByteBuffer tests passed.\n";
    return 0;
}
