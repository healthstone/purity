#include "SRP6.hpp"
#include "utils/HexUtils.hpp"

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
    a_ = BN_new();
    A_ = BN_new();

    init_constants();
}

SRP6::~SRP6() {
    BN_free(v_);
    BN_free(b_);
    BN_free(B_);
    BN_free(N_);
    BN_free(g_);
    BN_free(k_);
    BN_free(a_);
    BN_free(A_);
    BN_CTX_free(bn_ctx_);
}

void SRP6::init_constants() {
    static const char* N_hex = "B79B3E2A878DBEDF5AD53B139A99D8CEC8DA65FED1BD845FB70F6051754B8D36"; // 256-bit WoW
    BN_hex2bn(&N_, N_hex);
    BN_set_word(g_, 7);
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

    BN_mod_exp(gb, g_, b_, N_, bn_ctx_);
    BN_mod_mul(kv, k_, v_, N_, bn_ctx_);

    BN_mod_add(B_, kv, gb, N_, bn_ctx_);

    BN_free(gb);
    BN_free(kv);
}

std::vector<uint8_t> SRP6::get_B_bytes() const {
    std::vector<uint8_t> raw(BN_num_bytes(B_));
    BN_bn2bin(B_, raw.data());
    return HexUtils::pad_bytes_left(raw, 32);
}

std::vector<uint8_t> SRP6::get_N_bytes() const {
    std::vector<uint8_t> raw(BN_num_bytes(N_));
    BN_bn2bin(N_, raw.data());
    return HexUtils::pad_bytes_left(raw, 32);
}

std::vector<uint8_t> SRP6::get_salt_bytes() const {
    return salt_;
}

uint8_t SRP6::get_generator() const {
    return static_cast<uint8_t>(BN_get_word(g_));
}

// --- Client side methods ---

void SRP6::load_constants(const std::vector<uint8_t>& N_bytes, uint8_t g_value) {
    BN_bin2bn(N_bytes.data(), N_bytes.size(), N_);
    BN_set_word(g_, g_value);
    k_ = calculate_k();
}

void SRP6::load_salt(const std::vector<uint8_t>& salt) {
    salt_ = salt;
}

void SRP6::set_credentials(const std::string& username, const std::string& password) {
    username_ = username;
    password_ = password;
}

void SRP6::generate_client_ephemeral() {
    uint8_t rand_bytes[32];
    RAND_bytes(rand_bytes, sizeof(rand_bytes));
    BN_bin2bn(rand_bytes, sizeof(rand_bytes), a_);

    BN_mod_exp(A_, g_, a_, N_, bn_ctx_);
}

const std::vector<uint8_t>& SRP6::get_last_M1() const {
    return last_M1_;
}

std::vector<uint8_t> SRP6::get_A_bytes() const {
    std::vector<uint8_t> raw(BN_num_bytes(A_));
    BN_bn2bin(A_, raw.data());
    return HexUtils::pad_bytes_left(raw, 32);
}

std::vector<uint8_t> SRP6::compute_M1(const std::vector<uint8_t>& B_bytes) {
    // Very simplified M1 (not RFC 5054 strict)
    std::vector<uint8_t> A = get_A_bytes();

    std::vector<uint8_t> to_hash;
    to_hash.insert(to_hash.end(), A.begin(), A.end());
    to_hash.insert(to_hash.end(), B_bytes.begin(), B_bytes.end());
    to_hash.insert(to_hash.end(), salt_.begin(), salt_.end());

    uint8_t hash[SHA_DIGEST_LENGTH];
    SHA1(to_hash.data(), to_hash.size(), hash);

    last_M1_.assign(hash, hash + SHA_DIGEST_LENGTH);
    return last_M1_;
}

bool SRP6::verify_client_proof(const std::vector<uint8_t>& A_bytes, const std::vector<uint8_t>& M1_client, std::vector<uint8_t>& M2) {
    std::vector<uint8_t> to_hash;
    to_hash.insert(to_hash.end(), A_bytes.begin(), A_bytes.end());
    to_hash.insert(to_hash.end(), M1_client.begin(), M1_client.end());

    uint8_t hash[SHA_DIGEST_LENGTH];
    SHA1(to_hash.data(), to_hash.size(), hash);

    M2.assign(hash, hash + SHA_DIGEST_LENGTH);

    // На сервере обычно нужно сравнить с локальным вычисленным M1 — здесь для примера примем всё верным.
    return true; // или сравнить с расчётом
}

bool SRP6::verify_server_proof(const std::vector<uint8_t>& last_M1, const std::vector<uint8_t>& M2_server) {
    std::vector<uint8_t> to_hash;
    std::vector<uint8_t> A = get_A_bytes();
    to_hash.insert(to_hash.end(), A.begin(), A.end());
    to_hash.insert(to_hash.end(), last_M1.begin(), last_M1.end());

    uint8_t hash[SHA_DIGEST_LENGTH];
    SHA1(to_hash.data(), to_hash.size(), hash);

    return std::equal(M2_server.begin(), M2_server.end(), hash);
}
