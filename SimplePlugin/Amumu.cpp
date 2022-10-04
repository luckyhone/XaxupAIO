#include "../plugin_sdk/plugin_sdk.hpp"
#include "Amumu.h"

namespace amumu
{
    script_spell* q = nullptr;
    script_spell* w = nullptr;
    script_spell* e = nullptr;
    script_spell* r = nullptr;
    script_spell* flash = nullptr;

    TreeTab* main_tab = nullptr;

    namespace combo
    {
        TreeEntry* use_q = nullptr;
        TreeEntry* use_w = nullptr;
        TreeEntry* use_e = nullptr;
        TreeEntry* use_r = nullptr;
        TreeEntry* r_minimum_enemies = nullptr;
        TreeEntry* r_flash_minimum_enemies = nullptr;
        TreeEntry* use_r_flash = nullptr;
        TreeEntry* q_go_to_most_crowded_enemy = nullptr;
        TreeEntry* most_crowded_only_if_r_up = nullptr;
    }
    namespace harass
    {
        TreeEntry* use_q = nullptr;
        TreeEntry* use_w = nullptr;
        TreeEntry* use_e = nullptr;
    }
    namespace laneclear
    {
        TreeEntry* use_q = nullptr;
        TreeEntry* use_w = nullptr;
        TreeEntry* use_e = nullptr;
        TreeEntry* e_minimum_minions = nullptr;
        TreeEntry* w_minimum_minions = nullptr;
    }
    namespace jungleclear
    {
        TreeEntry* use_q = nullptr;
        TreeEntry* use_w = nullptr;
        TreeEntry* use_e = nullptr;
        TreeEntry* min_monster_hp_to_q = nullptr;
    }
    namespace misc
    {
        TreeEntry* auto_q_on_gapclose = nullptr;
        TreeEntry* auto_r_if_killable = nullptr;
    }
    namespace draw
    {
        TreeEntry* draw_range_q = nullptr;
        TreeEntry* draw_range_we = nullptr;
        TreeEntry* draw_range_r = nullptr;
        TreeEntry* q_color = nullptr;
        TreeEntry* we_color = nullptr;
        TreeEntry* r_color = nullptr;
    }
    enum Position
    {
        Line,
        Jungle
    };

    Position my_hero_region;  
    void on_draw();
    int count_enemy_heroes_in_range(float range, vector from);
    int count_enemy_minions_in_range(float range, vector from);
    void check_for_killable_enemy();
    float get_r_raw_damage();
    void on_update();
    void on_gapcloser(game_object_script sender, antigapcloser::antigapcloser_args* args);
    void q_combo_logic();
    void q_harass_logic();
    void w_logic();
    void e_logic();
    void r_logic();

    float r_radius = 550.0f;
    std::vector<float> r_damages = { 200,300,400 };
    float r_ap_coef = 0.8f;

