#include "../plugin_sdk/plugin_sdk.hpp"
#include "Ziggs.h"
#include "Utilities.h"
#include "permashow.hpp"

namespace ziggs
{
    bool is_killable_with_r_auto_kill(game_object_script enemy, float additionalHP);
    bool is_killable_with_w(game_object_script enemy);
    bool is_killable_with_r(game_object_script enemy);
    inline void draw_dmg_rl(game_object_script target, float damage, unsigned long color);
    void auto_r_if_killable();
    void auto_w_on_chanelling_spells();
    void check_for_destroyable_turret();
    bool is_killable_with_q(game_object_script enemy);
    bool is_killable_with_w_r(game_object_script enemy);
    void korean_hidden_combo();
    void harass_logic();
    void e_in_w_pos_combo_logic();
    void basic_combo_logic();
    void on_process_spell_cast(game_object_script sender, spell_instance_script spell);
    void on_create_object(game_object_script sender);
    void on_buff_gain(game_object_script sender, buff_instance_script buff);
    void on_buff_lose(game_object_script sender, buff_instance_script buff);
    void on_gapcloser(game_object_script sender, antigapcloser::antigapcloser_args* args);
    void on_after_attack(game_object_script target);
    void on_draw();
    void on_update();

    script_spell* q = nullptr;
    script_spell* w = nullptr;
    script_spell* e = nullptr;
    script_spell* r = nullptr;

    namespace combo
    {
        TreeEntry* use_q;
        TreeEntry* use_w;
        TreeEntry* use_w_only_when_e_ready;
        TreeEntry* use_e;
        TreeEntry* use_r;
        TreeEntry* w_hp_diff_to_push_away;
        TreeEntry* korea_combo;
        TreeEntry* combo_mode;
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
        TreeEntry* use_e = nullptr;
        TreeEntry* e_minimum_minions = nullptr;
    }
    namespace jungleclear
    {
        TreeEntry* use_q = nullptr;
        TreeEntry* use_e = nullptr;
    }
    namespace misc
    {
        TreeEntry* auto_e_on_gapclose = nullptr;
        TreeEntry* auto_w_on_gapclose = nullptr;
        TreeEntry* auto_w_on_channeling = nullptr;
        TreeEntry* auto_w_on_destroyable_turret = nullptr;
        TreeEntry* use_on_self = nullptr;
        TreeEntry* use_r_if_killable = nullptr;
        TreeEntry* only_if_no_enemies_nearby = nullptr;
    }
    namespace flee
    {
        TreeEntry* use_w = nullptr;
    }
    namespace permashow
    {
        TreeEntry* semi_r_key = nullptr;
        TreeEntry* korean_combo = nullptr;
    }
    namespace draw
    {
        TreeEntry* draw_r_killable = nullptr;
        TreeEntry* draw_range_q = nullptr;
        TreeEntry* draw_range_w = nullptr;
        TreeEntry* draw_range_e = nullptr;
        TreeEntry* draw_range_r = nullptr;
        TreeEntry* q_color = nullptr;
        TreeEntry* w_color = nullptr;
        TreeEntry* e_color = nullptr;
        TreeEntry* r_color = nullptr;
    }
    
    std::vector<float> q_damages = { 85,135,185,235,285 };
    std::vector<float> w_damages = { 70,105,140,175,210 };
    std::vector<float> r_damages = { 200,300,400 };
    std::vector<float> w_turret_damages_hp_perc = { 25,27.5f,30,32.5f,35 };

    float q_coef = 0.65f;
    float w_coef = 0.5f;
    float r_coef = 1.1f;
    vector w_cast_pos;
    bool q_casted = false;
    bool w_casted = false;
    bool e_casted = false;
    bool r_casted = false;
    bool threw_w = false;
    bool korean_executing = false;

    TreeTab* main_tab = nullptr;

