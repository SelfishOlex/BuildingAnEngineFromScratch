#include "CommandQueue.h"

#include <cassert>

#include "DX12App.h"

namespace Olex
{
    CommandQueue::CommandQueue( DX12App& application, D3D12_COMMAND_LIST_TYPE type )
        : m_app( application )
        , m_CommandListType( type )
    {
        D3D12_COMMAND_QUEUE_DESC desc = {};
        desc.Type = type;
        desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
        desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        desc.NodeMask = 0;

        m_d3d12CommandQueue = m_app.CreateCommandQueue( m_app.GetDevice(), D3D12_COMMAND_LIST_TYPE_DIRECT );
        m_d3d12Fence = m_app.CreateFence( m_app.GetDevice(), m_FenceValue );
        m_FenceEvent = m_app.CreateEventHandle();
    }

    CommandQueue::~CommandQueue()
    {
        ::CloseHandle( m_FenceEvent );
    }

    void CommandQueue::ThrowIfFailed( HRESULT hr )
    {
        if ( FAILED( hr ) )
        {
            throw std::exception();
        }
    }

    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CommandQueue::CreateCommandAllocator()
    {
        return m_app.CreateCommandAllocator( m_app.GetDevice(), m_CommandListType );
    }

    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> CommandQueue::CreateCommandList( Microsoft::WRL::ComPtr<ID3D12CommandAllocator> allocator )
    {
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList = m_app.CreateCommandList2( m_app.GetDevice(), allocator, m_CommandListType );
        return commandList;
    }

    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> CommandQueue::GetCommandList()
    {
        Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator;
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList;

        if ( !m_CommandAllocatorQueue.empty() && IsFenceComplete( m_CommandAllocatorQueue.front().fenceValue ) )
        {
            commandAllocator = m_CommandAllocatorQueue.front().commandAllocator;
            m_CommandAllocatorQueue.pop();

            ThrowIfFailed( commandAllocator->Reset() );
        }
        else
        {
            commandAllocator = CreateCommandAllocator();
        }

        if ( !m_CommandListQueue.empty() )
        {
            commandList = m_CommandListQueue.front();
            m_CommandListQueue.pop();

            ThrowIfFailed( commandList->Reset( commandAllocator.Get(), nullptr ) );
        }
        else
        {
            commandList = CreateCommandList( commandAllocator );
        }

        // Associate the command allocator with the command list so that it can be
        // retrieved when the command list is executed.
        ThrowIfFailed( commandList->SetPrivateDataInterface( __uuidof( ID3D12CommandAllocator ), commandAllocator.Get() ) );

        return commandList;
    }

    uint64_t CommandQueue::ExecuteCommandList( Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList )
    {
        commandList->Close();

        ID3D12CommandAllocator* commandAllocator;
        UINT dataSize = sizeof( commandAllocator );
        ThrowIfFailed( commandList->GetPrivateData( __uuidof( ID3D12CommandAllocator ), &dataSize, &commandAllocator ) );

        ID3D12CommandList* const ppCommandLists[] = {
            commandList.Get()
        };

        m_d3d12CommandQueue->ExecuteCommandLists( 1, ppCommandLists );
        const uint64_t fenceValue = Signal();

        m_CommandAllocatorQueue.emplace( CommandAllocatorEntry{ fenceValue, commandAllocator } );
        m_CommandListQueue.push( commandList );

        // The ownership of the command allocator has been transferred to the ComPtr
        // in the command allocator queue. It is safe to release the reference
        // in this temporary COM pointer here.
        commandAllocator->Release();

        return fenceValue;
    }

    uint64_t CommandQueue::Signal()
    {
        const uint64_t fenceValueForSignal = ++m_FenceValue;
        ThrowIfFailed( m_d3d12CommandQueue->Signal( m_d3d12Fence.Get(), fenceValueForSignal ) );
        return fenceValueForSignal;
    }

    bool CommandQueue::IsFenceComplete( uint64_t fenceValue )
    {
        return false;
    }

    void CommandQueue::WaitForFenceValue( uint64_t fenceValue, std::chrono::milliseconds duration )
    {
        if ( m_d3d12Fence->GetCompletedValue() < fenceValue )
        {
            ThrowIfFailed( m_d3d12Fence->SetEventOnCompletion( fenceValue, m_FenceEvent ) );
            ::WaitForSingleObject( m_FenceEvent, static_cast<DWORD>( duration.count() ) );
        }
    }

    void CommandQueue::Flush()
    {
        const uint64_t fenceValueForSignal = Signal();
        WaitForFenceValue( fenceValueForSignal );
    }
}
