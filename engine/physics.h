#pragma once


#include "../engine/vector.h"
#include "../engine/matrix.h"

class Geometry;


//
// Collision information
//
enum COLLISION_RESPONSE
{
    COLLISION_RESPONSE_NONE,
    COLLISION_RESPONSE_IMPULSE,
};
struct CollisionData
{
    vector3 point;
    vector3 planeNormal;
    float   depth;
};
struct Collision
{
    class Physics* a;
    class Physics* b;
    CollisionData data;
};

//
// Static physics data for an object (set at construction, never changes)
//
struct StaticPhysicsData
{
    vector3 m_gravity;
    vector3 m_initialPosition;
    vector3 m_initialRotation;

    float   m_mass = 0.0f;
    float   m_elasticity = 0.0f;
    float   m_staticFrictionCoeff = 0.0f; // for colliding objects
    float   m_dynamicFrictionCoeff = 0.0f; // for sliding objects
    COLLISION_RESPONSE m_collisionResponseType = COLLISION_RESPONSE_NONE;

    float   m_momentOfInertia = 0.0f;
    matrix3 m_inertiaTensor;
    matrix3 m_inverseInertiaTensor;
};

//
// Physics
//   For objects that exist in the physics sim, this describes the necessary parameters for them to
//   interact and be interacted with other objects
//
class Physics
{
public:
    Physics(Geometry* Geo, const StaticPhysicsData& physicsData);
    ~Physics();

public:
    void      Reset();
    void      Update(float dt);

    vector3 GetPosition() const { return m_position; }
    vector3 GetRotation() const { return m_rotation; }
    matrix4 GetTransform() const  { 
        // todo: should already have this...
        matrix4 transform;
        transform.translate(m_position);
        transform.rotate(m_rotation);
        return transform;
    }
    COLLISION_RESPONSE GetCollisionResponse() const { return m_static.m_collisionResponseType; }
    const Geometry* GetGeometry() const { return m_geometry; }
    
    void ApplyImpulseResponse(CollisionData& data, bool pushOut);

#ifdef TEST_PROGRAM
    void SetPosition(const vector3& v) { m_position = v; }
    void SetRotation(const vector3& v) { m_rotation = v; }
#endif
private:
    // constant, for reference
    const Geometry* m_geometry; // todo: this should be a 'physics geometry' - a limited subset of vertices
    const StaticPhysicsData  m_static;

    vector3   m_position;
    vector3   m_rotation;
    vector3   m_linearMomentum;
    vector3   m_angularMomentum;
};



// C-Style interface
void Physics_Update(float dt);


struct CollisionParams
{
    const Geometry* a;
    const Geometry* b;
    vector3  aPos;
    vector3  bPos;
};
bool DetectCollision(const CollisionParams& params, CollisionData* outCollision);
// separated out for testing purposes
#ifdef TEST_PROGRAM
enum COLLISION_STEP
{
    COLLISION_STEP_SUCCESS,
    COLLISION_STEP_FAILURE,
    COLLISION_STEP_CONTINUE,
};
COLLISION_STEP DetectCollisionStep(const CollisionParams& params, struct Simplex& simplex, const vector3& destination);
void GetWitnessPoints(const struct Simplex& s, vector3& outA, vector3& outB);
#endif