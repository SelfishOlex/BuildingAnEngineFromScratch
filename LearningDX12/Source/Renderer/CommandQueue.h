#pragma once

/**
 * Wrapper class for a ID3D12CommandQueue.
 */

#include <d3d12.h>  // For ID3D12CommandQueue, ID3D12Device2, and ID3D12Fence
#include <wrl.h>    // For Microsoft::WRL::ComPtr
#include <cstdint>  // For uint64_t
#include <array>

namespace Olex
{
    class DX12App;

    class FenceValue
    {
    public:
        explicit FenceValue( uint64_t value ) : m_fenceValue( value ) {}
        [[nodiscard]] uint64_t Get() const { return m_fenceValue; }
        FenceValue& operator++ () { ++m_fenceValue; return *this; }

    private:
        uint64_t m_fenceValue;
    };

    inline bool operator== (const FenceValue& lhs, const FenceValue& rhs) { return lhs.Get() == rhs.Get(); }

    class CommandQueue final
    {
    public:
        CommandQueue( DX12App& application, D3D12_COMMAND_LIST_TYPE type );
        ~CommandQueue();

        // Get an available command list from the command queue.
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> CreateCommandList();

        // Executes a command list.
        // Returns the fence value to wait for for this command list.
        FenceValue ExecuteCommandList( Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList );

        FenceValue Signal();
        void WaitForFenceValue( FenceValue fenceValue );
        void Flush();

        [[nodiscard]] Microsoft::WRL::ComPtr<ID3D12CommandQueue> GetD3D12CommandQueue() const { return m_d3d12CommandQueue; }

    private:
        DX12App& m_app;

        D3D12_COMMAND_LIST_TYPE                     m_CommandListType;
        Microsoft::WRL::ComPtr<ID3D12CommandQueue>  m_d3d12CommandQueue;
        Microsoft::WRL::ComPtr<ID3D12Fence>         m_d3d12Fence;
        HANDLE                                      m_FenceEvent;
        FenceValue                                  m_FenceValue{ 0 };

        struct CommandListInFlight
        {
            Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2>  m_commandList;
            FenceValue                                          m_fenceValue{0};
        };

        std::array<CommandListInFlight, 4> m_commandLists;

        void ThrowIfFailed( HRESULT hr );
    };
}
