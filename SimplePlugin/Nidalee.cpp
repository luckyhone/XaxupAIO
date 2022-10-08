#include "../plugin_sdk/plugin_sdk.hpp"
#include "Nidalee.h"
#include "Utilities.h"

namespace nidalee
{
    typedef void(__cdecl* PropertyChangeCallback)(TreeEntry*);

    script_spell* q_human = nullptr;
    script_spell* w_human = nullptr;
    script_spell* e_human = nullptr;    
    script_spell* q_panther = nullptr;
    script_spell* w_panther = nullptr;
    script_spell* e_panther = nullptr;
    script_spell* r = nullptr;
    script_spell* flash = nullptr;

    TreeTab* main_tab = nullptr;

    namespace combo
    {
        TreeEntry* use_aa_q_in_combo;
        TreeEntry* use_q_human;
        TreeEntry* q_human_range;
        TreeEntry* use_e_human;
        TreeEntry* use_q_panther;
        TreeEntry* use_w_panther;
        TreeEntry* use_e_panther;
    }   
    namespace harass
    {
        TreeEntry* use_q_human;
    }
    namespace laneclear
    {
        TreeEntry* use_q_panther;
        TreeEntry* use_w_panther;
        TreeEntry* use_e_panther;
        TreeEntry* w_panther_min_minions;
        TreeEntry* e_panther_min_minions;
        TreeEntry* auto_switch_to_panther;
        TreeEntry* dont_switch_under_turret;
    }
    namespace jungleclear
    {
        TreeEntry* use_q_human;
        TreeEntry* use_w_human;
        TreeEntry* use_e_human;
        TreeEntry* use_q_panther;
        TreeEntry* use_w_panther;
        TreeEntry* use_e_panther;
        TreeEntry* auto_switch_to_panther;
        TreeEntry* auto_switch_to_human;
        TreeEntry* min_monster_hp_to_q;
        TreeEntry* ignore_on_epic_monsters;
    }
    namespace misc
    {
        TreeEntry* auto_w_on_cc;
        TreeEntry* auto_switch_to_panther_on_gapclose;
        TreeEntry* auto_w_on_gapclose;
        TreeEntry* auto_e_allies;
        TreeEntry* auto_e_allies_hp_under;        
        TreeEntry* auto_e_on_self;
        TreeEntry* auto_e_self_hp_under;
    }
    namespace flee
    {
        TreeEntry* use_w_panther;
    }
    namespace draw
    {
        TreeEntry* draw_range_q = nullptr;
        TreeEntry* draw_range_w = nullptr;
        TreeEntry* draw_range_e = nullptr;
        TreeEntry* q_color = nullptr;
        TreeEntry* w_color = nullptr;
        TreeEntry* e_color = nullptr;
    }

    enum Position
    {
        Line,
        Jungle
    };

    Position my_hero_region;
    void auto_w_on_cc_logic();
    void auto_heal_allies();
    bool is_human();
    void on_process_spell_cast(game_object_script sender, spell_instance_script spell);
    void on_buff_gain(game_object_script sender, buff_instance_script buff);
    bool is_panther();
    void panther_w_logic();
    void panther_e_logic();
    void human_q_logic();
    void on_update();
    void on_draw();
    void on_gapcloser(game_object_script sender, antigapcloser::antigapcloser_args* args);
    void on_after_attack(game_object_script target);

    float w_cast_time = 0.25f;

