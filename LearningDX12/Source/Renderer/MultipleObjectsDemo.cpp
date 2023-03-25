#include "MultipleObjectsDemo.h"

#include <d3d12.h>
#include <d3dcompiler.h>
#include <filesystem>
#include <pix.h>
#include <backends/imgui_impl_win32.h>
#include <ImGui/backends/imgui_impl_dx12.h>
#include <wrl/client.h>
#include "d3dx12.h"
#include "DX12App.h"
#include "wrl/wrappers/corewrappers.h"

#include "FbxLoader.h"

namespace Olex
{
    using namespace Microsoft::WRL;

    MultipleObjectsDemo::MultipleObjectsDemo(DX12App& app) : BaseGameInterface(app)
        , m_ScissorRect(CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX))
        , m_FoV(45.0)
        , m_ContentLoaded(false)
    {
        RECT windowRect;
        ::GetWindowRect(m_app.GetWindowHandle(), &windowRect);

        m_Width = windowRect.right - windowRect.left;
        m_Height = windowRect.bottom - windowRect.top;
        m_Viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(m_Width), static_cast<float>(m_Height));

        m_gameWorld.Initialize();
        m_physx.Initialize(m_gameWorld);
        m_gameWorld.CreateWorld();
    }

    MultipleObjectsDemo::Texture MultipleObjectsDemo::LoadTexture(const wchar_t* filename)
    {
        return {};
    }

    void MultipleObjectsDemo::LoadResources()
    {
        LoadPipeline();
        LoadAssets();

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
                device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_SrvHeap)));
        }

        m_DSVHeap = m_app.CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1);

#if (_WIN32_WINNT >= 0x0A00 /*_WIN32_WINNT_WIN10*/)
        Microsoft::WRL::Wrappers::RoInitializeWrapper initialize(RO_INIT_MULTITHREADED);
        if (FAILED(initialize))
            // error
#else
        HRESULT hr = CoInitializeEx(nullptr, COINITBASE_MULTITHREADED);
        if (FAILED(hr))
            // error
