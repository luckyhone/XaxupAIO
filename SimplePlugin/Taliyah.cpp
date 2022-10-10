#include "../plugin_sdk/plugin_sdk.hpp"
#include "Taliyah.h"
#include "Utilities.h"
#include "permashow.hpp"

namespace taliyah
{
	void on_process_spell_cast(game_object_script sender, spell_instance_script spell);
	void on_draw();
	void auto_w_on_chanelling_spells();
	void on_gapcloser(game_object_script sender, antigapcloser::antigapcloser_args* args);
	bool is_under_ally_turret_modified(float additional);
	void harass_logic();
	void on_create_object(game_object_script sender);
	void on_buff_gain(game_object_script sender, buff_instance_script buff);
	void on_buff_lose(game_object_script sender, buff_instance_script buff);
	void combo_logic();
	void on_update();

	script_spell* q = nullptr;
	script_spell* w = nullptr;
	script_spell* e = nullptr;
	script_spell* flash = nullptr;

	bool use_e = false;
	bool should_use_magnet = false;
	game_object_script magnet_target = nullptr;

	TreeTab* main_tab = nullptr;

	namespace combo
	{
		TreeEntry* use_q;
		TreeEntry* use_w;
		TreeEntry* use_w_under_turret;
		TreeEntry* use_e;
		TreeEntry* use_e_only_when_w_ready;
	}
	namespace harass
	{
		TreeEntry* use_q;
		TreeEntry* use_w;
		TreeEntry* use_e;
		TreeEntry* use_e_only_when_w_ready;
	}
	namespace laneclear
	{
		TreeEntry* use_q;
		TreeEntry* use_w;
		TreeEntry* use_w_min_minions;
		TreeEntry* use_e;
	}
	namespace jungleclear
	{
		TreeEntry* use_q;
		TreeEntry* use_w;
		TreeEntry* use_w_min_monster_hp;
		TreeEntry* use_e;
	}
	namespace misc
	{
		TreeEntry* auto_w_on_channeling;
		TreeEntry* auto_w_on_gapclose;
	}
	namespace draw
	{
		TreeEntry* draw_q;
		TreeEntry* draw_w;
		TreeEntry* draw_e;
		TreeEntry* q_color = nullptr;
		TreeEntry* w_color = nullptr;
		TreeEntry* e_color = nullptr;
	}

