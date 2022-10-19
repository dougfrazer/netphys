#include "world_c.h"
#include "../netphys_common/common.h"
#include "../netphys_common/world.h"
#include "../netphys_common/lib.h"
#include <vector>
#include "../netphys_common/log.h"
#include "network_c.h"
#include "windows.h"



std::vector<CommandFrame> s_serverFrames;
static double s_lastTime = 0.0f;
//static double s_clientTime = 0.0f;
//static double s_clientTimeToServerTime = 0.0f;
static DWORD s_connectionStartTime = 0;
constexpr DWORD TIME_DELAY_MS = 300;

static int s_logFileHandle = 0;


#define WORLD_LOG(x,...)         SYSTEM_LOG(s_logFileHandle, x, __VA_ARGS__) 
#define WORLD_LOG_WARNING(x,...) SYSTEM_LOG_WARNING(s_logFileHandle, x, __VA_ARGS__) 
#define WORLD_ERROR(x, ...)      SYSTEM_LOG_ERROR(s_logFileHandle, x, __VA_ARGS__); __debugbreak();


void World_C_Init()
{
	s_logFileHandle = Log_InitSystem("Client_World");
}

void World_C_Deinit()
{
	Log_DeinitSystem(s_logFileHandle);
}

void World_C_HandleNewConnection(const ClientNewConnection* msg)
{
//	if (s_clientTimeToServerTime)
//	{
//		WORLD_ERROR("Got initialize but we're already initialized");
//	}

	if (s_serverFrames.size())
	{
		WORLD_ERROR("Somehow have server frames before initialization");
		s_serverFrames.clear();
	}

	//s_clientTimeToServerTime = msg->frame.timeMs;
	s_connectionStartTime = GetTickCount();

	s_serverFrames.push_back(msg->frame);
}

void World_C_HandleUpdate(const ClientWorldStateUpdatePacket* msg)
{
	//if (!s_clientTimeToServerTime)
	//{
	//	WORLD_ERROR("Got update before we got full world state");
	//	return;
	//}

	// todo: send down differences and construct the server frame
	s_serverFrames.push_back(msg->frame);
}

static double GetNextServerTime(float dt)
{
	// we want to move forward by dt
	// but we also want to keep our buffer with the server
	// ... todo: keep some kind of running average?
	// simple solution for now: just take the most recent frame time and go back a little
	return s_serverFrames.back().timeMs - TIME_DELAY_MS/2;
}

void World_C_Update(float dt)
{
	if (!s_serverFrames.size())
	{
		return;
	}
	
	if (GetTickCount() - s_connectionStartTime < TIME_DELAY_MS)
	{
		WORLD_LOG("Skipping update, waiting %d ms more for messages from server...", TIME_DELAY_MS - (GetTickCount() - s_connectionStartTime));
		return;
	}

	//s_clientTime += dt;
	//WORLD_LOG("Client Time: %.2f, mrst=%.2f , diff=%.2f", s_clientTime, s_serverFrames.back().timeMs, s_serverFrames.back().timeMs - s_clientTime);
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
		WORLD_LOG_WARNING("Couldn't get two frames to interpolate between");
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

	float lerp = (float)(lerp(serverTime, before->timeMs, after->timeMs, 0.0f, 1.0f));
	for (int i = 0; i < arrsize(before->objects); i++)
	{
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
		const Object& obj = World::Get()->GetInteract(i);
		dGeomSetPosition(obj.m_geomID, pos[0], pos[1], pos[2]);
		dGeomSetQuaternion(obj.m_geomID, rot);
	}

	// erase old server frames?
	const double MAX_SERVER_FRAME_LIFETIME = 2000.f; // 2 seconds seems like enough
	double mostRecentServerTime = s_serverFrames.back().timeMs;
	auto it = s_serverFrames.begin();
	for (; it != s_serverFrames.end(); ++it)
	{
		if (&*it == before)
		{
			break; // if we just used this frame, don't get rid of it
		}
		if (mostRecentServerTime - it->timeMs < MAX_SERVER_FRAME_LIFETIME)
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