    void load()
    {
        myhero->print_chat(0x3, "<font color=\"#FFFFFF\">[<b><font color=\"#3F704D\">Ziggs | XaxupAIO</font></b>]:</font> <font color=\"#90EE90\">Loaded</font>");
        myhero->print_chat(0x3, "<font color=\"#3F704D\"><b>Suggested Prediction: </b><font color=\"#90EE90\">Core</font></font>");

        q = plugin_sdk->register_spell(spellslot::q, 1225);
        w = plugin_sdk->register_spell(spellslot::w, 1000);
        e = plugin_sdk->register_spell(spellslot::e, 900);
        r = plugin_sdk->register_spell(spellslot::r, 5000);

        q->set_skillshot(0.25, 133, 1700, { collisionable_objects::yasuo_wall }, skillshot_type::skillshot_line);
        w->set_skillshot(0.25, 325, 1750, { collisionable_objects::yasuo_wall }, skillshot_type::skillshot_circle);
        e->set_skillshot(0.25, 325, 1550, { collisionable_objects::yasuo_wall }, skillshot_type::skillshot_circle);
        r->set_skillshot(0.375, 525, 2000, { }, skillshot_type::skillshot_circle);

        q->set_spell_lock(false);
        w->set_spell_lock(false);
        e->set_spell_lock(false);
        r->set_spell_lock(false);

        main_tab = menu->create_tab("ziggs", "XaxupAIO");
        main_tab->set_assigned_texture(myhero->get_square_icon_portrait());
        main_tab->add_separator(".predSep", "USE CORE PRED");
        auto combo = main_tab->add_tab(".combo", "Combo");
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
                combo::use_w_only_when_e_ready = w_config->add_checkbox(myhero->get_model() + ".comboUseWOnlyWhenE", "^~ use only when E ready or when killable", false);
                combo::w_hp_diff_to_push_away = w_config->add_slider(".qPushAwaySlider", "Push Enemy Away If EnemyHP%-YourHP% >", 20, 1, 99);
            }
            auto e_config = combo->add_tab(myhero->get_model() + ".comboEConfig", "E Settings");
            {
                combo::use_e = e_config->add_checkbox(myhero->get_model() + ".comboEConfigUseR", "Use E", true);
                combo::use_e->set_texture(myhero->get_spell(spellslot::e)->get_icon_texture());
            }
            combo::korea_combo = combo->add_checkbox(".koreanHiddenCombo", "Use Korean Hidden Combo", true);
            combo::combo_mode = combo->add_combobox(".comboMode", "Combo Mode", { {"Basic Combo", myhero->get_square_icon_portrait()},{"E into W predicted pos", myhero->get_square_icon_portrait()} }, 0);
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
                harass::use_w = w_config->add_checkbox(myhero->get_model() + ".harassUseW", "Use W", false);
                harass::use_w->set_texture(myhero->get_spell(spellslot::w)->get_icon_texture());
            }
            auto e_config = harass->add_tab(myhero->get_model() + ".harassEConfig", "E Settings");
            {
                harass::use_e = e_config->add_checkbox(myhero->get_model() + ".harassUseE", "Use E", true);
                harass::use_e->set_texture(myhero->get_spell(spellslot::e)->get_icon_texture());
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
            auto e_cofig = laneclear->add_tab(myhero->get_model() + ".laneClearEConfig", "E Settings");
            {
                laneclear::use_e = e_cofig->add_checkbox(myhero->get_model() + ".laneClearUseE", "Use E", true);
                laneclear::use_e->set_texture(myhero->get_spell(spellslot::e)->get_icon_texture());
                laneclear::e_minimum_minions = e_cofig->add_slider(myhero->get_model() + ".laneClearUseESlier", "^~ minimum minions to hit", 3, 1,6);
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
            auto e_config = jungleclear->add_tab(myhero->get_model() + ".jungleClearEConfig", "E Settings");
            {
                jungleclear::use_e = e_config->add_checkbox(myhero->get_model() + ".jungleClearUseE", "Use E", true);
                jungleclear::use_e->set_texture(myhero->get_spell(spellslot::e)->get_icon_texture());
            }
        }
        auto flee = main_tab->add_tab(myhero->get_model() + ".flee", "Flee");
        {
            flee->add_separator(".fleeSep", "Flee");
            flee::use_w = flee->add_checkbox(myhero->get_model() + ".fleeW", "Use W", true);
        }
        auto misc = main_tab->add_tab(myhero->get_model() + ".misc", "Misc");
        {
            misc->add_separator(".miscSep", "Misc");
            auto gapclose = misc->add_tab(myhero->get_model() + ".miscGapclose", "Gapclose");
            {
                misc::auto_w_on_gapclose = gapclose->add_checkbox(myhero->get_model() + ".miscAutoWGapclose", "Auto W On Gapclose", true);
                misc::auto_w_on_gapclose->set_texture(myhero->get_spell(spellslot::w)->get_icon_texture());
                misc::auto_w_on_gapclose->set_texture(myhero->get_spell(spellslot::w)->get_icon_texture());
                misc::auto_e_on_gapclose = gapclose->add_checkbox(myhero->get_model() + ".miscAutoEGapclose", "Auto E On Gapclose", true);
                misc::auto_e_on_gapclose->set_texture(myhero->get_spell(spellslot::e)->get_icon_texture());               
                misc::use_on_self = gapclose->add_checkbox(myhero->get_model() + ".miscAutoEGapcloseSelf", "^~ use on self position (disabled = enemy pos)", true);
            }
            misc::auto_w_on_destroyable_turret = misc->add_checkbox(myhero->get_model() + ".miscAutoWTurrets", "Auto W On Destroyable Turrets", true);
            misc::auto_w_on_destroyable_turret->set_texture(myhero->get_spell(spellslot::w)->get_icon_texture());
            misc::auto_w_on_channeling = misc->add_checkbox(myhero->get_model() + ".miscAutoWChanelling", "Auto W On Channeling Spells", true);
            misc::auto_w_on_destroyable_turret->set_texture(myhero->get_spell(spellslot::w)->get_icon_texture());
            misc::use_r_if_killable = misc->add_checkbox(myhero->get_model() + ".miscAutoRKilalble", "Auto R if killable", true);
            misc::use_r_if_killable->set_texture(myhero->get_spell(spellslot::r)->get_icon_texture());
            misc::only_if_no_enemies_nearby = misc->add_checkbox(myhero->get_model() + ".miscAutoRKilalbleEnemies", "^~ only if no enemies nearby", true);
        }
        auto hotkeys = main_tab->add_tab(myhero->get_model() + ".hotkeys", "Hotkeys");
        {
            hotkeys->add_separator(".hotkeysSep", "Hotkeys");
            permashow::semi_r_key = hotkeys->add_hotkey(".rManualHotkey", "Semi R", TreeHotkeyMode::Hold, 'A', true);
        }
        auto draw = main_tab->add_tab(myhero->get_model() + ".draw", "Drawings");
        {
            draw->add_separator(".drawSep", "Drawings");
            float color[] = { 1.0f, 1.0f, 0.0f, 1.0f };
            draw::draw_r_killable = draw->add_checkbox(myhero->get_model() + ".drawRKillable", "Draw R Killable Text", true);
            draw::draw_range_q = draw->add_checkbox(myhero->get_model() + ".drawQ", "Draw Q Range", true);
            draw::draw_range_w = draw->add_checkbox(myhero->get_model() + ".drawW", "Draw W Range", true);
            draw::draw_range_e = draw->add_checkbox(myhero->get_model() + ".drawE", "Draw E Range", true);
            draw::draw_range_r = draw->add_checkbox(myhero->get_model() + ".drawR", "Draw R Range", true);
            draw::q_color = draw->add_colorpick(myhero->get_model() + ".drawQColor", "Q Color", color);
            draw::w_color = draw->add_colorpick(myhero->get_model() + ".drawQColor", "W Color", color);
            draw::e_color = draw->add_colorpick(myhero->get_model() + ".drawQColor", "E Color", color);
            draw::r_color = draw->add_colorpick(myhero->get_model() + ".drawRColor", "R Color", color);
        }

        {
            Permashow::Instance.Init(main_tab);
            Permashow::Instance.AddElement("Semi R", permashow::semi_r_key);
            Permashow::Instance.AddElement("Korean Hidden Combo", combo::korea_combo);
            Permashow::Instance.AddElement("Auto R If Killable", misc::use_r_if_killable);
        }

        antigapcloser::add_event_handler(on_gapcloser); 
        event_handler<events::on_draw>::add_callback(on_draw);
        event_handler<events::on_after_attack_orbwalker>::add_callback(on_after_attack);
        event_handler<events::on_update>::add_callback(on_update);
        event_handler<events::on_buff_gain>::add_callback(on_buff_gain);
        event_handler<events::on_buff_lose>::add_callback(on_buff_lose);
        event_handler<events::on_process_spell_cast>::add_callback(on_process_spell_cast);
        event_handler<events::on_create_object>::add_callback(on_create_object);
    }

    void unload()
    {
        menu->delete_tab(main_tab);

        Permashow::Instance.Destroy();

        antigapcloser::remove_event_handler(on_gapcloser);
        event_handler<events::on_update>::remove_handler(on_update);
        event_handler<events::on_draw>::remove_handler(on_draw);
        event_handler<events::on_buff_gain>::remove_handler(on_buff_gain);
        event_handler<events::on_buff_lose>::remove_handler(on_buff_lose);
        event_handler<events::on_after_attack_orbwalker>::remove_handler(on_after_attack);
        event_handler<events::on_process_spell_cast>::remove_handler(on_process_spell_cast);
        event_handler<events::on_create_object>::remove_handler(on_create_object);
    }

    void on_update()
    {
        if (myhero->is_dead()) return;

        if (orbwalker->can_move(0.05f))
        {
            if (r->is_ready() && permashow::semi_r_key->get_bool())
            {
                auto target = target_selector->get_target(r, damage_type::magical);

                if (target != nullptr)
                {
                    auto cast_position = r->get_prediction(target).get_unit_position();
                    r->cast(cast_position);
                }
            }

            korean_hidden_combo();

            if (korean_executing) return;

            auto_r_if_killable();
            check_for_destroyable_turret();
            auto_w_on_chanelling_spells();

            if (orbwalker->combo_mode())
            {
                if (!combo::use_w->get_bool()) w_casted = false;

                if (combo::combo_mode->get_int() == 0)
                {
                    basic_combo_logic();
                }
                if (combo::combo_mode->get_int() == 1)
                {
                    e_in_w_pos_combo_logic();
                }
            }
            if (orbwalker->harass())
            {
                harass_logic();
            }
            if (orbwalker->flee_mode())
            {
                if (w->is_ready() && flee::use_w->get_bool())
                {
                    vector mouse_pos = hud->get_hud_input_logic()->get_game_cursor_position();
                    vector my_pos = myhero->get_position();
                    vector w_cast_pos = mouse_pos.extend(my_pos, mouse_pos.distance(my_pos) + 100);

                    w->cast(w_cast_pos);
                    scheduler->delay_action(0.285f, [] {  w->cast(); });
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

                if (!lane_minions.empty())
                {
                    //Lane clear logic
                    if (q->is_ready() && laneclear::use_q->get_bool())
                    {
                        if (q->cast(lane_minions.front()))
                        {
                            return;
                        }
                    }
                    if (e->is_ready() && laneclear::use_e->get_bool() )
                    {
                        vector target_minion_pos = lane_minions.front()->get_position();

                        if (e->cast(lane_minions.front()->get_position()) && utilities::count_enemy_minions_in_range(305, target_minion_pos) >= laneclear::e_minimum_minions->get_int())
                        {
                            return;
                        }
                    }
                }
                if (!monsters.empty())
                {
                    //Jungle clear logic

                    if (q->is_ready() && jungleclear::use_q->get_bool())
                    {
                        if (q->cast(monsters.front()))
                        {
                            return;
                        }
                    }
                    if (e->is_ready() && jungleclear::use_e->get_bool())
                    {
                        if (e->cast(monsters.front()->get_position()))
                        {
                            return;
                        }
                    }
                }
            }
        }
    }

    void check_for_destroyable_turret()
    {
        if (myhero->is_dead() || !w->is_ready() || korean_executing || !misc::auto_w_on_destroyable_turret->get_bool()) return;

        for (auto&& turret : entitylist->get_enemy_turrets())
        {
            if (turret->is_valid_target(w->range()))
            {
                if (turret->get_health_percent() < w_turret_damages_hp_perc[w->level() - 1])
                {
                    w->cast(turret->get_position());
                }
            }
        }
    }

    void korean_hidden_combo()
    {
        if (myhero->is_dead() || myhero->is_recalling()) return;

        game_object_script target;

        for (auto&& enemy : entitylist->get_enemy_heroes())
        {
            if (enemy->is_valid_target(1350) && w->is_ready() && r->is_ready())
            {
                if(!utilities::has_cc(enemy)) return;
                if (is_killable_with_q(enemy)) return;
                if (enemy->get_health_percent() < 10) return;

                auto buff = enemy->get_buff_by_type({ buff_type::Stun, buff_type::Snare, buff_type::Knockup, buff_type::Asleep, buff_type::Suppression, buff_type::Taunt, buff_type::Grounded });
                if (buff != nullptr && buff->get_remaining_time() < 0.2f) return;

                if (is_killable_with_w_r(enemy))
                {
                    korean_executing = true;
                    combo::use_w->set_bool(false);
                    combo::use_q->set_bool(false);
                    combo::use_e->set_bool(false);

                    if (enemy->is_valid_target(w->range() - 150))
                    {
                        vector w_cast_pos = myhero->get_position().extend(enemy->get_position(), myhero->get_distance(enemy->get_position()) + 165.5f);
                        vector r_cast_pos = myhero->get_position().extend(enemy->get_position(), myhero->get_distance(enemy->get_position()) - 425.0f);
                        r->cast(r_cast_pos);
                        scheduler->delay_action(0.36f, [w_cast_pos] { w->cast(w_cast_pos); });
                        scheduler->delay_action(0.26f, [] { w->cast(); });       
                        scheduler->delay_action(0.51f, [] { combo::use_w->set_bool(true); korean_executing = false; combo::use_q->set_bool(true); combo::use_e->set_bool(true); });
                    }
                }
            }
        }
    } 

    bool is_killable_with_q(game_object_script enemy)
    {
        float q_calculated_damage = damagelib->calculate_damage_on_unit(myhero, enemy, damage_type::magical, utilities::get_ap_raw_damage(q, q_coef, q_damages));

        if (q_calculated_damage > enemy->get_health() + 10)
        {
            return true;
        }

        return false;
    }

    bool is_killable_with_r(game_object_script enemy)
    {      
        float r_calculated_damage = damagelib->calculate_damage_on_unit(myhero, enemy, damage_type::magical, utilities::get_ap_raw_damage(r, r_coef, r_damages));

        if (r_calculated_damage > enemy->get_health() + 10)
        {
            return true;
        }
         
        return false;
    }


    bool is_killable_with_w(game_object_script enemy)
    {
        float w_calculated_damage = damagelib->calculate_damage_on_unit(myhero, enemy, damage_type::magical, utilities::get_ap_raw_damage(w, w_coef, w_damages));

        if (w_calculated_damage > enemy->get_health()-5)
        {
            return true;
        }

        return false;
    }
    bool is_killable_with_r_auto_kill(game_object_script enemy, float additionalHP)
    {
        float r_calculated_damage = damagelib->calculate_damage_on_unit(myhero, enemy, damage_type::magical, utilities::get_ap_raw_damage(r, r_coef, r_damages));

        if (r_calculated_damage > enemy->get_health() + additionalHP)
        {
            return true;
        }

        return false;
    }


    bool is_killable_with_w_r(game_object_script enemy)
    {
        float w_calculated_damage = damagelib->calculate_damage_on_unit(myhero, enemy, damage_type::magical, utilities::get_ap_raw_damage(w, w_coef, w_damages));
        float r_calculated_damage = damagelib->calculate_damage_on_unit(myhero, enemy, damage_type::magical, utilities::get_ap_raw_damage(r, r_coef, r_damages));

        float w_r_calc_damage = w_calculated_damage + r_calculated_damage;

        if (w_r_calc_damage > enemy->get_health() + 10)
        {
            return true;
        }

        return false;
    }

    void harass_logic()
    {
        auto target = target_selector->get_target(1300, damage_type::magical);

        if (target != nullptr)
        {
            if (q->is_ready() && harass::use_e->get_bool())
            {
                q->cast(target, hit_chance::medium);
            }
            if (w->is_ready() && harass::use_w->get_bool() && target->get_distance(myhero->get_position()) <= w->range())
            {
                w->cast(target->get_position());
            }
            if (e->is_ready() && harass::use_e->get_bool() && target->get_distance(myhero->get_position()) <= e->range())
            {
                e->cast(target, hit_chance::medium);
            }
        }
    }

    void auto_r_if_killable()
    {
        if (!misc::use_r_if_killable->get_bool()) return;

        auto target = target_selector->get_target(r->range(), damage_type::magical);

        if(target != nullptr)
        {
            if (is_killable_with_q(target) && q->is_ready() && target->is_valid_target(q->range())) return;
            if (target->count_allies_in_range(550) >= 2 && target->get_health_percent() < 13) return;

            for (auto&& enemy : entitylist->get_enemy_heroes())
            {
                if (enemy->is_valid_target(e->range()))
                {
                    return;
                }
            }

            if (target->count_allies_in_range(700) == 0 && is_killable_with_r_auto_kill(target, 120))
            {
                auto cast_position = r->get_prediction(target).get_unit_position();            
                r->cast(cast_position);
            }
            if (target->count_allies_in_range(700) == 1 && is_killable_with_r_auto_kill(target, 370))
            {
                auto cast_position = r->get_prediction(target).get_unit_position();
                r->cast(cast_position);
            }
            if (target->count_allies_in_range(700) == 2 && is_killable_with_r_auto_kill(target, 650))
            {
                auto cast_position = r->get_prediction(target).get_unit_position();
                r->cast(cast_position);
            }
            if (target->count_allies_in_range(700) > 2 && is_killable_with_r_auto_kill(target, 1100))
            {
                auto cast_position = r->get_prediction(target).get_unit_position();
                r->cast(cast_position);
            }
        }
    }

    void e_in_w_pos_combo_logic()
    {
        auto target = target_selector->get_target(1250, damage_type::magical);

        if (target != nullptr && !target->is_invulnerable())
        {
            if (q->is_ready() && !target->is_invulnerable() && combo::use_q->get_bool() && target->get_distance(myhero->get_position()) <= q->range())
            {
                q->cast(target, hit_chance::medium);
            }
            if (w->is_ready() && !target->is_invulnerable() && !w_casted && !myhero->has_buff(buff_hash("ZiggsW")) && combo::use_w->get_bool() && target->get_distance(myhero->get_position()) <= w->range())
            {
                if (is_killable_with_w(target))
                {
                    w->cast(target, hit_chance::medium);
                }

                if (combo::w_hp_diff_to_push_away->get_int() < target->get_health_percent() - myhero->get_health_percent())
                {
                    if (!target->is_facing(myhero) && myhero->is_facing(target))
                    {
                        if (myhero->get_distance(target) <= w->range()) {

                            auto target_ms = target->get_move_speed();
                            auto dist_traveled_until_w_casted = target_ms * 0.25f;

                            auto dist = myhero->get_distance(target);
                            auto pos = myhero->get_position().extend(target->get_position(), dist + 165.5f + (dist_traveled_until_w_casted));

                            w_cast_pos = pos;

                            if(combo::use_w_only_when_e_ready->get_bool() && e->is_ready())
                                w->cast(pos);
                            if (!combo::use_w_only_when_e_ready->get_bool())
                                w->cast(pos);
                        }
                    }
                    if (!myhero->is_facing(target) && target->is_facing(myhero))
                    {
                        if (myhero->get_distance(target) <= w->range())
                        {

                            auto target_ms = target->get_move_speed();
                            auto dist_traveled_until_w_casted = target_ms * 0.25f;

                            auto dist = myhero->get_distance(target);
                            auto pos = myhero->get_position().extend(target->get_position(), dist - 165.5f - (dist_traveled_until_w_casted));

                            w_cast_pos = pos;

                            if (is_killable_with_w(target))
                            {
                                w->cast(pos);
                            }

                            if (combo::use_w_only_when_e_ready->get_bool() && e->is_ready())
                                w->cast(pos);
                            if (!combo::use_w_only_when_e_ready->get_bool())
                                w->cast(pos);
                        }
                    }
                }
                else
                {
                    if (myhero->get_health_percent() > target->get_health_percent() + 20)
                    {
                        if (myhero->get_distance(target) <= w->range()) {

                            auto target_ms = target->get_move_speed();
                            auto dist_traveled_until_w_casted = target_ms * 0.25f;

                            auto dist = myhero->get_distance(target);
                            auto pos = myhero->get_position().extend(target->get_position(), dist + 165.5f + (dist_traveled_until_w_casted));

                            if (combo::use_w_only_when_e_ready->get_bool() && e->is_ready())
                                w->cast(pos);
                            if (!combo::use_w_only_when_e_ready->get_bool())
                                w->cast(pos);
                        }
                        else
                        {
                            if (combo::use_w_only_when_e_ready->get_bool() && e->is_ready())
                                w->cast(target->get_position());
                            if (!combo::use_w_only_when_e_ready->get_bool())
                                w->cast(target->get_position());
                        }
                    }
                    else
                    {
                        if (combo::use_w->get_bool())
                        {
                            if (combo::use_w_only_when_e_ready->get_bool() && e->is_ready())
                                w->cast(target->get_position());
                            if (!combo::use_w_only_when_e_ready->get_bool())
                                w->cast(target->get_position());
                        }
                    }
                }
            }
            if (w->is_ready() && myhero->has_buff(buff_hash("ZiggsW")) && combo::use_w->get_bool())
            {
                w->cast();
            }
            if (target->has_buff(buff_hash("moveawaycollision")) && target->is_valid_target(e->range()))
            {
                auto pred_output = e->get_prediction(target);
                if (pred_output.get_cast_position().is_valid() && e->is_ready() && combo::use_e->get_bool())
                {
                    e->cast(pred_output.get_cast_position());
                }
            }
            if (combo::use_w->get_bool())
            {
                if (e->is_ready() && !w->is_ready() && !target->has_buff(buff_hash("moveawaycollision")) && !threw_w && combo::use_e->get_bool())
                {
                    e->cast(target);
                }
            }
            else
            {
                if (e->is_ready() && combo::use_e->get_bool() && target->is_valid_target(e->range()))
                {
                    e->cast(target);
                }
            }
        }
    }

    void basic_combo_logic()
    {
        auto target = target_selector->get_target(1250, damage_type::magical);

        if (target != nullptr && !target->is_invulnerable())
        {
            if (e->is_ready() && combo::use_e->get_bool() && target->get_distance(myhero->get_position()) <= e->range())
            {
                e->cast(target, hit_chance::medium);
            }
            if (q->is_ready() && combo::use_q->get_bool())
            {
                q->cast(target, hit_chance::medium);
            }
            if (w->is_ready() && !w_casted && !myhero->has_buff(buff_hash("ZiggsW") && combo::use_w->get_bool()) && target->get_distance(myhero->get_position()) <= w->range())
            {
                if (combo::w_hp_diff_to_push_away->get_int() < target->get_health_percent() - myhero->get_health_percent())
                {
                    if (!target->is_facing(myhero) && myhero->is_facing(target))
                    {
                        if (myhero->get_distance(target) <= w->range()) {

                            auto target_ms = target->get_move_speed();
                            auto dist_traveled_until_w_casted = target_ms * 0.25f;

                            auto dist = myhero->get_distance(target);
                            auto pos = myhero->get_position().extend(target->get_position(), dist + 165.5f + (dist_traveled_until_w_casted));

                            if(combo::use_w_only_when_e_ready->get_bool() && e_casted)
                                w->cast(pos);
                            if (!combo::use_w_only_when_e_ready->get_bool())
                                w->cast(pos);
                        }
                    }
                    if (!myhero->is_facing(target) && target->is_facing(myhero))
                    {
                        if (myhero->get_distance(target) <= w->range()) 
                        {
                            auto target_ms = target->get_move_speed();
                            auto dist_traveled_until_w_casted = target_ms * 0.25f;

                            auto dist = myhero->get_distance(target);
                            auto pos = myhero->get_position().extend(target->get_position(), dist - 165.5f - (dist_traveled_until_w_casted));

                            if (combo::use_w_only_when_e_ready->get_bool() && e_casted)
                                w->cast(pos);
                            if (!combo::use_w_only_when_e_ready->get_bool())
                                w->cast(pos);
                        }
                    }
                }
                else
                {
                    if (myhero->get_health_percent() > target->get_health_percent() + 20)
                    {
                        if (myhero->get_distance(target) <= w->range()) {

                            auto target_ms = target->get_move_speed();
                            auto dist_traveled_until_w_casted = target_ms * 0.25f;

                            auto dist = myhero->get_distance(target);
                            auto pos = myhero->get_position().extend(target->get_position(), dist + 165.5f + (dist_traveled_until_w_casted));

                            if (combo::use_w_only_when_e_ready->get_bool() && e_casted)
                                w->cast(pos);
                            if (!combo::use_w_only_when_e_ready->get_bool())
                                w->cast(pos);
                        }
                        else
                        {
                            if (combo::use_w_only_when_e_ready->get_bool() && e_casted)
                                w->cast(target->get_position());
                            if (!combo::use_w_only_when_e_ready->get_bool())
                                w->cast(target->get_position());
                        }
                    }
                    else
                    {
                        if (combo::use_w->get_bool())
                        {
                            if (combo::use_w_only_when_e_ready->get_bool() && e_casted)
                                w->cast(target->get_position());
                            if (!combo::use_w_only_when_e_ready->get_bool())
                                w->cast(target->get_position());
                        }
                    }
                }
            }
            if (w->is_ready() && myhero->has_buff(buff_hash("ZiggsW")) && combo::use_w->get_bool())
            {
                w->cast();
            }
        }
    }

    void auto_w_on_chanelling_spells()
    {
        if (myhero->is_dead() || !w->is_ready() || korean_executing || !misc::auto_w_on_channeling->get_bool()) return;

        for (auto&& enemy : entitylist->get_enemy_heroes())
        {
            if (enemy->is_valid_target(w->range()))
            {
                if (enemy->is_casting_interruptible_spell())
                {
                    w->cast(enemy->get_position());
                }
            }
        }
    }

    void on_create_object(game_object_script sender)
    {
        
    }

    void on_process_spell_cast(game_object_script sender, spell_instance_script spell)
    {
        if (sender == myhero)
        {
            if (spell->get_spellslot() == spellslot::w)
            {
                threw_w = true;
                w_casted = !w_casted;
            }            
            if (spell->get_spellslot() == spellslot::r)
            {
                scheduler->delay_action(0.26f, [] { korean_executing = false; });
            }
            if (spell->get_spellslot() == spellslot::e)
            {
                e_casted = true;
                scheduler->delay_action(1.0f, [] { e_casted = false; });
            }
        }
    }

    void on_after_attack(game_object_script target)
    {

    }

    void on_gapcloser(game_object_script sender, antigapcloser::antigapcloser_args* args)
    {
        if (myhero->is_dead() || myhero->is_recalling()) return;

        if (misc::auto_e_on_gapclose->get_bool())
        {
            if (e->is_ready())
            {
                if (misc::use_on_self->get_bool())
                {
                    e->cast(myhero->get_position());
                }
                else
                {
                    e->cast(sender->get_position());
                }
            }
        }
        if (misc::auto_w_on_gapclose->get_bool())
        {
            if (w->is_ready())
            {
                vector w_cast_pos = myhero->get_position().extend(sender->get_position(), 0);
                w->cast(w_cast_pos);
                scheduler->delay_action(0.26f, [] { w->cast(); });
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

                draw_manager->add_filled_rect(bar_pos, size, MAKE_COLOR(0, 255, 0, 255));
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
            draw_manager->add_circle(myhero->get_position(), 850, draw::q_color->get_color());
        }
        if (w->is_ready() && draw::draw_range_w->get_bool())
        {
            draw_manager->add_circle(myhero->get_position(), w->range(), draw::w_color->get_color());
        }
        if (e->is_ready() && draw::draw_range_e->get_bool())
        {
            draw_manager->add_circle(myhero->get_position(), e->range(), draw::e_color->get_color());
        }
        if (r->is_ready() && draw::draw_range_r->get_bool())
        {
            draw_manager->add_circle(myhero->get_position(), r->range(), draw::r_color->get_color());
        }
     
        for (auto& enemy : entitylist->get_enemy_heroes()) 
        {
            if (!enemy->is_dead() && enemy->is_valid() && enemy->is_hpbar_recently_rendered() && r->is_ready() && !enemy->is_zombie())
            {
                draw_dmg_rl(enemy, r->get_damage(enemy), 4294929002);
                if (is_killable_with_r(enemy))
                {
                    if (!draw::draw_r_killable->get_bool()) return;

                    auto pos = myhero->get_position();
                    renderer->world_to_screen(pos, pos); 
                    vector vector = { (float)renderer->screen_width()/2 - 250 ,(float)renderer->screen_height()-300 };
                    draw_manager->add_text_on_screen(vector, 4278249728, 38, "%s KILLABLE IN R RANGE", enemy->get_model_cstr());
                }
            }
        }
    }

    void on_buff_gain(game_object_script sender, buff_instance_script buff)
    {
    
    }

    void on_buff_lose(game_object_script sender, buff_instance_script buff)
    {
        if (sender == myhero)
        {
            if (buff->get_name() == "ZiggsW")
            {
                threw_w = false;
                w_casted = false;
            }
        }
    }
}