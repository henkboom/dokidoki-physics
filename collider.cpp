extern "C" {
    #include <lua.h>
    #include <lualib.h>
    #include <lauxlib.h>
    #include "lua_utils.h"
    int luaopen_physics_collider(lua_State *L);
}

#include "collision.hpp"

class collider : public collision_controller
{
    public:
    collider(lua_State *L, collision_component *world, int component_ref,
             const btVector3 &size, unsigned short group_mask,
             unsigned short collision_mask);
    ~collider();

    void precollision();
    void on_collision(
        collision_controller *other,
        btVector3 point,
        btVector3 normal,
        btScalar depth);
    void push_component();

    unsigned short get_group_mask() const       { return group_mask; }
    unsigned short get_collision_mask() const   { return collision_mask; }

    private:
    lua_State *L;
    int component_ref;
    unsigned short group_mask;
    unsigned short collision_mask;

    btBoxShape shape;
    btCollisionObject collision_object;

    collision_component *world;
};

collider::collider(lua_State *L, collision_component *world, int component_ref,
                   const btVector3 &size, unsigned short group_mask,
                   unsigned short collision_mask)
:   L(L),
    component_ref(component_ref),
    group_mask(group_mask),
    collision_mask(collision_mask),
    shape(btVector3(size.x(), size.y(), size.z())),
    collision_object(),
    world(world)
{
    collision_object.setCollisionShape(&shape);
    world->add_collision_object(&collision_object, this);
}

collider::~collider()
{
    luaL_unref(L, LUA_REGISTRYINDEX, component_ref);
    world->remove_collision_object(&collision_object);
}

void collider::precollision()
{
    lua_rawgeti(L, LUA_REGISTRYINDEX, component_ref);
    get_by_field_path(L, -1, "parent.transform");

    btTransform *transform = &collision_object.getWorldTransform();

    {
        lua_getfield(L, -1, "pos");
        double x, y, z;
        l_tovect(L, -1, &x, &y, &z);
        lua_pop(L, 1);

        transform->setOrigin(btVector3(x, y, z));
    }

    {
        lua_getfield(L, -1, "orientation");
        double w, x, y, z;
        l_toquaternion(L, -1, &w, &x, &y, &z);
        lua_pop(L, 1);

        transform->setRotation(btQuaternion(x, y, z, w));
    }

    lua_pop(L, 2);
}

void collider::on_collision(
    collision_controller *other,
    btVector3 point,
    btVector3 normal,
    btScalar depth)
{
    push_component();
    lua_getfield(L, -1, "on_collision");
    if(lua_isnil(L, -1))
        lua_pop(L, 1);
    else
    {
        collider *other_collider = dynamic_cast<collider*>(other);
        if(other_collider)
            other_collider->push_component();
        else
            lua_pushboolean(L, 0);
        l_pushvect(L, point.x(), point.y(), point.z());
        l_pushvect(L, normal.x(), normal.y(), normal.z());
        lua_pushnumber(L, depth);
        lua_call(L, 4, 0);
    }
    lua_pop(L, 1);
}

void collider::push_component()
{
    lua_rawgeti(L, LUA_REGISTRYINDEX, component_ref);
}

int collider_get_udata(lua_State *L)
{
    lua_pushvalue(L, lua_upvalueindex(1));
    return 1;
}

int collider_on_removal(lua_State *L)
{
    collider **udata = (collider**)lua_touserdata(L, lua_upvalueindex(1));
    delete *udata;
    *udata = NULL;
    return 0;
}

int luaopen_physics_collider(lua_State *L)
{
    btVector3 size(1, 1, 1);
    unsigned short group_mask = 1;
    unsigned short collision_mask = 1;
    if(!lua_isnil(L, 1)) // get named arguments
    {
        lua_getfield(L, 1, "size");
        if(!lua_isnil(L, -1))
        {
            double x, y, z;
            l_tovect(L, -1, &x, &y, &z);
            lua_pop(L, 1);
            size = btVector3(x, y, z);
        }

        lua_getfield(L, 1, "group_mask");
        if(!lua_isnil(L, -1))
        {
            group_mask = lua_tonumber(L, -1);
            lua_pop(L, 1);
        }

        lua_getfield(L, 1, "collision_mask");
        if(!lua_isnil(L, -1))
        {
            collision_mask = lua_tonumber(L, -1);
            lua_pop(L, 1);
        }
    }

    get_by_field_path(L, LUA_ENVIRONINDEX, "game.collision.get_udata");
    lua_call(L, 0, 1);
    collision_component *world = *(collision_component **) lua_touserdata(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, LUA_ENVIRONINDEX, "self");
    int self_ref = luaL_ref(L, LUA_REGISTRYINDEX);
    
    collider *c = new collider(L, world, self_ref, size, group_mask, collision_mask);

    *(collider **)lua_newuserdata(L, sizeof(collision_component *)) = c;

    register_closure(L, "get_udata", collider_get_udata);
    register_closure(L, "on_removal", collider_on_removal);

    return 0;
}

