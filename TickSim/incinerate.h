#pragma once

#include <vector>
#include <cstdint>

#include "interface.h"

class IncinerateBase : virtual public ISpell, virtual public IChannel, virtual public IDamage {
  protected:
    const float cast_base;
    const uint64_t cost;
    const uint8_t max_stage;
    const float release_multi;

    int stage;
    float next_iter;

    float cast_time;
    float next_cast;

    float damage_in_tick;

  public:
    std::vector<int> stage_stamp;

    IncinerateBase(float cast_base, uint64_t cost, float cast_start, uint8_t max_stage, float release_multi);
    ~IncinerateBase();

    inline float damage() const override { return 50.f + 12.5f * stage; }

    void cast(Character& ch) override;

    void channel() override;
    void release() override;
};

class Incinerate : public IncinerateBase {
  public:
    Incinerate(uint64_t cost, float cast_start, uint8_t quality = 20)
        : IncinerateBase(0.2f, cost, cast_start, 8 + quality / 10, 6) {}
    ~Incinerate() {}
};

class IncinerateExpanse : public IncinerateBase {
  public:
    IncinerateExpanse(uint64_t cost, float cast_start, uint8_t quality = 20)
        : IncinerateBase(0.3f, cost, cast_start, 4, 8 + (float)quality / 20) {}
    ~IncinerateExpanse() {}
};

class IncinerateVenting : public IncinerateBase {
  public:
    IncinerateVenting(uint64_t cost, float cast_start, uint8_t quality = 20)
        : IncinerateBase(0.17f, cost, cast_start, 12 + quality / 10, 0) {}
    ~IncinerateVenting() {}
};
