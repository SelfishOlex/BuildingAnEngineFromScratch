#include "CommandQueue.h"

#include <cassert>

#include "DX12App.h"

namespace Olex
{
    using namespace Microsoft::WRL;

    CommandQueue::CommandQueue( DX12App& application, D3D12_COMMAND_LIST_TYPE type )
        : m_app( application )
        , m_CommandListType( type )
    {
        m_d3d12CommandQueue = m_app.CreateCommandQueue( D3D12_COMMAND_LIST_TYPE_DIRECT );
        m_d3d12Fence = m_app.CreateFence( m_FenceValue.Get() );
        m_FenceEvent = m_app.CreateEventHandle();
    }

    CommandQueue::~CommandQueue()
    {
        CloseHandle( m_FenceEvent );
    }

    void CommandQueue::ThrowIfFailed( HRESULT hr )
    {
        if ( FAILED( hr ) )
        {
            throw std::exception();
        }
    }

    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> CommandQueue::CreateCommandList()
    {
        ComPtr<ID3D12CommandAllocator> allocator = m_app.CreateCommandAllocator( m_CommandListType );
        ComPtr<ID3D12GraphicsCommandList2> commandList = m_app.CreateCommandList2( allocator, m_CommandListType );

        // Associate the command allocator with the command list so that it can be
        // retrieved when the command list is executed.
        ThrowIfFailed( commandList->SetPrivateDataInterface( __uuidof( ID3D12CommandAllocator ), allocator.Get() ) );

        return commandList;
    }

    FenceValue CommandQueue::ExecuteCommandList( ComPtr<ID3D12GraphicsCommandList2> commandList )
    {
        commandList->Close();

        ID3D12CommandList* const ppCommandLists[] = {
            commandList.Get()
        };

        m_d3d12CommandQueue->ExecuteCommandLists( 1, ppCommandLists );
        const FenceValue fenceValue = Signal();

        m_commandLists.push_back({commandList, fenceValue});
        return fenceValue;
    }

    FenceValue CommandQueue::Signal()
    {
        const FenceValue fenceValueForSignal = ++m_FenceValue;
        ThrowIfFailed( m_d3d12CommandQueue->Signal( m_d3d12Fence.Get(), fenceValueForSignal.Get() ) );
        return fenceValueForSignal;
    }


    void CommandQueue::WaitForFenceValue( FenceValue fenceValue )
    {
        if ( m_d3d12Fence->GetCompletedValue() < fenceValue.Get() )
        {
            ThrowIfFailed( m_d3d12Fence->SetEventOnCompletion( fenceValue.Get(), m_FenceEvent ) );
            WaitForSingleObject( m_FenceEvent, 9001 );
        }

        const auto itemByFenceValue = std::find_if(m_commandLists.begin(), m_commandLists.end(), [&fenceValue](const CommandListInFlight& item)
        {
            return item.m_fenceValue == fenceValue;
        });

        if (itemByFenceValue != m_commandLists.end())
        {
            m_commandLists.erase(itemByFenceValue);
        }
    }

    void CommandQueue::Flush()
    {
        const FenceValue fenceValueForSignal = Signal();
        WaitForFenceValue( fenceValueForSignal );
    }
}
