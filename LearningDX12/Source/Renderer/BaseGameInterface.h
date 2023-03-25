#pragma once
#include <wrl/client.h>

#include "CommandQueue.h"

namespace Olex
{
    struct UpdateEventArgs
    {
        double m_elapsedTime = 0;
        double m_totalTime = 0;
    };

    struct RenderEventArgs
    {
    };

    struct ResizeEventArgs
    {
        int Width;
        int Height;
    };

    class BaseGameInterface
    {
    public:
        explicit BaseGameInterface(DX12App& app);
        virtual ~BaseGameInterface() = default;

        virtual void LoadResources() = 0;
        virtual void UnloadResources() = 0;

        virtual void Update( UpdateEventArgs args ) = 0;
        virtual void Render( RenderEventArgs args ) = 0;
        virtual void Resize( ResizeEventArgs args ) = 0;

        void TransitionResource( Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList,
            Microsoft::WRL::ComPtr<ID3D12Resource> resource,
            D3D12_RESOURCE_STATES beforeState,
            D3D12_RESOURCE_STATES afterState );
        void ClearRTV( Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList,
            D3D12_CPU_DESCRIPTOR_HANDLE rtv,
            FLOAT* clearColor );
        void ClearDepth( Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList,
            D3D12_CPU_DESCRIPTOR_HANDLE dsv,
            FLOAT depth = 1.0f );

        void UpdateBufferResource( Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList,
            ID3D12Resource** pDestinationResource,
            ID3D12Resource** pIntermediateResource,
            size_t numElements,
            size_t elementSize,
            const void* bufferData,
            D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE );

        void ThrowIfFailed( HRESULT hr );

    protected:
        DX12App& m_app;
    };
}