    void load()
    {
        q = plugin_sdk->register_spell(spellslot::q, 1100);
        w = plugin_sdk->register_spell(spellslot::w, 0);
        e = plugin_sdk->register_spell(spellslot::e, 0);
        r = plugin_sdk->register_spell(spellslot::r, 0);
        q->set_skillshot(0.25f, 80.0f, 2000.0f, { collisionable_objects::heroes, collisionable_objects::minions, collisionable_objects::yasuo_wall }, skillshot_type::skillshot_line);

        if (myhero->get_spell(spellslot::summoner1)->get_spell_data()->get_name_hash() == spell_hash("SummonerFlash"))
            flash = plugin_sdk->register_spell(spellslot::summoner1, 400.f);
        else if (myhero->get_spell(spellslot::summoner2)->get_spell_data()->get_name_hash() == spell_hash("SummonerFlash"))
            flash = plugin_sdk->register_spell(spellslot::summoner2, 400.f);

        main_tab = menu->create_tab("amumu", "XaxupAIO");
        main_tab->set_assigned_texture(myhero->get_square_icon_portrait());
        {
            auto combo = main_tab->add_tab(myhero->get_model() + ".combo", "Combo");
            {
                auto q_config = combo->add_tab(myhero->get_model() + "comboQConfig", "Q Settings");
                {
                    combo::use_q = q_config->add_checkbox(myhero->get_model() + ".comboUseQ", "Use Q", true);
                    combo::use_q->set_texture(myhero->get_spell(spellslot::q)->get_icon_texture());
                    combo::q_go_to_most_crowded_enemy = q_config->add_checkbox(myhero->get_model() + ".comboMostCrowdedQ", "Cast To Most Crowded Enemy (For Best R)", true);
                    combo::most_crowded_only_if_r_up = q_config->add_checkbox(myhero->get_model() + ".comboMostCrowdedQIfR", "^~ only if R up", true);
                }
                auto w_config = combo->add_tab(myhero->get_model() + ".comboWConfig", "W Settings");
                {
                    combo::use_w = w_config->add_checkbox(myhero->get_model() + ".comboUseW", "Use W", true);
                    combo::use_w->set_texture(myhero->get_spell(spellslot::w)->get_icon_texture());
                }
                auto e_config = combo->add_tab(myhero->get_model() + ".comboEConfig", "E Settings");
                {
                    combo::use_e = e_config->add_checkbox(myhero->get_model() + ".comboEConfigUseR", "Use E", true);
                }             
                auto r_config = combo->add_tab(myhero->get_model() + ".comboRConfig", "R Settings");
                {
                    combo::use_r = r_config->add_checkbox(myhero->get_model() + ".comboRConfigUseR", "Use R", true);
                    combo::r_minimum_enemies = r_config->add_slider(myhero->get_model() + ".comboRConfigMinREnemies", "^~ minimum enemies in range", 3, 1, 5);
                    combo::use_r_flash = r_config->add_checkbox(myhero->get_model() + ".comboRConfigUseRFlash", "Use R+Flash", true);
                    combo::r_flash_minimum_enemies = r_config->add_slider(myhero->get_model() + ".comboRFlashConfigMinREnemies", "^~ minimum enemies stunned", 3, 1, 5);
                }
            }
            auto harass = main_tab->add_tab(myhero->get_model() + ".harass", "Haras");
            {
                auto q_config = harass->add_tab(myhero->get_model() + ".harassQConfig", "Q Settings");
                {
                    harass::use_q = q_config->add_checkbox(myhero->get_model() + ".harassUseQ", "Use Q", true);
                    harass::use_q->set_texture(myhero->get_spell(spellslot::q)->get_icon_texture());
                }
                auto w_config = harass->add_tab(myhero->get_model() + ".harassWConfig", "W Settings");
                {
                    harass::use_w = w_config->add_checkbox(myhero->get_model() + ".harassUseW", "Use W", true);
                    harass::use_w->set_texture(myhero->get_spell(spellslot::q)->get_icon_texture());
                }
                auto e_config = harass->add_tab(myhero->get_model() + ".harassEConfig", "E Settings");
                {
                    harass::use_e = e_config->add_checkbox(myhero->get_model() + ".harassUseE", "Use E", true);
                    harass::use_e->set_texture(myhero->get_spell(spellslot::q)->get_icon_texture());
                }
            }
            auto laneclear = main_tab->add_tab(myhero->get_model() + ".laneclear", "Laneclear");
            {
                auto q_config = laneclear->add_tab(myhero->get_model() + ".laneClearQConfig", "Q Settings");
                {
                    laneclear::use_q = q_config->add_checkbox(myhero->get_model() + ".laneClearUseQ", "Use Q", false);
                    laneclear::use_q->set_texture(myhero->get_spell(spellslot::q)->get_icon_texture());
                }
                auto w_config = laneclear->add_tab(myhero->get_model() + ".laneClearWConfig", "W Settings");
                {
                    laneclear::use_w = w_config->add_checkbox(myhero->get_model() + ".laneClearUseW", "Use W", true);
                    laneclear::use_w->set_texture(myhero->get_spell(spellslot::q)->get_icon_texture());
                    laneclear::w_minimum_minions = w_config->add_slider(myhero->get_model() + ".laneClearWMinMinions", "^~ minimum minions to cast", 3, 1, 6);
                }
                auto e_cofig = laneclear->add_tab(myhero->get_model() + ".laneClearEConfig", "E Settings");
                {
                    laneclear::use_e = e_cofig->add_checkbox(myhero->get_model() + ".laneClearUseE", "Use E", true);
                    laneclear::e_minimum_minions = e_cofig->add_slider(myhero->get_model() + ".laneClearEMinMinions", "^~ minimum minions to cast", 3, 1, 6);               
                    laneclear::use_e->set_texture(myhero->get_spell(spellslot::q)->get_icon_texture());
                }
            }
            auto jungleclear = main_tab->add_tab(myhero->get_model() + ".jungleClear", "Jungleclear");
            {
                auto q_config = jungleclear->add_tab(myhero->get_model() + ".jungleClearQConfig", "Q Settings");
                {
                    jungleclear::use_q = q_config->add_checkbox(myhero->get_model() + ".jungleClearUseQ", "Use Q", true);
                    jungleclear::use_q->set_texture(myhero->get_spell(spellslot::q)->get_icon_texture());
                }
                auto w_config = jungleclear->add_tab(myhero->get_model() + ".jungleClearWConfig", "W Settings");
                {
                    jungleclear::use_w = w_config->add_checkbox(myhero->get_model() + ".jungleClearUseW", "Use W", true);
                    jungleclear::use_w->set_texture(myhero->get_spell(spellslot::q)->get_icon_texture());
                }
                auto e_config = jungleclear->add_tab(myhero->get_model() + ".jungleClearEConfig", "E Settings");
                {
                    jungleclear::use_e = e_config->add_checkbox(myhero->get_model() + ".jungleClearUseE", "Use E", true);
                    jungleclear::use_e->set_texture(myhero->get_spell(spellslot::q)->get_icon_texture());
                }
            }
            auto misc = main_tab->add_tab(myhero->get_model() + ".misc", "Misc");
            {
                misc::auto_q_on_gapclose = misc->add_checkbox(myhero->get_model() + ".miscAutoQGapclose", "Auto Q On Gapclose", true);
                misc::auto_q_on_gapclose->set_texture(myhero->get_spell(spellslot::q)->get_icon_texture());
                misc::auto_r_if_killable = misc->add_checkbox(myhero->get_model() + ".miscAutoRIfKillable", "Auto R If Killable", true);
                misc::auto_r_if_killable->set_texture(myhero->get_spell(spellslot::r)->get_icon_texture());
            }
            auto draw = main_tab->add_tab(myhero->get_model() + ".draw", "Drawings");
            {
                float color[] = { 1.0f, 1.0f, 0.0f, 1.0f };
                draw::draw_range_q = draw->add_checkbox(myhero->get_model() + ".drawQ", "Draw Q Range", true);
                draw::draw_range_we = draw->add_checkbox(myhero->get_model() + ".drawWE", "Draw W/E Range", true);
                draw::draw_range_r = draw->add_checkbox(myhero->get_model() + ".drawR", "Draw R Range", true);
                draw::q_color = draw->add_colorpick(myhero->get_model() + ".drawQColor", "Q Color", color);
                draw::we_color = draw->add_colorpick(myhero->get_model() + ".drawWEColor", "W/E Color", color);
                draw::r_color = draw->add_colorpick(myhero->get_model() + ".drawRColor", "R Color", color);
            }
        }

        antigapcloser::add_event_handler(on_gapcloser);
        event_handler<events::on_draw>::add_callback(on_draw);
        event_handler<events::on_update>::add_callback(on_update);
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
        event_handler<events::on_draw>::remove_handler(on_draw);
        event_handler<events::on_update>::remove_handler(on_update);        
    }

