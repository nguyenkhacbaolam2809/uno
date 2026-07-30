#ifndef RNG_H
#define RNG_H

#include <random>

inline std::mt19937 & rng()
{
    static std::mt19937 instance(std::random_device{}());
    return instance;
}

inline int randomInt(int min, int max) noexcept
{
    std::uniform_int_distribution<int> dist(min, max);
    return dist(rng());
}

#endif
