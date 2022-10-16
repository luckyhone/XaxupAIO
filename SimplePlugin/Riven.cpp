#include "../plugin_sdk/plugin_sdk.hpp"
#include "Riven.h"
#include "Utilities.h"
#include "permashow.hpp"

namespace riven
{
	float get_w_radius();
	float get_aa_range();
	void auto_flash_kill_logic();
	void auto_r_if_about_to_expire();
	void auto_q_if_about_to_expire();
	void auto_r_if_will_hit_enough_enemies();
	void auto_w_if_enough_enemies();
	void auto_w_if_dashing();
	void auto_w_if_killable();
	void auto_w_on_chanelling_spells();
	void cast_r2();
	bool is_killable_with_r_full_combo(game_object_script enemy, float modified);
	float calculate_r_damage(game_object_script enemy, float additional);
	float full_combo_with_r_damage(game_object_script enemy);
	inline void draw_dmg_rl(game_object_script target, float damage, unsigned long color);
	float full_combo_without_r_damage(game_object_script enemy);
	bool is_killable_without_r_full_combo(game_object_script enemy);
	float calculate_q_damage(int stacks, game_object_script enemy);
	float calculate_aa_damage(int stacks, game_object_script enemy);
	float calculate_w_damage(game_object_script enemy);
	void on_network_packet(game_object_script sender, std::uint32_t network_id, pkttype_e type, void* args);
	void on_after_attack(game_object_script target);
	void on_process_spell_cast(game_object_script sender, spell_instance_script spell);
	void on_draw();
	void on_gapcloser(game_object_script sender, antigapcloser::antigapcloser_args* args);
	void on_buff_lose(game_object_script sender, buff_instance_script buff);
	void on_update();
	void cast_r1();

	script_spell* q = nullptr;
	script_spell* w = nullptr;
	script_spell* e = nullptr;
	script_spell* r = nullptr;
	script_spell* flash = nullptr;

	std::vector<float> q_damages = { 15,35,55,75,95 };
	std::vector<float> q_coefs = { 0.45,0.5,0.55,0.60,0.65 };

	std::vector<float> w_damages = { 65,95,125,155,185 };
	float w_coef_bonus = 1.0;

	std::vector<float> r_damages = { 100,150,200 };
	float r_coef_bonus = 0.6;

	bool flash_ks_executing = false;
	bool safe_harass_executing = false;
	bool use_r2 = false;
	bool use_q2_after_r2 = false;
	bool is_dancing = false;
	bool q_canceling_in_progress = false;
	bool r1_casted;
	TreeTab* main_tab = nullptr;

	namespace combo
	{
		TreeEntry* dance_q;
		TreeEntry* flash_ks_burst;
		TreeEntry* dont_cast_r1_if_killable_without;
		TreeEntry* dont_use_e_in_aa_range;
		TreeEntry* e_in_aa_range_unless_hp;
		TreeEntry* auto_q_expire;
	}
	namespace harass
	{
		TreeEntry* switch_to_safe_when_hp;
		TreeEntry* auto_q_expire;
	}
	namespace delays
	{
		TreeEntry* q1_delay;
		TreeEntry* q2_delay;
		TreeEntry* q3_delay;
	}
	namespace automatic
	{
		TreeEntry* auto_r_expire;
		TreeEntry* auto_r_if_hit_x_enemies;
		TreeEntry* auto_r_if_hit_x_enemies_slider;
		TreeEntry* auto_w_gapclose;
		TreeEntry* auto_w_dashing;
		TreeEntry* auto_w_channeling;
		TreeEntry* auto_w_on_multi_enemies;
		TreeEntry* auto_w_on_multi_enemies_slider;
		TreeEntry* auto_w_if_killable;
	}
	namespace laneclear
	{
		TreeEntry* use_q;
		TreeEntry* use_w;
		TreeEntry* use_w_min_minions;
	}
	namespace jungleclear
	{
		TreeEntry* use_q;
		TreeEntry* use_w;
		TreeEntry* use_e;
	}
	namespace flee
	{
		TreeEntry* use_q;
		TreeEntry* use_e;
	}
	namespace draw
	{
		TreeEntry* draw_q;
		TreeEntry* draw_w;
		TreeEntry* draw_e;
		TreeEntry* draw_r;
		TreeEntry* q_color = nullptr;
		TreeEntry* w_color = nullptr;
		TreeEntry* e_color = nullptr;
		TreeEntry* r_color = nullptr;
	}

