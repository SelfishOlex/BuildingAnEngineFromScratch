#include "TexturedDemoBoxGame.h"

#include <d3d12.h>
#include <d3dcompiler.h>
#include <wrl/client.h>


#include "d3dx12.h"
#include "DX12App.h"

namespace Olex
{
    using namespace Microsoft::WRL;

    TexturedDemoBoxGame::TexturedDemoBoxGame( DX12App& app ) : BaseGameInterface(app)
        , m_ScissorRect( CD3DX12_RECT( 0, 0, LONG_MAX, LONG_MAX ) )
        , m_FoV( 45.0 )
        , m_ContentLoaded( false )
    {
        RECT windowRect;
        ::GetWindowRect( m_app.GetWindowHandle(), &windowRect );

        m_Width = windowRect.right - windowRect.left;
        m_Height = windowRect.bottom - windowRect.top;
        m_Viewport = CD3DX12_VIEWPORT( 0.0f, 0.0f, static_cast<float>( m_Width ), static_cast<float>( m_Height ) );
    }

    void TexturedDemoBoxGame::LoadResources()
    {
        Microsoft::WRL::ComPtr<ID3D12Device2> device = m_app.GetDevice();

        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList = m_app.GetCommandQueue().GetCommandList();

        // Upload vertex buffer data.
        ComPtr<ID3D12Resource> intermediateVertexBuffer;
        UpdateBufferResource( commandList.Get(),
            &m_VertexBuffer, &intermediateVertexBuffer,
            _countof( m_Vertices ), sizeof( VertexPosColor ), m_Vertices );

        // Create the vertex buffer view.
        m_VertexBufferView.BufferLocation = m_VertexBuffer->GetGPUVirtualAddress();
        m_VertexBufferView.SizeInBytes = sizeof( m_Vertices );
        m_VertexBufferView.StrideInBytes = sizeof( VertexPosColor );

        // Upload index buffer data.
        ComPtr<ID3D12Resource> intermediateIndexBuffer;
        UpdateBufferResource( commandList.Get(),
            &m_IndexBuffer, &intermediateIndexBuffer,
            _countof( m_Indices ), sizeof( WORD ), m_Indices );

        // Create index buffer view.
        m_IndexBufferView.BufferLocation = m_IndexBuffer->GetGPUVirtualAddress();
        m_IndexBufferView.Format = DXGI_FORMAT_R16_UINT;
        m_IndexBufferView.SizeInBytes = sizeof( m_Indices );

        // Create the descriptor heap for the depth-stencil view.
        D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
        dsvHeapDesc.NumDescriptors = 1;
        dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        ThrowIfFailed( device->CreateDescriptorHeap( &dsvHeapDesc, IID_PPV_ARGS( &m_DSVHeap ) ) );

        // Load the vertex shader.
        ComPtr<ID3DBlob> vertexShaderBlob;
        ThrowIfFailed( D3DReadFileToBlob( L"VertexShader.cso", &vertexShaderBlob ) );

        // Load the pixel shader.
        ComPtr<ID3DBlob> pixelShaderBlob;
        ThrowIfFailed( D3DReadFileToBlob( L"PixelShader.cso", &pixelShaderBlob ) );

        // Create the vertex input layout
        D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        };

        // Create a root signature.
        D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
        if ( FAILED( device->CheckFeatureSupport( D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof( featureData ) ) ) )
        {
            featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
        }

        // Allow input layout and deny unnecessary access to certain pipeline stages.
        D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
            D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

        // A single 32-bit constant root parameter that is used by the vertex shader.
        CD3DX12_ROOT_PARAMETER1 rootParameters[1];
        rootParameters[0].InitAsConstants( sizeof( DirectX::XMMATRIX ) / 4, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX );

        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
        rootSignatureDescription.Init_1_1( _countof( rootParameters ), rootParameters, 0, nullptr, rootSignatureFlags );

