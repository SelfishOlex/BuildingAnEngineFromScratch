
#include "PhysxWorld.h"

#include <cassert>

#include "../GameWorld.h"


PhysxWorld::~PhysxWorld()
{
    m_cooking->release();
    m_physics->release();
    m_foundation->release();
}

void PhysxWorld::Initialize(GameWorld& world)
{
    m_foundation = PxCreateFoundation(PX_PHYSICS_VERSION, m_allocator, m_errorCallback);
    assert(m_foundation);

    constexpr bool recordMemoryAllocations = true;

    /*m_pvd = physx::PxCreatePvd(*m_foundation);
    assert(m_pvd);
    physx::PxPvdTransport* transport = physx::PxDefaultPvdSocketTransportCreate("localhost", 5425, 10);
    m_pvd->connect(*transport, physx::PxPvdInstrumentationFlag::eALL);*/

    m_physics = PxCreatePhysics(PX_PHYSICS_VERSION, *m_foundation, physx::PxTolerancesScale(), recordMemoryAllocations, m_pvd);
    assert(m_physics);

    const physx::PxCookingParams params(physx::PxTolerancesScale(1.f));
    m_cooking = PxCreateCooking(PX_PHYSICS_VERSION, *m_foundation, params);
    assert(m_cooking);

    assert(PxInitExtensions(*m_physics, m_pvd));

    // Create the scene
    physx::PxSceneDesc sceneDesc(m_physics->getTolerancesScale());
    sceneDesc.gravity = physx::PxVec3(0.0f, 0.0f, -9.81f);
    sceneDesc.cpuDispatcher = &m_dispatcher;
    sceneDesc.filterShader = physx::PxDefaultSimulationFilterShader;

    m_scene = m_physics->createScene(sceneDesc);
    assert(m_scene);

    m_defaultMaterial = m_physics->createMaterial(0.5f, 0.5f, 0.6f);

    world.m_world.observer<PhysicalRigidStatic>("OnAddPhysicalRigidStatic")
        .event(flecs::OnAdd)
        .each([this](const flecs::entity& e, PhysicalRigidStatic& rigidStatic)
            {
                if (const ShapeOfPlane* plane = e.get<ShapeOfPlane>())
                {
                    rigidStatic.rigidStatic = physx::PxCreatePlane(*m_physics,
                        physx::PxPlane(plane->x, plane->y, plane->z, plane->distance), *m_defaultMaterial);
	                m_scene->addActor(*rigidStatic.rigidStatic);
                }
            });

    world.m_world.observer<PhysicalRigidBody>("OnAddPhysicalRigidBody")
        .event(flecs::OnAdd)
        .each([this](const flecs::entity& e, PhysicalRigidBody& rigidBody)
            {
                const ShapeOfSphere* shape = e.get<ShapeOfSphere>();
                const Position* position = e.get<Position>();
                if (shape && position)
                {
                    const physx::PxTransform t(physx::PxVec3(position->x, position->y, position->z));

                    const physx::PxSphereGeometry geometry(shape->sphereRadius);

                    rigidBody.rigidDynamic = physx::PxCreateDynamic(*m_physics, t, geometry, *m_defaultMaterial, 10.0f);
                    rigidBody.rigidDynamic->setAngularDamping(0.5f);
                    m_scene->addActor(*rigidBody.rigidDynamic);
                }
            });

    world.m_world.observer<PhysicalRigidBody>("OnDeletePhysicalRigidBody")
        .event(flecs::OnDelete)
        .each([this](const flecs::entity& e, PhysicalRigidBody& rigidBody)
            {
                if (rigidBody.rigidDynamic)
                {
                    m_scene->removeActor(*rigidBody.rigidDynamic);
                    rigidBody.rigidDynamic->release();
                    rigidBody.rigidDynamic = nullptr;
                }
            });

    world.m_world.system<const PhysicalRigidBody, Position>("UpdateRigidBodies")
        .each([this](const PhysicalRigidBody& rigidBody, Position& p)
            {
                if (rigidBody.rigidDynamic)
                {
                    const physx::PxTransform t = rigidBody.rigidDynamic->getGlobalPose();
                    p.x = t.p.x;
                    p.y = t.p.y;
                    p.z = t.p.z;
                }
            });
}

void PhysxWorld::Update(float deltaSeconds)
{
    if (deltaSeconds > 1.f/60.f)
    {
        deltaSeconds = 1.f/60.f;
    }

    if (m_scene)
    {
        m_scene->simulate(deltaSeconds);
        m_scene->fetchResults(true);
    }
}

void* PhysxWorld::Allocator::allocate(size_t size, const char* typeName, const char* filename, int line)
{
    return malloc(size);
}

void PhysxWorld::Allocator::deallocate(void* ptr)
{
    free(ptr);
}

void PhysxWorld::ErrorCallback::reportError(physx::PxErrorCode::Enum code, const char* message, const char* file,
    int line)
{
    printf("PhysX error: [%s], file [%s], line [%d]", message, file, line);
}

void PhysxWorld::Dispatcher::submitTask(physx::PxBaseTask& task)
{
    task.run();
    task.release();
}

uint32_t PhysxWorld::Dispatcher::getWorkerCount() const
{
    return 1;
}
