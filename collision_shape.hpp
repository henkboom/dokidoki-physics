#ifndef PHYSICS_COLLISION_SHAPE_HPP
#define PHYSICS_COLLISION_SHAPE_HPP

#include <btBulletCollisionCommon.h>
extern "C"
{
    #include <lua.h>
}

btCollisionShape* check_collision_shape(lua_State *L, int index);

#endif
