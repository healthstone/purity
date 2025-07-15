#include <cassert>
#include <iostream>
#include <cmath>
#include "ByteBuffer.hpp"

void test_uint8() {
    ByteBuffer buf;
    buf.write_uint8(0xAB);

    const auto& data = buf.data();
    assert(data.size() == 1);
    assert(data[0] == 0xAB);

    ByteBuffer read_buf(data);
    uint8_t val = read_buf.read_uint8();
    assert(val == 0xAB);
}

void test_uint16() {
    ByteBuffer buf;
    buf.write_uint16(0x1234);

    const auto& data = buf.data();
    assert(data.size() == 2);
    assert(data[0] == 0x12);
    assert(data[1] == 0x34);

    ByteBuffer read_buf(data);
    uint16_t val = read_buf.read_uint16();
    assert(val == 0x1234);
}

void test_uint32() {
    ByteBuffer buf;
    buf.write_uint32(0x12345678);

    const auto& data = buf.data();
    assert(data.size() == 4);
    assert(data[0] == 0x12);
    assert(data[1] == 0x34);
    assert(data[2] == 0x56);
    assert(data[3] == 0x78);

    ByteBuffer read_buf(data);
    uint32_t val = read_buf.read_uint32();
    assert(val == 0x12345678);
}

void test_uint64() {
    ByteBuffer buf;
    buf.write_uint64(0x1122334455667788);

    const auto& data = buf.data();
    assert(data.size() == 8);
    assert(data[0] == 0x11);
    assert(data[1] == 0x22);
    assert(data[2] == 0x33);
    assert(data[3] == 0x44);
    assert(data[4] == 0x55);
    assert(data[5] == 0x66);
    assert(data[6] == 0x77);
    assert(data[7] == 0x88);

    ByteBuffer read_buf(data);
    uint64_t val = read_buf.read_uint64();
    assert(val == 0x1122334455667788);
}

void test_int8() {
    ByteBuffer buf;
    buf.write_int8(-42);

    const auto& data = buf.data();
    assert(data.size() == 1);
    assert(data[0] == static_cast<uint8_t>(-42));  // e.g. 0xD6

    ByteBuffer read_buf(data);
    int8_t val = read_buf.read_int8();
    assert(val == -42);
}

void test_int16() {
    ByteBuffer buf;
    buf.write_int16(-12345);

    const auto& data = buf.data();
    assert(data.size() == 2);
    assert(data[0] == static_cast<uint8_t>((-12345 >> 8) & 0xFF)); // should be 0xCF
    assert(data[1] == static_cast<uint8_t>(-12345 & 0xFF));        // should be 0xC7

    ByteBuffer read_buf(data);
    int16_t val = read_buf.read_int16();
    assert(val == -12345);
}

void test_int32() {
    ByteBuffer buf;
    buf.write_int32(-123456789);

    const auto& data = buf.data();
    assert(data.size() == 4);

    ByteBuffer read_buf(data);
    int32_t val = read_buf.read_int32();
    assert(val == -123456789);
}

void test_int64() {
    ByteBuffer buf;
    buf.write_int64(-1234567890123456789LL);

    const auto& data = buf.data();
    assert(data.size() == 8);

    ByteBuffer read_buf(data);
    int64_t val = read_buf.read_int64();
    assert(val == -1234567890123456789LL);
}

void test_float() {
    ByteBuffer buf;

    float original = 123.456f;
    buf.write_float(original);

    const auto& data = buf.data();
    assert(data.size() == 4);

    ByteBuffer read_buf(data);
    float result = read_buf.read_float();

    assert(std::fabs(result - original) < 1e-6f);
}

void test_double() {
    ByteBuffer buf;

    double original = 98765.4321;
    buf.write_double(original);

    const auto& data = buf.data();
    assert(data.size() == 8);

    ByteBuffer read_buf(data);
    double result = read_buf.read_double();

    assert(std::fabs(result - original) < 1e-12);
}

void test_bool() {
    ByteBuffer buf;
    buf.write_bool(true);
    buf.write_bool(false);

    const auto& data = buf.data();
    assert(data.size() == 2);
    assert(data[0] == 1);
    assert(data[1] == 0);

    ByteBuffer read_buf(data);
    assert(read_buf.read_bool() == true);
    assert(read_buf.read_bool() == false);
}

void test_string() {
    ByteBuffer buf;
    std::string str = "Hello!";
    buf.write_string(str);

    const auto& data = buf.data();
    assert(data.size() == 2 + str.size());
    assert(data[0] == 0x00);
    assert(data[1] == 0x06);
    assert(std::string(data.begin() + 2, data.end()) == "Hello!");

    ByteBuffer read_buf(data);
    std::string out = read_buf.read_string();
    assert(out == "Hello!");
}

int main() {
    test_uint8();
    test_uint16();
    test_uint32();
    test_uint64();
    test_int8();
    test_int16();
    test_int32();
    test_int64();
    test_float();
    test_double();
    test_bool();
    test_string();

    std::cout << "✅ All ByteBuffer tests passed.\n";
    return 0;
}
