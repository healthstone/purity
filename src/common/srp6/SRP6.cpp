#include "SRP6.hpp"
#include <openssl/rand.h>
#include <cstring>

SRP6::SRP6() {
    bn_ctx_ = BN_CTX_new();
    v_ = BN_new();
    b_ = BN_new();
    B_ = BN_new();
    N_ = BN_new();
    g_ = BN_new();
    k_ = BN_new();

    init_constants();
}

SRP6::~SRP6() {
    BN_free(v_);
    BN_free(b_);
    BN_free(B_);
    BN_free(N_);
    BN_free(g_);
    BN_free(k_);
    BN_CTX_free(bn_ctx_);
}

void SRP6::init_constants() {
    static const char* N_hex =
            "B79B3E2A878DBEDF5AD53B139A99D8CEC8DA65FED1BD845FB70F6051754B8D36805B97F93AAF3A6E6F4034F7B182B4D8768B7BHF3522698FA5F9E3CB4060A1F55D06C19DE577C6AAF7E429A775A39076905EF9AD0C7BC620CD35406C5744AC492C9C6F69FE5258CD40E7BD4F2CB2A575DBCC0C58F3C1530527E8DBFBAAAE36F";
    BN_hex2bn(&N_, N_hex);
    BN_set_word(g_, 7);

    // k = H(N | g)
    k_ = calculate_k();
}

BIGNUM* SRP6::calculate_k() const {
    std::vector<uint8_t> N_bin(BN_num_bytes(N_));
    std::vector<uint8_t> g_bin(BN_num_bytes(g_));

    BN_bn2bin(N_, N_bin.data());
    BN_bn2bin(g_, g_bin.data());

    std::vector<uint8_t> to_hash;
    to_hash.insert(to_hash.end(), N_bin.begin(), N_bin.end());
    to_hash.insert(to_hash.end(), g_bin.begin(), g_bin.end());

    uint8_t hash[SHA_DIGEST_LENGTH];
    SHA1(to_hash.data(), to_hash.size(), hash);

    BIGNUM* result = BN_new();
    BN_bin2bn(hash, SHA_DIGEST_LENGTH, result);
    return result;
}

void SRP6::load_verifier(const std::vector<uint8_t>& salt, const std::vector<uint8_t>& verifier) {
    salt_ = salt;
    BN_bin2bn(verifier.data(), verifier.size(), v_);
}

void SRP6::generate_server_ephemeral() {
    uint8_t rand_bytes[32];
    RAND_bytes(rand_bytes, sizeof(rand_bytes));
    BN_bin2bn(rand_bytes, sizeof(rand_bytes), b_);

    BIGNUM* gb = BN_new();
    BIGNUM* kv = BN_new();

    // g^b mod N
    BN_mod_exp(gb, g_, b_, N_, bn_ctx_);
    // k * v mod N
    BN_mod_mul(kv, k_, v_, N_, bn_ctx_);

    // B = kv + g^b mod N
    BN_mod_add(B_, kv, gb, N_, bn_ctx_);

    BN_free(gb);
    BN_free(kv);
}

std::vector<uint8_t> SRP6::get_B_bytes() const {
    std::vector<uint8_t> buf(BN_num_bytes(B_));
    BN_bn2bin(B_, buf.data());
    return buf;
}

std::vector<uint8_t> SRP6::get_N_bytes() const {
    std::vector<uint8_t> buf(BN_num_bytes(N_));
    BN_bn2bin(N_, buf.data());
    return buf;
}

std::vector<uint8_t> SRP6::get_salt_bytes() const {
    return salt_;
}

uint8_t SRP6::get_generator() const {
    return static_cast<uint8_t>(BN_get_word(g_));
}
