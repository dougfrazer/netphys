#include "world_c.h"
#include "../netphys_common/common.h"
#include "../netphys_common/world.h"
#include "../netphys_common/lib.h"
#include <vector>
#include "../netphys_common/log.h"
#include "network_c.h"
#include "windows.h"
#include "objectmanager_c.h"
#include "player_c.h"

struct CommandFrame
{
	FrameNum id;
	double timeMs;
	std::vector<CommandFrameObject> objects;
};

std::vector<CommandFrame> s_serverFrames;
static double s_lastTime = 0.0f;
static double s_clientTime = 0.0f;
static double s_clientTimeToServerTime = 0.0f;
static DWORD s_connectionStartTime = 0;
constexpr DWORD TIME_DELAY_MS = 100;


void World_C_Init()
{
	
}

void World_C_Deinit()
{
	
}

FrameNum World_C_HandleNewConnection(ClientNewConnection* msg)
{
	if (s_clientTimeToServerTime)
	{
		LOG_ERROR("Got initialize but we're already initialized");
	}

	if (s_serverFrames.size())
	{
		LOG_ERROR("Somehow have server frames before initialization");
		s_serverFrames.clear();
	}

	CommandFrame newFrame;
	newFrame.id = msg->GetID();
	newFrame.timeMs = msg->GetTimeMs();
	int numObjects = msg->GetNumObjects();
	for (int i = 0; i < numObjects; i++)
	{
		const CommandFrameObject* object = msg->GetFrameObject();
		newFrame.objects.push_back(*object);
	}
	s_serverFrames.push_back(newFrame);

	s_clientTimeToServerTime = newFrame.timeMs;
	s_connectionStartTime = GetTickCount();

	return newFrame.id;
}

void HandleNewObject(const NPGUID& guid, float x, float y, float z)
{
	switch (guid.GetType())
	{
		case ObjectType_Player:
		{
			Player_C* player = dynamic_cast<Player_C*>(ObjectManager_C_CreateObject(guid));
			assert(player);
			player->Init(x,y,z);
		}
		break;

		case ObjectType_WorldObject:
		{
			WorldObject* wo = dynamic_cast<WorldObject*>(ObjectManager_C_CreateObject(guid));
			assert(wo);
			wo->Init(x, y, z);
		}
		break;

		default:
		{
			LOG_ERROR("Got invalid guid type: %d",  (int)guid.GetType());
		}
		break;
	}

}

FrameNum World_C_HandleUpdate(ClientWorldStateUpdatePacket* msg)
{
	if (!s_clientTimeToServerTime)
	{
		LOG_ERROR("Got update before we got full world state");
		return 0;
	}

	CommandFrame newFrame;
	newFrame.id = msg->GetID();
	newFrame.timeMs = msg->GetTimeMs();
	int numObjects = msg->GetNumObjects();
	for (int i = 0; i < numObjects; i++)
	{
		const CommandFrameObject* object = msg->GetFrameObject();
		Object* obj = ObjectManager_C_LookupObject(object->guid);
		if (!obj)
		{
			HandleNewObject(object->guid, object->pos[0], object->pos[1], object->pos[2]);
		}
		newFrame.objects.push_back(*object);
	}
	s_serverFrames.push_back(newFrame);

	return newFrame.id;
}

static double GetNextServerTime(float dt)
{
	double nextClientTime = s_clientTime + dt;
	double nextServerTime = nextClientTime + s_clientTimeToServerTime;
	float timeFromLatestServer = float(s_serverFrames.back().timeMs - nextServerTime);

	float drift = fabsf(timeFromLatestServer - (float)TIME_DELAY_MS);
	if (drift > (1000.f / 30.f)) // more than 33ms
	{
		// we're drifting too far from our expected time delay,
		// speed up or slow down the simulation on the client to
		// get us closer to the expected TIME_DELAY_MS
		float new_dt = lerp(timeFromLatestServer, 0, (float)TIME_DELAY_MS * 2.f, 0.f, dt * 2.f);
		s_clientTime += new_dt;
	}
	else
	{
		s_clientTime += dt;
	}

	return s_clientTime + s_clientTimeToServerTime;
}

