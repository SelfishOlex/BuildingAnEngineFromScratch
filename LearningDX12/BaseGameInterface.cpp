#include "BaseGameInterface.h"

#include <wrl/client.h>

#include "CommandQueue.h"
#include "d3dx12.h"
#include "DX12App.h"

namespace Olex
{
    BaseGameInterface::BaseGameInterface( DX12App& app ) : m_app( app )
    {
    }

    // Transition a resource
    void BaseGameInterface::TransitionResource( Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList,
        Microsoft::WRL::ComPtr<ID3D12Resource> resource,
        D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState )
    {
        CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            resource.Get(),
            beforeState, afterState );

        commandList->ResourceBarrier( 1, &barrier );
    }

    // Clear a render target.
    void BaseGameInterface::ClearRTV( Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList,
        D3D12_CPU_DESCRIPTOR_HANDLE rtv, FLOAT* clearColor )
    {
        commandList->ClearRenderTargetView( rtv, clearColor, 0, nullptr );
    }

    void BaseGameInterface::ClearDepth( Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList,
        D3D12_CPU_DESCRIPTOR_HANDLE dsv, FLOAT depth )
    {
        commandList->ClearDepthStencilView( dsv, D3D12_CLEAR_FLAG_DEPTH, depth, 0, 0, nullptr );
    }


    void BaseGameInterface::UpdateBufferResource(
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList,
        ID3D12Resource** pDestinationResource,
        ID3D12Resource** pIntermediateResource,
        size_t numElements, size_t elementSize, const void* bufferData,
        D3D12_RESOURCE_FLAGS flags )
    {
        auto device = m_app.GetDevice();

        const size_t bufferSize = numElements * elementSize;

        const CD3DX12_HEAP_PROPERTIES heapPropertiesDefault( D3D12_HEAP_TYPE_DEFAULT );
        const CD3DX12_RESOURCE_DESC bufferDefault = CD3DX12_RESOURCE_DESC::Buffer(bufferSize, flags);

        // Create a committed resource for the GPU resource in a default heap.
        ThrowIfFailed( device->CreateCommittedResource(
            &heapPropertiesDefault,
            D3D12_HEAP_FLAG_NONE,
            &bufferDefault,
            //D3D12_RESOURCE_STATE_COPY_DEST,
            D3D12_RESOURCE_STATE_COMMON,
            nullptr,
            IID_PPV_ARGS( pDestinationResource ) ) );

        // Create an committed resource for the upload.
        if ( bufferData )
        {
            CD3DX12_HEAP_PROPERTIES heapPropertiesUpload( D3D12_HEAP_TYPE_UPLOAD );
            const auto bufferUpload = CD3DX12_RESOURCE_DESC::Buffer( bufferSize );

            ThrowIfFailed( device->CreateCommittedResource(
                &heapPropertiesUpload,
                D3D12_HEAP_FLAG_NONE,
                &bufferUpload,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS( pIntermediateResource ) ) );

            D3D12_SUBRESOURCE_DATA subresourceData = {};
            subresourceData.pData = bufferData;
            subresourceData.RowPitch = bufferSize;
            subresourceData.SlicePitch = subresourceData.RowPitch;

            UpdateSubresources( commandList.Get(),
                *pDestinationResource, *pIntermediateResource,
                0, 0, 1, &subresourceData );
        }
    }

    void BaseGameInterface::ThrowIfFailed( HRESULT hr )
    {
        if ( FAILED( hr ) )
        {
            throw std::exception();
        }
    }
}