	void load()
	{
		myhero->print_chat(0x3, "<font color=\"#FFFFFF\">[<b><font color=\"#3F704D\">Riven | XaxupAIO</font></b>]</font><font color=\"#3F704D\">:</font><font color=\"#90EE90\"> Loaded</font>");
		myhero->print_chat(0x3, "<font color=\"#3F704D\"><b>Suggested Prediction: </b><font color=\"#90EE90\">Core</font></font>");
		myhero->print_chat(0x3, "<font color=\"#3F704D\"><b>Enable <font color=\"#762817\">Blackout Activator</font> for best experience!</b></font>");
		myhero->print_chat(0x3, "<font color=\"#3F704D\"><b>Disable<font color=\"#762817\"> Magnet</font> for best experience!</b></font>");

		q = plugin_sdk->register_spell(spellslot::q, 150);
		w = plugin_sdk->register_spell(spellslot::w, 250);
		e = plugin_sdk->register_spell(spellslot::e, 0);
		r = plugin_sdk->register_spell(spellslot::r, 1150);
		r->set_skillshot(0.25f, 100, FLT_MAX, { collisionable_objects::yasuo_wall }, skillshot_type::skillshot_line);

		if (myhero->get_spell(spellslot::summoner1)->get_spell_data()->get_name_hash() == spell_hash("SummonerFlash"))
			flash = plugin_sdk->register_spell(spellslot::summoner1, 400.f);
		else if (myhero->get_spell(spellslot::summoner2)->get_spell_data()->get_name_hash() == spell_hash("SummonerFlash"))
			flash = plugin_sdk->register_spell(spellslot::summoner2, 400.f);

		main_tab = menu->create_tab("riven", "XaxupAIO");
		main_tab->set_assigned_texture(myhero->get_square_icon_portrait());
		{
			main_tab->add_separator(".predSep", "USE CORE PRED");
			auto combo = main_tab->add_tab(myhero->get_model() + ".combo", "Combo");
			{
				combo->add_separator(".comboSep", "Combo");
				combo::dance_q = combo->add_checkbox(".useDanceQ", "Enable Dance Q", false);
				combo::dance_q->set_texture(myhero->get_spell(spellslot::q)->get_icon_texture());
				combo::auto_q_expire = combo->add_checkbox(".comboAutoQExpire", "Auto Q If About To Expire", true);
				combo::dont_cast_r1_if_killable_without = combo->add_checkbox(".dontCastR1", "Don't Use R1 If Killable Without Ult", true);
				combo::dont_cast_r1_if_killable_without->set_texture(myhero->get_spell(spellslot::r)->get_icon_texture());
				combo::dont_use_e_in_aa_range = combo->add_checkbox(".dontUseEInAA", "Don't Use E If Target In AA Range", true);
				combo::dont_use_e_in_aa_range->set_texture(myhero->get_spell(spellslot::e)->get_icon_texture());
				combo::e_in_aa_range_unless_hp = combo->add_slider(".dontUseEInAAUnlessHP", "  ^~ ignore if my hp is lower than", 40, 1,99);
				combo::flash_ks_burst = combo->add_checkbox(".flashKillstealBurst", "Enable Flash Burst (if killable)", true);

				if (myhero->get_spell(spellslot::summoner1)->get_spell_data()->get_name_hash() == spell_hash("SummonerFlash"))
					combo::flash_ks_burst->set_texture(myhero->get_spell(spellslot::summoner1)->get_icon_texture());
				else if (myhero->get_spell(spellslot::summoner2)->get_spell_data()->get_name_hash() == spell_hash("SummonerFlash"))
					combo::flash_ks_burst->set_texture(myhero->get_spell(spellslot::summoner2)->get_icon_texture());

				combo::auto_q_expire->set_texture(myhero->get_spell(spellslot::q)->get_icon_texture());
			}
			auto harass = main_tab->add_tab(myhero->get_model() + ".harass", "Haras");
			{
				harass->add_separator(".harassSep", "Harass");
				harass::switch_to_safe_when_hp = harass->add_slider(".safeHarassSwitchSlider", "Switch To Safe Harass Mode When Hp Below", 50, 1, 99);
				harass::auto_q_expire = harass->add_checkbox(".harassAutoQExpire", "Auto Q If About To Expire", true);
				harass::auto_q_expire->set_texture(myhero->get_spell(spellslot::q)->get_icon_texture());

			}
			auto delays = main_tab->add_tab(myhero->get_model() + ".delays", "Delays");
			{
				delays->add_separator(".delaysNormalSep", "Normal Delays");
				delays->add_separator(".delaysNormalSepInfo", "---DON'T TOUCH UNLESS YOU KNOW HOW TO SET IT---");
				delays::q1_delay = delays->add_slider(".q1Delay", "Q1 Delay (ms)", 34, 21, 40);
				delays::q2_delay = delays->add_slider(".q2Delay", "Q2 Delay (ms)", 34, 21, 40);
				delays::q3_delay = delays->add_slider(".q3Delay", "Q3 Delay (ms)", 48, 40, 55);
			}
			auto automatic = main_tab->add_tab(myhero->get_model() + ".automatic", "Automatic");
			{
				automatic->add_separator(".autoamticSep", "Automatic");
				automatic::auto_w_if_killable = automatic->add_checkbox(".autoWKillable", "Auto W If Killable", true);
				automatic::auto_w_if_killable->set_texture(myhero->get_spell(spellslot::w)->get_icon_texture());
				automatic::auto_w_channeling = automatic->add_checkbox(".autoWChanneling", "Auto W On Channeling Spells", true);
				automatic::auto_w_channeling->set_texture(myhero->get_spell(spellslot::w)->get_icon_texture());
				automatic::auto_w_dashing = automatic->add_checkbox(".autoWDashing", "Auto W On Dashing Spells", false);
				automatic::auto_w_dashing->set_texture(myhero->get_spell(spellslot::w)->get_icon_texture());
				automatic::auto_w_gapclose = automatic->add_checkbox(".autoWGapclose", "Auto W On Gapclose", false);
				automatic::auto_w_gapclose->set_texture(myhero->get_spell(spellslot::w)->get_icon_texture());
				automatic::auto_w_on_multi_enemies = automatic->add_checkbox(".autoWMultiEnemies", "Auto W On Multiple Enemies", true);
				automatic::auto_w_on_multi_enemies->set_texture(myhero->get_spell(spellslot::w)->get_icon_texture());
				automatic::auto_w_on_multi_enemies_slider = automatic->add_slider(".autoWMultiEnemiesSlider", "  ^~ minimum enemies hit", 2, 2, 5);
				automatic::auto_r_expire = automatic->add_checkbox(".autoRIfExpire", "Auto R If About To Expire (if target valid)", true);
				automatic::auto_r_expire->set_texture(myhero->get_spell(spellslot::r)->get_icon_texture());
				automatic::auto_r_if_hit_x_enemies = automatic->add_checkbox(".autoRIfXEnemies", "Auto R If Will Hit X Enemies", true);
				automatic::auto_r_if_hit_x_enemies->set_texture(myhero->get_spell(spellslot::r)->get_icon_texture());
				automatic::auto_r_if_hit_x_enemies_slider = automatic->add_slider(".autoRIfXEnemiesSlider", "  ^~ minimum enemies hit", 4,2,5);
			}
			auto laneclear = main_tab->add_tab(myhero->get_model() + ".laneclear", "Laneclear");
			{
				laneclear->add_separator(".laneSep", "Laneclear");
				laneclear::use_q = laneclear->add_checkbox(".laneclearUseQ", "Use Q", true);
				laneclear::use_q->set_texture(myhero->get_spell(spellslot::q)->get_icon_texture());
				laneclear::use_w = laneclear->add_checkbox(".laneclearUseW", "Use W", true);
				laneclear::use_w->set_texture(myhero->get_spell(spellslot::w)->get_icon_texture());
				laneclear::use_w_min_minions = laneclear->add_slider(".laneclearUseWSlider", "  ^~ minimum minions hit", 3, 1,5);

			}
			auto jungleclear = main_tab->add_tab(myhero->get_model() + ".jungleClear", "Jungleclear");
			{
				jungleclear->add_separator(".jungleSep", "Jungleclear");
				jungleclear::use_q = jungleclear->add_checkbox(".jungleclearUseQ", "Use Q", true);
				jungleclear::use_q->set_texture(myhero->get_spell(spellslot::q)->get_icon_texture());
				jungleclear::use_w = jungleclear->add_checkbox(".jungleclearUseW", "Use W", true);
				jungleclear::use_w->set_texture(myhero->get_spell(spellslot::w)->get_icon_texture());
				jungleclear::use_e = jungleclear->add_checkbox(".jungleclearUseE", "Use E", true);
				jungleclear::use_e->set_texture(myhero->get_spell(spellslot::e)->get_icon_texture());
			}
			auto flee = main_tab->add_tab(myhero->get_model() + ".flee", "Flee");
			{
				flee->add_separator(".fleeSep", "Flee");
				flee::use_q = flee->add_checkbox(myhero->get_model() + ".fleeUseQ", "Use Q", true);
				flee::use_q->set_texture(myhero->get_spell(spellslot::q)->get_icon_texture());				
				flee::use_e = flee->add_checkbox(myhero->get_model() + ".fleeUseE", "Use E", true);
				flee::use_e->set_texture(myhero->get_spell(spellslot::e)->get_icon_texture());
			}
			auto draw = main_tab->add_tab(myhero->get_model() + ".draw", "Drawings");
			{
				draw->add_separator(".drawSep", "Drawings");
				float color[] = { 1.0f, 1.0f, 0.0f, 1.0f };
				draw::draw_w = draw->add_checkbox(myhero->get_model() + ".drawW", "Draw W Range", true);
				draw::draw_e = draw->add_checkbox(myhero->get_model() + ".drawE", "Draw E Range", true);
				draw::draw_r = draw->add_checkbox(myhero->get_model() + ".drawR", "Draw R Range", true);
				draw::w_color = draw->add_colorpick(myhero->get_model() + ".drawWColor", "W Color", color);
				draw::e_color = draw->add_colorpick(myhero->get_model() + ".drawEColor", "E Color", color);
				draw::r_color = draw->add_colorpick(myhero->get_model() + ".drawRColor", "R Color", color);

				draw::draw_w->set_texture(myhero->get_spell(spellslot::w)->get_icon_texture());
				draw::draw_e->set_texture(myhero->get_spell(spellslot::e)->get_icon_texture());
				draw::draw_r->set_texture(myhero->get_spell(spellslot::r)->get_icon_texture());
			}
		}

		antigapcloser::add_event_handler(on_gapcloser);
		event_handler<events::on_after_attack_orbwalker>::add_callback(on_after_attack);
		event_handler<events::on_buff_lose>::add_callback(on_buff_lose);
		event_handler<events::on_update>::add_callback(on_update);
		event_handler<events::on_process_spell_cast>::add_callback(on_process_spell_cast);
		event_handler<events::on_draw>::add_callback(on_draw);
		event_handler<events::on_network_packet>::add_callback(on_network_packet);
	}

