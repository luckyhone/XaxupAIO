#include "../plugin_sdk/plugin_sdk.hpp"

PLUGIN_NAME( "Xaxup Name Changer" );
PLUGIN_TYPE(plugin_type::utility);

PLUGIN_API bool on_sdk_load(plugin_sdk_core* plugin_sdk_good)
{
    DECLARE_GLOBALS(plugin_sdk_good);
    return true;
}

PLUGIN_API void on_sdk_unload()
{
    
}