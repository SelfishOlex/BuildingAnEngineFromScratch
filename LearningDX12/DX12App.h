#pragma once

#include <cstdint>
#include <d3d12.h>
#include <dxgi1_5.h>
#include <wrl/client.h>

#include "framework.h"

namespace Olex
{
    class DX12App
    {
    public:
        DX12App();

        void Init();

        void EnableDebugLayer();

        void ThrowIfFailed (HRESULT hr);

    private:

        // The number of swap chain back buffers.
        static constexpr uint8_t g_NumFrames = 3;
        // Use WARP adapter
        bool g_UseWarp = false;

        uint32_t g_ClientWidth = 1280;
        uint32_t g_ClientHeight = 720;

        // Set to true once the DX12 objects have been initialized.
        bool g_IsInitialized = false;

        // Window handle.
        HWND g_hWnd;
        // Window rectangle (used to toggle fullscreen state).
        RECT g_WindowRect;

        // DirectX 12 Objects
        Microsoft::WRL::ComPtr<ID3D12Device2> g_Device;
        Microsoft::WRL::ComPtr<ID3D12CommandQueue> g_CommandQueue;
        Microsoft::WRL::ComPtr<IDXGISwapChain4> g_SwapChain;
        Microsoft::WRL::ComPtr<ID3D12Resource> g_BackBuffers[g_NumFrames];
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> g_CommandList;
        Microsoft::WRL::ComPtr<ID3D12CommandAllocator> g_CommandAllocators[g_NumFrames];
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> g_RTVDescriptorHeap;
        UINT g_RTVDescriptorSize;
        UINT g_CurrentBackBufferIndex;

        // Synchronization objects
        Microsoft::WRL::ComPtr<ID3D12Fence> g_Fence;
        uint64_t g_FenceValue = 0;
        uint64_t g_FrameFenceValues[g_NumFrames] = {};
        HANDLE g_FenceEvent;

        // By default, enable V-Sync.
        // Can be toggled with the V key.
        bool g_VSync = true;
        bool g_TearingSupported = false;
        // By default, use windowed mode.
        // Can be toggled with the Alt+Enter or F11
        bool g_Fullscreen = false;
    };
}
