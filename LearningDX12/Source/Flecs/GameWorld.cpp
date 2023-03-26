
#include "GameWorld.h"

#include <ImGui/imgui.h>

#include "PhysicsWorld/PhysxWorld.h"


void GameWorld::Initialize()
{
    m_world.set<WorldTime>({ 0.f });

    m_world.system<Position, OscillatorOffset, const Mesh>("Move Mesh")
        .each([](const flecs::entity& e, Position& p, OscillatorOffset& offset, const Mesh&)
            {
                const WorldTime* worldTime = e.world().get<WorldTime>();
                offset.dz = 5.f * sinf((worldTime->timeSinceStart + offset.phase) * 2.f) * e.delta_time();
                p.z += offset.dz;
            });
}

void GameWorld::CreateWorld()
{
    {
        flecs::entity plane = m_world.entity("Ground");
        plane.add<Position>();
        plane.set<ShapeOfPlane>({ 0, 0, 1, 4 });
        plane.add<PhysicalRigidStatic>();
    }

    for (int i = 0; i < 70; ++i)
    {
        char name[10];
        const int written = sprintf_s(name, 10, "Ball %d", i);
        if (written > 0)
        {
            flecs::entity e = m_world.entity(name);

            const float x = -80 + static_cast<float>(i) * 10.f;
            const float y = static_cast<float>(i * i) * 0.1f;
            const float z = 1 + static_cast<float>(i);

            e.set<Position>({ x, y, z });
            e.add<Mesh>();
            e.set<ShapeOfSphere>({ 1.f });
            e.add<PhysicalRigidBody>();
        }
    }
}

void GameWorld::Update(float deltaTime)
{
    m_lastFrameTime = deltaTime;

    const WorldTime* worldTime = m_world.get<WorldTime>();
    m_world.set<WorldTime>({ worldTime->timeSinceStart + deltaTime });

    const bool result = m_world.progress(deltaTime);
}

void GameWorld::DrawImGui()
{
    ImGui::SetNextWindowSize(ImVec2(250, 100));
    ImGui::Begin("FPS Window");

    if (m_lastFrameTime > 0.f)
    {
        ImGui::Text("fps %0.f", 1.f / m_lastFrameTime);
        ImGui::Text("frame %0.f ms", m_lastFrameTime*1000.f);
    }

    ImGui::End();
}
