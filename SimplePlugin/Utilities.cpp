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
}