	void load()
	{
		myhero->print_chat(0x3, "<font color=\"#FFFFFF\">[<b><font color=\"#3F704D\">Taliyah | XaxupAIO</font></b>]:</font> <font color=\"#90EE90\">Loaded</font>");
		myhero->print_chat(0x3, "<font color=\"#3F704D\"><b>Suggested Prediction: </b><font color=\"#90EE90\">Aurora</font></font>");

		q = plugin_sdk->register_spell(spellslot::q, 1000);
		w = plugin_sdk->register_spell(spellslot::w, 900);
		e = plugin_sdk->register_spell(spellslot::e, 950);

		q->set_skillshot(0.25f, 100, 3300, { collisionable_objects::heroes, collisionable_objects::yasuo_wall, collisionable_objects::minions }, skillshot_type::skillshot_line);
		w->set_skillshot(0.25f, 112, FLT_MAX, {}, skillshot_type::skillshot_circle);
		//e->set_skillshot(0.25f, 800, 1700, {}, skillshot_type::skillshot_line);

		if (myhero->get_spell(spellslot::summoner1)->get_spell_data()->get_name_hash() == spell_hash("SummonerFlash"))
			flash = plugin_sdk->register_spell(spellslot::summoner1, 400.f);
		else if (myhero->get_spell(spellslot::summoner2)->get_spell_data()->get_name_hash() == spell_hash("SummonerFlash"))
			flash = plugin_sdk->register_spell(spellslot::summoner2, 400.f);

		main_tab = menu->create_tab("taliyah", "XaxupAIO");
		main_tab->set_assigned_texture(myhero->get_square_icon_portrait());
		{
			main_tab->add_separator(".predSep", "USE AURORA PRED");
			auto combo = main_tab->add_tab(myhero->get_model() + ".combo", "Combo");
			{
				combo->add_separator(".comboSep", "Combo");
			}
			auto q_config = combo->add_tab(myhero->get_model() + "comboQConfig", "Q Settings");
			{
				combo::use_q = q_config->add_checkbox(myhero->get_model() + ".comboUseQ", "Use Q", true);
				combo::use_q->set_texture(myhero->get_spell(spellslot::q)->get_icon_texture());
			}
			auto w_config = combo->add_tab(myhero->get_model() + ".comboWConfig", "W Settings");
			{
				combo::use_w = w_config->add_checkbox(myhero->get_model() + ".comboUseW", "Use W", true);
				combo::use_w->set_texture(myhero->get_spell(spellslot::w)->get_icon_texture());
				combo::use_w_under_turret = w_config->add_checkbox(myhero->get_model() + ".comboUseWUnderTurret", "^~ if possible throw enemy under turret", true);
				combo::use_w->set_texture(myhero->get_spell(spellslot::w)->get_icon_texture());
			}
			auto e_config = combo->add_tab(myhero->get_model() + ".comboEConfig", "E Settings");
			{
				combo::use_e = e_config->add_checkbox(myhero->get_model() + ".comboEConfigUse", "Use E", true);
				combo::use_e->set_texture(myhero->get_spell(spellslot::e)->get_icon_texture());
				combo::use_e_only_when_w_ready = e_config->add_checkbox(myhero->get_model() + ".comboEConfigUseWhenW", "^~ use only when W ready", true);
			}
			auto harass = main_tab->add_tab(myhero->get_model() + ".harass", "Haras");
			{
				harass->add_separator(".harassSep", "Harass");
				auto q_config = harass->add_tab(myhero->get_model() + ".harassQConfig", "Q Settings");
				{
					harass::use_q = q_config->add_checkbox(myhero->get_model() + ".harassUseQ", "Use Q", true);
					harass::use_q->set_texture(myhero->get_spell(spellslot::q)->get_icon_texture());
				}
				auto w_config = harass->add_tab(myhero->get_model() + ".harassWConfig", "W Settings");
				{
					harass::use_w = w_config->add_checkbox(myhero->get_model() + ".harassUseW", "Use W", true);
					harass::use_w->set_texture(myhero->get_spell(spellslot::w)->get_icon_texture());
				}
				auto e_config = harass->add_tab(myhero->get_model() + ".harassEConfig", "E Settings");
				{
					harass::use_e = e_config->add_checkbox(myhero->get_model() + ".harassUseE", "Use E", false);
					harass::use_e->set_texture(myhero->get_spell(spellslot::e)->get_icon_texture());
					harass::use_e_only_when_w_ready = e_config->add_checkbox(myhero->get_model() + ".harassEConfigUseWhenW", "^~ use only when W ready", true);
				}
			}
			auto laneclear = main_tab->add_tab(myhero->get_model() + ".laneclear", "Laneclear");
			{
				laneclear->add_separator(".laneSep", "Laneclear");
				auto q_config = laneclear->add_tab(myhero->get_model() + ".laneClearQConfig", "Q Settings");
				{
					laneclear::use_q = q_config->add_checkbox(myhero->get_model() + ".laneClearUseQ", "Use Q", true);
					laneclear::use_q->set_texture(myhero->get_spell(spellslot::q)->get_icon_texture());
				}
				auto w_config = laneclear->add_tab(myhero->get_model() + ".laneClearWConfig", "W Settings");
				{
					laneclear::use_w = w_config->add_checkbox(myhero->get_model() + ".laneClearUseW", "Use W", true);
					laneclear::use_w->set_texture(myhero->get_spell(spellslot::w)->get_icon_texture());
					laneclear::use_w_min_minions = w_config->add_slider(myhero->get_model() + ".laneClearUseWMinSlider", "^~ minimum minions to hit", 3, 1, 6);
				}
				auto e_cofig = laneclear->add_tab(myhero->get_model() + ".laneClearEConfig", "E Settings");
				{
					laneclear::use_e = e_cofig->add_checkbox(myhero->get_model() + ".laneClearUseE", "Use E", true);
					laneclear::use_e->set_texture(myhero->get_spell(spellslot::e)->get_icon_texture());
				}
			}
			auto jungleclear = main_tab->add_tab(myhero->get_model() + ".jungleClear", "Jungleclear");
			{
				jungleclear->add_separator(".jungleSep", "Jungleclear");
				auto q_config = jungleclear->add_tab(myhero->get_model() + ".jungleClearQConfig", "Q Settings");
				{
					jungleclear::use_q = q_config->add_checkbox(myhero->get_model() + ".jungleClearUseQ", "Use Q", true);
					jungleclear::use_q->set_texture(myhero->get_spell(spellslot::q)->get_icon_texture());
				}
				auto w_config = jungleclear->add_tab(myhero->get_model() + ".jungleClearWConfig", "W Settings");
				{
					jungleclear::use_w = w_config->add_checkbox(myhero->get_model() + ".jungleClearUseW", "Use W", true);
					jungleclear::use_w->set_texture(myhero->get_spell(spellslot::w)->get_icon_texture());
					jungleclear::use_w_min_monster_hp = w_config->add_slider(myhero->get_model() + ".jungleClearUseWSlider", "^~ only when monster HP > than", 10, 1,99);
				}
				auto e_config = jungleclear->add_tab(myhero->get_model() + ".jungleClearEConfig", "E Settings");
				{
					jungleclear::use_e = e_config->add_checkbox(myhero->get_model() + ".jungleClearUseE", "Use E", true);
					jungleclear::use_e->set_texture(myhero->get_spell(spellslot::e)->get_icon_texture());
				}
			}
			auto misc = main_tab->add_tab(myhero->get_model() + ".misc", "Misc");
			{
				misc->add_separator(".miscSep", "Misc");
				misc::auto_w_on_gapclose = misc->add_checkbox(myhero->get_model() + ".miscAutoWGapclose", "Auto W On Gapclose", true);
				misc::auto_w_on_gapclose->set_texture(myhero->get_spell(spellslot::w)->get_icon_texture());
				misc::auto_w_on_channeling = misc->add_checkbox(myhero->get_model() + ".miscAutoWChanneling", "Auto W On Channeling", true);
				misc::auto_w_on_channeling->set_texture(myhero->get_spell(spellslot::w)->get_icon_texture());
			}
			auto draw = main_tab->add_tab(myhero->get_model() + ".draw", "Drawings");
			{
				draw->add_separator(".drawSep", "Drawings");
				float color[] = { 1.0f, 1.0f, 0.0f, 1.0f };
				draw::draw_q = draw->add_checkbox(myhero->get_model() + ".drawQ", "Draw Q Range", true);
				draw::draw_w = draw->add_checkbox(myhero->get_model() + ".drawW", "Draw W Range", true);
				draw::draw_e = draw->add_checkbox(myhero->get_model() + ".drawR", "Draw E Range", true);
				draw::q_color = draw->add_colorpick(myhero->get_model() + ".drawQColor", "Q Color", color);
				draw::w_color = draw->add_colorpick(myhero->get_model() + ".drawWColor", "W Color", color);
				draw::e_color = draw->add_colorpick(myhero->get_model() + ".drawEColor", "E Color", color);
			}
		}

		antigapcloser::add_event_handler(on_gapcloser);
		event_handler<events::on_buff_gain>::add_callback(on_buff_gain);
		event_handler<events::on_buff_lose>::add_callback(on_buff_lose);
		event_handler<events::on_update>::add_callback(on_update);
		event_handler<events::on_process_spell_cast>::add_callback(on_process_spell_cast);
		event_handler<events::on_create_object>::add_callback(on_create_object);
		event_handler<events::on_draw>::add_callback(on_draw);
	}