#endif

        //////// Loading the texture ////////////////

        ComPtr<ID3D12GraphicsCommandList> commandList;
        m_app.CreateCommandList()
        // Create the command list.
        ThrowIfFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, 
             m_app m_commandAllocator.Get(), m_pipelineState.Get(), IID_PPV_ARGS(&commandList)));


        // Note: ComPtr's are CPU objects but this resource needs to stay in scope until
        // the command list that references it has finished executing on the GPU.
        // We will flush the GPU at the end of this method to ensure the resource is not
        // prematurely destroyed.
        ComPtr<ID3D12Resource> textureUploadHeap;

        // Create the texture.
        {
            auto texture = LoadTexture(L"texture.bmp");

            // Describe and create a Texture2D.
            D3D12_RESOURCE_DESC textureDesc = {};
            textureDesc.MipLevels = 1;
            textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            textureDesc.Width = texture.width;
            textureDesc.Height = texture.height;
            textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
            textureDesc.DepthOrArraySize = 1;
            textureDesc.SampleDesc.Count = 1;
            textureDesc.SampleDesc.Quality = 0;
            textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

            const CD3DX12_HEAP_PROPERTIES heapPropertiesDefault(D3D12_HEAP_TYPE_DEFAULT);

            ThrowIfFailed(device->CreateCommittedResource(
                &heapPropertiesDefault,
                D3D12_HEAP_FLAG_NONE,
                &textureDesc,
                D3D12_RESOURCE_STATE_COPY_DEST,
                nullptr,
                IID_PPV_ARGS(&m_texture)));

            const UINT64 uploadBufferSize = GetRequiredIntermediateSize(m_texture.Get(), 0, 1);

            const CD3DX12_HEAP_PROPERTIES heapPropertiesUpload(D3D12_HEAP_TYPE_UPLOAD);
            const auto bufferUpload = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);

            // Create the GPU upload buffer.
            ThrowIfFailed(device->CreateCommittedResource(
                &heapPropertiesUpload,
                D3D12_HEAP_FLAG_NONE,
                &bufferUpload,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(&textureUploadHeap)));

            // Copy data to the intermediate upload heap and then schedule a copy 
            // from the upload heap to the Texture2D.

            D3D12_SUBRESOURCE_DATA textureData = {};
            textureData.pData = &texture.data.get()[0];
            textureData.RowPitch = texture.width * texture.pixelSize;
            textureData.SlicePitch = textureData.RowPitch * texture.height;

            UpdateSubresources(commandList.Get(), m_texture.Get(), textureUploadHeap.Get(), 0, 0, 1, &textureData);

            commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_texture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

            // Describe and create a SRV for the texture.
            D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
            srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            srvDesc.Format = textureDesc.Format;
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Texture2D.MipLevels = 1;
            device->CreateShaderResourceView(m_texture.Get(), &srvDesc,  m_SrvHeap->GetCPUDescriptorHandleForHeapStart());
        }
        
        // Close the command list and execute it to begin the initial GPU setup.
        ThrowIfFailed(commandList->Close());
        ID3D12CommandList* ppCommandLists[] = { commandList.Get() };
        m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

        // Create synchronization objects and wait until assets have been uploaded to the GPU.
        {
            ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
            m_fenceValue = 1;

            // Create an event handle to use for frame synchronization.
            m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
            if (m_fenceEvent == nullptr)
            {
                ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
            }

            // Wait for the command list to execute; we are reusing the same command 
            // list in our main loop but for now, we just want to wait for setup to 
            // complete before continuing.
            WaitForPreviousFrame();
        }

        ///////// Shaders /////////////////

        // Create the vertex input layout
        D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12 + 8, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        };

        // Load the vertex shader.
        ComPtr<ID3DBlob> vertexShaderBlob;
        ThrowIfFailed(D3DReadFileToBlob(L"VertexShader_Textured_Light.cso", &vertexShaderBlob));

        // Load the pixel shader.
        ComPtr<ID3DBlob> pixelShaderBlob;
        ThrowIfFailed(D3DReadFileToBlob(L"PixelShader_Textured_Light.cso", &pixelShaderBlob));

        // Describe and create the graphics pipeline state object (PSO).
        CD3DX12_DEPTH_STENCIL_DESC depthStencilState{ CD3DX12_DEFAULT() };

        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.InputLayout = { inputLayout, _countof(inputLayout) };
        psoDesc.pRootSignature = m_RootSignature.Get();
        psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShaderBlob.Get());
        psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShaderBlob.Get());
        psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        psoDesc.DepthStencilState = static_cast<D3D12_DEPTH_STENCIL_DESC>(depthStencilState);
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
        psoDesc.SampleDesc.Count = 1;
        ThrowIfFailed(m_app.GetDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(m_PipelineState.ReleaseAndGetAddressOf())));

        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList = m_app.GetCommandQueue().CreateCommandList();

        m_fbxLoader = std::make_unique<FbxLoader>("model.fbx");
        const FbxLoader::Mesh& mesh = m_fbxLoader->GetMeshes()[0];

        // Upload vertex buffer data.
        ComPtr<ID3D12Resource> intermediateVertexBuffer;
        UpdateBufferResource(commandList.Get(),
            &m_VertexBuffer, &intermediateVertexBuffer,
            mesh.m_vertices.size(), sizeof(FbxLoader::Mesh::VertexInfo), mesh.m_vertices.data());

        // Create the vertex buffer view.
        m_VertexBufferView.BufferLocation = m_VertexBuffer->GetGPUVirtualAddress();
        m_VertexBufferView.SizeInBytes = static_cast<UINT>(mesh.m_vertices.size() * sizeof(FbxLoader::Mesh::VertexInfo));
        m_VertexBufferView.StrideInBytes = sizeof(FbxLoader::Mesh::VertexInfo);

        // Upload index buffer data.
        ComPtr<ID3D12Resource> intermediateIndexBuffer;
        UpdateBufferResource(commandList.Get(),
            &m_IndexBuffer, &intermediateIndexBuffer,
            mesh.m_indices.size(), sizeof(DirectX::XMINT3), mesh.m_indices.data());

        // Create index buffer view.
        m_IndexBufferView.BufferLocation = m_IndexBuffer->GetGPUVirtualAddress();
        m_IndexBufferView.Format = DXGI_FORMAT_R32_UINT;
        m_IndexBufferView.SizeInBytes = static_cast<UINT>(mesh.m_indices.size() * sizeof(DirectX::XMINT3));

        const FenceValue fenceValue = m_app.GetCommandQueue().ExecuteCommandList(commandList);
        m_app.GetCommandQueue().WaitForFenceValue(fenceValue);

        m_ContentLoaded = true;

        // Resize/Create the depth buffer.
        ResizeDepthBuffer(GetClientWidth(), GetClientHeight());

        // ImGui        
        ImGui_ImplDX12_Init(m_app.GetDevice().Get(), 1,
            DXGI_FORMAT_R8G8B8A8_UNORM, m_SrvHeap.Get(),
            m_SrvHeap->GetCPUDescriptorHandleForHeapStart(),
            m_SrvHeap->GetGPUDescriptorHandleForHeapStart());
    }

    void MultipleObjectsDemo::CreateRootSignature()
    {
        // Create a root signature.
        D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
        if (FAILED(m_app.GetDevice()->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
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
        CD3DX12_ROOT_PARAMETER1 rootParameters[3] = {};
        rootParameters[0].InitAsConstants(sizeof(ObjectInfo) / 4, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);

        // shader resource view, for texture sampling
        CD3DX12_DESCRIPTOR_RANGE1 descriptorRange = {};
        descriptorRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_NONE, 0);
        rootParameters[1].InitAsDescriptorTable(1, &descriptorRange, D3D12_SHADER_VISIBILITY_PIXEL);

        // light info
        rootParameters[2].InitAsConstants(sizeof(LightInfo) / 4, 1, 0, D3D12_SHADER_VISIBILITY_PIXEL);

        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
        rootSignatureDescription.Init_1_1(_countof(rootParameters), rootParameters, 1, &samplerDesc, rootSignatureFlags);

        // Serialize the root signature.
        ComPtr<ID3DBlob> rootSignatureBlob;
        ComPtr<ID3DBlob> errorBlob;
        const HRESULT hr = D3DX12SerializeVersionedRootSignature(&rootSignatureDescription,
            featureData.HighestVersion, &rootSignatureBlob,
            &errorBlob);
        if (FAILED(hr))
        {
            if (errorBlob)
            {
                OutputDebugStringA(reinterpret_cast<const char*>(errorBlob->GetBufferPointer()));
            }
            ThrowIfFailed(hr);
        }

        // Create the root signature.
        ThrowIfFailed(m_app.GetDevice()->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(),
            rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&m_RootSignature)));
    }

    void MultipleObjectsDemo::LoadPipeline()
    {
    }

    void MultipleObjectsDemo::LoadAssets()
    {
    }

    void MultipleObjectsDemo::PopulateCommandList()
    {
    }

    void MultipleObjectsDemo::WaitForPreviousFrame()
    {
    }

    void MultipleObjectsDemo::ResizeDepthBuffer(int width, int height)
    {
        if (m_ContentLoaded)
        {
            width = std::max(1, width);
            height = std::max(1, height);

            Microsoft::WRL::ComPtr<ID3D12Device2> device = m_app.GetDevice();

            // Resize screen dependent resources.
            // Create a depth buffer.
            D3D12_CLEAR_VALUE optimizedClearValue = {};
            optimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
            optimizedClearValue.DepthStencil = { 1.0f, 0 };

            CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_DEFAULT);
            const auto tex2D = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, width, height,
                1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

            ThrowIfFailed(device->CreateCommittedResource(
                &heapProperties,
                D3D12_HEAP_FLAG_NONE,
                &tex2D,
                D3D12_RESOURCE_STATE_DEPTH_WRITE,
                &optimizedClearValue,
                IID_PPV_ARGS(&m_DepthBuffer)
            ));

            // Update the depth-stencil view.
            D3D12_DEPTH_STENCIL_VIEW_DESC dsv = {};
            dsv.Format = DXGI_FORMAT_D32_FLOAT;
            dsv.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
            dsv.Flags = D3D12_DSV_FLAG_NONE;
            dsv.Texture2D.MipSlice = 0;

            device->CreateDepthStencilView(m_DepthBuffer.Get(), &dsv,
                m_DSVHeap->GetCPUDescriptorHandleForHeapStart());

            m_Viewport = CD3DX12_VIEWPORT(0.0f, 0.0f,
                static_cast<float>(width), static_cast<float>(height));
        }
    }

    void MultipleObjectsDemo::Resize(ResizeEventArgs args)
    {
        if (args.Width != GetClientWidth() || args.Height != GetClientHeight())
        {
            ResizeDepthBuffer(args.Width, args.Height);
        }
    }

    void MultipleObjectsDemo::UnloadResources()
    {
        ImGui_ImplDX12_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
    }

    void MultipleObjectsDemo::Update(UpdateEventArgs args)
    {
        static uint64_t frameCount = 0;
        static double totalTime = 0.0;

        m_physx.Update(static_cast<float>(args.m_elapsedTime));
        m_gameWorld.Update(static_cast<float>(args.m_elapsedTime));

        totalTime += args.m_elapsedTime;
        frameCount++;

        if (totalTime > 1.0)
        {
            const double fps = frameCount / totalTime;

            wchar_t buffer[512];
            swprintf_s(buffer, _countof(buffer), L"FPS: %f\n", fps);
            OutputDebugString(buffer);

            frameCount = 0;
            totalTime = 0.0;
        }

        using namespace DirectX;

        // Update the model matrix.
        m_ModelMatrix = XMMatrixRotationRollPitchYaw(0, 0, 0);

        // Update the view matrix.
        const XMVECTOR eyePosition = XMVectorSet(-100, 0, 0, 1);
        const XMVECTOR focusPoint = XMVectorSet(0, 0, 0, 1);
        const XMVECTOR upDirection = XMVectorSet(0, 0, 1, 0);
        m_ViewMatrix = XMMatrixLookAtLH(eyePosition, focusPoint, upDirection);

        // Update the projection matrix.
        const float aspectRatio = static_cast<float>(GetClientWidth()) / static_cast<float>(GetClientHeight());
        m_ProjectionMatrix = XMMatrixPerspectiveFovLH(XMConvertToRadians(m_FoV), aspectRatio, 0.1f, 1000.0f);

        // update light information
        m_lightInfo.m_eyePosition = { 0, -10, 0 }; // TODO remove duplication here
        m_lightInfo.m_directionLight.m_color = { 1, 1, 1 };
        m_lightInfo.m_directionLight.m_intensity = 1.f;
        m_lightInfo.m_directionLight.m_direction = { 0, 0, -1 };

        ++m_frameCount;
    }

    void MultipleObjectsDemo::Render(RenderEventArgs args)
    {
        if (m_frameCount == 0) return;

        PIXBeginEvent(PIX_COLOR_DEFAULT, L"Render");

        using namespace DirectX;

        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList = m_app.GetCommandQueue().CreateCommandList();

        Microsoft::WRL::ComPtr<ID3D12Resource> backBuffer = m_app.GetCurrentBackBuffer();
        D3D12_CPU_DESCRIPTOR_HANDLE renderTargetView = m_app.GetCurrentRenderTargetView();
        D3D12_CPU_DESCRIPTOR_HANDLE depthStencilView = m_DSVHeap->GetCPUDescriptorHandleForHeapStart();

        // Clear the render targets.
        {
            TransitionResource(commandList, backBuffer,
                D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

            FLOAT clearColor[] = { 0.4f, 0.6f, 0.9f, 1.0f };

            ClearRTV(commandList, renderTargetView, clearColor);
            ClearDepth(commandList, depthStencilView);
        }

        commandList->SetGraphicsRootSignature(m_RootSignature.Get());
        commandList->SetPipelineState(m_PipelineState.Get());
        struct ID3D12DescriptorHeap* srvHeap = m_SrvHeap.Get();
        commandList->SetDescriptorHeaps(1, &srvHeap);

        // bind the texture for the draw call
        commandList->SetGraphicsRootDescriptorTable(1, m_SrvHeap->GetGPUDescriptorHandleForHeapStart());

        // Update light info
        commandList->SetGraphicsRoot32BitConstants(2, sizeof(LightInfo) / 4, &m_lightInfo, 0);

        // IA = Input Assembler
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        commandList->IASetVertexBuffers(0, 1, &m_VertexBufferView);
        commandList->IASetIndexBuffer(&m_IndexBufferView);

        // RS = Rasterizer State
        commandList->RSSetViewports(1, &m_Viewport);
        commandList->RSSetScissorRects(1, &m_ScissorRect);

        // OM = Output Merger
        commandList->OMSetRenderTargets(1, &renderTargetView, FALSE, &depthStencilView);

        // System declaration
        const flecs::filter<const Position, const Mesh> filter = m_gameWorld.m_world.filter<const Position, const Mesh>("DrawFilter");

        filter.each([this, commandList](const Position& p, [[maybe_unused]] const Mesh& mesh)
            {
                // Each is invoked for each entity

                // Update the MVP matrix
                const XMMATRIX position = XMMatrixTranslation(p.x, p.y, p.z);
                const XMMATRIX thisModelMatrix = XMMatrixMultiply(m_ModelMatrix, position);

                XMMATRIX mvpMatrix = XMMatrixMultiply(thisModelMatrix, m_ViewMatrix);
                mvpMatrix = XMMatrixMultiply(mvpMatrix, m_ProjectionMatrix);

                ObjectInfo info{ mvpMatrix };

                commandList->SetGraphicsRoot32BitConstants(0, sizeof(ObjectInfo) / 4, &info, 0);

                // draw the model
                static const UINT indexCount = static_cast<UINT>(m_fbxLoader->GetMeshes()[0].m_indices.size() * 3);
                commandList->DrawIndexedInstanced(indexCount, 1, 0, 0, 0);
            });

        PIXEndEvent();
               
        //RenderImGui(args);

        PIXBeginEvent(PIX_COLOR_DEFAULT, L"Present");

        // Present
        TransitionResource(commandList, backBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

        m_lastFenceValue = m_app.GetCommandQueue().ExecuteCommandList(commandList);
                        

        m_app.Present();
        m_app.GetCommandQueue().WaitForFenceValue(m_lastFenceValue);

        PIXEndEvent();
    }
    
    void MultipleObjectsDemo::RenderImGui(RenderEventArgs args)
    {
        // Start the Dear ImGui frame
        ImGui_ImplDX12_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        ImGui::ShowDemoWindow();

        ImGui::EndFrame();

        // Rendering
        ImGui::Render();

        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList = m_app.GetCommandQueue().CreateCommandList();
        
        /*Microsoft::WRL::ComPtr<ID3D12Resource> backBuffer = m_app.GetCurrentBackBuffer();
        TransitionResource(commandList, backBuffer, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);*/

        
        Microsoft::WRL::ComPtr<ID3D12Resource> backBuffer = m_app.GetCurrentBackBuffer();
        D3D12_CPU_DESCRIPTOR_HANDLE renderTargetView = m_app.GetCurrentRenderTargetView();
        
        /*const ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
        const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
        commandList->ClearRenderTargetView(renderTargetView, clear_color_with_alpha, 0, nullptr);
        commandList->OMSetRenderTargets(1, &renderTargetView, FALSE, nullptr);
        commandList->SetDescriptorHeaps(1, &m_SrvHeap);*/

        ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList.Get());

        //TransitionResource(commandList, backBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
        m_lastFenceValue = m_app.GetCommandQueue().ExecuteCommandList(commandList);

        ID3D12CommandList* const ppCommandLists[] = {
            commandList.Get()
        };

        m_app.GetCommandQueue().GetD3D12CommandQueue()->ExecuteCommandLists(1, ppCommandLists);
    }
}
