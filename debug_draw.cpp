#include "debug_draw.hpp"

#include <GL/glfw.h>

void debug_draw::drawLine(
    const btVector3& from,
    const btVector3& to,
    const btVector3& color)
{
    drawLine(from, to, color, color);
}

void debug_draw::drawLine(
    const btVector3& from,
    const btVector3& to,
    const btVector3& fromColor,
    const btVector3& toColor)
{
    glBegin(GL_LINES);
    glColor3d(fromColor.x(), fromColor.y(), fromColor.z());
    glVertex3d(from.x(), from.y(), from.z());
    glColor3d(toColor.x(), toColor.y(), toColor.z());
    glVertex3d(to.x(), to.y(), to.z());
    glEnd();
    glColor3d(1, 1, 1);
}

void debug_draw::drawContactPoint(
    const btVector3& PointOnB,
    const btVector3& normalOnB,
    btScalar distance,
    int lifeTime,
    const btVector3& color)
{
    // do nothing
}

void debug_draw::reportErrorWarning(const char* warningString)
{
    // do nothing
}

void debug_draw::draw3dText(const btVector3& location,const char* textString)
{
    // do nothing
}

void debug_draw::setDebugMode(int debugMode)
{
    // do nothing
}

int debug_draw::getDebugMode() const
{
    return DBG_DrawWireframe;
}
