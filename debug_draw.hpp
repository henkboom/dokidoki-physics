#ifndef DEBUG_DRAW_HPP
#define DEBUG_DRAW_HPP

#include <LinearMath/btIDebugDraw.h>

class debug_draw : public btIDebugDraw
{
    void drawLine(
        const btVector3& from,
        const btVector3& to,
        const btVector3& color);

    void drawLine(
        const btVector3& from,
        const btVector3& to,
        const btVector3& fromColor,
        const btVector3& toColor);

    void drawContactPoint(
        const btVector3& PointOnB,
        const btVector3& normalOnB,
        btScalar distance,
        int lifeTime,
        const btVector3& color);

    void reportErrorWarning(const char* warningString);

    void draw3dText(const btVector3& location,const char* textString);

    void setDebugMode(int debugMode);
    int getDebugMode() const;
};

#endif