	void unload()
	{
		menu->delete_tab(main_tab);

		plugin_sdk->remove_spell(q);
		plugin_sdk->remove_spell(w);
		plugin_sdk->remove_spell(e);

		if (flash)
			plugin_sdk->remove_spell(flash);

		antigapcloser::remove_event_handler(on_gapcloser);
		event_handler<events::on_buff_gain>::remove_handler(on_buff_gain);
		event_handler<events::on_buff_lose>::remove_handler(on_buff_lose);
		event_handler<events::on_update>::remove_handler(on_update);
		event_handler<events::on_create_object>::remove_handler(on_create_object);
		event_handler<events::on_draw>::remove_handler(on_draw);
		event_handler<events::on_process_spell_cast>::remove_handler(on_process_spell_cast);
	}

	void on_update()
	{
		if (orbwalker->can_move(0.05f))
		{
			auto_w_on_chanelling_spells();

			if (orbwalker->combo_mode())
			{
				combo_logic();
			}
			if (orbwalker->harass())
			{
				harass_logic();
			}
			if (orbwalker->lane_clear_mode())
			{
				auto lane_minions = entitylist->get_enemy_minions();
				auto monsters = entitylist->get_jugnle_mobs_minions();

				#pragma region  SortMinions
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
					if (e->is_ready() && laneclear::use_e->get_bool() && lane_minions.front()->is_valid_target(e->range()))
					{
						e->cast(lane_minions.front()->get_position());
					}
					if (q->is_ready() && laneclear::use_q->get_bool() && lane_minions.front()->is_valid_target(q->range()))
					{
						q->cast(lane_minions.front()->get_position());
					}
					if (w->is_ready() && laneclear::use_w->get_bool() && lane_minions.front()->is_valid_target(w->range()))
					{
						if (utilities::count_enemy_minions_in_range(250, lane_minions.front()->get_position()) >= laneclear::use_w_min_minions->get_int())
						{
							auto cast_position = w->get_prediction(lane_minions.front()).get_unit_position();
							w->cast(cast_position, myhero->get_position());
						}
					}
				}
				if (!monsters.empty())
				{
					if (e->is_ready() && jungleclear::use_e->get_bool() && monsters.front()->is_valid_target(e->range()))
					{
						e->cast(monsters.front()->get_position());
					}
					if (q->is_ready() && jungleclear::use_q->get_bool() && monsters.front()->is_valid_target(q->range()))
					{
						q->cast(monsters.front()->get_position());
					}
					if (w->is_ready() && jungleclear::use_w->get_bool() && monsters.front()->is_valid_target(w->range()))
					{
						if (monsters.front()->get_health_percent() > jungleclear::use_w_min_monster_hp->get_int())
						{
							auto cast_position = w->get_prediction(monsters.front()).get_unit_position();
							w->cast(cast_position, myhero->get_position());
						}
					}
				}
			}
		}
	}

	void auto_w_on_chanelling_spells()
	{
		if (myhero->is_dead() || !w->is_ready() || !misc::auto_w_on_channeling->get_bool()) return;

		for (auto&& enemy : entitylist->get_enemy_heroes())
		{
			if (enemy->is_valid_target(w->range()))
			{
				if (enemy->is_casting_interruptible_spell())
				{
					auto cast_position = w->get_prediction(enemy).get_unit_position();
					w->cast(cast_position, myhero->get_position());
				}
			}
		}
	}

	void harass_logic()
	{
		auto target = target_selector->get_target(1100, damage_type::magical);

		if (target == nullptr) return;

		if (e->is_ready() && target->is_valid_target(e->range()) && !target->is_invulnerable() && harass::use_e->get_bool())
		{
			if (!harass::use_e_only_when_w_ready->get_bool())
				e->cast(target->get_position());
		}
		if (q->is_ready() && target->is_valid_target(q->range()) && !target->is_invulnerable() && harass::use_q->get_bool())
		{
			q->cast(target, hit_chance::medium);
		}
		if (w->is_ready() && target->is_valid_target(w->range()) && !target->is_invulnerable() && harass::use_w->get_bool())
		{
			if (is_under_ally_turret_modified(160) && myhero->get_distance(target->get_position()) < 450 && combo::use_w_under_turret->get_bool())
			{
				auto cast_position = w->get_prediction(target).get_unit_position();
				w->cast(cast_position, myhero->get_position());
			}
			if (myhero->get_distance(target->get_position()) < 500 && w->is_ready())
			{
				auto cast_position = w->get_prediction(target).get_unit_position();
				w->cast(cast_position, myhero->get_position().extend(target->get_position(), myhero->get_distance(target->get_position()) + 50));
			}
			else
			{
				if (w->is_ready())
				{
					auto cast_position = w->get_prediction(target).get_unit_position();
					w->cast(cast_position, myhero->get_position());
				}
			}
		}
	}

	void combo_logic()
	{
		auto target = target_selector->get_target(1100, damage_type::magical);

		if (target == nullptr) return;

		if (e->is_ready() && target->is_valid_target(e->range()) && !target->is_invulnerable() && combo::use_e->get_bool())
		{
			if(!combo::use_e_only_when_w_ready->get_bool())
				e->cast(target->get_position());
		}
		if (q->is_ready() && target->is_valid_target(q->range()) && !target->is_invulnerable() && combo::use_q->get_bool())
		{
			q->cast(target, hit_chance::medium);
		}
		if (w->is_ready() && target->is_valid_target(w->range()) && !target->is_invulnerable() && combo::use_w->get_bool())
		{
			if (target->is_under_ally_turret() && combo::use_w_under_turret->get_bool() && w->is_ready())
			{
				auto cast_position = w->get_prediction(target).get_unit_position();
				w->cast(cast_position, myhero->get_position());
			}
			if (is_under_ally_turret_modified(160) && myhero->get_distance(target->get_position()) < 450 && combo::use_w_under_turret->get_bool() && w->is_ready())
			{
				auto cast_position = w->get_prediction(target).get_unit_position();
				w->cast(cast_position, myhero->get_position());
			}
			if (myhero->get_distance(target->get_position()) < 500 && w->is_ready())
			{
				auto cast_position = w->get_prediction(target).get_unit_position();
				w->cast(cast_position, myhero->get_position().extend(target->get_position(), myhero->get_distance(target->get_position()) + 50));		
			}
			else
			{
				if (w->is_ready())
				{
					auto cast_position = w->get_prediction(target).get_unit_position();
					w->cast(cast_position, myhero->get_position());
				}
			}
		}
	}

	bool is_under_ally_turret_modified(float additional)
	{	
		for (auto&& turret : entitylist->get_ally_turrets())
		{
			if (turret->get_distance(myhero->get_position()) < 730 + additional)
				return true;
		}

		return false;
	}

	void on_gapcloser(game_object_script sender, antigapcloser::antigapcloser_args* args)
	{
		if (w->is_ready() && misc::auto_w_on_gapclose->get_bool())
		{
			if (sender->is_valid_target(w->range() + sender->get_bounding_radius()))
			{
				auto cast_position = w->get_prediction(sender).get_unit_position();
				w->cast(cast_position, myhero->get_position().extend(sender->get_position(), myhero->get_distance(sender->get_position()) + 50));
			}
		}
	}

	void on_process_spell_cast(game_object_script sender, spell_instance_script spell)
	{
		if (sender == myhero)
		{
			if (spell->get_spellslot() == spellslot::w)
			{
				if (combo::use_e_only_when_w_ready->get_bool())
				{
					auto target = target_selector->get_target(e->range(), damage_type::magical);
					if (target == nullptr) return;

					use_e = true;

					if(target->is_valid_target(e->range()) && (orbwalker->combo_mode() || orbwalker->harass()))
						scheduler->delay_action(0.2f, [target] 
							{ 
								if(use_e)
									e->cast(target->get_position()); 
								scheduler->delay_action(1.0f, [] { use_e = false; });
							});
				}
			}
		}
	}

	void on_buff_gain(game_object_script sender, buff_instance_script buff)
	{

	}

	void on_buff_lose(game_object_script sender, buff_instance_script buff)
	{

	}

	void on_create_object(game_object_script sender)
	{
		
	}

	void on_draw()
	{
		if (myhero->is_dead())
		{
			return;
		}

		if (q->is_ready() && draw::draw_q->get_bool())
		{
			draw_manager->add_circle(myhero->get_position(), q->range(), draw::q_color->get_color());
		}
		if (w->is_ready() && draw::draw_w->get_bool())
		{
			draw_manager->add_circle(myhero->get_position(), w->range(), draw::w_color->get_color());
		}
		if (e->is_ready() && draw::draw_e->get_bool())
		{
			draw_manager->add_circle(myhero->get_position(), e->range()+15, draw::e_color->get_color());
		}
	}
}