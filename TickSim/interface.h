#pragma once

#include "character.h"

#include <vector>

class ISpell {
  public:
    virtual void cast(Character& ch) = 0;
};

class IDamage {
  public:
    std::vector<float> damage_stamp;

    IDamage(): damage_stamp(300, 0) {}
    ~IDamage() {}

    virtual float damage() const = 0;

    inline void deal(float damage) {
        damage_stamp.push_back(damage);
        if (damage_stamp.size() > 300)
            damage_stamp.erase(damage_stamp.begin());
    }
};

class IChannel {
  public:
    virtual void channel() = 0;
    virtual void release() = 0;
};
