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
    vector3 pointA,pointB;
    vector3 planeNormal;
    float   depth;
    bool success = false;
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
    matrix4  aTransform;
    matrix4  bTransform;
    vector3 aDir;
    // todo: need bDir as well?  both objects can be moving..
    bool solve3D = true;
};
bool DetectCollision(const CollisionParams& params, CollisionData* outCollision);
// separated out for testing purposes
#ifdef TEST_PROGRAM
enum COLLISION_RESULT
{
    COLLISION_RESULT_NONE,

    COLLISION_RESULT_OVERLAP,
    COLLISION_RESULT_NO_OVERLAP,
    COLLISION_RESULT_CONTINUE,
};
COLLISION_RESULT DetectCollisionStep(const CollisionParams& params, struct Simplex& simplex);
vector3 GetSearchDirection(const struct Simplex& simplex);
bool FindCollisionDepthStep(const CollisionParams& params, Simplex& simplex, float& outDepth, vector3& outA, vector3& outB, vector3& outp);
#endif