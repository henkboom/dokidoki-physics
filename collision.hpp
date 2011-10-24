#ifndef COLLISION_HPP
#define COLLISION_HPP

#include <btBulletCollisionCommon.h>
#include "debug_draw.hpp"

extern "C" {
    #include <lua.h>
    #include <lualib.h>
    #include <lauxlib.h>
}

class collision_controller
{
    public:
    virtual ~collision_controller() {
        // do nothing
    };
    virtual void precollision() = 0;
    virtual void on_collision(
        collision_controller *other,
        btVector3 point,
        btVector3 normal,
        btScalar depth) = 0;
    // these must be constant, the collision detector won't follow changes in
    // these values
    virtual unsigned short get_group_mask() const = 0;
    virtual unsigned short get_collision_mask() const = 0;
};

class filter_callback : public btOverlapFilterCallback
{
    bool needBroadphaseCollision(
        btBroadphaseProxy* proxy0,
        btBroadphaseProxy* proxy1) const;
};

// separate implementation into cpp file, with just a public interface class
// here?
class collision_component
{
    public:
    collision_component();

    // overwrites the user pointer on collision_object
    void add_collision_object(
        btCollisionObject *collision_object,
        collision_controller *controller);
    void remove_collision_object(
        btCollisionObject *collision_object);

    //TODO: figure out naming
    void draw_debug();
    void update();

    private:
    btDefaultCollisionConfiguration collision_configuration;
    btCollisionDispatcher collision_dispatcher;
    btDbvtBroadphase broadphase;
    filter_callback filter;
    debug_draw debug;
    btCollisionWorld world;
};

#endif
