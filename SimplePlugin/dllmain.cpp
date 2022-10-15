#include "../plugin_sdk/plugin_sdk.hpp"

PLUGIN_NAME( "XaxupAIO" );
SUPPORTED_CHAMPIONS(champion_id::Teemo, champion_id::Amumu, champion_id::Nidalee, champion_id::Ziggs, champion_id::Taliyah, champion_id::Riven);

#include "Teemo.h"
#include "Amumu.h"
#include "Nidalee.h"
#include "Ziggs.h"
#include "Taliyah.h"
#include "Riven.h"

PLUGIN_API bool on_sdk_load(plugin_sdk_core* plugin_sdk_good)
{
    DECLARE_GLOBALS(plugin_sdk_good);

    switch (myhero->get_champion())
    {
    case champion_id::Teemo:
        teemo::load();
        break;
    case champion_id::Amumu:
        amumu::load();
        break;    
    case champion_id::Nidalee:
        nidalee::load();
        break;    
    case champion_id::Ziggs:
        ziggs::load();
        break;   
    case champion_id::Taliyah:
        taliyah::load();
        break;
    case champion_id::Riven:
        riven::load();
        break;
    default:
        console->print("Champion %s is not supported!", myhero->get_model_cstr());
        return false;
    }
    return true;
}

PLUGIN_API void on_sdk_unload()
{
    switch (myhero->get_champion())
    {
    case champion_id::Teemo:
        teemo::unload();
        break;
    case champion_id::Amumu:
        amumu::unload();
        break;
    case champion_id::Nidalee:
        nidalee::unload();
        break;    
    case champion_id::Ziggs:
        ziggs::unload();
        break;    
    case champion_id::Taliyah:
        taliyah::unload();
        break;
    case champion_id::Riven:
        riven::unload();
        break;
    default:
        break;
    }
}
