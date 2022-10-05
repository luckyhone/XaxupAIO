#include "../plugin_sdk/plugin_sdk.hpp"

PLUGIN_NAME( "XaxupAIO" );
SUPPORTED_CHAMPIONS(champion_id::Teemo, champion_id::Amumu, champion_id::Nidalee);

#include "Teemo.h"
#include "Amumu.h"
#include "Nidalee.h"

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
    default:
        break;
    }
}