    void load()
    {
        myhero->print_chat(0x3, "<font color=\"#FFFFFF\">[<b><font color=\"#3F704D\">Nidalee | XaxupAIO</font></b>]:</font> <font color=\"#90EE90\">Loaded</font>");
        myhero->print_chat(0x3, "<font color=\"#3F704D\"><b>Suggested Prediction: </b><font color=\"#90EE90\">Aurora</font></font>");

        q_human = plugin_sdk->register_spell(spellslot::q, 1500);
        w_human = plugin_sdk->register_spell(spellslot::w, 1000);
        e_human = plugin_sdk->register_spell(spellslot::e, 900);        
        q_panther = plugin_sdk->register_spell(spellslot::q, 0);
        w_panther = plugin_sdk->register_spell(spellslot::w, 500);
        e_panther = plugin_sdk->register_spell(spellslot::e, 0);
        r = plugin_sdk->register_spell(spellslot::r, 0);

        q_human->set_skillshot(0.25f, 40, 1300, { collisionable_objects::heroes, collisionable_objects::minions, collisionable_objects::yasuo_wall }, skillshot_type::skillshot_line);      

        if (myhero->get_spell(spellslot::summoner1)->get_spell_data()->get_name_hash() == spell_hash("SummonerFlash"))
            flash = plugin_sdk->register_spell(spellslot::summoner1, 400.f);
        else if (myhero->get_spell(spellslot::summoner2)->get_spell_data()->get_name_hash() == spell_hash("SummonerFlash"))
            flash = plugin_sdk->register_spell(spellslot::summoner2, 400.f);

        main_tab = menu->create_tab("nidalee", "XaxupAIO");
        main_tab->set_assigned_texture(myhero->get_square_icon_portrait());
        {
            main_tab->add_separator(".predSep", "USE AURORA PRED");
            auto combo = main_tab->add_tab(myhero->get_model() + ".combo", "Combo");
            {
                combo->add_separator(".humanComboSeparator", "Human Settings");
                auto humanQCombo = combo->add_tab(".humanQComboConfig", "Q Settings");
                {
                    combo::use_q_human = humanQCombo->add_checkbox(".useQHumanCombo", "Use Q", true);
                    combo::q_human_range = humanQCombo->add_slider(".useQHumanComboRangeSlider", "Q Max Range", 1450, 500, 1500);
                }                
                auto humanECombo = combo->add_tab(".humanEComboConfig", "E Settings");
                {
                    combo::use_e_human = humanECombo->add_checkbox(".useEHumanCombo", "Use E after Q (for AS boost and heal)", true);                  
                }
                combo->add_separator(".pantherComboSeparator", "Panther Settings");
                combo::use_aa_q_in_combo = combo->add_checkbox(".dontAAQPantherCombo", "Use AA-Q in combo (off = just Q)", false);
                auto pantherQCombo = combo->add_tab(".pantherQComboConfig", "Q Settings");
                {
                    combo::use_q_panther = pantherQCombo->add_checkbox(".useQPantherCombo", "Use Q", true);
                }
                auto pantherWCombo = combo->add_tab(".pantherWComboConfig", "W Settings");
                {
                    combo::use_w_panther = pantherWCombo->add_checkbox(".useWPantherCombo", "Use W", true);
                }
                auto pantherECombo = combo->add_tab(".pantherEComboConfig", "E Settings");
                {
                    combo::use_e_panther = pantherECombo->add_checkbox(".useEPantherCombo", "Use E", true);
                }                
            }
            auto harass = main_tab->add_tab(myhero->get_model() + "harassS", "Harass");
            {
                auto humanQHarass = harass->add_tab(".humanQHrassConfig", "Q Settings");
                {
                    harass::use_q_human = humanQHarass->add_checkbox(".useQHumanHarass", "Use Q", true);
                }
            }
            auto laneclear = main_tab->add_tab(myhero->get_model() + ".laneclear", "Laneclear");
            {
                laneclear->add_separator(".pantherLaneclearSeparator", "Panther Settings");
                auto pantherQLaneclear = laneclear->add_tab(".pantherQLaneclear", "Q Settings");
                {
                    laneclear::use_q_panther = pantherQLaneclear->add_checkbox(".useQPantherLaneclear", "Use Q", true);
                }
                auto pantherWLaneclear = laneclear->add_tab(".pantherWLaneclear", "W Settings");
                {
                    laneclear::use_w_panther = pantherWLaneclear->add_checkbox(".useWPantherLaneclear", "Use W", true);
                    laneclear::w_panther_min_minions = pantherWLaneclear->add_slider(".useWPantherLaneclearSlider", "^~ minimum minions to hit", 1, 1, 6);
                }
                auto pantherELaneclear = laneclear->add_tab(".pantherELaneclear", "E Settings");
                {
                    laneclear::use_e_panther = pantherELaneclear->add_checkbox(".useEPantherLaneclear", "Use E", true);
                    laneclear::e_panther_min_minions = pantherELaneclear->add_slider(".useEPantherLaneclearSlider", "^~ minimum minions to hit", 2, 1, 6);
                }
                laneclear::auto_switch_to_panther = laneclear->add_checkbox(".laneclearAutoSwitchPanther", "Auto Switch To Panther", true);
                laneclear::dont_switch_under_turret = laneclear->add_checkbox(".laneclearAutoSwitchPantherTurret", "^~ don't switch under enemy turret", true);
            }
            auto jungleclear = main_tab->add_tab(myhero->get_model() + ".jungleClear", "Jungleclear");
            {
                jungleclear->add_separator(".humanJungleSeparator", "Human Settings");
                auto humanQJungle = jungleclear->add_tab(".humanQJungleConfig", "Q Settings");
                {
                    jungleclear::use_q_human = humanQJungle->add_checkbox(".useQHumanJungle", "Use Q", true);
                    jungleclear::min_monster_hp_to_q = humanQJungle->add_slider(".useQHumanJungleSlider", "^~ only if monster HP > X%", 25,1,99);
                    jungleclear::ignore_on_epic_monsters = humanQJungle->add_checkbox(".useQHumanJungleSliderIgnore", "  ^~ ignore on epic monsters", true);
                }               
                auto humanWJungle = jungleclear->add_tab(".humanWJungleConfig", "W Settings");
                {
                    jungleclear::use_w_human = humanWJungle->add_checkbox(".useWHumanJungle", "Use W", true);
                }
                auto humanEJungle = jungleclear->add_tab(".humanEJungleConfig", "E Settings");
                {
                    jungleclear::use_e_human = humanEJungle->add_checkbox(".useEHumanJungle", "Use E", true);
                }
                jungleclear->add_separator(".pantherJungleSeparator", "Panther Settings");
                auto pantherQJungle = jungleclear->add_tab(".pantherQJungleConfig", "Q Settings");
                {
                    jungleclear::use_q_panther = pantherQJungle->add_checkbox(".useQPantherJungle", "Use Q", true);
                }
                auto pantherWJungle = jungleclear->add_tab(".pantherWJungleConfig", "W Settings");
                {
                    jungleclear::use_w_panther = pantherWJungle->add_checkbox(".useWPantherJungle", "Use W", true);
                }
                auto pantherEJungle = jungleclear->add_tab(".pantherEComboJungle", "E Settings");
                {
                    jungleclear::use_e_panther = pantherEJungle->add_checkbox(".useEPantherJungle", "Use E", true);
                }
                jungleclear->add_separator(".pantherJungleGeneralSeparator", "General Settings");
                jungleclear::auto_switch_to_panther = jungleclear->add_checkbox(".jungleClearSwitchPanther", "Auto Switch To Panther", true);
                jungleclear::auto_switch_to_human = jungleclear->add_checkbox(".jungleClearSwitchHuman", "Auto Switch To Human", true);
            }
            auto misc = main_tab->add_tab(myhero->get_model() + ".misc", "Misc");
            {
                misc->add_separator(".miscSep", "Misc");
                misc::auto_switch_to_panther_on_gapclose = misc->add_checkbox(myhero->get_model() + ".miscAutoPantherGapclose", "Auto Switch To Panther On Gapclose", true);
                auto miscW = misc->add_tab(".miscW", "W Misc");
                {
                    misc::auto_w_on_cc = miscW->add_checkbox(myhero->get_model() + ".miscAutoWCC", "Auto W (human) On CC", true);
                    misc::auto_w_on_gapclose = miscW->add_checkbox(myhero->get_model() + ".miscAutoWGapclose", "Auto W (human) On Gapclose", true);
                }               
                auto miscE = misc->add_tab(".miscE", "E Misc");
                {
                    misc::auto_e_allies = miscE->add_checkbox(myhero->get_model() + ".miscAutoEAllies", "Auto E (human) Allies", true);
                    misc::auto_e_allies_hp_under = miscE->add_slider(myhero->get_model() + ".miscAutoEAlliesSlider", "^~ only when ally HP < X%", 15, 1, 99);
                    misc::auto_e_on_self = miscE->add_checkbox(myhero->get_model() + ".miscAutoESelf", "Auto E (human) Self", true);
                    misc::auto_e_self_hp_under = miscE->add_slider(myhero->get_model() + ".miscAutoESelfSlider", "^~ only when my HP < X%", 25, 1, 99);
                }
            }
            auto flee = main_tab->add_tab(myhero->get_model() + ".flee", "Flee");
            {
                flee->add_separator(".fleeSep", "Flee");
                flee::use_w_panther = flee->add_checkbox(myhero->get_model() + ".fleeUseWPanther", "Use W (auto switch to panther)", true);
            }
            auto draw = main_tab->add_tab(myhero->get_model() + ".draw", "Drawings");
            {
                draw->add_separator(".drawSep", "Drawings");
                float color[] = { 1.0f, 1.0f, 0.0f, 1.0f };
                draw::draw_range_q = draw->add_checkbox(myhero->get_model() + ".drawQ", "Draw Q Range", true);
                draw::draw_range_w = draw->add_checkbox(myhero->get_model() + ".drawW", "Draw W Range", true);
                draw::draw_range_e = draw->add_checkbox(myhero->get_model() + ".drawE", "Draw E Range", true);
                draw::q_color = draw->add_colorpick(myhero->get_model() + ".drawQColor", "Q Color", color);
                draw::w_color = draw->add_colorpick(myhero->get_model() + ".drawWColor", "W Color", color);
                draw::e_color = draw->add_colorpick(myhero->get_model() + ".drawEColor", "E Color", color);
            }
        }
        antigapcloser::add_event_handler(on_gapcloser);
        event_handler<events::on_draw>::add_callback(on_draw);
        event_handler<events::on_after_attack_orbwalker>::add_callback(on_after_attack);
        event_handler<events::on_update>::add_callback(on_update);
        event_handler<events::on_buff_gain>::add_callback(on_buff_gain);
        event_handler<events::on_process_spell_cast>::add_callback(on_process_spell_cast);
    }

