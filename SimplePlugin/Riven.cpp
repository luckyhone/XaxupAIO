#include "../plugin_sdk/plugin_sdk.hpp"
#include "Riven.h"
#include "Utilities.h"
#include "permashow.hpp"

namespace riven
{
	void on_issue_order(game_object_script& target, vector& pos, _issue_order_type& type, bool* process);
	void on_after_attack(game_object_script target);
	void on_process_spell_cast(game_object_script sender, spell_instance_script spell);
	void on_draw();
	void on_gapcloser(game_object_script sender, antigapcloser::antigapcloser_args* args);
	void on_create_object(game_object_script sender);
	void on_buff_gain(game_object_script sender, buff_instance_script buff);
	void on_buff_lose(game_object_script sender, buff_instance_script buff);
	void on_update();

	script_spell* q = nullptr;
	script_spell* w = nullptr;
	script_spell* e = nullptr;
	script_spell* r = nullptr;
	script_spell* flash = nullptr;

	int q_counter = 0;
	TreeTab* main_tab = nullptr;

	namespace combo
	{

	}
	namespace harass
	{

	}
	namespace laneclear
	{

	}
	namespace jungleclear
	{

	}
	namespace misc
	{

	}
	namespace permashow
	{

	}
	namespace draw
	{

	}

	void load()
	{
		myhero->print_chat(0x3, "<font color=\"#FFFFFF\">[<b><font color=\"#3F704D\">Riven | XaxupAIO</font></b>]</font><font color=\"#3F704D\">:</font><font color=\"#90EE90\"> Loaded</font>");
		myhero->print_chat(0x3, "<font color=\"#3F704D\"><b>Suggested Prediction: </b><font color=\"#90EE90\">Core</font></font>");

		q = plugin_sdk->register_spell(spellslot::q, 150);
		w = plugin_sdk->register_spell(spellslot::w, 250);
		e = plugin_sdk->register_spell(spellslot::e, 0);
		r = plugin_sdk->register_spell(spellslot::r, 0);


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
				auto q_config = combo->add_tab(myhero->get_model() + "comboQConfig", "Q Settings");
				{

				}
				auto w_config = combo->add_tab(myhero->get_model() + ".comboWConfig", "W Settings");
				{

				}
				auto e_config = combo->add_tab(myhero->get_model() + ".comboEConfig", "E Settings");
				{

				}
			}

			auto harass = main_tab->add_tab(myhero->get_model() + ".harass", "Haras");
			{
				harass->add_separator(".harassSep", "Harass");
				auto q_config = harass->add_tab(myhero->get_model() + ".harassQConfig", "Q Settings");
				{

				}
				auto w_config = harass->add_tab(myhero->get_model() + ".harassWConfig", "W Settings");
				{

				}
				auto e_config = harass->add_tab(myhero->get_model() + ".harassEConfig", "E Settings");
				{

				}
			}
			auto laneclear = main_tab->add_tab(myhero->get_model() + ".laneclear", "Laneclear");
			{
				laneclear->add_separator(".laneSep", "Laneclear");
				auto q_config = laneclear->add_tab(myhero->get_model() + ".laneClearQConfig", "Q Settings");
				{

				}
				auto w_config = laneclear->add_tab(myhero->get_model() + ".laneClearWConfig", "W Settings");
				{

				}
				auto e_cofig = laneclear->add_tab(myhero->get_model() + ".laneClearEConfig", "E Settings");
				{

				}
			}
			auto jungleclear = main_tab->add_tab(myhero->get_model() + ".jungleClear", "Jungleclear");
			{
				jungleclear->add_separator(".jungleSep", "Jungleclear");
				auto q_config = jungleclear->add_tab(myhero->get_model() + ".jungleClearQConfig", "Q Settings");
				{

				}
				auto w_config = jungleclear->add_tab(myhero->get_model() + ".jungleClearWConfig", "W Settings");
				{

				}
				auto e_config = jungleclear->add_tab(myhero->get_model() + ".jungleClearEConfig", "E Settings");
				{

				}
			}
			auto misc = main_tab->add_tab(myhero->get_model() + ".misc", "Misc");
			{
				misc->add_separator(".miscSep", "Misc");
			}
			auto hotkeys = main_tab->add_tab(myhero->get_model() + ".hotkeys", "Hotkeys");
			{
				hotkeys->add_separator(".hotkeysSep", "Hotkeys");
			}
			auto draw = main_tab->add_tab(myhero->get_model() + ".draw", "Drawings");
			{
				draw->add_separator(".drawSep", "Drawings");
				float color[] = { 1.0f, 1.0f, 0.0f, 1.0f };
			}
		}

		{
			Permashow::Instance.Init(main_tab);

		}

		antigapcloser::add_event_handler(on_gapcloser);
		event_handler<events::on_buff_gain>::add_callback(on_buff_gain);
		event_handler<events::on_after_attack_orbwalker>::add_callback(on_after_attack);
		event_handler<events::on_buff_lose>::add_callback(on_buff_lose);
		event_handler<events::on_update>::add_callback(on_update);
		event_handler<events::on_process_spell_cast>::add_callback(on_process_spell_cast);
		event_handler<events::on_create_object>::add_callback(on_create_object);
		event_handler<events::on_draw>::add_callback(on_draw);
		event_handler<events::on_issue_order>::add_callback(on_issue_order);
	}

	void unload()
	{
		menu->delete_tab(main_tab);

		plugin_sdk->remove_spell(q);
		plugin_sdk->remove_spell(w);
		plugin_sdk->remove_spell(e);
		plugin_sdk->remove_spell(r);

		Permashow::Instance.Destroy();

		if (flash)
			plugin_sdk->remove_spell(flash);

		antigapcloser::remove_event_handler(on_gapcloser);
		event_handler<events::on_buff_gain>::remove_handler(on_buff_gain);
		event_handler<events::on_buff_lose>::remove_handler(on_buff_lose);
		event_handler<events::on_update>::remove_handler(on_update);
		event_handler<events::on_create_object>::remove_handler(on_create_object);
		event_handler<events::on_draw>::remove_handler(on_draw);
		event_handler<events::on_after_attack_orbwalker>::remove_handler(on_after_attack);
		event_handler<events::on_process_spell_cast>::remove_handler(on_process_spell_cast);
		event_handler<events::on_issue_order>::remove_handler(on_issue_order);
	}

	void on_update()
	{
		if (orbwalker->can_move(0.05f))
		{
			if (orbwalker->combo_mode())
			{
				myhero->print_chat(0x0, "Q Counter: %i", q_counter);
				auto target = target_selector->get_target(500, damage_type::physical);

				if (target != nullptr)
				{				
					if (e->is_ready())
					{
						e->cast(target);
					}
					if (w->is_ready() && target->is_valid_target(w->range()))
					{
						w->cast();
					}		
				}
			}

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


			}
			if (!monsters.empty())
			{

			}
		}
	}

	void on_after_attack(game_object_script target)
	{
		if (q->is_ready() && orbwalker->combo_mode())
		{							
			if (q->is_ready())
			{
				q->cast(target);
			}
		}
	}

	void on_issue_order(game_object_script& target, vector& pos, _issue_order_type& type, bool* process)
	{

	}

	void on_gapcloser(game_object_script sender, antigapcloser::antigapcloser_args* args)
	{

	}

	void on_process_spell_cast(game_object_script sender, spell_instance_script spell)
	{
		if (sender == myhero)
		{

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

	}
}

