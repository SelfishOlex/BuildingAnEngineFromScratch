#include "DX12App.h"

// DirectX 12 specific headers.
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <exception>

// D3D12 extension library.
#include "d3dx12.h"

namespace Olex
{
    DX12App::DX12App()
    {
    }

    void DX12App::Init()
    {
        EnableDebugLayer();
    }

    void DX12App::EnableDebugLayer()
    {
#if defined(_DEBUG)
        // Always enable the debug layer before doing anything DX12 related
        // so all possible errors generated while creating DX12 objects
        // are caught by the debug layer.
        Microsoft::WRL::ComPtr<ID3D12Debug> debugInterface;
        ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface)));
        debugInterface->EnableDebugLayer();
#endif
    }

    void DX12App::ThrowIfFailed( HRESULT hr )
    {
        if ( FAILED( hr ) )
        {
            throw std::exception();
        }
    }
}
