#pragma once

#include <vector>

class Character {
  public:
    int cast_inc = 379;
    int action_speed = 0;
    int mana_max = 10;
    float mana_regen = 111;

    float mana = 10;
    float cost_mult = 1;

    std::vector<float> mana_stamp;

    Character() : mana_stamp(300, 0) {
    }

    void record_mana()
    {
        mana_stamp.push_back(mana);
        if (mana_stamp.size() > 300)
            mana_stamp.erase(mana_stamp.begin());
    }

    void reset()
    {
        mana = mana_max;
    }
};