    void on_update()
    {
        if (myhero->is_dead()) return;

        if (orbwalker->can_move(0.05f))
        {
            check_for_killable_enemy();

            if (orbwalker->combo_mode())
            {
                if (q->is_ready() && combo::use_q->get_bool())
                {
                    q_combo_logic();
                }
                if (r->is_ready() && combo::use_r->get_bool())
                {
                    r_logic();
                }
                if (w->is_ready() && combo::use_w->get_bool())
                {
                    w_logic();
                }
                if (e->is_ready() && combo::use_e->get_bool())
                {
                    e_logic();
                }
            }
            if (orbwalker->harass())
            {
                if (q->is_ready() && harass::use_q->get_bool())
                {
                    q_harass_logic();
                }
                if (w->is_ready() && harass::use_w->get_bool())
                {
                    w_logic();
                }
                if (e->is_ready() && harass::use_e->get_bool())
                {
                    e_logic();
                }
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

                                if (!lane_minions.empty()) my_hero_region = Position::Line;
                                else if (!monsters.empty()) my_hero_region = Position::Jungle;
                #pragma endregion

                if (!lane_minions.empty())
                {
                    if (q->is_ready() && laneclear::use_q->get_bool())
                    {
                        if (q->cast(lane_minions.front()))
                        {
                            return;
                        }                       
                    }
                    if (w->is_ready() && laneclear::use_w->get_bool())
                    {
                        auto minions_around = count_enemy_minions_in_range(345.0f, myhero->get_position());
                        if (minions_around >= laneclear::w_minimum_minions->get_int() && !myhero->has_buff(1839643913))
                        {
                            if (w->cast())
                            {
                                return;
                            }
                        }
                    }                    
                    if (e->is_ready() && laneclear::use_e->get_bool())
                    {
                        auto minions_around = count_enemy_minions_in_range(345.0f, myhero->get_position());
                        if (minions_around >= laneclear::e_minimum_minions->get_int())
                        {
                            if (e->cast())
                            {
                                return;
                            }
                        }
                    }
                }
                if (!monsters.empty())
                {
                    //Jungleclear logic
                    if (q->is_ready() && jungleclear::use_q->get_bool())
                    {
                        if (q->cast(monsters.front()))
                        {
                            return;
                        }
                    }
                    if (w->is_ready() && jungleclear::use_w->get_bool())
                    {
                        if (!myhero->has_buff(1839643913) && monsters.front()->get_distance(myhero) <= 350.0f)
                        {
                            if (w->cast())
                            {
                                return;
                            }
                        }
                    }
                    if (e->is_ready() && jungleclear::use_e->get_bool() && monsters.front()->get_distance(myhero) <= 350.0f)
                    {
                        if (e->cast())
                        {
                            return;
                        }
                    }
                }
            }
        }
    }

