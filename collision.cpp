#include "collision.hpp"

extern "C" {
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

void collision_component::draw_debug()
{
    world.debugDrawWorld();
}

#include <iostream>
using namespace std;

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
    register_closure(L, "get_udata", collision_get_udata);
    register_closure(L, "on_removal", collision_on_removal);

    return 0;
}
