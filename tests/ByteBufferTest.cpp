#include <catch2/catch.hpp>
#include "ByteBuffer.hpp"

TEST_CASE("ByteBuffer Big-Endian read/write") {
    ByteBuffer buf;

    buf.write_uint8_be(0x12);
    buf.write_int8_be(-1);
    buf.write_uint16_be(0x1234);
    buf.write_int16_be(-1234);
    buf.write_uint32_be(0x12345678);
    buf.write_int32_be(-12345678);
    buf.write_uint64_be(0x1122334455667788ULL);
    buf.write_int64_be(-123456789LL);

    buf.write_float_be(3.14f);
    buf.write_double_be(6.28);

    buf.write_bool(true);
    buf.write_bool(false);

    buf.write_string_nt("Hello");
    buf.write_string_raw("World");

    REQUIRE(buf.read_uint8_be() == 0x12);
    REQUIRE(buf.read_int8_be() == -1);
    REQUIRE(buf.read_uint16_be() == 0x1234);
    REQUIRE(buf.read_int16_be() == -1234);
    REQUIRE(buf.read_uint32_be() == 0x12345678);
    REQUIRE(buf.read_int32_be() == -12345678);
    REQUIRE(buf.read_uint64_be() == 0x1122334455667788ULL);
    REQUIRE(buf.read_int64_be() == -123456789LL);

    REQUIRE(buf.read_float_be() == Approx(3.14f));
    REQUIRE(buf.read_double_be() == Approx(6.28));

    REQUIRE(buf.read_bool() == true);
    REQUIRE(buf.read_bool() == false);

    REQUIRE(buf.read_string_nt() == "Hello");
    REQUIRE(buf.read_string_raw(5) == "World");
    std::cout << "✅ 'ByteBuffer Big-Endian read/write' passed.\n";
}

TEST_CASE("ByteBuffer Little-Endian read/write") {
    ByteBuffer buf;

    buf.write_uint8_le(0xAB);
    buf.write_int8_le(-42);
    buf.write_uint16_le(0xABCD);
    buf.write_int16_le(-12345);
    buf.write_uint32_le(0xABCDEF01);
    buf.write_int32_le(-654321);
    buf.write_uint64_le(0x0102030405060708ULL);
    buf.write_int64_le(-987654321LL);

    buf.write_float_le(1.23f);
    buf.write_double_le(4.56);

    REQUIRE(buf.read_uint8_le() == 0xAB);
    REQUIRE(buf.read_int8_le() == -42);
    REQUIRE(buf.read_uint16_le() == 0xABCD);
    REQUIRE(buf.read_int16_le() == -12345);
    REQUIRE(buf.read_uint32_le() == 0xABCDEF01);
    REQUIRE(buf.read_int32_le() == -654321);
    REQUIRE(buf.read_uint64_le() == 0x0102030405060708ULL);
    REQUIRE(buf.read_int64_le() == -987654321LL);

    REQUIRE(buf.read_float_le() == Approx(1.23f));
    REQUIRE(buf.read_double_le() == Approx(4.56));
    std::cout << "✅ 'ByteBuffer Little-Endian read/write' passed.\n";
}

TEST_CASE("ByteBuffer skip and exception") {
    ByteBuffer buf;
    buf.write_uint8_be(0x01);
    buf.write_uint8_be(0x02);
    buf.write_uint8_be(0x03);

    buf.skip(2);
    REQUIRE(buf.read_uint8_be() == 0x03);

    REQUIRE_THROWS_AS(buf.read_uint8_be(), std::out_of_range);
    std::cout << "✅ 'ByteBuffer skip and exception' passed.\n";
}