	void unload()
	{
		menu->delete_tab(main_tab);

		plugin_sdk->remove_spell(q);
		plugin_sdk->remove_spell(w);
		plugin_sdk->remove_spell(e);
		plugin_sdk->remove_spell(r);

		if (flash)
			plugin_sdk->remove_spell(flash);

		antigapcloser::remove_event_handler(on_gapcloser);
		event_handler<events::on_buff_lose>::remove_handler(on_buff_lose);
		event_handler<events::on_update>::remove_handler(on_update);
		event_handler<events::on_draw>::remove_handler(on_draw);
		event_handler<events::on_after_attack_orbwalker>::remove_handler(on_after_attack);
		event_handler<events::on_process_spell_cast>::remove_handler(on_process_spell_cast);
		event_handler<events::on_network_packet>::remove_handler(on_network_packet);
	}

	void on_update()
	{
		auto_r_if_about_to_expire();

		#pragma region Combo
				if (orbwalker->combo_mode())
				{
					//R2
					if (r1_casted)
					{
						cast_r2();
					}

					//Auto things			
					auto_flash_kill_logic();
					if (combo::auto_q_expire->get_bool()) auto_q_if_about_to_expire();
					auto_r_if_will_hit_enough_enemies();
					auto_w_if_killable();
					auto_w_if_enough_enemies();
					auto_w_on_chanelling_spells();
					auto_w_if_dashing();

					//Logic
					auto target = target_selector->get_target(500, damage_type::physical);

					if (target == nullptr || !target->is_valid_target()) return;

					if (is_dancing)
					{
						auto dance_pos = myhero->get_position().extend(target->get_position(), myhero->get_distance(target->get_position()) + 25);
						orbwalker->move_to(dance_pos);
					}

					//R1
					cast_r1();

					if ((!q->is_ready() || target->get_distance(myhero->get_position()) > get_aa_range()) && e->is_ready() && w->is_ready())
					{
						if (target != nullptr)
							e->cast(target);
					}
					else if (e->is_ready())
					{
						//Target in AA range
						if (target->get_distance(myhero->get_position()) < get_aa_range() && combo::dont_use_e_in_aa_range->get_bool())
						{
							if (myhero->get_health_percent() < combo::e_in_aa_range_unless_hp->get_int())
							{
								if (target != nullptr)
									e->cast(target);
							}
						}
						else
						{
							if (target != nullptr)
								e->cast(target);
						}
					}

					//Gapclose
					if (myhero->get_distance(target) > get_aa_range() && myhero->get_distance(target) < 700 && !flash_ks_executing)
					{
						if (e->is_ready() && q->is_ready())
						{
							scheduler->delay_action(0.325f, [target] { if (target != nullptr && myhero->get_distance(target) > get_aa_range() && myhero->get_distance(target) < 700) { if (q->is_ready()) q->cast(target);  if (e->is_ready()) e->cast(target); } });
						}
						else if (e->is_ready() && !q->is_ready())
						{
							scheduler->delay_action(0.325f, [target] { if (target != nullptr && myhero->get_distance(target) > get_aa_range() && myhero->get_distance(target) < 550) { if (e->is_ready()) e->cast(target); } });
						}
						else if (!e->is_ready() && q->is_ready())
						{
							scheduler->delay_action(0.325f, [target] { if (target != nullptr && myhero->get_distance(target) > get_aa_range() - 10 && myhero->get_distance(target) < 550 && !q_canceling_in_progress) { if (q->is_ready()) q->cast(target); } });
						}
					}
				}
		#pragma endregion
		#pragma region Harass
				if (orbwalker->harass())
				{
					if (harass::auto_q_expire->get_bool()) auto_q_if_about_to_expire();

					auto target = target_selector->get_target(500, damage_type::physical);

					if (target == nullptr || !target->is_valid_target()) return;

					if (myhero->get_health_percent() < harass::switch_to_safe_when_hp->get_int())
					{
						if (q->is_ready() && (q->ammo() == 0 || q->ammo() == 1 || q->ammo() == 2) && w->is_ready() && e->is_ready())
						{

							scheduler->delay_action(0.03f, [target] { if (q->is_ready() && (q->ammo() == 0 || q->ammo() == 1 || q->ammo() == 2) && w->is_ready() && e->is_ready() && target != nullptr && !safe_harass_executing) q->cast(target); safe_harass_executing = true; scheduler->delay_action(1.0f, [] { safe_harass_executing = false; });  });
						}
					}
					else
					{
						if (target == nullptr || !target->is_valid_target()) return;

						if (q->is_ready())
						{
							scheduler->delay_action(0.325f, [target] { if (target != nullptr && myhero->get_distance(target) > 130 && myhero->get_distance(target) < 400 && !q_canceling_in_progress) { if (q->is_ready()) q->cast(target); } });
						}

						if (myhero->count_enemies_in_range(get_w_radius()) >= 1 && w->is_ready())
						{
							if (q->ammo() == 1 || q->ammo() == 2)
								w->cast();
						}
						if (e->is_ready() && !w->is_ready() && !q->is_ready() && q->ammo() < 1)
						{
							vector e_cast_pos = { 0,0 };
							float distance = FLT_MAX;

							for (auto&& turret : entitylist->get_ally_turrets())
							{
								if (turret->get_distance(myhero) < distance)
								{
									e_cast_pos = turret->get_position();
									distance = turret->get_distance(myhero);
								}
							}

							e->cast(e_cast_pos);
						}
					}
				}
		#pragma endregion
		#pragma region Flee
				if (orbwalker->flee_mode())
				{
					if (e->is_ready() && flee::use_e->get_bool())
					{
						e->cast(hud->get_hud_input_logic()->get_game_cursor_position());
					}
					if (q->is_ready() && flee::use_q->get_bool())
					{
						q->cast(hud->get_hud_input_logic()->get_game_cursor_position());
					}
				}
		#pragma endregion
		#pragma region LaneClear/JungleClear

				if (orbwalker->lane_clear_mode())
				{
#pragma region  SortMinions
					auto lane_minions = entitylist->get_enemy_minions();
					auto monsters = entitylist->get_jugnle_mobs_minions();

					lane_minions.erase(std::remove_if(lane_minions.begin(), lane_minions.end(), [](game_object_script x)
						{
							return !x->is_valid_target(q->range());
						}), lane_minions.end());

					monsters.erase(std::remove_if(monsters.begin(), monsters.end(), [](game_object_script x)
						{
							return !x->is_valid_target(q->range());
						}), monsters.end());

					std::sort(lane_minions.begin(), lane_minions.end(), [](game_object_script a, game_object_script b)
						{
							return a->get_position().distance(myhero->get_position()) < b->get_position().distance(myhero->get_position());
						});

					std::sort(monsters.begin(), monsters.end(), [](game_object_script a, game_object_script b)
						{
							return a->get_max_health() > b->get_max_health();
						});

#pragma endregion

					if (!lane_minions.empty())
					{
						if (laneclear::use_w->get_bool() && w->is_ready())
						{
							if (utilities::count_enemy_minions_in_range(get_w_radius(), myhero->get_position()) >= laneclear::use_w_min_minions->get_int())
							{
								w->cast();
							}
						}
					}
					if (!monsters.empty())
					{
						if (jungleclear::use_e->get_bool())
						{
							e->cast(monsters.front());
						}
						if (jungleclear::use_w->get_bool() && monsters.front()->get_distance(myhero) <= get_w_radius())
						{
							w->cast();
						}
					}
				}
		#pragma endregion
	}

