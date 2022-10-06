#include "Utilities.h"

namespace utilities
{
    int count_enemy_minions_in_range(float range, vector from)
    {
        int count = 0;
        for (auto&& t : entitylist->get_enemy_minions())
        {
            if (t->is_valid_target(range, from))
                count++;
        }
        return count;
    }

    float get_ap_raw_damage(script_spell* spell, float coef, std::vector<float> damages)
    {
        float ap = myhero->get_total_ability_power();

        float raw_damage = damages[spell->level() - 1] + (coef * ap);
        return raw_damage;
    }

    bool has_cc(game_object_script enemy)
    {
        if (enemy->is_dead()) return false;
        
        auto buff = enemy->get_buff_by_type({ buff_type::Stun, buff_type::Snare, buff_type::Knockup, buff_type::Asleep, buff_type::Suppression, buff_type::Taunt });
        
        if (buff) return true;
        return false;
    }
}