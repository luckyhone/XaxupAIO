#include "../plugin_sdk/plugin_sdk.hpp"
#include "Teemo.h"

namespace teemo
{
    script_spell* q = nullptr;
    script_spell* w = nullptr;
    script_spell* r = nullptr;
    script_spell* flash = nullptr;

    TreeTab* main_tab = nullptr;

    namespace combo
    {
        TreeEntry* use_q = nullptr;
        TreeEntry* use_w = nullptr;
        TreeEntry* w_range = nullptr;
        TreeEntry* use_r = nullptr;
        TreeEntry* r_minimum_enemies = nullptr;
    }
    namespace harass
    {
        TreeEntry* use_q = nullptr;
    }
    namespace laneclear
    {
        TreeEntry* use_q = nullptr;
        TreeEntry* use_r = nullptr;
        TreeEntry* minimum_minions_to_r = nullptr;
    }
    namespace jungleclear
    {
        TreeEntry* use_q = nullptr;
        TreeEntry* use_r = nullptr;
        TreeEntry* place_on_teemo = nullptr;
        TreeEntry* min_monster_hp_to_r = nullptr;
    }
    namespace misc
    {
        TreeEntry* auto_q_on_gapclose = nullptr;
        TreeEntry* auto_q_if_killable = nullptr;
        TreeEntry* auto_w_on_gapclose = nullptr;
        TreeEntry* auto_r_cc = nullptr;
        TreeEntry* auto_r_cc_only_in_combo = nullptr;
        TreeEntry* auto_q_on_gapclose_dont_use_under_tower = nullptr;
    }
    namespace flee
    {
        TreeEntry* use_w = nullptr;
    }
    namespace draw
    {
        TreeEntry* draw_range_q = nullptr;
        TreeEntry* draw_range_r = nullptr;
        TreeEntry* q_color = nullptr;
        TreeEntry* r_color = nullptr;
    }

    enum Position
    {
        Line,
        Jungle
    };

    Position my_hero_region;

    void on_draw();
    int count_enemy_minions_in_range(float range, vector from);
    float get_q_raw_damage();
    void auto_q_if_killable();
    void on_update();
    void on_gapcloser(game_object_script sender, antigapcloser::antigapcloser_args* args);
    void q_logic();
    void r_combo_logic();
    void r_on_cc_logic();
    void w_logic();
    void on_after_attack(game_object_script target);

    float r_ranges[] = { 600.0f, 750.0f, 900.0f };
    std::vector<float> q_damages = { 80,125,170,215,260 };
    float q_ap_coef = 0.8f;
    float last_r_use_time = 0.0f;
    float r_arm_time = 1.0f;
    float r_missile_speed = 1000.0f;
    float buff_remaining_time_additional_time = 0.1f;
    float delay_between_r = 7.0f;
    float explosion_range = 425.0f;

