#include "collider.hpp"

extern "C" {
    #include "lua_utils.h"
    int luaopen_physics_collider(lua_State *L);
}

#include "collision_shape.hpp"

collider::collider(
        lua_State *L,
        collision_component *world,
        int component_ref,
        int shape_ref,
        unsigned short group_mask,
        unsigned short collision_mask)
:   L(L),
    component_ref(component_ref),
    shape_ref(LUA_NOREF),
    group_mask(group_mask),
    collision_mask(collision_mask),
    collision_object(),
    world(world)
{
    set_collision_shape(shape_ref);
    world->add_collision_object(&collision_object, this);
}

collider::~collider()
{
    world->remove_collision_object(&collision_object);
    luaL_unref(L, LUA_REGISTRYINDEX, component_ref);
    luaL_unref(L, LUA_REGISTRYINDEX, shape_ref);
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

void collider::set_group_mask(unsigned short group_mask)
{
    world->remove_collision_object(&collision_object);
    this->group_mask = group_mask;
    world->add_collision_object(&collision_object, this);
}

void collider::set_collision_mask(unsigned short collision_mask)
{
    world->remove_collision_object(&collision_object);
    this->collision_mask = collision_mask;
    world->add_collision_object(&collision_object, this);
}

void collider::set_collision_shape(int shape_ref)
{
    assert(shape_ref != LUA_NOREF);

    if(this->shape_ref != LUA_NOREF)
        world->remove_collision_object(&collision_object);

    lua_rawgeti(L, LUA_REGISTRYINDEX, shape_ref);
    collision_object.setCollisionShape(check_collision_shape(L, -1));
    lua_pop(L, 1);

    if(this->shape_ref != LUA_NOREF)
        world->add_collision_object(&collision_object, this);

    this->shape_ref = shape_ref;
}

//// lua stuff ////////////////////////////////////////////////////////////////

int collider_set_collision_shape(lua_State *L)
{
    check_collision_shape(L, 1);
    collider **udata = (collider**)lua_touserdata(L, lua_upvalueindex(1));
    (*udata)->set_collision_shape(luaL_ref(L, LUA_REGISTRYINDEX));
    return 0;
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
    unsigned short group_mask = 1;
    unsigned short collision_mask = 1;

    // get named arguments
    luaL_checktype(L, 1, LUA_TTABLE);

    lua_getfield(L, 1, "collision_shape");
    check_collision_shape(L, -1);
    int collision_shape_ref = luaL_ref(L, LUA_REGISTRYINDEX);

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

    // get collision world
    get_by_field_path(L, LUA_ENVIRONINDEX, "game.collision.get_udata");
    lua_call(L, 0, 1);
    collision_component *world = *(collision_component **) lua_touserdata(L, -1);
    lua_pop(L, 1);

    // get self
    lua_getfield(L, LUA_ENVIRONINDEX, "self");
    int self_ref = luaL_ref(L, LUA_REGISTRYINDEX);
    
    // make collision obect and leave it on top of the stack
    collider *c = new collider(
        L, world, self_ref, collision_shape_ref, group_mask, collision_mask);
    *(collider **)lua_newuserdata(L, sizeof(collision_component *)) = c;

    // methods
    register_closure(L, "set_collision_shape", collider_set_collision_shape);
    register_closure(L, "get_udata", collider_get_udata);
    register_closure(L, "on_removal", collider_on_removal);

    return 0;
}

