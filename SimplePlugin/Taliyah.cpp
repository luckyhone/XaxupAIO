#include "../plugin_sdk/plugin_sdk.hpp"
#include "Taliyah.h"
#include "Utilities.h"
#include "permashow.hpp"

namespace taliyah
{
	void magnet_to_enemy();
	void on_create_object(game_object_script sender);
	void on_buff_gain(game_object_script sender, buff_instance_script buff);
	void on_buff_lose(game_object_script sender, buff_instance_script buff);
	void combo_logic();
	void on_update();

	script_spell* q = nullptr;
	script_spell* w = nullptr;
	script_spell* e = nullptr;
	script_spell* flash = nullptr;

	bool should_use_magnet = false;
	game_object_script magnet_target = nullptr;

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
	namespace flee
	{

	}
	namespace draw
	{

	}

	void load()
	{
		myhero->print_chat(0x3, "<font color=\"#FFFFFF\">[<b><font color=\"#3F704D\">Taliyah | XaxupAIO</font></b>]:</font> <font color=\"#90EE90\">Loaded</font>");
		myhero->print_chat(0x3, "<font color=\"#3F704D\"><b>Suggested Prediction: </b><font color=\"#90EE90\">Core</font></font>");

		q = plugin_sdk->register_spell(spellslot::q, 1000);
		w = plugin_sdk->register_spell(spellslot::w, 900);
		e = plugin_sdk->register_spell(spellslot::e, 820);

		q->set_skillshot(0.25f, 100, 3300, { collisionable_objects::heroes, collisionable_objects::yasuo_wall, collisionable_objects::minions }, skillshot_type::skillshot_line);
		w->set_skillshot(0.25f, 112, FLT_MAX, {}, skillshot_type::skillshot_circle);
		e->set_skillshot(0.25f, 800, 1700, {}, skillshot_type::skillshot_line);

		if (myhero->get_spell(spellslot::summoner1)->get_spell_data()->get_name_hash() == spell_hash("SummonerFlash"))
			flash = plugin_sdk->register_spell(spellslot::summoner1, 400.f);
		else if (myhero->get_spell(spellslot::summoner2)->get_spell_data()->get_name_hash() == spell_hash("SummonerFlash"))
			flash = plugin_sdk->register_spell(spellslot::summoner2, 400.f);

		main_tab = menu->create_tab("taliyah", "XaxupAIO");
		main_tab->set_assigned_texture(myhero->get_square_icon_portrait());

		event_handler<events::on_buff_gain>::add_callback(on_buff_gain);
		event_handler<events::on_buff_lose>::add_callback(on_buff_lose);
		event_handler<events::on_update>::add_callback(on_update);
		event_handler<events::on_create_object>::add_callback(on_create_object);
	}

	void unload()
	{
		menu->delete_tab(main_tab);

		plugin_sdk->remove_spell(q);
		plugin_sdk->remove_spell(w);
		plugin_sdk->remove_spell(e);

		if (flash)
			plugin_sdk->remove_spell(flash);

		event_handler<events::on_buff_gain>::remove_handler(on_buff_gain);
		event_handler<events::on_buff_lose>::remove_handler(on_buff_lose);
		event_handler<events::on_update>::remove_handler(on_update);
		event_handler<events::on_create_object>::remove_handler(on_create_object);
	}

	void on_update()
	{
		if (orbwalker->can_move(0.05f))
		{
			if (orbwalker->combo_mode())
			{
				combo_logic();
			}
		}
	}

	void combo_logic()
	{
		auto target = target_selector->get_target(1100, damage_type::magical);

		if (target == nullptr) return;

		if (e->is_ready() && target->is_valid_target(e->range()) && !target->is_invulnerable())
		{
			e->cast(target, hit_chance::medium);
		}
		if (q->is_ready() && target->is_valid_target(q->range()) && !target->is_invulnerable())
		{
			q->cast(target, hit_chance::medium);
		}
		if (w->is_ready() && target->is_valid_target(w->range()) && !target->is_invulnerable())
		{
			if (myhero->get_distance(target->get_position()) < 420)
			{
				auto cast_position = w->get_prediction(target).get_unit_position();
				w->cast(cast_position, myhero->get_position().extend(target->get_position(), myhero->get_distance(target->get_position()) + 50));		
			}
			else
			{
				auto cast_position = w->get_prediction(target).get_unit_position();
				w->cast(cast_position, myhero->get_position());
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
		if (sender->is_valid() && !sender->is_dead() && sender->get_name() == "TaliyahQMis")
		{
			should_use_magnet = true;
			scheduler->delay_action(2.1f, [] {  should_use_magnet = false; });
		}
	}
}