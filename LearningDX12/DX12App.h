#pragma once

#include <cstdint>
#include <d3d12.h>
#include <dxgi1_5.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <chrono>
#include <memory>

#include "BaseGameInterface.h"
#include "CommandQueue.h"
#include "framework.h"

namespace Olex
{
    class DX12App
    {
    public:
        DX12App() = default;
        ~DX12App();

        // public API
        void Init( HWND windowsHandle );
        HWND GetWindowHandle() { return m_hWnd; }

        bool IsInitialized() const { return m_IsInitialized; }

        void SetGame( std::unique_ptr<BaseGameInterface> game );

        void OnPaintEvent();
        void OnKeyEvent( WPARAM wParam );
        void OnResize();

        void Present();


        // API for derivative classes
        Microsoft::WRL::ComPtr<ID3D12Device2>& GetDevice() { return m_Device; }

        void EnableDebugLayer();

        void ThrowIfFailed( HRESULT hr );

        Microsoft::WRL::ComPtr<IDXGIAdapter4> GetAdapter( bool useWarp );

        Microsoft::WRL::ComPtr<ID3D12Device2> CreateDevice( Microsoft::WRL::ComPtr<IDXGIAdapter4> adapter );

        Microsoft::WRL::ComPtr<ID3D12CommandQueue> CreateCommandQueue( D3D12_COMMAND_LIST_TYPE type );

        bool CheckTearingSupport();

        Microsoft::WRL::ComPtr<IDXGISwapChain4> CreateSwapChain( HWND hWnd, Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue, uint32_t width, uint32_t height, uint32_t bufferCount );

        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap( D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors );

        void UpdateRenderTargetViews( Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain, Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap );

        Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CreateCommandAllocator( D3D12_COMMAND_LIST_TYPE type );

        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> CreateCommandList( Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator,
            D3D12_COMMAND_LIST_TYPE type );

        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> CreateCommandList2( Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator, D3D12_COMMAND_LIST_TYPE type );

        Microsoft::WRL::ComPtr<ID3D12Fence> CreateFence( UINT64 initialValue );

        HANDLE CreateEventHandle();

        uint64_t Signal( Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue, Microsoft::WRL::ComPtr<ID3D12Fence> fence, uint64_t& fenceValue );

        void WaitForFenceValue( Microsoft::WRL::ComPtr<ID3D12Fence> fence, uint64_t fenceValue, HANDLE fenceEvent, std::chrono::milliseconds duration = std::chrono::milliseconds::max() );

        void Flush( Microsoft::WRL::ComPtr<struct ID3D12CommandQueue> commandQueue, Microsoft::WRL::ComPtr<struct ID3D12Fence> fence, uint64_t& fenceValue, HANDLE fenceEvent );

        void Update();
        void Render();
        void Resize( uint32_t width, uint32_t height );
        void SetFullscreen( bool fullscreen );

        UINT GetCurrentBackBufferIndex() { return m_SwapChain->GetCurrentBackBufferIndex(); }
        Microsoft::WRL::ComPtr<ID3D12Resource> GetCurrentBackBuffer() { return m_BackBuffers[GetCurrentBackBufferIndex()]; }
        D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentRenderTargetView() const;

        CommandQueue& GetCommandQueue() { return *m_CommandQueue; }

    private:

        std::unique_ptr<BaseGameInterface> m_currentGame;

        // The number of swap chain back buffers.
        static constexpr uint8_t m_NumFrames = 3;
        // Use WARP adapter
        bool m_UseWarp = false;

        uint32_t m_ClientWidth = 1280;
        uint32_t m_ClientHeight = 720;

        // Set to true once the DX12 objects have been initialized.
        bool m_IsInitialized = false;

        // Window handle.
        HWND m_hWnd;
        // Window rectangle (used to toggle fullscreen state).
        RECT m_WindowRect;

        // DirectX 12 Objects
        Microsoft::WRL::ComPtr<ID3D12Device2> m_Device;

        std::unique_ptr<CommandQueue> m_CommandQueue;

        Microsoft::WRL::ComPtr<IDXGISwapChain4> m_SwapChain;
        Microsoft::WRL::ComPtr<ID3D12Resource> m_BackBuffers[m_NumFrames];
        //Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_CommandList;
        //Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_CommandAllocators[m_NumFrames];
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_RTVDescriptorHeap;
        UINT m_RTVDescriptorSize;
        UINT m_CurrentBackBufferIndex;

        //// Synchronization objects
        //Microsoft::WRL::ComPtr<ID3D12Fence> m_Fence;
        //uint64_t m_FenceValue = 0;
        //uint64_t m_FrameFenceValues[m_NumFrames] = {};
        //HANDLE m_FenceEvent;

        // Can be toggled with the V key.
        bool m_VSync = false;
        bool m_TearingSupported = false;
        // By default, use windowed mode.
        // Can be toggled with the Alt+Enter or F11
        bool m_Fullscreen = false;
    };
}