	void cast_r1()
	{
		auto target = target_selector->get_target(600, damage_type::physical);

		if (target == nullptr || !target->is_valid_target()) return;

		if (combo::dont_cast_r1_if_killable_without->get_bool() && is_killable_without_r_full_combo(target))
		{
			return;
		}
		else
		{
			if (!r1_casted && r->is_ready() && (is_killable_with_r_full_combo(target, 450)))
			{
				if (e->is_ready())
				{
					if (target != nullptr)
						e->cast(target);
					scheduler->delay_action(0.05f, [target] { if (target != nullptr && r->is_ready()) r->cast(target); });
				}
				else if (q->ammo() == 2 && q->is_ready() && e->is_ready())
				{
					if (target != nullptr)
						e->cast(target);
					scheduler->delay_action(0.05f, [target] { if (target != nullptr && r->is_ready()) r->cast(target); });
				}
				else if (w->is_ready() && myhero->count_enemies_in_range(300) > 0)
				{
					scheduler->delay_action(0.05f, [target] { if (target != nullptr && r->is_ready()) r->cast(target); });
					myhero->issue_order(_issue_order_type::Stop);
					w->cast();
				}
				else
				{
					scheduler->delay_action(0.05f, [target] { if (target != nullptr && r->is_ready()) r->cast(target); });
				}
			}
		}
	}

