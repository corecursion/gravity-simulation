// randomize.hh
// Copyright (C) 2023 by Shawn Yarbrough

#pragma once

#include <random>

class Randomize {
    size_t seed_value;
    std::random_device seed;
    std::mt19937 gen;
    std::uniform_int_distribution<std::int64_t> dist;

public:
    Randomize(std::int64_t n1, std::int64_t n2)
       : seed_value(seed())
       , gen(seed_value)
       , dist(n1, n2) {
        std::cout << "seed " << seed_value << std::endl;
    }

    Randomize(std::int64_t n1, std::int64_t n2, size_t seed_value)
       : seed_value(seed_value)
       , gen(seed_value)
       , dist(n1, n2) {
        std::cout << "seed " << seed_value << std::endl;
    }

    std::int64_t get() {
        return dist(gen);
    }
};    // class Randomize
