#include <catch2/catch.hpp>
#include <iostream>
#include "SRP.hpp"

TEST_CASE("SRP generate_verifier and load_verifier", "[SRP]") {
    SRP srp1;
    std::string salt_hex, verifier_hex;

    srp1.generate_verifier("user", "password", salt_hex, verifier_hex);

    REQUIRE_FALSE(salt_hex.empty());
    REQUIRE_FALSE(verifier_hex.empty());

    SRP srp2;
    srp2.load_verifier(salt_hex, verifier_hex);

    std::string loaded_verifier = srp2.bn_to_hex_str(srp2.get_verifier());
    REQUIRE(loaded_verifier == verifier_hex);
    std::cout << "✅ 'SRP generate_verifier and load_verifier\n";
}

TEST_CASE("SRP generate_server_ephemeral produces non-zero B", "[SRP]") {
    SRP srp;
    std::string salt_hex, verifier_hex;

    srp.generate_verifier("user", "password", salt_hex, verifier_hex);
    srp.load_verifier(salt_hex, verifier_hex);

    srp.generate_server_ephemeral();

    auto B_bytes = srp.get_B_bytes();

    bool all_zero = true;
    for (auto b : B_bytes) {
        if (b != 0) {
            all_zero = false;
            break;
        }
    }

    REQUIRE_FALSE(all_zero);
    std::cout << "✅ 'SRP generate_server_ephemeral produces non-zero B\n";
}

TEST_CASE("SRP get_generator returns 7", "[SRP]") {
    SRP srp;
    uint8_t g = srp.get_generator();
    REQUIRE(g == 7);
    std::cout << "✅ 'SRP get_generator returns 7\n";
}
