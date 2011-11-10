#include "collision.hpp"

#include "collider.hpp"
#include "collision_shape.hpp"

extern "C"
{
    #include "lua_utils.h"
    int luaopen_physics_collision(lua_State *L);
}

#include <cassert>

bool filter_callback::needBroadphaseCollision(
    btBroadphaseProxy* proxy0,
    btBroadphaseProxy* proxy1) const
{
    return
        (proxy0->m_collisionFilterGroup & proxy1->m_collisionFilterMask) ||
        (proxy1->m_collisionFilterGroup & proxy0->m_collisionFilterMask);
}

//// collision_component Implementation ///////////////////////////////////////

collision_component::collision_component() :
    collision_configuration(),
    collision_dispatcher(&collision_configuration),
    broadphase(),
    filter(),
    debug(),
    world(&collision_dispatcher, &broadphase, &collision_configuration)
{
    world.setDebugDrawer(&debug);
    world.getPairCache()->setOverlapFilterCallback(&filter);
}

void collision_component::add_collision_object(
    btCollisionObject *collision_object,
    collision_controller *controller)
{
    collision_object->setUserPointer(controller);
    world.addCollisionObject(
        collision_object,
        controller->get_group_mask(),
        controller->get_collision_mask());
}

void collision_component::remove_collision_object(
    btCollisionObject *collision_object)
{
    world.removeCollisionObject(collision_object);
}

void collision_component::convex_sweep_test(
    btConvexShape *shape,
    const btTransform &from,
    const btTransform &to,
    btCollisionWorld::ConvexResultCallback &callback)
{
    world.convexSweepTest(shape, from, to, callback, 0.1);
}

void collision_component::draw_debug()
{
    world.debugDrawWorld();
}

void collision_component::update()
{
    int object_count = world.getNumCollisionObjects();
    for(int i = 0; i < object_count; i++)
    {
        btCollisionObject *object = world.getCollisionObjectArray()[i];
        ((collision_controller *)object->getUserPointer())->precollision();
    }

    world.performDiscreteCollisionDetection();

    int manifold_count = collision_dispatcher.getNumManifolds();
    for(int i = 0; i < manifold_count; i++)
    {
        btPersistentManifold* manifold =
            collision_dispatcher.getManifoldByIndexInternal(i);

        int contact_count = manifold->getNumContacts();
        
        if(contact_count > 0)
        {
            btManifoldPoint *deepest_contact = &manifold->getContactPoint(0);
            for(int j = 1; j < contact_count; j++)
            {
                btManifoldPoint *contact = &manifold->getContactPoint(j);
                if(contact->getDistance() < deepest_contact->getDistance())
                    deepest_contact = contact;
            }
            if(deepest_contact->getDistance() < 0)
            {
                btCollisionObject *a_object =
                    ((btCollisionObject *)manifold->getBody0());
                collision_controller *a_controller =
                    (collision_controller*)a_object->getUserPointer();

                btCollisionObject *b_object =
                    (btCollisionObject *)manifold->getBody1();
                collision_controller *b_controller =
                    (collision_controller*)b_object->getUserPointer();

                assert(a_controller && b_controller &&
                    "messed up user pointer on collision body");

                btVector3 a_pos = deepest_contact->getPositionWorldOnA();
                btVector3 b_pos = deepest_contact->getPositionWorldOnB();
                btVector3 b_normal = deepest_contact->m_normalWorldOnB;
                btScalar depth = -deepest_contact->getDistance();

                if(a_controller->get_collision_mask() &
                   b_controller->get_group_mask())
                {
                    a_controller->on_collision(b_controller, a_pos, b_normal, depth);
                }

                if(b_controller->get_collision_mask() &
                   a_controller->get_group_mask())
                {
                    b_controller->on_collision(a_controller, b_pos, -b_normal, depth);
                }
            }
        }
    }
}

//// Lua Component ////////////////////////////////////////////////////////////

collision_component *get_collision_upvalue(lua_State *L)
{
    return *(collision_component **)lua_touserdata(L, lua_upvalueindex(1));
}

int collision_debugdraw(lua_State *L)
{
    get_collision_upvalue(L)->draw_debug();

    return 0;
}