    void q_combo_logic()
    {
        if (combo::q_go_to_most_crowded_enemy->get_bool() && combo::most_crowded_only_if_r_up->get_bool() && r->is_ready())
        {
            game_object_script best_enemy;
            int current_enemies = 0;
            for (auto&& enemy : entitylist->get_enemy_heroes())
            {
                if (enemy != nullptr && enemy->is_valid_target(q->range()))
                {
                    auto predicted_position = q->get_prediction(enemy);
                    auto enemies_in_r_range = predicted_position.get_cast_position().count_enemies_in_range(r_radius);

                    if (enemies_in_r_range >= current_enemies)
                    {
                        best_enemy = enemy;
                        current_enemies = enemies_in_r_range;
                    }
                }
            }

            if (q->cast(best_enemy, hit_chance::medium))
            {
                return;
            }
        }
        if (combo::q_go_to_most_crowded_enemy->get_bool() && !combo::most_crowded_only_if_r_up->get_bool())
        {
            game_object_script best_enemy;
            int current_enemies = 0;
            for (auto&& enemy : entitylist->get_enemy_heroes())
            {
                if (enemy != nullptr && enemy->is_valid_target(q->range()))
                {
                    auto predicted_position = q->get_prediction(enemy);
                    auto enemies_in_r_range = predicted_position.get_cast_position().count_enemies_in_range(r_radius);
                    
                    if (enemies_in_r_range >= current_enemies)
                    {
                        best_enemy = enemy;
                        current_enemies = enemies_in_r_range;
                    }
                }
            }

            if (q->cast(best_enemy, hit_chance::high))
            {
                return;
            }
        }

        auto target = target_selector->get_target(q->range(), damage_type::magical);

        if (target == nullptr) return;

        if (target->get_distance(myhero) <= q->range())
        {
            if (q->cast(target, hit_chance::high))
            {
                return;
            }
        }     
    }

    void q_harass_logic()
    {
        auto target = target_selector->get_target(q->range(), damage_type::magical);

        if (target == nullptr) return;

        if (target->get_distance(myhero) <= q->range())
        {
            if (q->cast(target, hit_chance::high))
            {
                return;
            }
        }
    }   

