#include "physics.h"

#include "geometry.h"
#include <vector>
#include <list>

#define DEBUG_ENERGY 0

static std::vector<Physics*> s_physicsList(0);

Physics::Physics(Geometry* Geo, const StaticPhysicsData& physicsData)
    : m_geometry(Geo)
    , m_static(physicsData)
{
    m_position = physicsData.m_initialPosition;
    m_rotation = physicsData.m_initialRotation;
    s_physicsList.push_back(this);
}

Physics::~Physics()
{
    // TODO: remove them from the list
}


//-------------------------------------------------------------------------------------------------
void Physics::ApplyImpulseResponse(CollisionData& data, bool pushOut)
{
    vector3 n = vector3(0.0f, 0.0f, 1.0f); // TODO commenting out quickly, probably does need to be the normal of the intersection //data.planeNormal;
    vector3 r = data.penetrationDirection;

    // Attempt to move the bodies out of the collision
    vector3 v = m_linearMomentum * (1.0f / m_static.m_mass);
    vector3 w = m_static.m_inverseInertiaTensor * m_angularMomentum;

    // Apply the forces to the bodies
    // rigid body impulse response
    vector3 velocityAtPoint = v + w.cross(r);
    float i = -(1.0f + m_static.m_elasticity) * velocityAtPoint.dot(n);
    float k = (1.0f / m_static.m_mass) + ((m_static.m_inverseInertiaTensor * r.cross(n)).cross(r)).dot(n);
    float j = i / k;
    m_linearMomentum = m_linearMomentum + n * j;
    m_angularMomentum = m_angularMomentum + r.cross(n) * j;

    if (pushOut)
    {
        const float adjust = data.depth * 1.01f;
        m_position = m_position + n * adjust;
    }

    // apply friction
    //if (m_static.m_staticFrictionCoeff)
    //{
    //    v = m_linearMomentum * (1.0f / m_static.m_mass);
    //    velocityAtPoint = v + w.cross(r);
    //    float relVelNormal = velocityAtPoint.dot(n);
    //    if (!FloatEquals(relVelNormal, 0.0f))
    //    {
    //        vector3 vt = velocityAtPoint - n * relVelNormal; // velocity in directions other than contact normal
    //        vt.normalize();
    //        float it = velocityAtPoint.dot(vt);
    //        float kt = (1.0f / m_static.m_mass) + ((m_static.m_inverseInertiaTensor * r.cross(vt)).cross(r)).dot(vt);
    //        float frictionCoeff = m_static.m_staticFrictionCoeff * j;
    //        float jt = clamp(-it / kt, -frictionCoeff, frictionCoeff);
    //        m_linearMomentum = m_linearMomentum + vt * jt;
    //        m_angularMomentum = m_angularMomentum + r.cross(vt) * jt;
    //    }
    //    else
    //    {
    //        // if the sum of all external forces on the body are non-zero, we can use that?  
    //        // see https://en.wikipedia.org/wiki/Collision_response
    //    }
    //
    //
    //}


#if DEBUG_ENERGY
    if (!FloatEquals(m_static.m_mass, 0.0f))
    {
        vector3 vel = m_linearMomentum * (1.0f / m_static.m_mass);
        vector3 rot = m_static.m_inverseInertiaTensor * m_angularMomentum;
        float Ke_translate = 0.5f * m_static.m_mass * vel.magnitude() * vel.magnitude();
        float Pe_gravity = m_static.m_mass * m_static.m_gravity.magnitude() * m_position.y;
        float Ke_rotate = 0.5f * m_static.m_momentOfInertia * rot.magnitude() * rot.magnitude();
        printf("Kinetic Energy After: %f (tran=%f (P=%f,K=%f) | rot=%f)\n", Ke_translate + Ke_rotate + Pe_gravity, Ke_translate + Pe_gravity, Pe_gravity, Ke_translate, Ke_rotate);
    }
#endif
}
//-------------------------------------------------------------------------------------------------
// pushOut is a temporary solution to resolve collisions where we adjust the position so its no longer
// pertruding... ideally we could make it so we can re-run the sim until there are no collisions
//
// $TODO continuous-advancement
//
void OnCollision(Collision& data, bool pushOut)
{
    switch (data.a->GetCollisionResponse())
    {
        case COLLISION_RESPONSE_IMPULSE:
        {
            data.a->ApplyImpulseResponse(data.data, pushOut);
        }
        default:
            break;
    }

    switch (data.b->GetCollisionResponse())
    {
        case COLLISION_RESPONSE_IMPULSE:
        {
            data.b->ApplyImpulseResponse(data.data, pushOut);
        }
        default:
            break;
    }
}
//-------------------------------------------------------------------------------------------------
std::vector<Collision> GetCollisions(std::vector<Physics*> updateList)
{
    std::vector<Collision> collisions;
    for (auto it = updateList.begin(); it != updateList.end(); ++it)
    {
        for (auto innerIt = it; innerIt != updateList.end(); ++innerIt)
        {
            if (it == innerIt)
                continue; // can't collide with yourself
            Physics* a = *it;
            Physics* b = *innerIt;
            CollisionData collisionData;
            CollisionParams p;
            p.a = a->GetGeometry();
            #ifdef TEST_PROGRAM
            vector3 r = a->GetRotation();
            vector3 d = a->GetPosition();
            #endif
            p.aTransform = a->GetTransform();
            p.b = b->GetGeometry();
            p.bTransform = b->GetTransform();
            if (DetectCollision3D(p, &collisionData))
            {
                collisions.push_back({ *it, *innerIt, collisionData });
                break;
            }
        }
    }
    return collisions;
}

//-------------------------------------------------------------------------------------------------
void Physics_Update(float dt)
{
    // update the objects dynamic physics state for the time passed
    for (auto it = s_physicsList.begin(); it != s_physicsList.end(); ++it)
    {
        Physics* phys = *it;
        phys->Update(dt);
    }

    // Check for collisions and apply responses to collisions
    std::vector<Collision> collisions = GetCollisions(s_physicsList);
    for (int i = 0; i < collisions.size(); i++)
    {
        OnCollision(collisions[i], true);
    }
}

//-------------------------------------------------------------------------------------------------
void Physics::Reset()
{
    m_position = m_static.m_initialPosition;
    m_rotation = m_static.m_initialRotation;
    m_angularMomentum = { 0.0f };
    m_linearMomentum = { 0.0f };
}

//-------------------------------------------------------------------------------------------------
void Physics::Update(float deltaTime)
{
    // update for gravity
    if (!m_static.m_gravity.IsNone())
    {
        m_linearMomentum = m_linearMomentum + (m_static.m_gravity * m_static.m_mass) * deltaTime;
    }

    // should probably be more explicit... but assume massless objects don't move
    if (!FloatEquals(m_static.m_mass, 0.0f))
    {
        // compute linear and angular velocity
        vector3 v = m_linearMomentum * (1.0f / m_static.m_mass);
        vector3 w = m_static.m_inverseInertiaTensor * m_angularMomentum;

        // update position based on velocities
        m_position = m_position + v * deltaTime;
        m_rotation = m_rotation + w * deltaTime;
    }
}