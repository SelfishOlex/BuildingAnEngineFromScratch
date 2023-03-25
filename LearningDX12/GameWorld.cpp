
#include "GameWorld.h"


void GameWorld::Initialize()
{
    for (int i = 0; i < 2000; ++i)
    {
        char name[10];
        const int written = sprintf_s(name, 10, "Ball %d", i);
        if (written > 0)
        {
            flecs::entity e = m_world.entity(name);

            const float x = -80 + static_cast<float>(i) * 10.f;
            const float y = static_cast<float>(i * i) * 0.1f;
            const float z = -4 + static_cast<float>(i);

            // e.add<Position>(); // position is automatically added on set()
            e.set<Position>({ x, y, z });
            e.add<Mesh>();
            e.set<OscillatorOffset>({ i * 0.1f, 0.f });
        }
    }

    m_world.set<WorldTime>({ 0.f });

    flecs::entity e = m_world.lookup("Ball 99");
    const auto check = e.is_alive();
    
    m_world.system<Position, OscillatorOffset, const Mesh>("Move Mesh")
        .each([](const flecs::entity& e, Position& p, OscillatorOffset& offset, const Mesh&)
            {
                const WorldTime* worldTime = e.world().get<WorldTime>();
                offset.dz = 0.05f * sinf((worldTime->timeSinceStart + offset.phase) * 2.f);
                p.z += offset.dz;
            });
}

void GameWorld::Update(float deltaTime)
{
    const WorldTime* worldTime = m_world.get<WorldTime>();
    m_world.set<WorldTime>({ worldTime->timeSinceStart + deltaTime });

    const bool result = m_world.progress(deltaTime);
}
