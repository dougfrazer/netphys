#pragma once

//void DebugDraw_Init();

struct DebugDrawColor
{
	float r;
	float g;
	float b;
};

constexpr DebugDrawColor TEXT_COLOR_RED   ({ 1.0f , 0.0f , 0.0f });
constexpr DebugDrawColor TEXT_COLOR_GREEN ({ 0.0f , 1.0f , 0.0f });
constexpr DebugDrawColor TEXT_COLOR_BLUE  ({ 0.0f , 0.0f , 1.0f });
constexpr DebugDrawColor TEXT_COLOR_WHITE ({ 1.0f , 1.0f , 1.0f });

int DebugDraw_AddString(const char* string, int x, int y, DebugDrawColor color);
void DebugDraw_RemoveString(int identifier);
void DebugDraw_RemoveAll();
void DebugDraw_Render();