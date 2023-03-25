
#pragma once

#include <PxPhysicsAPI.h>

class PhysxWorld
{
public:
    ~PhysxWorld();

    void Initialize();
    void Update(float deltaSeconds);

    struct Allocator : physx::PxAllocatorCallback
    {
        void* allocate(size_t size, const char* typeName, const char* filename, int line) override;
        void deallocate(void* ptr) override;
    };

    Allocator m_allocator;

    
    struct ErrorCallback : physx::PxErrorCallback
    {
        void reportError(physx::PxErrorCode::Enum code, const char* message, const char* file, int line) override;
    };

    ErrorCallback m_errorCallback;

    struct Dispatcher : physx::PxCpuDispatcher
    {
        void submitTask(physx::PxBaseTask& task) override;
        uint32_t getWorkerCount() const override;
    };

    Dispatcher m_dispatcher;
    
    physx::PxFoundation* m_foundation;
    physx::PxPvd* m_pvd = nullptr;
    physx::PxPvdTransport* m_transport;
    physx::PxPhysics* m_physics;
    physx::PxCooking* m_cooking;
    physx::PxScene* m_scene;

    physx::PxMaterial* m_defaultMaterial;
};
