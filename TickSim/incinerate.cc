#include "incinerate.h"

IncinerateBase::IncinerateBase(float cast_base, uint64_t cost, float cast_start, uint8_t max_stage, float release_multi)
    : cast_base(cast_base), cost(cost), next_cast(cast_start), stage(-1), max_stage(max_stage), release_multi(release_multi), stage_stamp(300, 0)
{
}

IncinerateBase::~IncinerateBase()
{
}

void IncinerateBase::cast(Character& ch)
{
    uint64_t cost_atm;
    float mana;
    float last_cast;

    cast_time = cast_base / ((float)ch.cast_inc / 100 + 1) / ((float)ch.action_speed / 100 + 1);
    cost_atm = (uint64_t)(cost * ch.cost_mult);
    mana = ch.mana;
    next_iter = next_cast;
    last_cast = 0;
    damage_in_tick = 0;

    while (next_cast <= 0.033) {
        mana = std::min((float)ch.mana_max, mana + ch.mana_regen * next_iter);

        if (mana >= cost_atm) {
            last_cast = next_cast;
            mana -= cost_atm;
            channel();
        } else {
            release();
        }
    }

    deal(damage_in_tick);

    next_cast -= 0.033f;
    mana = std::min((float)ch.mana_max, mana + (0.033f - last_cast) * ch.mana_regen);

    ch.mana = mana;
    stage_stamp.push_back(stage);
    if (stage_stamp.size() > 300)
        stage_stamp.erase(stage_stamp.begin());
}

void IncinerateBase::channel()
{
    if (stage < max_stage)
        stage++;

    damage_in_tick += damage();

    next_iter = stage > 0 ? cast_time : cast_time * 0.58f;
    next_cast += next_iter;
}

void IncinerateBase::release()
{
    if (stage > 0)
        damage_in_tick += release_multi * damage();

    stage = -1;
    next_cast = 0.066f;
}