	void cast_r2()
	{
		auto target = target_selector->get_target(1130, damage_type::physical);

		if (target == nullptr || !target->is_valid_target() || target->is_zombie() || target->is_invulnerable()) return;

		if (q->ammo() == 2 && q->is_ready() && ((calculate_q_damage(1, target) + calculate_r_damage(target, 100) + calculate_aa_damage(2, target)) > target->get_health()))
		{
			myhero->issue_order(_issue_order_type::Stop);

			if (r->is_ready())
				r->cast(target);
			
			//It has to be like that otherwise sometimes it's not casting
			scheduler->delay_action(0.05f, [target] { if (q->is_ready() && target != nullptr) q->cast(target); });
			scheduler->delay_action(0.1f, [target] { if (q->is_ready() && target != nullptr) q->cast(target); });
			scheduler->delay_action(0.15f, [target] { if (q->is_ready() && target != nullptr) q->cast(target); });
			scheduler->delay_action(0.22f, [target] { if (q->is_ready() && target != nullptr) q->cast(target); });
		}
		else if (calculate_r_damage(target, 50) > target->get_health() + calculate_q_damage(2, target) && q->is_ready() && (q->ammo() == 0 || q->ammo() == 1))
		{
			if (r->is_ready())
				r->cast(target);

			//It has to be like that otherwise sometimes it's not casting
			scheduler->delay_action(0.05f, [target] { if (q->is_ready() && target != nullptr) q->cast(target); });
			scheduler->delay_action(0.1f, [target] { if (q->is_ready() && target != nullptr) q->cast(target); });
			scheduler->delay_action(0.15f, [target] { if (q->is_ready() && target != nullptr) q->cast(target); });
			scheduler->delay_action(0.22f, [target] { if (q->is_ready() && target != nullptr) q->cast(target); });
		}
		else if (calculate_r_damage(target, 50) > target->get_health() && !w->is_ready() && !q->is_ready() && !e->is_ready())
		{
			if (r->is_ready())
				r->cast(target);
		}
		else if ((calculate_r_damage(target, 10) + calculate_aa_damage(1, target)) > target->get_health() && !w->is_ready() && !q->is_ready() && e->is_ready())
		{
			myhero->issue_order(_issue_order_type::Stop);
			e->cast(target);
			myhero->issue_order(_issue_order_type::Stop);
			if (r->is_ready())
				r->cast(target);
		}
		else if (w->is_ready() && target->is_valid_target(get_w_radius()) && ((calculate_w_damage(target) + calculate_r_damage(target, 50) + calculate_aa_damage(1, target)) > target->get_health()))
		{
			myhero->issue_order(_issue_order_type::Stop);
			if (r->is_ready())
				r->cast(target);
			myhero->issue_order(_issue_order_type::Stop);
			w->cast();
		}
		else if (calculate_r_damage(target, 10) > target->get_health())
		{
			if (r->is_ready())
				r->cast(target);
		}
	}


	void on_after_attack(game_object_script target)
	{
		if (target == nullptr || myhero->is_dead()) return;

		if (orbwalker->lane_clear_mode())
		{
			if (target->is_lane_minion() && laneclear::use_q->get_bool() && target != nullptr)
				q->cast(target->get_position());
			else if (target->is_monster() && jungleclear::use_q->get_bool() && target != nullptr)
				q->cast(target->get_position());
		}

		if (orbwalker->harass() && target->is_ai_hero())
		{
			if (safe_harass_executing)
			{
				vector e_cast_pos = { 0,0 };
				float distance = FLT_MAX;

				for (auto&& turret : entitylist->get_ally_turrets())
				{
					if (turret->get_distance(myhero) < distance)
					{
						e_cast_pos = turret->get_position();
						distance = turret->get_distance(myhero);
					}
				}

				e->cast(e_cast_pos);

				scheduler->delay_action(0.05f, [] { w->cast(); });

				return;
			}

			if (w->is_ready() && e->is_ready() && q->ammo() >= 2)
			{
				vector e_cast_pos = { 0,0 };
				float distance = FLT_MAX;

				for (auto&& turret : entitylist->get_ally_turrets())
				{
					if (turret->get_distance(myhero) < distance)
					{
						e_cast_pos = turret->get_position();
						distance = turret->get_distance(myhero);
					}
				}

				e->cast(e_cast_pos);

				scheduler->delay_action(0.05f, [] { w->cast(); });
			}
		}

		if (q->is_ready() && (orbwalker->combo_mode() || orbwalker->harass()) && !flash_ks_executing && target->is_ai_hero())
		{
			if (q->ammo() == 2 && target != nullptr)
			{
				q->cast(target);
			}
			else
			{
				if (orbwalker->combo_mode())
				{
					if (combo::dance_q->get_bool())
					{
						auto n_target = target_selector->get_target(500, damage_type::physical);

						if ((n_target == nullptr || !n_target->is_valid_target()) && target != nullptr)
						{
							q->cast(target);
							return;
						}

						auto dance_pos = myhero->get_position().extend(n_target->get_position(), myhero->get_distance(n_target->get_position()) + 50);
						orbwalker->move_to(dance_pos);
						is_dancing = true;
						scheduler->delay_action(0.23f, [dance_pos] { q->cast(dance_pos); is_dancing = false; });
					}
					else
					{
						q->cast(target);
					}
				}
				else if (orbwalker->harass() && target != nullptr)
				{
					q->cast(target);
				}
			}

		}
		if (w->is_ready() && (orbwalker->combo_mode() || orbwalker->harass()) && !q->is_ready() && target->is_ai_hero())
		{
			scheduler->delay_action(0.0375f, [] { w->cast(); });

		}
	}

