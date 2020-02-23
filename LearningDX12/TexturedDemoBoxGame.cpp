#include "TexturedDemoBoxGame.h"

#include <d3d12.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <filesystem>
#include <WICTextureLoader.h>
#include <ResourceUploadBatch.h>

#include "d3dx12.h"
#include "DX12App.h"
#include "wrl/wrappers/corewrappers.h"

#include <pix.h>

namespace Olex
{
    using namespace Microsoft::WRL;

    TexturedDemoBoxGame::TexturedDemoBoxGame( DX12App& app ) : BaseGameInterface( app )
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

        CreateRootSignature();

        // Create the descriptor heap for the texture view.
        {
            // Describe and create a shader resource view (SRV) heap for the texture.
            D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
            srvHeapDesc.NumDescriptors = 1;
            srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
            srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
            ThrowIfFailed(
                device->CreateDescriptorHeap( &srvHeapDesc, IID_PPV_ARGS( &m_SrvHeap ) ) );
        }

        m_DSVHeap = m_app.CreateDescriptorHeap( D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1 );

#if (_WIN32_WINNT >= 0x0A00 /*_WIN32_WINNT_WIN10*/)
        Microsoft::WRL::Wrappers::RoInitializeWrapper initialize( RO_INIT_MULTITHREADED );
        if ( FAILED( initialize ) )
            // error
#else
        HRESULT hr = CoInitializeEx( nullptr, COINITBASE_MULTITHREADED );
        if ( FAILED( hr ) )
            // error
#endif

            Microsoft::WRL::ComPtr<ID3D12Resource> texture;

        using namespace std::filesystem;
        DirectX::ResourceUploadBatch uploadBatch( m_app.GetDevice().Get() );
        uploadBatch.Begin();

        ThrowIfFailed( DirectX::CreateWICTextureFromFile(
            m_app.GetDevice().Get(),
            uploadBatch,
            L"texture.jpg",
            &m_texture,
            true ) );

        const std::future<void> uploadTask = uploadBatch.End( m_app.GetCommandQueue().GetD3D12CommandQueue().Get() );
        uploadTask.wait();

        {
            D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
            srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING; // no re-ordering of RGBA components
            srvDesc.Format = /*DXGI_FORMAT_R8G8B8A8_UNORM*/ m_texture->GetDesc().Format;
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D; // The resource is a 2D texture.
            srvDesc.Texture2D.MostDetailedMip = 0; // so the first in memory is the largest mipmap level
            srvDesc.Texture2D.MipLevels = 1;
            srvDesc.Texture2D.ResourceMinLODClamp = 0.0f; // A value to clamp sample LOD values to.

            device->CreateShaderResourceView( m_texture.Get(), &srvDesc, m_SrvHeap->GetCPUDescriptorHandleForHeapStart() );
        }