        // Serialize the root signature.
        ComPtr<ID3DBlob> rootSignatureBlob;
        ComPtr<ID3DBlob> errorBlob;
        ThrowIfFailed( D3DX12SerializeVersionedRootSignature( &rootSignatureDescription,
            featureData.HighestVersion, &rootSignatureBlob, &errorBlob ) );
        // Create the root signature.
        ThrowIfFailed( device->CreateRootSignature( 0, rootSignatureBlob->GetBufferPointer(),
            rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS( &m_RootSignature ) ) );

        struct PipelineStateStream
        {
            CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
            CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT InputLayout;
            CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY PrimitiveTopologyType;
            CD3DX12_PIPELINE_STATE_STREAM_VS VS;
            CD3DX12_PIPELINE_STATE_STREAM_PS PS;
            CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT DSVFormat;
            CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RTVFormats;
        } pipelineStateStream;

        D3D12_RT_FORMAT_ARRAY rtvFormats = {};
        rtvFormats.NumRenderTargets = 1;
        rtvFormats.RTFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

        pipelineStateStream.pRootSignature = m_RootSignature.Get();
        pipelineStateStream.InputLayout = { inputLayout, _countof( inputLayout ) };
        pipelineStateStream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        pipelineStateStream.VS = CD3DX12_SHADER_BYTECODE( vertexShaderBlob.Get() );
        pipelineStateStream.PS = CD3DX12_SHADER_BYTECODE( pixelShaderBlob.Get() );
        pipelineStateStream.DSVFormat = DXGI_FORMAT_D32_FLOAT;
        pipelineStateStream.RTVFormats = rtvFormats;

        D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = {
    sizeof( PipelineStateStream ), &pipelineStateStream
        };
        ThrowIfFailed( device->CreatePipelineState( &pipelineStateStreamDesc, IID_PPV_ARGS( &m_PipelineState ) ) );

        const uint64_t fenceValue = m_app.GetCommandQueue().ExecuteCommandList( commandList );
        m_app.GetCommandQueue().WaitForFenceValue( fenceValue );

        m_ContentLoaded = true;

