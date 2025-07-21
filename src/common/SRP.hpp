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

    // Генерация соли и верификатора по имени пользователя и паролю
    void generate_verifier(const std::string& username, const std::string& password,
                           std::string& out_salt_hex, std::string& out_verifier_hex);

    // Загрузка верификатора и соли из hex-строк
    void load_verifier(const std::string& salt_hex, const std::string& verifier_hex);

    // Генерация серверного эфемерного ключа B
    void generate_server_ephemeral();

    // Обработка публичного ключа клиента A (hex)
    void process_client_public(const std::string& A_hex);

    // Проверка доказательства клиента M1 (hex)
    bool verify_proof(const std::string& client_M1_hex);

    // Генерация фейкового вызова для защиты от атак
    void generate_fake_challenge();

    // Генерация верификатора из доказательства клиента (например, для восстановления)
    void generate_verifier_from_proof(const std::string& A_hex, const std::string& client_M1_hex,
                                      std::string& out_salt_hex, std::string& out_verifier_hex);

    // Преобразование BIGNUM в hex-строку
    std::string bn_to_hex_str(const BIGNUM* bn) const;

    // Статический метод для конвертации массива байт в hex-строку
    static std::string bytes_to_hex(const unsigned char* bytes, size_t len);

    // Геттеры для ключевых параметров
    const BIGNUM* get_A() const { return A; }
    const BIGNUM* get_B() const { return B; }
    const BIGNUM* get_verifier() const { return v; }
    const std::string& get_salt() const { return salt; }

    // Установка A из байтового вектора
    void set_A_from_bytes(const std::vector<uint8_t>& bytes) {
        if (A) {
            BN_free(A);
            A = nullptr;
        }
        A = BN_bin2bn(bytes.data(), static_cast<int>(bytes.size()), nullptr);
    }

private:
    // Вспомогательная функция SHA1 хеширования строки
    void hash_sha1(const std::string& input, unsigned char output[SHA_DIGEST_LENGTH]) const;

    BN_CTX* ctx = nullptr;

    BIGNUM* N = nullptr;  // Модуль (общий параметр)
    BIGNUM* g = nullptr;  // Генератор (общий параметр)
    BIGNUM* v = nullptr;  // Верификатор
    BIGNUM* b = nullptr;  // Серверный секретный эфемерный ключ
    BIGNUM* B = nullptr;  // Серверный публичный эфемерный ключ
    BIGNUM* A = nullptr;  // Клиентский публичный ключ
    BIGNUM* u = nullptr;  // Скремблер
    BIGNUM* S = nullptr;  // Общий секретный ключ

    std::string salt;    // Соль в hex-формате
};