	void on_gapcloser(game_object_script sender, antigapcloser::antigapcloser_args* args)
	{
		if (w->is_ready())
		{
			if (sender->is_valid_target(get_w_radius() + sender->get_bounding_radius()))
			{
				w->cast();
			}
		}
	}

	void on_process_spell_cast(game_object_script sender, spell_instance_script spell)
	{
		if (sender->is_ai_hero())
		{
			auto target = spell->get_last_target_id() > 0 ? entitylist->get_object(spell->get_last_target_id()) : nullptr;

			if (target != nullptr && target == myhero)
			{
				if (spell->get_spell_data()->get_name() == "RenektonExecute")
				{
					if (w->is_ready())
					{
						w->cast();
					}

					if (e->is_ready())
					{
						vector e_cast_pos = { 0,0 };
						float distance = FLT_MAX;

						for (auto&& turret : entitylist->get_ally_turrets())
						{
							if (turret->get_distance(myhero) < distance)
							{
								e_cast_pos = turret->get_position();
								distance = turret->get_distance(myhero);
							}
						}

						e->cast(e_cast_pos);
					}
				}
				else if (spell->get_spell_data()->get_name() == "RenektonSuperExecute")
				{
					if (w->is_ready())
					{
						w->cast();
					}

					if (e->is_ready())
					{
						vector e_cast_pos = { 0,0 };
						float distance = FLT_MAX;

						for (auto&& turret : entitylist->get_ally_turrets())
						{
							if (turret->get_distance(myhero) < distance)
							{
								e_cast_pos = turret->get_position();
								distance = turret->get_distance(myhero);
							}
						}

						e->cast(e_cast_pos);
					}
				}
				else if (spell->get_spell_data()->get_name() == "GarenQAttack")
				{
					if (e->is_ready())
					{
						vector e_cast_pos = { 0,0 };
						float distance = FLT_MAX;

						for (auto&& turret : entitylist->get_ally_turrets())
						{
							if (turret->get_distance(myhero) < distance)
							{
								e_cast_pos = turret->get_position();
								distance = turret->get_distance(myhero);
							}
						}

						e->cast(e_cast_pos);
					}
				}
				else if (spell->get_spell_data()->get_name() == "JaxLeapStrike")
				{
					if (e->is_ready())
					{
						vector e_cast_pos = { 0,0 };
						float distance = FLT_MAX;

						for (auto&& turret : entitylist->get_ally_turrets())
						{
							if (turret->get_distance(myhero) < distance)
							{
								e_cast_pos = turret->get_position();
								distance = turret->get_distance(myhero);
							}
						}

						scheduler->delay_action(0.62f, [e_cast_pos] { e->cast(e_cast_pos); scheduler->delay_action(0.62f, [] { w->cast(); }); });
						
					}
				}
				else if (spell->get_spell_data()->get_name() == "GarenR")
				{
					if (e->is_ready())
					{
						vector e_cast_pos = { 0,0 };
						float distance = FLT_MAX;

						for (auto&& turret : entitylist->get_ally_turrets())
						{
							if (turret->get_distance(myhero) < distance)
							{
								e_cast_pos = turret->get_position();
								distance = turret->get_distance(myhero);
							}
						}

						e->cast(e_cast_pos);
					}
				}
			}
		}

		if (sender == myhero)
		{
			if (spell->get_spellslot() == spellslot::q)
			{
				is_dancing = false;
			}
		}
	}

	float get_w_radius()
	{
		if (myhero->has_buff(buff_hash("RivenFengShuiEngine")))
			return 249;
		else
			return 299;
	}

	float get_aa_range()
	{
		if (myhero->has_buff(buff_hash("RivenFengShuiEngine")))
			return 199;
		else
			return 124;
	}

	void cancel_q(game_object_script target)
	{
		if (!combo::dance_q->get_bool())
		{
			auto pos = hud->get_hud_input_logic()->get_game_cursor_position();
			orbwalker->move_to(pos);
			scheduler->delay_action(0.15f, [] { orbwalker->set_orbwalking_point(vector(0, 0, 0)); });
		}
		else
		{
			if (target != nullptr)
			{
				auto pos = myhero->get_position().extend(target->get_position(), myhero->get_distance(target->get_position()) + 25);
				orbwalker->move_to(pos);
				scheduler->delay_action(0.15f, [] { orbwalker->set_orbwalking_point(vector(0, 0, 0)); });
			}
			else
			{
				auto pos = hud->get_hud_input_logic()->get_game_cursor_position();
				orbwalker->move_to(pos);
				scheduler->delay_action(0.15f, [] { orbwalker->set_orbwalking_point(vector(0, 0, 0)); });
			}
		}

		orbwalker->set_attack(false);
		q_canceling_in_progress = true;
	}

