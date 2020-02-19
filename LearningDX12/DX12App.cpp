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
        ThrowIfFailed( D3D12GetDebugInterface( IID_PPV_ARGS( &debugInterface ) ) );
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

    Microsoft::WRL::ComPtr<IDXGIAdapter4> DX12App::GetAdapter( bool useWarp )
    {
        using namespace Microsoft::WRL;

        ComPtr<IDXGIFactory4> dxgiFactory;
        UINT createFactoryFlags = 0;
#if defined(_DEBUG)
        createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

        ThrowIfFailed( CreateDXGIFactory2( createFactoryFlags, IID_PPV_ARGS( &dxgiFactory ) ) );

        ComPtr<IDXGIAdapter1> dxgiAdapter1;
        ComPtr<IDXGIAdapter4> dxgiAdapter4;

        if ( useWarp )
        {
            ThrowIfFailed( dxgiFactory->EnumWarpAdapter( IID_PPV_ARGS( &dxgiAdapter1 ) ) );
            ThrowIfFailed( dxgiAdapter1.As( &dxgiAdapter4 ) );
        }
        else
        {
            SIZE_T maxDedicatedVideoMemory = 0;
            for ( UINT i = 0; dxgiFactory->EnumAdapters1( i, &dxgiAdapter1 ) != DXGI_ERROR_NOT_FOUND; ++i )
            {
                DXGI_ADAPTER_DESC1 dxgiAdapterDesc1;
                dxgiAdapter1->GetDesc1( &dxgiAdapterDesc1 );

                // Check to see if the adapter can create a D3D12 device without actually
                // creating it. The adapter with the largest dedicated video memory
                // is favored.
                if ( ( dxgiAdapterDesc1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE ) == 0 &&
                    SUCCEEDED( D3D12CreateDevice( dxgiAdapter1.Get(),
                        D3D_FEATURE_LEVEL_11_0, __uuidof( ID3D12Device ), nullptr ) ) &&
                    dxgiAdapterDesc1.DedicatedVideoMemory > maxDedicatedVideoMemory )
                {
                    maxDedicatedVideoMemory = dxgiAdapterDesc1.DedicatedVideoMemory;
                    ThrowIfFailed( dxgiAdapter1.As( &dxgiAdapter4 ) );
                }
            }
        }

        return dxgiAdapter4;
    }
}