    void w_logic()
    {
        auto enemy_in_w_range = myhero->count_enemies_in_range(345.0f);

        if (enemy_in_w_range <= 0 && !myhero->has_buff(1839643913)) return;
        if (myhero->has_buff(1839643913) && myhero->count_enemies_in_range(450.0f) <= 0)
        {
            w->cast();
            return;
        }

        if (!myhero->has_buff(1839643913))
        {
            if (w->cast())
            {
                return;
            }
        }
    }

    void e_logic()
    {
        auto enemy_in_w_range = myhero->count_enemies_in_range(345.0f);

        if (enemy_in_w_range <= 0) return;

        if (e->cast())
        {
            return;
        }
    }

    void r_logic()
    {
        auto enemies_in_r_range = myhero->count_enemies_in_range(r_radius);

        if (enemies_in_r_range >= combo::r_minimum_enemies->get_int())
        {
            if (r->cast())
            {
                return;
            }
        }
        else
        {
            if (!combo::use_r_flash->get_bool()) return;

            game_object_script best_enemy;
            int current_max_enemies = 0;

            for (auto&& enemy : entitylist->get_enemy_heroes())
            {
                if (enemy != nullptr && enemy->is_valid_target(r_radius+flash->range()))
                {
                    vector enemy_position = enemy->get_position();
                    vector position_after_flash = myhero->get_position().extend(enemy_position, flash->range());
      
                    auto enemies_in_r_range = position_after_flash.count_enemies_in_range(r_radius-20);

                    if (enemies_in_r_range >= current_max_enemies)
                    {
                        best_enemy = enemy;
                        current_max_enemies = enemies_in_r_range;
                    }
                }
            }

            if (best_enemy != nullptr && current_max_enemies >= combo::r_flash_minimum_enemies->get_int() && myhero->get_distance(best_enemy) < (flash->range() + r_radius - 20))
            {
                flash->cast(best_enemy);
                r->cast();
            }
        }
    }

    void on_gapcloser(game_object_script sender, antigapcloser::antigapcloser_args* args)
    {
        if (q->is_ready() && misc::auto_q_on_gapclose->get_bool())
        {
            if (sender->is_valid_target(q->range() + sender->get_bounding_radius()))
            {
                if (q->cast(sender, hit_chance::medium))
                {
                    return;
                }
            }
        }
    }

    void check_for_killable_enemy()
    {
        if (!misc::auto_r_if_killable) return;

        auto enemies_in_r_range = myhero->count_enemies_in_range(r_radius);

        if (enemies_in_r_range == 0 || !r->is_ready()) return;

        for (auto&& enemy : entitylist->get_enemy_heroes())
        {
            if (enemy != nullptr && enemy->is_valid_target(r_radius))
            {
                float calculated_damage = damagelib->calculate_damage_on_unit(myhero, enemy, damage_type::magical, get_r_raw_damage());
                //5f to make sure HP doesn't regenerate before Q hit
                if (calculated_damage >= enemy->get_health() + 5.0f)
                {
                    if (r->cast() && r->is_ready())
                    {
                        return;
                    }
                }
            }
        }
    }

    float get_r_raw_damage()
    {
        float ap = myhero->get_total_ability_power();

        float raw_damage = r_damages[r->level() - 1] + (r_ap_coef * ap);
        return raw_damage;
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
    int count_enemy_heroes_in_range(float range, vector from)
    {
        int count = 0;
        for (auto&& t : entitylist->get_enemy_heroes())
        {
            if (t->is_valid_target(range, from))
                count++;
        }
        return count;
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

        if (w->is_ready() && draw::draw_range_we->get_bool())
        {
            draw_manager->add_circle(myhero->get_position(), 350.0f, draw::we_color->get_color());
        }
        if (e->is_ready() && draw::draw_range_we->get_bool())
        {
            draw_manager->add_circle(myhero->get_position(), 350.0f, draw::we_color->get_color());
        }
        if (r->is_ready() && draw::draw_range_r->get_bool())
        {
            draw_manager->add_circle(myhero->get_position(), r_radius, draw::r_color->get_color());
        }
    }
}