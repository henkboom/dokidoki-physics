#include "collision_shape.hpp"

#include "collision.hpp"

#include <BulletCollision/CollisionShapes/btBox2dShape.h>

extern "C"
{
    #include "lua_utils.h"
    int luaopen_physics_box_shape(lua_State *L);
    int luaopen_physics_box_2d_shape(lua_State *L);
    int luaopen_physics_triangle_mesh_shape(lua_State *L);
}

//// Collision Shape //////////////////////////////////////////////////////////

#define COLLISION_SHAPE_UDATA "physics.collision_shape"

btCollisionShape* check_collision_shape(lua_State *L, int index)
{
    return *(btCollisionShape**)luaL_checkudata(L, index, COLLISION_SHAPE_UDATA);
}

static int collision_shape_gc(lua_State *L)
{
    delete check_collision_shape(L, 1);
    return 0;
}

static void push_collision_shape(lua_State *L, btCollisionShape *shape)
{
    *(btCollisionShape**)lua_newuserdata(L, sizeof(shape)) = shape;

    if(luaL_newmetatable(L, COLLISION_SHAPE_UDATA))
    {
        lua_pushcfunction(L, collision_shape_gc);
        lua_setfield(L, -2, "__gc");
    }
    lua_setmetatable(L, -2);
}

//// Box Shape ////////////////////////////////////////////////////////////////

static int box_shape_make(lua_State *L)
{
    double x, y, z;
    l_tovect(L, -1, &x, &y, &z);
    lua_pop(L, 1);
    
    push_collision_shape(L, new btBoxShape(btVector3(x, y, z)));

    return 1;
}

int luaopen_physics_box_shape(lua_State *L)
{
    lua_pushcfunction(L, box_shape_make);
    return 1;
}

//// Box2D Shape ////////////////////////////////////////////////////////////////

static int box_2d_shape_make(lua_State *L)
{
    double x, y, z;
    l_tovect(L, -1, &x, &y, &z);
    lua_pop(L, 1);

    push_collision_shape(L, new btBox2dShape(btVector3(x, y, z)));

    return 1;
}

int luaopen_physics_box_2d_shape(lua_State *L)
{
    lua_pushcfunction(L, box_2d_shape_make);
    return 1;
}

//// Triangle Mesh Shape //////////////////////////////////////////////////////

class triangle_mesh_shape : public btBvhTriangleMeshShape
{
    public:
    triangle_mesh_shape(btStridingMeshInterface *mesh);
    virtual ~triangle_mesh_shape();

    private:
    btStridingMeshInterface *mesh;
};

triangle_mesh_shape::triangle_mesh_shape(btStridingMeshInterface *mesh) :
    btBvhTriangleMeshShape(mesh, true)
{
    this->mesh = mesh;
}

triangle_mesh_shape::~triangle_mesh_shape()
{
    delete this->mesh;
}

static int triangle_mesh_shape_make(lua_State *L)
{
    luaL_checktype(L, 1, LUA_TTABLE);
    int length = lua_objlen(L, 1);

    btTriangleMesh *mesh = new btTriangleMesh(length > 1<<16, false);

    int face_index = 1;
    lua_pushinteger(L, face_index);
    lua_gettable(L, face_index);
    while(!lua_isnil(L, -1))
    {
        btVector3 vertices[4];

        int vertex_index = 1;
        lua_pushinteger(L, vertex_index);
        lua_gettable(L, -2);
        while(!lua_isnil(L, -1))
        {
            assert(vertex_index <= 4 && "max 4 vertices in a face");

            lua_getfield(L, -1, "position");
            double x, y, z;
            l_tovect(L, -1, &x, &y, &z);
            lua_pop(L, 2); // pop position, vertex

            vertices[vertex_index-1] = btVector3(x, y, z);

            vertex_index++;
            lua_pushinteger(L, vertex_index);
            lua_gettable(L, -2);
        }
        lua_pop(L, 2); // pop nil, face

        mesh->addTriangle(vertices[0], vertices[1], vertices[2]);
        if(vertex_index > 4)
            mesh->addTriangle(vertices[2], vertices[3], vertices[0]);

        face_index++;
        lua_pushinteger(L, face_index);
        lua_gettable(L, -2);
    }
    lua_pop(L, 1); // pop face

    push_collision_shape(L, new triangle_mesh_shape(mesh));
    return 1;
}

int luaopen_physics_triangle_mesh_shape(lua_State *L)
{
    lua_pushcfunction(L, triangle_mesh_shape_make);
    return 1;
}