	void on_network_packet(game_object_script sender, std::uint32_t network_id, pkttype_e type, void* args)
	{
		if (sender == myhero)
		{
			if (type == pkttype_e::PKT_S2C_PlayAnimation_s)
			{
				const auto animation_name = static_cast<PKT_S2C_PlayAnimationArgs*>(args)->animation_name;

				std::string spell1a = "Spell1a";
				std::string spell1b = "Spell1b";
				std::string spell1c = "Spell1c";
				std::string spell3 = "Spell3";
				std::string spell4a = "Spell4_Blade";

				auto target = target_selector->get_target(1130, damage_type::physical);

				if (spell1a == animation_name)
				{
					cancel_q(target);

					scheduler->delay_action((float)delays::q1_delay->get_int()/100.0f, [] { orbwalker->reset_auto_attack_timer(); orbwalker->set_attack(true); scheduler->delay_action(1.5f, [] { q_canceling_in_progress = false; }); });
				}
				if (spell1b == animation_name)
				{
					cancel_q(target);

					scheduler->delay_action((float)delays::q2_delay->get_int() / 100.0f, [] { orbwalker->reset_auto_attack_timer(); orbwalker->set_attack(true); scheduler->delay_action(1.5f, [] { q_canceling_in_progress = false; }); });
				}
				if (spell1c == animation_name)
				{
					cancel_q(target);

					scheduler->delay_action((float)delays::q3_delay->get_int() / 100.0f, [] { orbwalker->reset_auto_attack_timer(); orbwalker->set_attack(true); scheduler->delay_action(1.9f, [] { q_canceling_in_progress = false; }); });

				}
				if (spell4a == animation_name)
				{
					r1_casted = true;
				}
			}
		}
	}

	void on_buff_lose(game_object_script sender, buff_instance_script buff)
	{
		if (buff->get_name() == "rivenwindslashready")
		{
			r1_casted = false;
		}
	}

	void on_draw()
	{
		if (myhero->is_dead())
		{
			return;
		}

		if (w->is_ready() && draw::draw_w->get_bool())
		{
			draw_manager->add_circle(myhero->get_position(), get_w_radius(), draw::w_color->get_color());
		}
		if (e->is_ready() && draw::draw_e->get_bool())
		{
			draw_manager->add_circle(myhero->get_position(), 250, draw::e_color->get_color());
		}
		if (r->is_ready() && r1_casted && draw::draw_r->get_bool())
		{
			draw_manager->add_circle(myhero->get_position(), 1100, draw::e_color->get_color());
		}

		for (auto& enemy : entitylist->get_enemy_heroes())
		{
			if (!enemy->is_dead() && enemy->is_valid() && enemy->is_hpbar_recently_rendered() && !enemy->is_zombie())
			{
				float final_dmg = 0;

				if (q->is_ready())
				{
					final_dmg += calculate_q_damage(3 - q->ammo(), enemy);
				}
				if (w->is_ready())
				{
					final_dmg += calculate_w_damage(enemy);
				}
				if (r->is_ready())
				{
					final_dmg += calculate_r_damage(enemy, 10);
				}
				final_dmg += calculate_aa_damage(4, enemy);

				auto enemy_hp_percent_draw = (final_dmg / enemy->get_health()) * 100;

				if (enemy_hp_percent_draw <= 55)
				{
					draw_dmg_rl(enemy, final_dmg, MAKE_COLOR(120, 13, 13, 230));
				}
				else if (enemy_hp_percent_draw > 55 && enemy_hp_percent_draw < 80)
				{
					draw_dmg_rl(enemy, final_dmg, MAKE_COLOR(173, 183, 34, 230));
				}
				else if (enemy_hp_percent_draw > 80)
				{
					draw_dmg_rl(enemy, final_dmg, MAKE_COLOR(22, 73, 12, 230));
				}
			}
		}
	}

	inline void draw_dmg_rl(game_object_script target, float damage, unsigned long color)
	{
		if (target != nullptr && target->is_valid() && target->is_hpbar_recently_rendered() && !target->is_zombie())
		{
			auto bar_pos = target->get_hpbar_pos();

			if (bar_pos.is_valid() && !target->is_dead() && target->is_visible())
			{
				const auto health = target->get_health();

				bar_pos = vector(bar_pos.x + (105 * (health / target->get_max_health())), bar_pos.y -= 10);

				auto damage_size = (105 * (damage / target->get_max_health()));

				if (damage >= health)
				{
					damage_size = (105 * (health / target->get_max_health()));
				}

				if (damage_size > 105)
				{
					damage_size = 105;
				}

				const auto size = vector(bar_pos.x + (damage_size * -1), bar_pos.y + 11);

				draw_manager->add_filled_rect(bar_pos, size, color);
			}
		}
	}

	void auto_flash_kill_logic()
	{
		if (!combo::flash_ks_burst->get_bool()) return;
		if (myhero->is_dead() || myhero->is_recalling()) return;


		if (e->is_ready() && q->is_ready() && w->is_ready() && flash && flash != nullptr && flash->is_ready())
		{
			auto target = target_selector->get_target(250 + 400 + 80, damage_type::physical);

			if (target == nullptr || !target->is_valid_target() || target->is_invulnerable() || target->is_zombie()) return;

			if (calculate_aa_damage(1, target) + calculate_q_damage(1, target) + calculate_w_damage(target) > target->get_health() + 30)
			{
				if (target->get_distance(myhero) > 650)
				{
					flash_ks_executing = true;
					e->cast(target);
					scheduler->delay_action(0.3f, [target] {if (target != nullptr) flash->cast(target); scheduler->delay_action(0.05f, [target] { w->cast();  scheduler->delay_action(0.5f, [target] { flash_ks_executing = false; if (target != nullptr) q->cast(target);  }); }); });
				}
			}
		}
	}

	void auto_q_if_about_to_expire()
	{
		if (myhero->is_dead() || myhero->is_recalling() || !q->is_ready()) return;

		if (myhero->has_buff(buff_hash("RivenTriCleave")))
		{
			auto buff = myhero->get_buff(buff_hash("RivenTriCleave"));

			if (buff->get_remaining_time() < 0.1f)
			{
				q->cast();
			}
		}
	}

	void auto_r_if_about_to_expire()
	{
		if (!automatic::auto_r_expire->get_bool() || !r->is_ready()) return;
		if (myhero->is_dead() || myhero->is_recalling() || !r1_casted) return;

		if (myhero->has_buff(buff_hash("RivenFengShuiEngine")))
		{
			auto buff = myhero->get_buff(buff_hash("RivenFengShuiEngine"));

			if (buff->get_remaining_time() < 0.1f)
			{
				auto target = target_selector->get_target(1130, damage_type::physical);

				if (target == nullptr || !target->is_valid_target()) return;

				r->cast(target);
			}
		}
	}

