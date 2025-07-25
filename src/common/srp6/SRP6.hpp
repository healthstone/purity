#pragma once

#include <vector>
#include <cstdint>
#include <openssl/bn.h>
#include <openssl/sha.h>
#include <string>

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

    // --- Для клиента ---
    void load_constants(const std::vector<uint8_t>& N, uint8_t g);
    void load_salt(const std::vector<uint8_t>& salt);
    void set_credentials(const std::string& username, const std::string& password);
    void generate_client_ephemeral();

    const std::vector<uint8_t>& get_last_M1() const;
    std::vector<uint8_t> get_A_bytes() const;
    std::vector<uint8_t> compute_M1(const std::vector<uint8_t>& B_bytes);
    bool verify_client_proof(const std::vector<uint8_t>& A_bytes, const std::vector<uint8_t>& M1_client, std::vector<uint8_t>& M2);
    bool verify_server_proof(const std::vector<uint8_t>& last_M1, const std::vector<uint8_t>& M2_server);

private:
    void init_constants();
    BIGNUM* calculate_k() const;

    std::vector<uint8_t> salt_;
    std::string username_;
    std::string password_;

    BIGNUM* v_ = nullptr;
    BIGNUM* b_ = nullptr;
    BIGNUM* B_ = nullptr;
    BIGNUM* N_ = nullptr;
    BIGNUM* g_ = nullptr;
    BIGNUM* k_ = nullptr;
    BIGNUM* a_ = nullptr;
    BIGNUM* A_ = nullptr;

    BN_CTX* bn_ctx_ = nullptr;

    std::vector<uint8_t> last_M1_;
};
