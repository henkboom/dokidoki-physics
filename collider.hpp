#ifndef PHYSICS_COLLIDER_HPP
#define PHYSICS_COLLIDER_HPP

#include <btBulletCollisionCommon.h>

extern "C"
{
    #include <lua.h>
}

#include "collision.hpp"

class collider : public collision_controller
{
    public:
    collider(lua_State *L, collision_component *world, int component_ref,
             int shape_ref, unsigned short group_mask,
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

    void set_group_mask(unsigned short group_mask);
    void set_collision_mask(unsigned short collision_mask);
    // collider takes ownership of this reference
    void set_collision_shape(int shape_ref);

    private:
    lua_State *L;
    int component_ref;
    int shape_ref;
    unsigned short group_mask;
    unsigned short collision_mask;

    btCollisionObject collision_object;

    collision_component *world;
};

#endif
