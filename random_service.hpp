#pragma once

#include <algorithm>
#include <cstdint>
#include <random>
#include <vector>

class RandomService {
public:
    RandomService()
        : fixedSeed(false),
          seedValue(static_cast<std::uint32_t>(std::random_device()())),
          engine(seedValue) {
    }

    explicit RandomService(std::uint32_t seed)
        : fixedSeed(true),
          seedValue(seed),
          engine(seedValue) {
    }

    int uniformInt(int minValue, int maxValue) {
        std::uniform_int_distribution<int> distribution(minValue, maxValue);
        return distribution(engine);
    }

    int roll10() {
        return uniformInt(1, 10);
    }

    template <typename T>
    void shuffle(std::vector<T>& values) {
        if (values.size() < 2) {
            return;
        }
        std::shuffle(values.begin(), values.end(), engine);
    }

    static bool isOdd(int value) {
        return (value % 2) != 0;
    }

    static bool isEven(int value) {
        return (value % 2) == 0;
    }

    static bool inRange(int value, int minValue, int maxValue) {
        return value >= minValue && value <= maxValue;
    }

    bool usesFixedSeed() const {
        return fixedSeed;
    }

    std::uint32_t seed() const {
        return seedValue;
    }

private:
    bool fixedSeed;
    std::uint32_t seedValue;
    std::mt19937 engine;
};
