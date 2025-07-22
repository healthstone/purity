#pragma once

#include <openssl/bn.h>
#include <openssl/sha.h>
#include <openssl/rand.h>
#include <string>
#include <vector>

class SRP {
public:
    SRP();
    ~SRP();

    // Генерация соли и верификатора по имени пользователя и паролю (пароль в открытом виде)
    void generate_verifier(const std::string& username, const std::string& password,
                           std::string& out_salt_hex, std::string& out_verifier_hex);

    void load_verifier(const std::string& salt_hex, const std::string& verifier_hex);

    void generate_server_ephemeral();

    void process_client_public(const std::string& A_hex);

    bool verify_proof(const std::string& client_M1_hex);

    void generate_fake_challenge();

    void generate_verifier_from_proof(const std::string& A_hex, const std::string& client_M1_hex,
                                      std::string& out_salt_hex, std::string& out_verifier_hex);

    std::string bn_to_hex_str(const BIGNUM* bn) const;

    // Возвращает серверный публичный ключ B в байтах (для отправки в пакет)
    std::vector<uint8_t> get_B_bytes() const;

    // Возвращает модуль N в байтах
    std::vector<uint8_t> get_N_bytes() const;

    // Возвращает генератор g (как uint8_t)
    uint8_t get_generator() const;

    // Сеттер для A из байт
    void set_A_from_bytes(const std::vector<uint8_t>& bytes);

    const BIGNUM* get_A() const { return A; }
    const BIGNUM* get_B() const { return B; }
    const BIGNUM* get_verifier() const { return v; }
    const BIGNUM* get_N() const { return N; }
    const std::string& get_salt() const { return salt; }

private:
    void hash_sha1(const std::string& input, unsigned char output[SHA_DIGEST_LENGTH]) const;

    BN_CTX* ctx = nullptr;

    BIGNUM* N = nullptr;
    BIGNUM* g = nullptr;
    BIGNUM* v = nullptr;
    BIGNUM* b = nullptr;
    BIGNUM* B = nullptr;
    BIGNUM* A = nullptr;
    BIGNUM* u = nullptr;
    BIGNUM* S = nullptr;

    std::string salt;
};
