#include "lua_utils.h"

#include <lualib.h>
#include <lauxlib.h>

void register_closure(
    lua_State *L,
    const char *name,
    lua_CFunction f)
{
    lua_pushvalue(L, -1);
    lua_pushcclosure(L, f, 1);
    lua_setfield(L, LUA_ENVIRONINDEX, name);
}

void get_by_field_path(lua_State *L, int index, const char *path)
{
    lua_pushvalue(L, index);
    while(*path != 0)
    {
        const char *next = path;
        while(*next != '.' && *next != 0)
            next++;

        lua_pushlstring(L, path, next - path);
        lua_gettable(L, -2);
        lua_remove(L, -2);

        path = next;
        if(*path == '.')
            path++;
    }
}

void l_tovect(lua_State *L, int index, double *x, double *y, double *z)
{
    lua_rawgeti(L, index, 1);
    *x = lua_tonumber(L, -1);
    lua_pop(L, 1);

    lua_rawgeti(L, index, 2);
    *y = lua_tonumber(L, -1);
    lua_pop(L, 1);

    lua_rawgeti(L, index, 3);
    *z = lua_tonumber(L, -1);
    lua_pop(L, 1);
}

void l_pushvect(lua_State *L, double x, double y, double z)
{
    lua_getglobal(L, "require");
    lua_pushstring(L, "dokidoki.vect");
    lua_call(L, 1, 1);
    // just the 'vect' module on the stack

    lua_pushnumber(L, x);
    lua_pushnumber(L, y);
    lua_pushnumber(L, z);
    lua_call(L, 3, 1);

    // leave the resulting vector on the stack
}

void l_toquaternion(
    lua_State *L, int index, double *w, double *x, double *y, double *z)
{
    lua_rawgeti(L, index, 1);
    *w = lua_tonumber(L, -1);
    lua_pop(L, 1);

    lua_rawgeti(L, index, 2);
    *x = lua_tonumber(L, -1);
    lua_pop(L, 1);

    lua_rawgeti(L, index, 3);
    *y = lua_tonumber(L, -1);
    lua_pop(L, 1);

    lua_rawgeti(L, index, 4);
    *z = lua_tonumber(L, -1);
    lua_pop(L, 1);
}

void l_pushquaternion(
    lua_State *L, double w, double x, double y, double z)
{
    lua_getglobal(L, "require");
    lua_pushstring(L, "dokidoki.quaternion");
    lua_call(L, 1, 1);
    // just the 'quaternion' module on the stack

    lua_pushnumber(L, w);
    lua_pushnumber(L, x);
    lua_pushnumber(L, y);
    lua_pushnumber(L, z);
    lua_call(L, 4, 1);

    // leave the resulting quaternion on the stack
}