    void unload()
    {
        menu->delete_tab(main_tab);

        plugin_sdk->remove_spell(q_human);
        plugin_sdk->remove_spell(w_human);
        plugin_sdk->remove_spell(e_human);       
        plugin_sdk->remove_spell(q_panther);
        plugin_sdk->remove_spell(w_panther);
        plugin_sdk->remove_spell(e_panther);
        plugin_sdk->remove_spell(r);

        if (flash)
            plugin_sdk->remove_spell(flash);
         
        antigapcloser::remove_event_handler(on_gapcloser);
        event_handler<events::on_update>::remove_handler(on_update);
        event_handler<events::on_draw>::remove_handler(on_draw);
        event_handler<events::on_buff_gain>::remove_handler(on_buff_gain);
        event_handler<events::on_after_attack_orbwalker>::remove_handler(on_after_attack);
        event_handler<events::on_process_spell_cast>::remove_handler(on_process_spell_cast);
    }

    void on_update()
    {
        if (myhero->is_dead()) return;

        if (orbwalker->can_move(0.05f))
        {   
            auto_w_on_cc_logic();

            if (orbwalker->harass() && harass::use_q_human->get_bool())
            {
                if (is_human())
                {
                    if (q_human->is_ready() && harass::use_q_human->get_bool())
                    {
                        human_q_logic();
                    }
                }
            }
            if (orbwalker->combo_mode())
            {
                if (is_human())
                {
                    if (q_human->is_ready())
                    {
                        human_q_logic();
                    }
                }
                else
                {
                    if (combo::use_q_human->get_bool() && !combo::use_aa_q_in_combo->get_bool() && q_panther->is_ready())
                    {
                        q_panther->cast();
                    }
                    if (w_panther->is_ready() && combo::use_w_panther->get_bool())
                    {
                        panther_w_logic();
                    }
                    if (e_panther->is_ready() && combo::use_e_panther->get_bool())
                    {
                        panther_e_logic();
                    }
                }
            }
            if (orbwalker->lane_clear_mode())
            {
                auto lane_minions = entitylist->get_enemy_minions();
                auto monsters = entitylist->get_jugnle_mobs_minions();

                lane_minions.erase(std::remove_if(lane_minions.begin(), lane_minions.end(), [](game_object_script x)
                    {
                        return !x->is_valid_target(800);
                    }), lane_minions.end());

                monsters.erase(std::remove_if(monsters.begin(), monsters.end(), [](game_object_script x)
                    {
                        return !x->is_valid_target(800);
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
                    //Lane clear logic
                    if (is_human())
                    {
                        if (laneclear::auto_switch_to_panther->get_bool())
                        {
                            if (laneclear::dont_switch_under_turret->get_bool())
                            {
                                if (!myhero->is_under_enemy_turret())
                                {
                                    if (r->is_ready())
                                    {
                                        r->cast();
                                    }
                                }
                            }
                            else
                            {
                                if (r->is_ready())
                                {
                                    r->cast();
                                }
                            }
                        }
                    }
                    else
                    {
                        if (laneclear::use_w_panther->get_bool())
                        {
                            if (w_panther->is_ready() && lane_minions.front()->get_distance(myhero->get_position()) <= 370 && utilities::count_enemy_minions_in_range(350, myhero->get_position()) >= laneclear::w_panther_min_minions->get_int())
                            {
                                w_panther->cast(lane_minions.front());
                            }
                        }                        
                        if (laneclear::use_e_panther->get_bool())
                        {
                            if (e_panther->is_ready() && utilities::count_enemy_minions_in_range(275, myhero->get_position()) >= laneclear::e_panther_min_minions->get_int() && lane_minions.front()->get_distance(myhero->get_position()) <= 310)
                            {
                                e_panther->cast(lane_minions.front());
                            }
                        }
                    }
                }               
                if (!monsters.empty())
                {
#pragma region JungleClear
                    //Jungle clear logic
                    if (is_human())
                    {
                        if (jungleclear::ignore_on_epic_monsters->get_bool())
                        {
                            if (!monsters.front()->is_epic_monster())
                            {
                                if (q_human->is_ready() && jungleclear::use_q_human->get_bool() && monsters.front()->get_health_percent() > jungleclear::min_monster_hp_to_q->get_int())
                                {
                                    q_human->cast(monsters.back());
                                }
                            }
                            if (monsters.front()->is_epic_monster())
                            {
                                if (q_human->is_ready() && jungleclear::use_q_human->get_bool())
                                {
                                    q_human->cast(monsters.back());
                                }
                            }
                        }
                        else
                        {
                            if (monsters.front()->is_epic_monster() && monsters.front()->get_health_percent() > jungleclear::min_monster_hp_to_q->get_int())
                            {
                                if (q_human->is_ready() && jungleclear::use_q_human->get_bool())
                                {
                                    q_human->cast(monsters.back());
                                }
                            }
                        }
                        if (w_human->is_ready() && jungleclear::use_w_human->get_bool())
                        {
                            w_human->cast(monsters.back());
                        }
                        if (e_human->is_ready() && jungleclear::use_e_human->get_bool())
                        {
                            scheduler->delay_action(0.26f, [] { e_human->cast(myhero); });
                        }
                        if (r->is_ready() && jungleclear::auto_switch_to_panther->get_bool() && !q_human->is_ready() && !e_human->is_ready() && !w_human->is_ready())
                        {
                            scheduler->delay_action(0.26f, [] { r->cast(); });
                        }
                    }
                    else
                    {
                        if (w_panther->is_ready() && jungleclear::use_w_panther->get_bool())
                        {
                            if (myhero->has_buff(buff_hash("NidaleePassiveHunting")))
                            {
                                if (monsters.front()->get_distance(myhero->get_position()) <= 750)
                                {
                                    w_panther->cast(monsters.front());
                                }
                            }
                            else
                            {
                                if (monsters.front()->get_distance(myhero->get_position()) <= 375)
                                {
                                    w_panther->cast(monsters.front());
                                }
                            }
                        }
                        if (e_panther->is_ready() && jungleclear::use_e_panther->get_bool())
                        {
                            if (monsters.front()->get_distance(myhero->get_position()) <= 310)
                            {
                                e_panther->cast(monsters.front()->get_position());
                            }
                        }
                        if (r->is_ready() && jungleclear::auto_switch_to_human->get_bool() && !w_panther->is_ready() && !e_panther->is_ready() && !q_panther->is_ready())
                        {
                            scheduler->delay_action(0.3f, [] { r->cast(); });
                        }
                    }
#pragma endregion


                }
            }
            if (orbwalker->flee_mode())
            {
                if (is_human())
                {
                    if (r->is_ready())
                    {
                        r->cast();
                    }
                }
                else
                {
                    if (w_panther->is_ready() && flee::use_w_panther->get_bool())
                    {
                        auto mouse_position = hud->get_hud_input_logic()->get_game_cursor_position();
                        w_panther->cast(mouse_position);
                    }
                }
            }
            auto_heal_allies();
        }
    }

    void on_process_spell_cast(game_object_script sender, spell_instance_script spell)
    {
        if (sender == myhero)
        {
            if (spell->get_spellslot() == spellslot::q && is_human() && orbwalker->combo_mode())
            {
                if (combo::use_e_human->get_bool())
                {
                    if (e_human->is_ready())
                    {
                        scheduler->delay_action(0.3f, [] { e_human->cast(myhero); });
                    }
                }
            }
        }
    }

    void on_after_attack(game_object_script target)
    {
        if (is_panther())
        {
            if (combo::use_aa_q_in_combo)
            {
                if (q_panther->is_ready() && target != nullptr && target->is_ai_hero())
                {
                    if (((orbwalker->combo_mode() && combo::use_q_panther->get_bool())))
                    {
                        if (q_panther->cast(target))
                        {
                            return;
                        }
                    }
                }
            }
            if (orbwalker->lane_clear_mode())
            {
                if (jungleclear::use_q_panther && q_panther->is_ready() && target != nullptr)
                {
                    if (q_panther->cast(target))
                    {
                        return;
                    }
                }
            }            
            if (orbwalker->lane_clear_mode())
            {
                if (laneclear::use_q_panther && q_panther->is_ready() && target != nullptr)
                {
                    if (q_panther->cast(target))
                    {
                        return;
                    }
                }
            }
        }
    }

    void on_gapcloser(game_object_script sender, antigapcloser::antigapcloser_args* args)
    {
        if (sender != nullptr && sender->is_enemy())
        {
            if (is_human() && w_human->is_ready() && misc::auto_w_on_gapclose->get_bool())
            {
                w_human->cast(myhero->get_position());
            }
            if (is_human() && misc::auto_switch_to_panther_on_gapclose->get_bool() && r->is_ready())
            {
                scheduler->delay_action(0.3f, [] { r->cast(); });
            }
        }
    }

    void on_draw()
    {
        if (myhero->is_dead())
        {
            return;
        }

        if (is_panther()) return;

        if (q_human->is_ready() && draw::draw_range_q->get_bool())
        {
            draw_manager->add_circle(myhero->get_position(), combo::q_human_range->get_int(), draw::q_color->get_color());
        }        
        if (w_human->is_ready() && draw::draw_range_w->get_bool())
        {
            draw_manager->add_circle(myhero->get_position(), 900, draw::w_color->get_color());
        }        
        if (e_human->is_ready() && draw::draw_range_e->get_bool())
        {
            draw_manager->add_circle(myhero->get_position(), 900, draw::e_color->get_color());
        }
    }

    void on_buff_gain(game_object_script sender, buff_instance_script buff)
    {
        if (sender->is_ai_hero())
        {
            //console->print("Name: %s", buff->get_name_cstr());
            //console->print("Hash: %i",buff->get_hash_name());
        }
    }

    void human_q_logic()
    {
        auto target = target_selector->get_target(q_human, damage_type::magical);

        if (target != nullptr && target->is_valid_target(combo::q_human_range->get_int()))
        {
            if (q_human->is_ready())
            {
                q_human->cast(target, hit_chance::high);
            }
        }
    }

    void panther_w_logic()
    {
        auto target = target_selector->get_target(800, damage_type::magical);

        if (target != nullptr)
        {
           if(target->has_buff(-226832744))
           {
               if (target->is_valid_target(730))
               {
                   if (w_panther->is_ready())
                   {
                       if (!target->is_invulnerable() && w_panther->cast(target))
                       {
                           return;
                       }
                   }
               }
           }
           else
           {
               if (target->is_valid_target(375+170))
               {
                   if (w_panther->is_ready())
                   {
                       if (!target->is_invulnerable() && w_panther->cast(target))
                       {
                           return;  
                       }
                   }
               }
           }
        }
    }

    void panther_e_logic()
    {
        auto target = target_selector->get_target(310, damage_type::magical);

        if (target != nullptr)
        {
            if (e_panther->is_ready() && target->is_valid_target(310))
            {
                if (!target->is_invulnerable() && e_panther->cast(target->get_position()))
                {
                    return;
                }
            }
        }
    }

    void auto_w_on_cc_logic()
    {
        if (is_human() && w_human->is_ready() && misc::auto_w_on_cc->get_bool())
        {
            for (auto&& enemy : entitylist->get_enemy_heroes())
            {
                if (enemy != nullptr && myhero->get_distance(enemy->get_position()) <= w_human->range())
                {
                    auto buff = enemy->get_buff_by_type({ buff_type::Stun, buff_type::Snare, buff_type::Knockup, buff_type::Asleep, buff_type::Suppression, buff_type::Taunt });
                    auto zhonya = enemy->get_buff(1036096934);
                    auto ga = enemy->get_buff(-718911512);

                    if (buff != nullptr)
                    {
                        w_human->cast(enemy->get_position());
                    }
                    if (zhonya != nullptr)
                    {
                        w_human->cast(enemy->get_position());
                    }
                    if (ga != nullptr)
                    {
                        w_human->cast(enemy->get_position());
                    }
                }
            }
        }
    }

    void auto_heal_allies()
    {
        if (myhero->is_dead() || myhero->is_recalling()) return;

        if (is_human() && e_human->is_ready() && misc::auto_e_on_self->get_bool())
        {
            if (myhero->get_health_percent() < (float)misc::auto_e_self_hp_under->get_int())
            {
                e_human->cast(myhero);
            }
        }

        if (is_human() && e_human->is_ready() && misc::auto_e_allies->get_bool())
        {
            for (auto&& ally : entitylist->get_ally_heroes())
            {
                if (ally != nullptr && ally->is_valid() && myhero->get_distance(ally) < e_human->range() && ally != myhero)
                {
                    if (ally->get_health_percent() < (float)misc::auto_e_allies_hp_under->get_int())
                    {
                        e_human->cast(ally);
                        break;
                    }
                }
            }
        }
    }

    bool is_panther()
    {
        return !is_human();
    }

    bool is_human()
    {
        return myhero->get_spell(spellslot::q)->get_name() == "JavelinToss";
    }
}