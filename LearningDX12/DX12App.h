#pragma once

#include <cstdint>
#include <d3d12.h>
#include <dxgi1_5.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

#include "framework.h"

namespace Olex
{
    class DX12App
    {
    public:
        DX12App();

        void Init(HWND windowsHandle);

    protected:

        void EnableDebugLayer();

        void ThrowIfFailed( HRESULT hr );

        Microsoft::WRL::ComPtr<IDXGIAdapter4> GetAdapter( bool useWarp );

        Microsoft::WRL::ComPtr<ID3D12Device2> CreateDevice( Microsoft::WRL::ComPtr<IDXGIAdapter4> adapter );

        Microsoft::WRL::ComPtr<ID3D12CommandQueue> CreateCommandQueue( Microsoft::WRL::ComPtr<ID3D12Device2> device,
            D3D12_COMMAND_LIST_TYPE type );

        bool CheckTearingSupport();

        Microsoft::WRL::ComPtr<IDXGISwapChain4> CreateSwapChain( HWND hWnd,
            Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue,
            uint32_t width, uint32_t height, uint32_t bufferCount );

    private:

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
        Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_CommandQueue;
        Microsoft::WRL::ComPtr<IDXGISwapChain4> m_SwapChain;
        Microsoft::WRL::ComPtr<ID3D12Resource> m_BackBuffers[m_NumFrames];
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_CommandList;
        Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_CommandAllocators[m_NumFrames];
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_RTVDescriptorHeap;
        UINT m_RTVDescriptorSize;
        UINT m_CurrentBackBufferIndex;

        // Synchronization objects
        Microsoft::WRL::ComPtr<ID3D12Fence> m_Fence;
        uint64_t m_FenceValue = 0;
        uint64_t m_FrameFenceValues[m_NumFrames] = {};
        HANDLE m_FenceEvent;

        // By default, enable V-Sync.
        // Can be toggled with the V key.
        bool m_VSync = true;
        bool m_TearingSupported = false;
        // By default, use windowed mode.
        // Can be toggled with the Alt+Enter or F11
        bool m_Fullscreen = false;
    };
}
