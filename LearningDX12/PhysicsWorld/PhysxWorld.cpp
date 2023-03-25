
#include "PhysxWorld.h"

#include <cassert>


PhysxWorld::~PhysxWorld()
{
    m_cooking->release();
    m_physics->release();
    m_foundation->release();
}

void PhysxWorld::Initialize()
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
    sceneDesc.gravity = physx::PxVec3(0.0f, -9.81f, 0.0f);
    sceneDesc.cpuDispatcher = &m_dispatcher;
    physx::PxSimulationFilterShader filterShader = [](
        physx::PxFilterObjectAttributes attributes0, physx::PxFilterData filterData0,
        physx::PxFilterObjectAttributes attributes1, physx::PxFilterData filterData1,
        physx::PxPairFlags& pairFlags, const void* constantBlock, physx::PxU32 constantBlockSize) -> physx::PxFilterFlags
    {
        return physx::PxFilterFlags();
    };
    sceneDesc.filterShader = filterShader;

    m_scene = m_physics->createScene(sceneDesc);
    assert(m_scene);

    m_defaultMaterial = m_physics->createMaterial(0.5f, 0.5f, 0.6f);
    physx::PxRigidStatic* groundPlane = physx::PxCreatePlane(*m_physics, physx::PxPlane(0, 1, 0, 0), *m_defaultMaterial);
    m_scene->addActor(*groundPlane);
}

void PhysxWorld::Update(float deltaSeconds)
{
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
}

uint32_t PhysxWorld::Dispatcher::getWorkerCount() const
{
    return 1;
}
