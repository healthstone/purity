#include "GeneratorUtils.hpp"

namespace {
    // Один генератор на все вызовы
    std::random_device rd;
    std::mt19937 gen(rd());
}

uint8_t GeneratorUtils::random_uint8() {
    std::uniform_int_distribution<uint16_t> dist(0, UINT8_MAX);
    return static_cast<uint8_t>(dist(gen));
}

uint16_t GeneratorUtils::random_uint16() {
    std::uniform_int_distribution<uint16_t> dist(0, UINT16_MAX);
    return dist(gen);
}

uint32_t GeneratorUtils::random_uint32() {
    std::uniform_int_distribution<uint32_t> dist;
    return dist(gen);
}

uint64_t GeneratorUtils::random_uint64() {
    std::uniform_int_distribution<uint64_t> dist;
    return dist(gen);
}

int8_t GeneratorUtils::random_int8() {
    std::uniform_int_distribution<int16_t> dist(INT8_MIN, INT8_MAX);
    return static_cast<int8_t>(dist(gen));
}

int16_t GeneratorUtils::random_int16() {
    std::uniform_int_distribution<int16_t> dist(INT16_MIN, INT16_MAX);
    return dist(gen);
}

int32_t GeneratorUtils::random_int32() {
    std::uniform_int_distribution<int32_t> dist(INT32_MIN, INT32_MAX);
    return dist(gen);
}

int64_t GeneratorUtils::random_int64() {
    std::uniform_int_distribution<int64_t> dist(INT64_MIN, INT64_MAX);
    return dist(gen);
}

float GeneratorUtils::random_float(float min, float max) {
    std::uniform_real_distribution<float> dist(min, max);
    return dist(gen);
}

double GeneratorUtils::random_double(double min, double max) {
    std::uniform_real_distribution<double> dist(min, max);
    return dist(gen);
}

bool GeneratorUtils::random_bool() {
    std::bernoulli_distribution dist(0.5);
    return dist(gen);
}
