#pragma once

#include "flecs.h"

struct Position
{
    float x;
    float y;
    float z;
};

struct OscillatorOffset
{
    float phase;
    float dz;
};

struct Mesh
{    
};

struct WorldTime
{
    float timeSinceStart;
};

class GameWorld
{
public:
    GameWorld() = default;

    void Initialize();
    void CreateWorld();

    void Update(float deltaTime);

    void DrawImGui();

    flecs::world m_world;

private:
    float m_lastFrameTime = 0.f;
};