void World_C_Update(float dt)
{
	if (!s_serverFrames.size())
	{
		return;
	}
	
	if (GetTickCount() - s_connectionStartTime < TIME_DELAY_MS)
	{
		LOG("Skipping update, waiting %d ms more for messages from server...", TIME_DELAY_MS - (GetTickCount() - s_connectionStartTime));
		return;
	}


	double serverTime = GetNextServerTime(dt);

	CommandFrame* before = nullptr;
	CommandFrame* after = nullptr;
	for(int i = 0; i < (int)s_serverFrames.size(); i++)
	{
		if(s_serverFrames[i].timeMs < serverTime)
		{
			before = &s_serverFrames[i];
		}
		if(!after && s_serverFrames[i].timeMs > serverTime)
		{
			after = &s_serverFrames[i];
			break;
		}
	}
	if(!before || !after)
	{
		LOG_WARNING("Couldn't get two frames to interpolate between");
		if (before && !after)
		{
			after = before;
		}
		else if (after && !before)
		{
			before = after;
		}
		else
		{
			return;
		}
	}

	float lerp = (float)(lerp(serverTime, before->timeMs, after->timeMs, 0.0l, 1.0l));
	for (unsigned int i = 0; i < before->objects.size(); i++)
	{
		const Object* obj = ObjectManager_C_LookupObject(before->objects[i].guid);
		if (!obj)
			continue; // todo: this seems bad... we probably should have already created this object
		dBodyID bodyID = obj->GetBodyID();
		if(!bodyID)
			continue; // todo: this also seems bad.... objects should exist in the world

		dVector3 pos = {
			(before->objects[i].pos[0] * (1.0f - lerp)) + (after->objects[i].pos[0] * lerp),
			(before->objects[i].pos[1] * (1.0f - lerp)) + (after->objects[i].pos[1] * lerp),
			(before->objects[i].pos[2] * (1.0f - lerp)) + (after->objects[i].pos[2] * lerp),
		};
		dQuaternion rot = {
			(before->objects[i].rot[0] * (1.0f - lerp)) + (after->objects[i].rot[0] * lerp),
			(before->objects[i].rot[1] * (1.0f - lerp)) + (after->objects[i].rot[1] * lerp),
			(before->objects[i].rot[2] * (1.0f - lerp)) + (after->objects[i].rot[2] * lerp),
			(before->objects[i].rot[3] * (1.0f - lerp)) + (after->objects[i].rot[3] * lerp),
		};

		//dCopyVector3(obj.serverPos, pos);
		//dCopyVector4(obj.serverRot, rot);
		dBodySetPosition(bodyID, pos[0], pos[1], pos[2]);
		dBodySetQuaternion(bodyID, rot);
		if (before->objects[i].isEnabled)
		{
			dBodyEnable(bodyID);
		}
		else
		{
			dBodyDisable(bodyID);
		}
	}

	// erase old server frames?
	const double MAX_SERVER_FRAME_LIFETIME = 2000.f; // 2 seconds seems like enough
	double mostRecentServerTime = s_serverFrames.back().timeMs;
	auto it = s_serverFrames.begin();
	for (; it != s_serverFrames.end(); ++it)
	{
		if (&(*it) == before)
		{
			break; // if we just used this frame, don't get rid of it
		}
		if (mostRecentServerTime - (*it).timeMs < MAX_SERVER_FRAME_LIFETIME)
		{
			break; // seems reasonable to only keep a few seconds...
		}
	}
	
	if (it != s_serverFrames.begin())
	{
		it = it - 1; // go back a frame
		if (it != s_serverFrames.begin())
		{
			s_serverFrames.erase(s_serverFrames.begin(), it);
		}
	}
}