#ifndef PHYSICS_COLLISION_HPP
#define PHYSICS_COLLISION_HPP

#include <btBulletCollisionCommon.h>
#include "debug_draw.hpp"

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

struct convex_sweep_result
{
    btCollisionObject *collision_object;
    btVector3 point;
    btVector3 normal;
    btScalar fraction;
};

// just makes the mask test one-directional, to be consistent with the discrete
// filtering
struct convex_sweep_result_callback : btCollisionWorld::ConvexResultCallback
{
    virtual bool needsCollision(btBroadphaseProxy *proxy0) const
    {
        return (proxy0->m_collisionFilterGroup & m_collisionFilterMask) != 0;
    }
};

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

    void convex_sweep_test(
        btConvexShape *shape,
        const btTransform &from,
        const btTransform &to,
        btCollisionWorld::ConvexResultCallback &callback);

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
