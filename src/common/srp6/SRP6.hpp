#pragma once

#include <vector>
#include <cstdint>
#include <openssl/bn.h>
#include <openssl/sha.h>

class SRP6 {
public:
    SRP6();
    ~SRP6();

    void load_verifier(const std::vector<uint8_t>& salt, const std::vector<uint8_t>& verifier);
    void generate_server_ephemeral();

    std::vector<uint8_t> get_B_bytes() const;
    std::vector<uint8_t> get_N_bytes() const;
    std::vector<uint8_t> get_salt_bytes() const;
    uint8_t get_generator() const;

private:
    std::vector<uint8_t> salt_;

    BIGNUM* v_ = nullptr;   // verifier
    BIGNUM* b_ = nullptr;   // серверный секрет
    BIGNUM* B_ = nullptr;   // публичный ключ B

    BIGNUM* N_ = nullptr;   // прайм
    BIGNUM* g_ = nullptr;   // генератор
    BIGNUM* k_ = nullptr;   // множитель

    BN_CTX* bn_ctx_ = nullptr;

    void init_constants();
    BIGNUM* calculate_k() const;
};
