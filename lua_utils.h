#ifndef LUA_UTILS_H
#define LUA_UTILS_H

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

// registers a function with the top stack element as an upvalue
// the function is registered in the current function environment
//
// leaves the upvalue element on the stack
void register_closure(
    lua_State *L,
    const char *name,
    lua_CFunction f);

void get_by_field_path(lua_State *L, int index, const char *path);

void l_tovect(lua_State *L, int index, double *x, double *y, double *z);
void l_pushvect(lua_State *L, double x, double y, double z);
void l_toquaternion(
    lua_State *L, int index, double *w, double *x, double *y, double *z);
void l_pushquaternion( lua_State *L, double w, double x, double y, double z);

#endif
