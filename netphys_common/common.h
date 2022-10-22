#pragma once

#ifndef dDOUBLE
#define dDOUBLE
#endif

#include "../netphys_common/lib.h"
#include "intrin.h"

#define arrsize(x) sizeof(x) / sizeof(*x)
#define lerp(x, x0, x1, y0, y1)  ((fabsf(x0-x1)<0.000001) ? (x0) : (y0 + ((float)(y1 - y0) * ((x - x0) / (float)(x1 - x0)))))

enum ObjectType
{
    ObjectType_Invalid     = 0,
    ObjectType_Player      = (1 << 0),
    ObjectType_WorldObject = (1 << 1),
};
static const char* s_objectNames[] =
{
    "Player",
    "WorldObject"
};
static constexpr unsigned int ObjectTypeMask = 0xFF000000;

struct NPGUID
{
public:
    NPGUID(unsigned int uniqueID, ObjectType objectType)
    {
        assert((uniqueID & ObjectTypeMask) == 0); // we're using the upper bits
        v = (uniqueID) | (objectType << 24);
    }
    unsigned int GetUniqueID() const { return v & ~ObjectTypeMask; }
    ObjectType GetType() const { return (ObjectType)(v >> 24); }
    bool operator==(const NPGUID& rhs) const { return v == rhs.v; }
private:
    unsigned int v;

};
static unsigned int s_guid = 1;
static NPGUID GetNewGUID(ObjectType type) { return NPGUID(s_guid++, type); }

#define F_GUID "%s-%d"
#define VA_GUID(x) s_objectNames[_tzcnt_u32(x.GetType())],x.GetUniqueID() 

enum PLAYER_INPUT
{
    INPUT_SPACE            = (1 << 0),
    INPUT_RESET_WORLD      = (1 << 1),

    INPUT_MOVE_FORWARD     = (1 << 2),
    INPUT_MOVE_BACKWARD    = (1 << 3),
    INPUT_MOVE_LEFT        = (1 << 4),
    INPUT_MOVE_RIGHT       = (1 << 5),
};

static constexpr int NUM_ROWS_COLS = 3;
static constexpr int NUM_INTERACTS = NUM_ROWS_COLS * NUM_ROWS_COLS;

// world constants
static constexpr float GRAVITY = 9.8f; // m/s^2
static constexpr float BOX_SIZE = 0.5f;
static constexpr float DENSITY = 5.0f;
static constexpr float TOTAL_MASS = DENSITY * BOX_SIZE * BOX_SIZE * BOX_SIZE;

//
// Packets
//
// this is crappy and doesn't consider endianness or anything
// its just an initial implementation to get stood up quickly

struct Packet
{
    Packet() { }
    Packet(int _id) : m_id(_id) {}
    int GetType() const { return m_id; }
    int Serialize(char* buffer, unsigned int size)
    {
        const unsigned int packetSize = 8 + data.GetSize();
        if (size < packetSize)
        {
            return 0;
        }
        int dataSize = data.GetSize();
        memcpy(&buffer[0], &m_id, sizeof(int));
        memcpy(&buffer[4], &dataSize, sizeof(int));
        if (dataSize)
        {
            memcpy(&buffer[8], data.GetData(), data.GetSize());
            data.Free();
        }
        return packetSize;
    }
    int Deserialize(char* buffer)
    {
        int dataSize = 0;
        memcpy(&m_id, &buffer[0], sizeof(int));
        memcpy(&dataSize, &buffer[4], sizeof(int));
        if (dataSize)
        {
            data.Push(&buffer[8], dataSize);
        }
        data.Finalize();
        return dataSize + 8;
    }
    void Finalize() { data.Finalize(); }
public:
    DataStore data;
private:
    int m_id;
};

//
// to client
//
static constexpr int CLIENT_WORLD_STATE_UPDATE_ID = 2342144;
static constexpr int CLIENT_HANDLE_WORLD_RESET_ID = 2389;
static constexpr int CLIENT_NEW_CONNECTION_ID = 2342341;

struct CommandFrameObject
{
    CommandFrameObject(const NPGUID& _guid) : guid(_guid) {}
    const NPGUID guid;
    float pos[3];
    float rot[4];
    bool isValid;
    bool isEnabled;
};
typedef unsigned int FrameNum;

struct ClientNewConnection : public Packet
{
    ClientNewConnection() : Packet(CLIENT_NEW_CONNECTION_ID) {}

    void PutID(FrameNum id) { data.PushInt(id); }
    void PutTimeMs(double timeMs) { data.PushDouble(timeMs); }
    void PutNumObjects(int numObjects) { data.PushInt(numObjects); }
    void PutFrameObject(const CommandFrameObject& obj) { data.Push((char*)&obj, sizeof(CommandFrameObject)); }

    void Finalize() { data.Finalize(); }

    FrameNum GetID()    { return data.GetInt(); }
    double GetTimeMs()  { return data.GetDouble(); }
    int GetNumObjects() { return data.GetInt(); }
    const CommandFrameObject* GetFrameObject() { return (const CommandFrameObject*)data.Get(sizeof(CommandFrameObject)); }
};


struct ClientWorldStateUpdatePacket : public Packet
{
    ClientWorldStateUpdatePacket() : Packet(CLIENT_WORLD_STATE_UPDATE_ID) {}

    void PutID(FrameNum id) { data.PushInt(id); }
    void PutTimeMs(double timeMs) { data.PushDouble(timeMs); }
    void PutNumObjects(int num) { data.PushInt(num); }
    void PutFrameObject(const CommandFrameObject& obj) { data.Push((char*)&obj, sizeof(CommandFrameObject)); }

    void Finalize() { data.Finalize(); }

    FrameNum GetID()    { return data.GetInt(); }
    double GetTimeMs()  { return data.GetDouble(); }
    int GetNumObjects() { return data.GetInt(); }
    const CommandFrameObject* GetFrameObject() { return (const CommandFrameObject*)data.Get(sizeof(CommandFrameObject)); }
};

struct ClientHandleWorldStateResetPacket : public Packet
{
    ClientHandleWorldStateResetPacket() : Packet(CLIENT_HANDLE_WORLD_RESET_ID) {}
};


//
// to server
//
static constexpr int SERVER_NEW_CONNECTION_ID = 349898;
static constexpr int SERVER_NEW_CONNECTION_ACK_ID = 438798;
static constexpr int SERVER_WORLD_UPDATE_ACK_ID = 3478922;
static constexpr int SERVER_INPUT_PACKET_ID = 123456;

struct ServerNewConnection : public Packet
{
    ServerNewConnection() : Packet(SERVER_NEW_CONNECTION_ID) {}
};

struct ServerNewConnectionAck : public Packet
{
    ServerNewConnectionAck() : Packet(SERVER_NEW_CONNECTION_ACK_ID) {}
    
    void PutFrameNum(FrameNum f) { data.PushUint(f); }
    int GetFrameNum() { return data.GetUint(); }
};

struct ServerWorldUpdateAck : public Packet
{
    ServerWorldUpdateAck() : Packet(SERVER_WORLD_UPDATE_ACK_ID) {}

    void PutFrameNum(FrameNum f) { data.PushUint(f); }
    int GetFrameNum() { return data.GetUint(); }
};

struct ServerInputPacket : public Packet
{
    ServerInputPacket() : Packet(SERVER_INPUT_PACKET_ID) {}

    void PutMask(FrameNum f) { data.PushInt(f); }
    int GetMask() { return data.GetInt(); }
};