	void auto_r_if_will_hit_enough_enemies()
	{
		if (!automatic::auto_r_if_hit_x_enemies->get_bool()) return;
		if (myhero->is_dead() || !r->is_ready()) return;
		if (!r1_casted) return;

		std::vector<game_object_script> hit_by_r;

		for (auto& enemy : entitylist->get_enemy_heroes())
		{
			if (enemy->is_valid() && !enemy->is_dead() && !enemy->is_zombie() && enemy->is_valid_target(r->range()))
			{
				auto pred = prediction->get_prediction(enemy, r->get_delay(), r->get_radius(), r->get_speed());
				if (pred.hitchance >= hit_chance::medium)
				{
					hit_by_r.push_back(enemy);
				}
			}
		}

		if (hit_by_r.size() >= automatic::auto_r_if_hit_x_enemies_slider->get_int())
		{
			auto pred = prediction->get_prediction(hit_by_r.front(), r->get_delay(), r->get_radius(), r->get_speed());
			r->cast(pred.get_unit_position());
		}

	}

	void auto_w_on_chanelling_spells()
	{
		if (!automatic::auto_w_channeling->get_bool()) return;
		if (myhero->is_dead() || !w->is_ready()) return;

		for (auto&& enemy : entitylist->get_enemy_heroes())
		{
			if (enemy->is_valid_target(245 + enemy->get_bounding_radius()))
			{
				if (enemy->is_casting_interruptible_spell())
				{
					w->cast();
				}
			}
		}
	}

	void auto_w_if_killable()
	{
		if (!automatic::auto_w_if_killable->get_bool()) return;
		if (myhero->is_dead() || !w->is_ready()) return;

		for (auto&& enemy : entitylist->get_enemy_heroes())
		{
			if (enemy->is_valid_target(get_w_radius() + enemy->get_bounding_radius()))
			{
				if (calculate_w_damage(enemy) > enemy->get_health())
				{
					w->cast();
				}
			}
		}
	}

	void auto_w_if_enough_enemies()
	{
		if (!automatic::auto_w_on_multi_enemies->get_bool()) return;
		if (myhero->is_dead() || !w->is_ready()) return;

		if (myhero->count_enemies_in_range(get_w_radius()) >= automatic::auto_w_on_multi_enemies_slider->get_int())
		{
			w->cast();
		}
	}

	void auto_w_if_dashing()
	{
		if (!automatic::auto_w_dashing->get_bool()) return;
		if (myhero->is_dead() || !w->is_ready()) return;

		for (auto&& enemy : entitylist->get_enemy_heroes())
		{
			if (enemy->is_valid_target(get_w_radius() + enemy->get_bounding_radius()))
			{
				if (enemy->is_dashing())
				{
					w->cast();
				}
			}
		}
	}

	float calculate_q_damage(int stacks, game_object_script enemy)
	{
		float q_raw_damage = q_damages[q->level() - 1] + (myhero->get_total_attack_damage() * q_coefs[q->level() - 1]);
		float q_calculated_damage = damagelib->calculate_damage_on_unit(myhero, enemy, damage_type::physical, q_raw_damage);

		q_calculated_damage *= stacks;
		return q_calculated_damage;
	}

	float calculate_w_damage(game_object_script enemy)
	{
		float w_raw_damage = w_damages[w->level() - 1] + (myhero->get_additional_attack_damage() * w_coef_bonus);
		float w_calculated_damage = damagelib->calculate_damage_on_unit(myhero, enemy, damage_type::physical, w_raw_damage);

		return w_calculated_damage;
	}

	float calculate_aa_damage(int stacks, game_object_script enemy)
	{
		float aa_damage = myhero->get_auto_attack_damage(enemy);
		aa_damage *= stacks;

		float aa_calculated_damage = damagelib->calculate_damage_on_unit(myhero, enemy, damage_type::physical, aa_damage);

		return aa_calculated_damage;
	}

	float calculate_r_damage(game_object_script enemy, float additional)
	{
		float r_raw_damage = r_damages[r->level() - 1] + (myhero->get_additional_attack_damage() * r_coef_bonus);

		float missing_hp_perc = 100 - enemy->get_health_percent();
		if (missing_hp_perc > 75) missing_hp_perc = 75;
		r_raw_damage = r_raw_damage + (r_raw_damage * (0.02 * missing_hp_perc));

		float r_calculated_damage = damagelib->calculate_damage_on_unit(myhero, enemy, damage_type::physical, r_raw_damage);

		return r_calculated_damage + additional + 20;
	}

	bool is_killable_without_r_full_combo(game_object_script enemy)
	{
		float overall_damage = full_combo_without_r_damage(enemy);

		if (overall_damage >= enemy->get_health())
		{
			return true;
		}
		return false;
	}

	bool is_killable_with_r_full_combo(game_object_script enemy, float modified)
	{
		float overall_damage = full_combo_with_r_damage(enemy);

		if (overall_damage + modified >= enemy->get_health())
		{
			return true;
		}
		return false;
	}

	float full_combo_without_r_damage(game_object_script enemy)
	{
		float q_damage = calculate_q_damage(3 - q->ammo(), enemy);
		float w_damage = calculate_w_damage(enemy);
		float aa_damage = calculate_aa_damage(4, enemy);

		float overall_damage = q_damage + w_damage + aa_damage;
		return overall_damage;
	}

	float full_combo_with_r_damage(game_object_script enemy)
	{
		float q_damage = calculate_q_damage(3 - q->ammo(), enemy);
		float w_damage = calculate_w_damage(enemy);
		float aa_damage = calculate_aa_damage(4, enemy);
		float r_damage = calculate_r_damage(enemy, 10);

		float overall_damage = q_damage + w_damage + aa_damage + r_damage;
		return overall_damage;
	}
}