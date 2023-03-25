#pragma once
#pragma once
#include <d3d12.h>
#include <d3dx12.h>
#include <DirectXMath.h>
#include <dxgi1_4.h>
#include <memory>
#include <string>
#include <wrl/client.h>


#include "BaseGameInterface.h"
#include "CommandQueue.h"
#include "FbxLoader.h"
#include "GameWorld.h"
#include "PhysicsWorld/PhysxWorld.h"

namespace Olex
{
    class DX12App;

    class MultipleObjectsDemo final
        : public BaseGameInterface
    {
    public:
        MultipleObjectsDemo( DX12App& app );

        void LoadResources() override;
        void ResizeDepthBuffer( int width, int height );
        void UnloadResources() override;

        int GetClientWidth() const
        {
            return m_Width;
        }

        int GetClientHeight() const
        {
            return m_Height;
        }

        void Update( UpdateEventArgs args ) override;
        void Render( RenderEventArgs args ) override;
        void Resize( ResizeEventArgs args ) override;

    private:

        void RenderImGui( RenderEventArgs args );

        struct Texture
        {
            int width;
            int height;
            int pixelSize;
            std::unique_ptr<int8_t*> data;
        };

        Texture LoadTexture( const wchar_t* filename );

        /*
         * Entity Component System
         */

        GameWorld m_gameWorld;

        PhysxWorld m_physx;

        /*
         * DirectX 12 Stuff
         */

        // Vertex buffer for the cube.
        Microsoft::WRL::ComPtr<ID3D12Resource> m_VertexBuffer;
        D3D12_VERTEX_BUFFER_VIEW m_VertexBufferView;
        // Index buffer for the cube.
        Microsoft::WRL::ComPtr<ID3D12Resource> m_IndexBuffer;
        D3D12_INDEX_BUFFER_VIEW m_IndexBufferView;

        // Depth buffer.
        Microsoft::WRL::ComPtr<ID3D12Resource> m_DepthBuffer;
        // Descriptor heap for depth buffer.
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_DSVHeap;
        // To hold a view of a texture
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_SrvHeap;

        // Root signature
        Microsoft::WRL::ComPtr<ID3D12RootSignature> m_RootSignature;

        // Pipeline state object.
        Microsoft::WRL::ComPtr<ID3D12PipelineState> m_PipelineState;

        D3D12_VIEWPORT m_Viewport;
        D3D12_RECT m_ScissorRect;

        float m_FoV;

        DirectX::XMMATRIX m_ModelMatrix;
        DirectX::XMMATRIX m_ViewMatrix;
        DirectX::XMMATRIX m_ProjectionMatrix;

        struct LightInfo
        {
            struct DirectionalLight
            {
                DirectX::XMFLOAT3 m_color = { 1, 1, 1 };
                DirectX::XMFLOAT3 m_direction = { 0, 0, 1 };
                float m_intensity = 1.f;
            };

            DirectionalLight m_directionLight;

            DirectX::XMFLOAT3 m_eyePosition = { 0, 0, 0 };
        };

        LightInfo m_lightInfo;

        struct ObjectInfo
        {
            DirectX::XMMATRIX m_ProjectionMatrix;
        };

        bool m_ContentLoaded;

        int m_Width;
        int m_Height;

        void CreateRootSignature();

        UINT m_frameCount = 0;
        Microsoft::WRL::ComPtr<ID3D12Resource> m_texture;

        FenceValue m_lastFenceValue{ 0 };

        std::unique_ptr<FbxLoader> m_fbxLoader;



        // DX12 objects

        static const UINT FrameCount = 2;

        // Pipeline objects.
        CD3DX12_VIEWPORT m_viewport;
        CD3DX12_RECT m_scissorRect;
        Microsoft::WRL::ComPtr<IDXGISwapChain3> m_swapChain;
        Microsoft::WRL::ComPtr<ID3D12Device> m_device;
        Microsoft::WRL::ComPtr<ID3D12Resource> m_renderTargets[FrameCount];
        Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_commandAllocator;
        Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_commandQueue;
        Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_srvHeap;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipelineState;
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_commandList;
        UINT m_rtvDescriptorSize;

        // App resources.
        Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBuffer;
        D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;

        // Synchronization objects.
        UINT m_frameIndex;
        HANDLE m_fenceEvent;
        Microsoft::WRL::ComPtr<ID3D12Fence> m_fence;
        UINT64 m_fenceValue;

        void LoadPipeline();
        void LoadAssets();
        void PopulateCommandList();
        void WaitForPreviousFrame();
    };
}
