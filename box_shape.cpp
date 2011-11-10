extern "C" {
    #include "lua_utils.h"
    int luaopen_physics_box_shape(lua_State *L);
}

#include "collision.hpp"

static int box_shape__make(lua_State *L)
{
}

int luaopen_physics_box_shape(lua_State *L)
{
    lua_pushcfunction(L, box_shape__make);

    return 1;
}
