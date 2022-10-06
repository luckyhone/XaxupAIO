#pragma once
#include "../plugin_sdk/plugin_sdk.hpp"

namespace utilities
{
	int count_enemy_minions_in_range(float range, vector from);
	float get_ap_raw_damage(script_spell* spell, float coef, std::vector<float>);
	bool has_cc(game_object_script enemy);
};