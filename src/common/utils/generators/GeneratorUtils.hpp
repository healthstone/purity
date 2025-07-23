#pragma once

#include <random>
#include <cstdint>

class GeneratorUtils {
public:
    static uint8_t random_uint8();
    static uint16_t random_uint16();
    static uint32_t random_uint32();
    static uint64_t random_uint64();

    static int8_t random_int8();
    static int16_t random_int16();
    static int32_t random_int32();
    static int64_t random_int64();

    static float random_float(float min = 0.0f, float max = 1.0f);
    static double random_double(double min = 0.0, double max = 1.0);

    static bool random_bool();
};