        // Resize/Create the depth buffer.
        ResizeDepthBuffer( GetClientWidth(), GetClientHeight() );
    }

    void TexturedDemoBoxGame::ResizeDepthBuffer( int width, int height )
    {
        if ( m_ContentLoaded )
        {
            // Flush any GPU commands that might be referencing the depth buffer.
            //m_app.Flush();

            width = std::max( 1, width );
            height = std::max( 1, height );

            auto device = m_app.GetDevice();

            // Resize screen dependent resources.
            // Create a depth buffer.
            D3D12_CLEAR_VALUE optimizedClearValue = {};
            optimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
            optimizedClearValue.DepthStencil = { 1.0f, 0 };

            ThrowIfFailed( device->CreateCommittedResource(
                &CD3DX12_HEAP_PROPERTIES( D3D12_HEAP_TYPE_DEFAULT ),
                D3D12_HEAP_FLAG_NONE,
                &CD3DX12_RESOURCE_DESC::Tex2D( DXGI_FORMAT_D32_FLOAT, width, height,
                    1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL ),
                D3D12_RESOURCE_STATE_DEPTH_WRITE,
                &optimizedClearValue,
                IID_PPV_ARGS( &m_DepthBuffer )
            ) );

            // Update the depth-stencil view.
            D3D12_DEPTH_STENCIL_VIEW_DESC dsv = {};
            dsv.Format = DXGI_FORMAT_D32_FLOAT;
            dsv.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
            dsv.Texture2D.MipSlice = 0;
            dsv.Flags = D3D12_DSV_FLAG_NONE;

            device->CreateDepthStencilView( m_DepthBuffer.Get(), &dsv,
                m_DSVHeap->GetCPUDescriptorHandleForHeapStart() );
        }
    }

    void TexturedDemoBoxGame::Resize( ResizeEventArgs args )
    {
        if ( args.Width != GetClientWidth() || args.Height != GetClientHeight() )
        {
            m_Viewport = CD3DX12_VIEWPORT( 0.0f, 0.0f,
                static_cast<float>( args.Width ), static_cast<float>( args.Height ) );

            ResizeDepthBuffer( args.Width, args.Height );
        }
    }

    void TexturedDemoBoxGame::UnloadResources()
    {
    }

    void TexturedDemoBoxGame::Update( UpdateEventArgs args )
    {
        static uint64_t frameCount = 0;
        static double totalTime = 0.0;

        totalTime += args.m_elapsedTime;
        frameCount++;

        if ( totalTime > 1.0 )
        {
            const double fps = frameCount / totalTime;

            wchar_t buffer[512];
            swprintf_s( buffer, _countof( buffer ), L"FPS: %f\n", fps );
            OutputDebugString( buffer );

            frameCount = 0;
            totalTime = 0.0;
        }

        using namespace DirectX;

        // Update the model matrix.
        const float angle = static_cast<float>( args.m_totalTime * 90.0 );
        const XMVECTOR rotationAxis = XMVectorSet( 0, 1, 1, 0 );
        m_ModelMatrix = XMMatrixRotationAxis( rotationAxis, XMConvertToRadians( angle ) );

        // Update the view matrix.
        const XMVECTOR eyePosition = XMVectorSet( 0, 0, -10, 1 );
        const XMVECTOR focusPoint = XMVectorSet( 0, 0, 0, 1 );
        const XMVECTOR upDirection = XMVectorSet( 0, 1, 0, 0 );
        m_ViewMatrix = XMMatrixLookAtLH( eyePosition, focusPoint, upDirection );

        // Update the projection matrix.
        const float aspectRatio = static_cast<float>( GetClientWidth() ) / static_cast<float>( GetClientHeight() );
        m_ProjectionMatrix = XMMatrixPerspectiveFovLH( XMConvertToRadians( m_FoV ), aspectRatio, 0.1f, 100.0f );
    }

    void TexturedDemoBoxGame::Render( RenderEventArgs args )
    {
        using namespace DirectX;

        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList = m_app.GetCommandQueue().GetCommandList();

        Microsoft::WRL::ComPtr<ID3D12Resource> backBuffer = m_app.GetCurrentBackBuffer();
        D3D12_CPU_DESCRIPTOR_HANDLE rtv = m_app.GetCurrentRenderTargetView();
        D3D12_CPU_DESCRIPTOR_HANDLE dsv = m_DSVHeap->GetCPUDescriptorHandleForHeapStart();

        // Clear the render targets.
        {
            TransitionResource( commandList, backBuffer,
                D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET );

            FLOAT clearColor[] = { 0.4f, 0.6f, 0.9f, 1.0f };

            ClearRTV( commandList, rtv, clearColor );
            ClearDepth( commandList, dsv );
        }

        commandList->SetPipelineState( m_PipelineState.Get() );
        commandList->SetGraphicsRootSignature( m_RootSignature.Get() );

        // IA = Input Assembler
        commandList->IASetPrimitiveTopology( D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
        commandList->IASetVertexBuffers( 0, 1, &m_VertexBufferView );
        commandList->IASetIndexBuffer( &m_IndexBufferView );

        // RS = Rasterizer State
        commandList->RSSetViewports( 1, &m_Viewport );
        commandList->RSSetScissorRects( 1, &m_ScissorRect );

        // OM = Output Merger
        commandList->OMSetRenderTargets( 1, &rtv, FALSE, &dsv );

        // Update the MVP matrix
        XMMATRIX mvpMatrix = XMMatrixMultiply( m_ModelMatrix, m_ViewMatrix );
        mvpMatrix = XMMatrixMultiply( mvpMatrix, m_ProjectionMatrix );
        commandList->SetGraphicsRoot32BitConstants( 0, sizeof( XMMATRIX ) / 4, &mvpMatrix, 0 );

        // draw the cube
        commandList->DrawIndexedInstanced( _countof( m_Indices ), 1, 0, 0, 0 );

        // Present
        {
            TransitionResource( commandList, backBuffer,
                D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT );

            const uint64_t fenceValue = m_app.GetCommandQueue().ExecuteCommandList( commandList );

            m_app.Present();

            m_app.GetCommandQueue().WaitForFenceValue( fenceValue );
        }
    }
}