        // Create the vertex input layout
        D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        };

        // Load the vertex shader.
        ComPtr<ID3DBlob> vertexShaderBlob;
        ThrowIfFailed( D3DReadFileToBlob( L"VertexShader_Textured.cso", &vertexShaderBlob ) );

        // Load the pixel shader.
        ComPtr<ID3DBlob> pixelShaderBlob;
        ThrowIfFailed( D3DReadFileToBlob( L"PixelShader_Textured.cso", &pixelShaderBlob ) );

        // Describe and create the graphics pipeline state object (PSO).
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.InputLayout = { inputLayout, _countof( inputLayout ) };
        psoDesc.pRootSignature = m_RootSignature.Get();
        psoDesc.VS = CD3DX12_SHADER_BYTECODE( vertexShaderBlob.Get() );
        psoDesc.PS = CD3DX12_SHADER_BYTECODE( pixelShaderBlob.Get() );
        psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC( D3D12_DEFAULT );
        psoDesc.BlendState = CD3DX12_BLEND_DESC( D3D12_DEFAULT );
        psoDesc.DepthStencilState.DepthEnable = FALSE;
        psoDesc.DepthStencilState.StencilEnable = FALSE;
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        psoDesc.SampleDesc.Count = 1;
        ThrowIfFailed( m_app.GetDevice()->CreateGraphicsPipelineState( &psoDesc, IID_PPV_ARGS( m_PipelineState.ReleaseAndGetAddressOf() ) ) );

        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList = m_app.GetCommandQueue().CreateCommandList();

        // Upload vertex buffer data.
        ComPtr<ID3D12Resource> intermediateVertexBuffer;
        UpdateBufferResource( commandList.Get(),
            &m_VertexBuffer, &intermediateVertexBuffer,
            _countof( m_Vertices ), sizeof( VertexPosUV ), m_Vertices );

        // Create the vertex buffer view.
        m_VertexBufferView.BufferLocation = m_VertexBuffer->GetGPUVirtualAddress();
        m_VertexBufferView.SizeInBytes = sizeof( m_Vertices );
        m_VertexBufferView.StrideInBytes = sizeof( VertexPosUV );

        // Upload index buffer data.
        ComPtr<ID3D12Resource> intermediateIndexBuffer;
        UpdateBufferResource( commandList.Get(),
            &m_IndexBuffer, &intermediateIndexBuffer,
            _countof( m_Indices ), sizeof( WORD ), m_Indices );

        // Create index buffer view.
        m_IndexBufferView.BufferLocation = m_IndexBuffer->GetGPUVirtualAddress();
        m_IndexBufferView.Format = DXGI_FORMAT_R16_UINT;
        m_IndexBufferView.SizeInBytes = sizeof( m_Indices );

        const FenceValue fenceValue = m_app.GetCommandQueue().ExecuteCommandList( commandList );
        m_app.GetCommandQueue().WaitForFenceValue( fenceValue );

        m_ContentLoaded = true;

        // Resize/Create the depth buffer.
        ResizeDepthBuffer( GetClientWidth(), GetClientHeight() );
    }

    void TexturedDemoBoxGame::CreateRootSignature()
    {
        // Create a root signature.
        D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
        if ( FAILED( m_app.GetDevice()->CheckFeatureSupport( D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof( featureData ) ) ) )
        {
            featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
        }

        // Allow input layout and deny unnecessary access to certain pipeline stages.
        const D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
            D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

        D3D12_STATIC_SAMPLER_DESC samplerDesc = {};
        samplerDesc.Filter = D3D12_FILTER_ANISOTROPIC;
        samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        samplerDesc.MaxAnisotropy = 16;
        samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
        samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
        samplerDesc.MinLOD = 0;
        samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
        samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        // A single 32-bit constant root parameter that is used by the vertex shader.
        CD3DX12_ROOT_PARAMETER1 rootParameters[2] = {};
        rootParameters[0].InitAsConstants( sizeof( DirectX::XMMATRIX ) / 4, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX );
        {
            CD3DX12_DESCRIPTOR_RANGE1 descriptorRange = {};
            descriptorRange.Init( D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0 );
            rootParameters[1].InitAsDescriptorTable( 1, &descriptorRange, D3D12_SHADER_VISIBILITY_PIXEL );
        }

        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
        rootSignatureDescription.Init_1_1( _countof( rootParameters ), rootParameters, 1, &samplerDesc, rootSignatureFlags );

        // Serialize the root signature.
        ComPtr<ID3DBlob> rootSignatureBlob;
        ComPtr<ID3DBlob> errorBlob;
        const HRESULT hr = D3DX12SerializeVersionedRootSignature( &rootSignatureDescription,
            featureData.HighestVersion, &rootSignatureBlob,
            &errorBlob );
        if ( FAILED( hr ) )
        {
            if ( errorBlob )
            {
                OutputDebugStringA( reinterpret_cast<const char*>( errorBlob->GetBufferPointer() ) );
            }
            ThrowIfFailed( hr );
        }

        // Create the root signature.
        ThrowIfFailed( m_app.GetDevice()->CreateRootSignature( 0, rootSignatureBlob->GetBufferPointer(),
            rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS( &m_RootSignature ) ) );
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

    Microsoft::WRL::ComPtr<ID3D12Resource> TexturedDemoBoxGame::LoadTextureFromFile( const wchar_t* fileName )
    {
        Microsoft::WRL::ComPtr<ID3D12Resource> textureResource;

        using namespace std::filesystem;
        const path texturePath( fileName );
        if ( exists( texturePath ) == true )
        {
            DirectX::ResourceUploadBatch uploadBatch( m_app.GetDevice().Get() );
            uploadBatch.Begin();

            ThrowIfFailed( DirectX::CreateWICTextureFromFile(
                m_app.GetDevice().Get(),
                uploadBatch,
                fileName,
                textureResource.ReleaseAndGetAddressOf(),
                false ) );

            const std::future<void> uploadTask = uploadBatch.End( m_app.GetCommandQueue().GetD3D12CommandQueue().Get() );
            uploadTask.wait();
        }

        return textureResource;
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

        ++m_frameCount;
    }

    void TexturedDemoBoxGame::Render( RenderEventArgs args )
    {
        if ( m_frameCount == 0 ) return;

        PIXBeginEvent( PIX_COLOR_DEFAULT, L"Render" );

        using namespace DirectX;

        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList = m_app.GetCommandQueue().CreateCommandList();

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

        commandList->SetGraphicsRootSignature( m_RootSignature.Get() );
        commandList->SetPipelineState( m_PipelineState.Get() );
        struct ID3D12DescriptorHeap* srvHeap = m_SrvHeap.Get();
        commandList->SetDescriptorHeaps( 1, &srvHeap );

        // bind the texture for the draw call
        commandList->SetGraphicsRootDescriptorTable( 1, m_SrvHeap->GetGPUDescriptorHandleForHeapStart() );

        // Update the MVP matrix
        XMMATRIX mvpMatrix = XMMatrixMultiply( m_ModelMatrix, m_ViewMatrix );
        mvpMatrix = XMMatrixMultiply( mvpMatrix, m_ProjectionMatrix );
        commandList->SetGraphicsRoot32BitConstants( 0, sizeof( XMMATRIX ) / 4, &mvpMatrix, 0 );

        // IA = Input Assembler
        commandList->IASetPrimitiveTopology( D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
        commandList->IASetVertexBuffers( 0, 1, &m_VertexBufferView );
        commandList->IASetIndexBuffer( &m_IndexBufferView );

        // RS = Rasterizer State
        commandList->RSSetViewports( 1, &m_Viewport );
        commandList->RSSetScissorRects( 1, &m_ScissorRect );

        // OM = Output Merger
        commandList->OMSetRenderTargets( 1, &rtv, FALSE, &dsv );



        // draw the cube
        commandList->DrawIndexedInstanced( _countof( m_Indices ), 1, 0, 0, 0 );

        PIXEndEvent();
        PIXBeginEvent( PIX_COLOR_DEFAULT, L"Present" );

        // Present
        {
            TransitionResource( commandList, backBuffer,
                D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT );

            const FenceValue fenceValue = m_app.GetCommandQueue().ExecuteCommandList( commandList );

            m_app.Present();

            m_app.GetCommandQueue().WaitForFenceValue( fenceValue );
        }

        PIXEndEvent();
    }
}
