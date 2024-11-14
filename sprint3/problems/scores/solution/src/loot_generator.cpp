#include "loot_generator.h"

#include <algorithm>
#include <cmath>
#include <random>

namespace loot_gen {

double GenerateRandomBase() {
    static std::random_device rd; // Источник энтропии
    static std::mt19937 generator(rd()); // Генератор случайных чисел
    static std::uniform_real_distribution<double> distribution(0.0, 1.0); // Диапазон от 0 до 1

    return distribution(generator);
}

unsigned GenerateRandomUnsigned(const unsigned min, const unsigned max) {
    return min + static_cast<unsigned>((max - min + 1) * GenerateRandomBase());
}

double GenerateRandomDouble(const double min, const double max) {
    return min + (max - min) * GenerateRandomBase();
}

LootGenerator::LootGenerator(TimeInterval base_interval, double probability, RandomGenerator random_gen): base_interval_{base_interval}
    , probability_{probability}
    , random_generator_{std::move(random_gen)} {
}

unsigned LootGenerator::Generate(TimeInterval time_delta, unsigned loot_count,
                                 unsigned looter_count) {
    time_without_loot_ += time_delta;
    const unsigned loot_shortage = loot_count > looter_count ? 0u : looter_count - loot_count;
    const double ratio = std::chrono::duration<double>{time_without_loot_} / base_interval_;
    const double probability
        = std::clamp((1.0 - std::pow(1.0 - probability_, ratio)) * random_generator_(), 0.0, 1.0);
    const unsigned generated_loot = static_cast<unsigned>(std::round(loot_shortage * probability));
    if (generated_loot > 0) {
        time_without_loot_ = {};
    }
    return generated_loot;
}

LootGenerator::TimeInterval LootGenerator::GetTimeInterval() const {
    return base_interval_;
}

double LootGenerator::GetProbability() const {
    return probability_;
}

double LootGenerator::DefaultGenerator() noexcept {
    return 1.0;
}
} // namespace loot_gen
