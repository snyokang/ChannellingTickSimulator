#pragma once

#include <vector>
#include <cstdint>

#include "interface.h"

class FlameblastBase : virtual public ISpell, virtual public IChannel, virtual public IDamage {
  protected:
    const float cast_base;
    const uint64_t cost;
    const uint8_t max_stage;

    int stage;
    float next_iter;

    float cast_time;
    float next_cast;

    float damage_in_tick;

    FlameblastBase(float cast_base, uint64_t cost, float cast_start, uint8_t max_stage);
    ~FlameblastBase();

  public:
    std::vector<int> stage_stamp;

    void cast(Character& ch) override;

    void channel() override;
    void release() override;
};


class Flameblast : public FlameblastBase {
public:
    Flameblast(uint64_t cost, float cast_start, uint8_t quality = 20)
        : FlameblastBase(0.2, cost, cast_start, 10 + quality / 20) {}
    ~Flameblast() {}

    inline float damage() const override { return 90. * (1. + 2.65 * stage); }

};


class FlameblastCelerity : public FlameblastBase {
public:
    FlameblastCelerity(uint64_t cost, float cast_start, uint8_t quality = 20)
        : FlameblastBase(0.25, cost, cast_start, 3) {}
    ~FlameblastCelerity() {}

    inline float damage() const override { return 100. * (1. + 2.65 * stage); }

    void channel() override;
};


class FlameblastContraction : public FlameblastBase {
public:
    FlameblastContraction(uint64_t cost, float cast_start, uint8_t quality = 20)
        : FlameblastBase(0.2, cost, cast_start, 10 + quality / 20) {}
    ~FlameblastContraction() {}

    inline float damage() const override { return 30. * (1. + 7. * stage); }
};