int collision_update(lua_State *L)
{
    get_collision_upvalue(L)->update();

    return 0;
}

struct lua_convex_sweep_result_callback : convex_sweep_result_callback
{
    lua_State *L;
    btScalar addSingleResult(
        btCollisionWorld::LocalConvexResult &convexResult,
        bool normalInWorldSpace);
};

btScalar lua_convex_sweep_result_callback::addSingleResult(
    btCollisionWorld::LocalConvexResult &convexResult,
    bool normalInWorldSpace)
{
    lua_pushvalue(L, -1); // dup callback

    lua_newtable(L); // collision record

    btCollisionObject *collision_object = convexResult.m_hitCollisionObject;
    collider *other_collider = dynamic_cast<collider*>(
        (collision_controller*)collision_object->getUserPointer());
    if(other_collider)
        other_collider->push_component();
    else
        lua_pushboolean(L, 0);
    lua_setfield(L, -2, "collider");

    const btVector3 &point = convexResult.m_hitPointLocal;
    l_pushvect(L, point.x(), point.y(), point.z());
    lua_setfield(L, -2, "point");

    lua_pushnumber(L, convexResult.m_hitFraction);
    lua_setfield(L, -2, "fraction");

    btVector3 normal;
    if(normalInWorldSpace)
        normal = convexResult.m_hitNormalLocal;
    else
    {
        btTransform *t = &collision_object->getWorldTransform();
        normal = t->getBasis() * convexResult.m_hitNormalLocal;
    }
    l_pushvect(L, normal.x(), normal.y(), normal.z());
    lua_setfield(L, -2, "normal");

    // TODO we don't catch errors
    lua_call(L, 1, 0);

    return 0; // return value not actually used
}

void totransform(lua_State *L, int index, btTransform *transform)
{
    luaL_checktype(L, index, LUA_TTABLE);

    lua_getfield(L, index, "pos");
    double x, y, z;
    l_tovect(L, -1, &x, &y, &z);
    lua_pop(L, 1); // pop position
    transform->setOrigin(btVector3(x, y, z));

    lua_getfield(L, index, "orientation");
    double w;
    l_toquaternion(L, -1, &w, &x, &y, &z);
    lua_pop(L, 1); // pop orientation
    transform->setRotation(btQuaternion(x, y, z, w));
}

int collision_convex_sweep_test(lua_State *L)
{
    collision_component *world = get_collision_upvalue(L);

    // args are (collision_shape, from_transform, to_transform, mask, callback)

    btConvexShape *convex_shape =
        dynamic_cast<btConvexShape*>(check_collision_shape(L, 1));
    if(convex_shape == NULL)
        luaL_argerror(L, 1, "the collision shape must be convex");

    btTransform from_transform;
    totransform(L, 2, &from_transform);

    btTransform to_transform;
    totransform(L, 3, &to_transform);

    unsigned short mask;
    mask = lua_tointeger(L, 4);

    if(lua_gettop(L) < 5)
        luaL_argerror(L, 5, "expected callback");
    if(lua_gettop(L) > 5)
        luaL_error(L, "too many arguments");

    lua_convex_sweep_result_callback cb;
    cb.L = L;
    cb.m_collisionFilterMask = mask;

    world->convex_sweep_test(convex_shape, from_transform, to_transform, cb);

    return 0;
}

int collision_get_udata(lua_State *L)
{
    lua_pushvalue(L, lua_upvalueindex(1));
    return 1;
}

int collision_on_removal(lua_State *L)
{
    collision_component **udata =
        (collision_component **)lua_touserdata(L, lua_upvalueindex(1));
    delete *udata;
    *udata = NULL;
    return 0;
}

int luaopen_physics_collision(lua_State *L)
{
    collision_component *component = new collision_component();
    *(collision_component **)lua_newuserdata(L, sizeof(collision_component *)) =
        component;

    register_closure(L, "debugdraw", collision_debugdraw);
    register_closure(L, "update", collision_update);
    register_closure(L, "convex_sweep_test", collision_convex_sweep_test);
    register_closure(L, "get_udata", collision_get_udata);
    register_closure(L, "on_removal", collision_on_removal);

    return 0;
}