    void load()
    {
        myhero->print_chat(0x3, "<font color=\"#FFFFFF\">[<b><font color=\"#3F704D\">Teemo | XaxupAIO</font></b>]:</font> <font color=\"#90EE90\">Loaded</font>");
        myhero->print_chat(0x3, "<font color=\"#3F704D\"><b>Suggested Prediction: </b><font color=\"#90EE90\">Aurora</font></font>");

        q = plugin_sdk->register_spell(spellslot::q, 680);
        w = plugin_sdk->register_spell(spellslot::w, 0);
        r = plugin_sdk->register_spell(spellslot::r, 400);
        r = plugin_sdk->register_spell(spellslot::r, r_ranges[0]);
        r->set_skillshot(0.25f, 75.0f, 1600.0f, { }, skillshot_type::skillshot_circle);

        if (myhero->get_spell(spellslot::summoner1)->get_spell_data()->get_name_hash() == spell_hash("SummonerFlash"))
            flash = plugin_sdk->register_spell(spellslot::summoner1, 400.f);
        else if (myhero->get_spell(spellslot::summoner2)->get_spell_data()->get_name_hash() == spell_hash("SummonerFlash"))
            flash = plugin_sdk->register_spell(spellslot::summoner2, 400.f);

        main_tab = menu->create_tab("teemo", "XaxupAIO");
        main_tab->set_assigned_texture(myhero->get_square_icon_portrait());
        {
            main_tab->add_separator(".predSep", "USE AURORA PRED");
            auto combo = main_tab->add_tab(myhero->get_model() + ".combo", "Combo");
            {
                combo->add_separator(".comboSep", "Combo");
                auto q_config = combo->add_tab(myhero->get_model() + "comboQConfig", "Q Settings");
                {
                    combo::use_q = q_config->add_checkbox(myhero->get_model() + ".comboUseQ", "Use Q", true);
                    combo::use_q->set_texture(myhero->get_spell(spellslot::q)->get_icon_texture());
                }
                auto w_config = combo->add_tab(myhero->get_model() + ".comboWConfig", "W Settings");
                {
                    combo::use_w = w_config->add_checkbox(myhero->get_model() + ".comboUseW", "Use W", true);
                    combo::use_w->set_texture(myhero->get_spell(spellslot::w)->get_icon_texture());
                    combo::w_range = w_config->add_slider(myhero->get_model() + ".comboWConfigRange", "Enemy In Range To Activate", 1000, 850, 1500);
                }
                auto r_config = combo->add_tab(myhero->get_model() + ".comboRConfig", "R Settings");
                {
                    combo::use_r = r_config->add_checkbox(myhero->get_model() + ".comboRConfigUseR", "Use R", true);
                    combo::r_minimum_enemies = r_config->add_slider(myhero->get_model() + ".comboRConfigMinREnemies", "Minimum Enemies In Explosion Range", 3, 1, 5);
                }
            }
            auto harass = main_tab->add_tab(myhero->get_model() + ".harass", "Harass");
            {
                harass->add_separator(".harassSep", "Harass");
                auto q_config = harass->add_tab(myhero->get_model() + ".harassQConfig", "Q Settings");
                {
                    harass::use_q = q_config->add_checkbox(myhero->get_model() + ".harassUseQ", "Use Q", true);
                    harass::use_q->set_texture(myhero->get_spell(spellslot::q)->get_icon_texture());
                }
            }
            auto laneclear = main_tab->add_tab(myhero->get_model() + ".laneclear", "Laneclear");
            {
                laneclear->add_separator(".laneSep", "Laneclear");
                auto q_config = laneclear->add_tab(myhero->get_model() + ".laneClearQConfig", "Q Settings");
                {
                    laneclear::use_q = q_config->add_checkbox(myhero->get_model() + ".laneClearUseQ", "Use Q", false);
                    laneclear::use_q->set_texture(myhero->get_spell(spellslot::q)->get_icon_texture());
                }
                auto r_config = laneclear->add_tab(myhero->get_model() + ".laneClearRConfig", "R Settings");
                {
                    laneclear::use_r = r_config->add_checkbox(myhero->get_model() + ".laneClearUseR", "Use R", true);
                    laneclear::use_r->set_texture(myhero->get_spell(spellslot::r)->get_icon_texture());
                    laneclear::minimum_minions_to_r = r_config->add_slider(myhero->get_model() + ".laneClearRMinMinions", "Minimum Minions To Use R", 3, 1, 6);
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
                auto r_config = jungleclear->add_tab(myhero->get_model() + ".jungleClearRConfig", "R Settings");
                {
                    jungleclear::use_r = r_config->add_checkbox(myhero->get_model() + ".jungleClearUseR", "Use R", true);
                    jungleclear::place_on_teemo = r_config->add_checkbox(myhero->get_model() + ".jungleClearPlaceOnTeemo", "Place On Teemo", true);
                    jungleclear::min_monster_hp_to_r = r_config->add_slider(myhero->get_model() + ".jungleClearMinHpToR", "Only Above X% Monster HP", 50, 0,100);
                    jungleclear::use_r->set_texture(myhero->get_spell(spellslot::r)->get_icon_texture());
                }
            }
            auto misc = main_tab->add_tab(myhero->get_model() + ".misc", "Misc");
            {
                misc->add_separator(".miscSep", "Misc");
                auto gapclose = misc->add_tab(myhero->get_model() + ".miscGapclose", "Gapclose");
                {
                    misc::auto_q_on_gapclose = gapclose->add_checkbox(myhero->get_model() + ".miscAutoQGapclose", "Auto Q On Gapclose", true);
                    misc::auto_q_on_gapclose_dont_use_under_tower = gapclose->add_checkbox(myhero->get_model() + ".miscAutoQGapcloseTower", "^~ don't use under tower", true);
                    misc::auto_q_on_gapclose->set_texture(myhero->get_spell(spellslot::q)->get_icon_texture());                   
                    misc::auto_w_on_gapclose = gapclose->add_checkbox(myhero->get_model() + ".miscAutoWGapclose", "Auto W On Gapclose", true);
                    misc::auto_w_on_gapclose->set_texture(myhero->get_spell(spellslot::w)->get_icon_texture());
                }
                auto r_on_cc = misc->add_tab(myhero->get_model() + ".miscRCC", "Auto R On CC");
                {
                    misc::auto_r_cc = r_on_cc->add_checkbox(myhero->get_model() + ".miscAutoRCC", "Enabled", true);
                    misc::auto_r_cc->set_texture(myhero->get_spell(spellslot::r)->get_icon_texture());
                    misc::auto_r_cc_only_in_combo = r_on_cc->add_checkbox(myhero->get_model() + ".miscAutoRCCInCombo", "^~ only in combo", false);
                }                
                auto auto_q_killable = misc->add_tab(myhero->get_model() + ".miscQKillable", "Auto Q If Killable");
                {
                    misc::auto_q_if_killable = auto_q_killable->add_checkbox(myhero->get_model() + ".miscAutoQKillableConfig", "Enabled", true);
                    misc::auto_q_if_killable->set_texture(myhero->get_spell(spellslot::q)->get_icon_texture());
                }
            }
            auto flee = main_tab->add_tab(myhero->get_model() + ".flee", "Flee");
            {
                flee->add_separator(".fleeSep", "Flee");
                flee::use_w = flee->add_checkbox(myhero->get_model() + ".fleeUseW", "Use W", true);
                flee::use_w->set_texture(myhero->get_spell(spellslot::w)->get_icon_texture());
            }
            auto draw = main_tab->add_tab(myhero->get_model() + ".draw", "Drawings");
            {
                draw->add_separator(".drawSep", "Drawings");
                float color[] = { 1.0f, 1.0f, 0.0f, 1.0f };
                draw::draw_range_q = draw->add_checkbox(myhero->get_model() + ".drawQ", "Draw Q Range", true);
                draw::draw_range_r = draw->add_checkbox(myhero->get_model() + ".drawR", "Draw R Range", true);
                draw::q_color = draw->add_colorpick(myhero->get_model() + ".drawQColor", "Q Color", color);
                draw::r_color = draw->add_colorpick(myhero->get_model() + ".drawRColor", "R Color", color);
            }
        }
        antigapcloser::add_event_handler(on_gapcloser);
        event_handler<events::on_draw>::add_callback(on_draw);
        event_handler<events::on_after_attack_orbwalker>::add_callback(on_after_attack);
        event_handler<events::on_update>::add_callback(on_update);
    }

    void unload()
    {
        menu->delete_tab(main_tab);

        plugin_sdk->remove_spell(q);
        plugin_sdk->remove_spell(w);
        plugin_sdk->remove_spell(r);

        if (flash)
            plugin_sdk->remove_spell(flash);

        antigapcloser::remove_event_handler(on_gapcloser);

        event_handler<events::on_update>::remove_handler(on_update);
        event_handler<events::on_draw>::remove_handler(on_draw);
        event_handler<events::on_after_attack_orbwalker>::remove_handler(on_after_attack);
    }

    void on_update()
    {
        if (myhero->is_dead()) return;

        if (orbwalker->can_move(0.05f))
        {
            auto_q_if_killable();

            if (r->is_ready() && misc::auto_r_cc->get_bool() && !misc::auto_r_cc_only_in_combo->get_bool() && gametime->get_time() > last_r_use_time + delay_between_r)
            {
                r_on_cc_logic();
            }

            if (orbwalker->combo_mode())
            {
                if (r->is_ready() && misc::auto_r_cc->get_bool() && misc::auto_r_cc_only_in_combo->get_bool() && gametime->get_time() > last_r_use_time + delay_between_r)
                {
                    r_on_cc_logic();
                }
                if (w->is_ready() && combo::use_w->get_bool())
                {
                    w_logic();
                }
                if (r->is_ready() && combo::use_r->get_bool() && gametime->get_time() > last_r_use_time + delay_between_r)
                {
                    r_combo_logic();
                }
            }
            if (orbwalker->harass())
            {
                if (q->is_ready() && harass::use_q->get_bool())
                {
                    q_logic();
                }
            }
            if (orbwalker->lane_clear_mode())
            {
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

                if (!lane_minions.empty()) my_hero_region = Position::Line;
                else if (!monsters.empty()) my_hero_region = Position::Jungle;
                    
                if (!lane_minions.empty())
                {
                    if (q->is_ready() && laneclear::use_q->get_bool())
                    {
                        if (q->cast(lane_minions.front()))
                        {
                            return;
                        }
                    }
                    if (r->is_ready() && laneclear::use_r->get_bool() && gametime->get_time() > last_r_use_time + delay_between_r)
                    {
                        for (auto&& enemy_minion : entitylist->get_enemy_minions())
                        {
                            if (enemy_minion != nullptr && enemy_minion->is_valid_target(r->range()))
                            {
                                auto predicted_position = r->get_prediction(enemy_minion).get_cast_position();
                                
                                if (count_enemy_minions_in_range(explosion_range, predicted_position) >= laneclear::minimum_minions_to_r->get_int() && enemy_minion->get_health_percent() > 10.0f)
                                {
                                    if (r->cast(enemy_minion))
                                    {
                                        last_r_use_time = gametime->get_time();
                                        return;
                                    }
                                }
                            }
                        }
                    }
                }      
                if (!monsters.empty())
                {
                    if (q->is_ready() && jungleclear::use_q->get_bool())
                    {
                        if (q->cast(monsters.front()))
                        {
                            return;
                        }
                    }

                    auto monster_hp_percentage = monsters.front()->get_health_percent();

                    if (r->is_ready() && jungleclear::use_r->get_bool() && gametime->get_time() > last_r_use_time + delay_between_r && monster_hp_percentage >= jungleclear::min_monster_hp_to_r->get_int())
                    {
                        if (jungleclear::place_on_teemo->get_bool() && r->cast(myhero))
                        {
                            last_r_use_time = gametime->get_time();
                            return;
                        }
                        if (!jungleclear::place_on_teemo->get_bool() && r->fast_cast(monsters.front()->get_position()))
                        {
                            last_r_use_time = gametime->get_time();
                            return;
                        }
                    }
                }
            }
            if (orbwalker->flee_mode())
            {
                if (w->is_ready() && flee::use_w->get_bool())
                {
                    if (w->cast())
                    {
                        return;
                    }
                }
            }
        }
    }

    void on_after_attack(game_object_script target)
    {
        if (q->is_ready() && target->is_ai_hero() && target->is_valid_target(q->range()))
        {
            if (((orbwalker->combo_mode() && combo::use_q->get_bool()) || (orbwalker->harass() && harass::use_q->get_bool())))
            {
                if (q->cast(target))
                {
                    myhero->issue_order(target);
                }
            }
        }
    }

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

    void q_logic()
    {
        auto target = target_selector->get_target(q->range(), damage_type::magical);

        if (target == nullptr || target_selector->has_spellshield(target)) return;

        if (target->get_distance(myhero) <= q->range())
        {
            q->cast(target);
        }
    }

    void auto_q_if_killable()
    {
        if (!misc::auto_q_if_killable || !q->is_ready()) return;

        for (auto&& enemy : entitylist->get_enemy_heroes())
        {
            if (enemy != nullptr && enemy->is_valid_target(q->range()))
            {
                float calculated_damage = damagelib->calculate_damage_on_unit(myhero, enemy, damage_type::magical, get_q_raw_damage());

                if (calculated_damage >= enemy->get_health()+5.0f)
                {
                    if (q->cast(enemy))
                    {
                        return;
                    }
                }
            }
        }
    }

    float get_q_raw_damage()
    {
        float ap = myhero->get_total_ability_power();

        float raw_damage = q_damages[q->level() - 1] + (q_ap_coef * ap);
        return raw_damage;
    }

    void w_logic()
    {
        int enemies_in_range = myhero->count_enemies_in_range(combo::w_range->get_int());

        if (enemies_in_range == 0) return;
        else
        {
            w->cast();
        }
    }

    void r_combo_logic()
    {
        for (auto&& enemy : entitylist->get_enemy_heroes())
        {
            if (enemy != nullptr && enemy->is_valid_target(r->range()))
            {
                auto predicted_position = r->get_prediction(enemy).get_cast_position();
                auto enemies_in_explosion_range = predicted_position.count_enemies_in_range(explosion_range);
                
                if (enemies_in_explosion_range >= combo::r_minimum_enemies->get_int())
                {
                    if (r->cast(enemy, hit_chance::high))
                    {
                        last_r_use_time = gametime->get_time();
                        return;
                    }
                }
                else
                {
                    continue;
                }
            }
        }
    }

    void r_on_cc_logic()
    {
        for (auto&& enemy : entitylist->get_enemy_heroes())
        {
            if (enemy != nullptr && myhero->get_distance(enemy->get_position()) <= r->range())
            {
                auto buff = enemy->get_buff_by_type({ buff_type::Stun, buff_type::Snare, buff_type::Knockup, buff_type::Asleep, buff_type::Suppression, buff_type::Taunt});
                auto zhonya = enemy->get_buff(1036096934);
                auto ga = enemy->get_buff(-718911512);

                if (buff != nullptr)
                {
                    float remaining_time = buff->get_remaining_time();
                    float r_travel_time = enemy->get_distance(myhero) / r_missile_speed;
                    
                    if (remaining_time + buff_remaining_time_additional_time >= r->get_delay() + r_arm_time + r_travel_time)
                    {
                        if (r->cast(enemy))
                        {
                            last_r_use_time = gametime->get_time();
                            return;
                        }
                    }
                }
                if (zhonya != nullptr)
                {
                    float remaining_time = zhonya->get_remaining_time();
                    float r_travel_time = enemy->get_distance(myhero) / r_missile_speed;

                    if (remaining_time + buff_remaining_time_additional_time >= r->get_delay() + r_arm_time + r_travel_time)
                    {
                        if (r->cast(enemy->get_position()))
                        {
                            last_r_use_time = gametime->get_time();
                            return;
                        }
                    }
                }
                if (ga != nullptr)
                {
                    float remaining_time = ga->get_remaining_time();
                    float r_travel_time = enemy->get_distance(myhero) / r_missile_speed;

                    if (remaining_time + buff_remaining_time_additional_time >= r->get_delay() + r_arm_time + r_travel_time)
                    {
                        if (r->cast(enemy->get_position()))
                        {
                            last_r_use_time = gametime->get_time();
                            return;
                        }
                    }
                }
            }
        }
    }

    void check_r_range()
    {
        if (!r->is_ready()) return;

        r->set_range(r_ranges[r->level() - 1]);
    }

    void on_gapcloser(game_object_script sender, antigapcloser::antigapcloser_args* args)
    {
        if (myhero->is_dead() || myhero->is_recalling()) return;

        if (myhero->is_under_enemy_turret())
        {
            if (!misc::auto_q_on_gapclose_dont_use_under_tower->get_bool())
            {
                if (q->is_ready() && misc::auto_q_on_gapclose->get_bool())
                {
                    if (sender->is_valid_target(q->range() + sender->get_bounding_radius()))
                    {
                        if (q->cast(sender))
                        {
                            return;
                        }
                    }
                }
            }
        }
        else
        {
            if (q->is_ready() && misc::auto_q_on_gapclose->get_bool())
            {
                if (sender->is_valid_target(q->range() + sender->get_bounding_radius()))
                {
                    if (q->cast(sender))
                    {
                        return;
                    }
                }
            }
        }
        if (w->is_ready() && misc::auto_w_on_gapclose->get_bool())
        {
            if (w->cast())
            {
                return;
            }
        }
    }

    void on_draw()
    {
        if (myhero->is_dead())
        {
            return;
        }

        if (q->is_ready() && draw::draw_range_q->get_bool())
        {
            draw_manager->add_circle(myhero->get_position(), q->range(), draw::q_color->get_color());
        }

        if (r->is_ready() && draw::draw_range_r->get_bool())
        {
            draw_manager->add_circle(myhero->get_position(), r_ranges[r->level()-1], draw::r_color->get_color());
        }
    }
}